#include "s57_source_model.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <system_error>

namespace chart_view::runtime::s57 {

namespace {

struct Crc32Table
{
  std::uint32_t entries[256]{};

  constexpr Crc32Table()
  {
    for(std::uint32_t i = 0; i < 256; ++i) {
      std::uint32_t crc = i;
      for(int j = 0; j < 8; ++j) {
        crc = (crc & 1u) != 0u ? (crc >> 1u) ^ 0xEDB88320u : (crc >> 1u);
      }
      entries[i] = crc;
    }
  }
};

inline constexpr Crc32Table kCrc32Table{};

[[nodiscard]] std::uint32_t crc32(std::span<const std::uint8_t> data) noexcept
{
  std::uint32_t crc = 0xFFFFFFFFu;
  for(const auto byte : data) {
    crc = kCrc32Table.entries[(crc ^ byte) & 0xFFu] ^ (crc >> 8u);
  }
  return crc ^ 0xFFFFFFFFu;
}

[[nodiscard]] std::int64_t toUnixSeconds(std::filesystem::file_time_type timePoint) noexcept
{
  using namespace std::chrono;
  const auto systemTime = time_point_cast<seconds>(
    timePoint - std::filesystem::file_time_type::clock::now() + system_clock::now());
  return systemTime.time_since_epoch().count();
}

[[nodiscard]] bool tryReadFile(
  const std::string &path,
  std::vector<std::uint8_t> &out)
{
  std::ifstream stream(path, std::ios::binary | std::ios::ate);
  if(!stream) {
    return false;
  }

  const auto size = static_cast<std::size_t>(stream.tellg());
  stream.seekg(0, std::ios::beg);
  out.resize(size);
  if(size == 0) {
    return true;
  }

  return stream.read(reinterpret_cast<char *>(out.data()), static_cast<std::streamsize>(size)).good();
}

[[nodiscard]] std::optional<std::uint32_t> parseNumericUpdateExtension(
  const std::filesystem::path &path) noexcept
{
  const auto ext = path.extension().string();
  if(ext.size() != 4 || ext[0] != '.') {
    return std::nullopt;
  }

  std::uint32_t value = 0;
  for(std::size_t i = 1; i < ext.size(); ++i) {
    const auto ch = ext[i];
    if(ch < '0' || ch > '9') {
      return std::nullopt;
    }
    value = value * 10u + static_cast<std::uint32_t>(ch - '0');
  }

  if(value == 0) {
    return std::nullopt;
  }

  return value;
}

}// namespace

S57UpdateManifest buildUpdateManifestForBasePath(
  const std::string &basePath,
  std::uint32_t edition,
  std::uint32_t baseUpdate)
{
  S57UpdateManifest manifest;
  manifest.basePath = basePath;
  manifest.baseName = std::filesystem::path(basePath).filename().string();
  manifest.edition = edition;
  manifest.baseUpdate = baseUpdate;
  manifest.highestContiguousUpdate = baseUpdate;
  manifest.lastAppliedUpdate = baseUpdate;
  manifest.nextMissingUpdate = baseUpdate + 1u;

  std::error_code ec;
  const auto base = std::filesystem::path(basePath);
  const auto parent = base.parent_path();
  if(parent.empty() || !std::filesystem::exists(parent, ec)) {
    return manifest;
  }

  for(const auto &entry : std::filesystem::directory_iterator(parent, ec)) {
    if(ec || !entry.is_regular_file()) {
      continue;
    }

    const auto entryPath = entry.path();
    if(entryPath.stem() != base.stem()) {
      continue;
    }

    const auto updateNumber = parseNumericUpdateExtension(entryPath);
    if(!updateNumber.has_value()) {
      continue;
    }

    S57UpdateFile updateFile;
    updateFile.path = entryPath.string();
    updateFile.name = entryPath.filename().string();
    updateFile.updateNumber = *updateNumber;
    updateFile.sourceSize = entry.file_size(ec);
    if(!ec) {
      updateFile.sourceTimestamp = toUnixSeconds(entry.last_write_time(ec));
    }

    manifest.availableUpdates.push_back(std::move(updateFile));
  }

  std::sort(
    manifest.availableUpdates.begin(),
    manifest.availableUpdates.end(),
    [](const S57UpdateFile &lhs, const S57UpdateFile &rhs) {
      return lhs.updateNumber < rhs.updateNumber;
    });

  auto expected = baseUpdate + 1u;
  for(const auto &update : manifest.availableUpdates) {
    if(update.updateNumber != expected) {
      break;
    }
    manifest.highestContiguousUpdate = update.updateNumber;
    ++expected;
  }
  manifest.nextMissingUpdate = expected;

  return manifest;
}

senc::SourceManifest buildSourceManifestForPath(
  const std::string &path,
  const std::string &sourceName,
  std::uint32_t edition,
  std::uint32_t update)
{
  senc::SourceManifest manifest;
  manifest.name = sourceName;
  manifest.sourceType = chart_view_chart_source_s57;
  manifest.edition = edition;
  manifest.update = update;

  std::error_code ec;
  manifest.sourceSize = std::filesystem::file_size(path, ec);
  if(!ec) {
    manifest.sourceTimestamp = toUnixSeconds(std::filesystem::last_write_time(path, ec));
  }

  std::vector<std::uint8_t> bytes;
  if(tryReadFile(path, bytes) && !bytes.empty()) {
    manifest.sourceHash = crc32(bytes);
  }

  return manifest;
}

senc::SourceManifest buildSourceManifestForBuffer(
  const std::string &sourceName,
  std::span<const std::uint8_t> data,
  std::uint32_t edition,
  std::uint32_t update)
{
  senc::SourceManifest manifest;
  manifest.name = sourceName;
  manifest.sourceType = chart_view_chart_source_s57;
  manifest.sourceSize = data.size();
  manifest.sourceHash = data.empty() ? 0u : crc32(data);
  manifest.edition = edition;
  manifest.update = update;
  return manifest;
}

}// namespace chart_view::runtime::s57
