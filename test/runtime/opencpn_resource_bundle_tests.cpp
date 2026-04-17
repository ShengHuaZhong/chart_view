#include <catch2/catch_test_macros.hpp>

#include <QCryptographicHash>
#include <QFile>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct ManifestFileEntry
{
  std::string relativePath;
  std::uintmax_t size{0};
  std::string sha256;
};

struct ManifestData
{
  std::map<std::string, std::string> fields;
  std::vector<ManifestFileEntry> files;
};

std::string trim(std::string value)
{
  const auto isSpace = [](unsigned char ch) { return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n'; };
  value.erase(value.begin(),
              std::find_if(value.begin(), value.end(), [&](unsigned char ch) { return !isSpace(ch); }));
  value.erase(std::find_if(value.rbegin(), value.rend(), [&](unsigned char ch) { return !isSpace(ch); }).base(),
              value.end());
  return value;
}

ManifestData parseManifest(const std::filesystem::path &path)
{
  std::ifstream stream(path);
  if(!stream) {
    throw std::runtime_error("failed to open manifest");
  }

  ManifestData manifest;
  std::string line;
  while(std::getline(stream, line)) {
    line = trim(line);
    if(line.empty() || line.front() == '#') {
      continue;
    }

    const auto separator = line.find('=');
    if(separator == std::string::npos) {
      throw std::runtime_error("invalid manifest line");
    }

    const auto key = trim(line.substr(0, separator));
    const auto value = trim(line.substr(separator + 1));
    if(key == "file") {
      std::istringstream entryStream(value);
      ManifestFileEntry entry;
      std::string sizeToken;
      if(!std::getline(entryStream, entry.relativePath, '|') || !std::getline(entryStream, sizeToken, '|') ||
         !std::getline(entryStream, entry.sha256)) {
        throw std::runtime_error("invalid file entry");
      }

      entry.relativePath = trim(entry.relativePath);
      entry.sha256 = trim(entry.sha256);
      entry.size = static_cast<std::uintmax_t>(std::stoull(trim(sizeToken)));
      manifest.files.push_back(entry);
    } else {
      manifest.fields[key] = value;
    }
  }

  return manifest;
}

std::string sha256OfFile(const std::filesystem::path &path)
{
  QFile file(QString::fromStdWString(path.wstring()));
  if(!file.open(QIODevice::ReadOnly)) {
    throw std::runtime_error("failed to open bundle file");
  }

  QCryptographicHash hash(QCryptographicHash::Sha256);
  while(!file.atEnd()) {
    hash.addData(file.read(64 * 1024));
  }

  return hash.result().toHex().toStdString();
}

std::filesystem::path bundleRoot()
{
  return std::filesystem::path(CHART_VIEW_PROJECT_SOURCE_DIR) / "vendor" / "opencpn_s57data" / "Release_5.14.0";
}

} // namespace

TEST_CASE("OpenCPN Phase 6A resource bundle provenance is pinned to the expected upstream snapshot",
          "[assets][opencpn][s52]")
{
  const auto root = bundleRoot();
  const auto manifestPath = root / "PROVENANCE.manifest";

  REQUIRE(std::filesystem::exists(root));
  REQUIRE(std::filesystem::exists(manifestPath));

  const auto manifest = parseManifest(manifestPath);

  REQUIRE(manifest.fields.at("schema") == "opencpn_s57data_snapshot_v1");
  REQUIRE(manifest.fields.at("snapshot_role") == "engineering_input_only");
  REQUIRE(manifest.fields.at("upstream_repo") == "https://github.com/OpenCPN/OpenCPN.git");
  REQUIRE(manifest.fields.at("upstream_ref") == "Release_5.14.0");
  REQUIRE(manifest.fields.at("upstream_commit") == "91f3b674366068a6ecd61a5e9aba204bba85f57e");
  REQUIRE(manifest.fields.at("normative_truth") == "IHO S-52 6.1.1; Annex A 4.0.4; S-64 3.0.3");
  REQUIRE(manifest.fields.at("upstream_license_file") == "COPYING.gplv2");
  REQUIRE(manifest.files.size() == 11);
}

TEST_CASE("OpenCPN Phase 6A resource bundle files exist and match the recorded sizes and hashes",
          "[assets][opencpn][s52]")
{
  const auto root = bundleRoot();
  const auto manifest = parseManifest(root / "PROVENANCE.manifest");

  std::set<std::string> expectedCoreFiles{
    "COPYING.gplv2",
    "s57data/chartsymbols.xml",
    "s57data/rastersymbols-day.png",
    "s57data/rastersymbols-dusk.png",
    "s57data/rastersymbols-dark.png",
    "s57data/S52RAZDS.RLE",
    "s57data/s57objectclasses.csv",
  };

  std::set<std::string> seenFiles;
  for(const auto &entry : manifest.files) {
    const auto path = root / entry.relativePath;
    INFO("verifying " << entry.relativePath);
    REQUIRE(std::filesystem::exists(path));
    REQUIRE(std::filesystem::file_size(path) == entry.size);
    REQUIRE(sha256OfFile(path) == entry.sha256);
    seenFiles.insert(entry.relativePath);
  }

  for(const auto &required : expectedCoreFiles) {
    REQUIRE(seenFiles.count(required) == 1U);
  }
}
