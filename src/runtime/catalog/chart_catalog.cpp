#include "chart_catalog.hpp"

#include "../senc/senc_reader.hpp"

#include <algorithm>
#include <cctype>
#include <system_error>
#include <utility>

namespace chart_view::runtime::catalog {

namespace {

std::string toLowerAscii(std::string value)
{
  std::transform(
    value.begin(),
    value.end(),
    value.begin(),
    [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return value;
}

bool isSencFile(const std::filesystem::path &path)
{
  return toLowerAscii(path.extension().string()) == ".senc";
}

std::string chooseCatalogId(const senc::SencCatalogMetaReadResult &metaResult,
                            const std::filesystem::path &path)
{
  if(metaResult.manifest && !metaResult.manifest->name.empty()) {
    return metaResult.manifest->name;
  }
  if(!metaResult.meta.name.empty()) {
    return metaResult.meta.name;
  }
  return path.stem().string();
}

bool compareEntries(const ChartCatalogEntry &lhs, const ChartCatalogEntry &rhs)
{
  if(lhs.id != rhs.id) {
    return lhs.id < rhs.id;
  }
  return lhs.sencPath.generic_string() < rhs.sencPath.generic_string();
}

}// namespace

bool ChartCatalog::loadDirectory(const std::filesystem::path &directory)
{
  clear();

  std::error_code ec;
  if(directory.empty()) {
    m_lastError = "catalog directory is empty";
    return false;
  }
  if(!std::filesystem::exists(directory, ec)) {
    m_lastError = "catalog directory does not exist: " + directory.string();
    return false;
  }
  if(ec) {
    m_lastError = "failed to stat catalog directory: " + directory.string();
    return false;
  }
  if(!std::filesystem::is_directory(directory, ec)) {
    m_lastError = "catalog path is not a directory: " + directory.string();
    return false;
  }
  if(ec) {
    m_lastError = "failed to inspect catalog directory: " + directory.string();
    return false;
  }

  senc::SencReader reader;
  for(const auto &entry : std::filesystem::directory_iterator(directory, ec)) {
    if(ec) {
      m_entries.clear();
      m_lastError = "failed to iterate catalog directory: " + directory.string();
      return false;
    }

    if(!entry.is_regular_file(ec)) {
      continue;
    }
    if(ec || !isSencFile(entry.path())) {
      continue;
    }

    const auto metaResult = reader.readCatalogMetaFromFile(entry.path().string());
    if(!metaResult.ok) {
      m_entries.clear();
      m_lastError =
        "failed to read SENC metadata from " + entry.path().string() + ": " + metaResult.error;
      return false;
    }

    ChartCatalogEntry catalogEntry;
    catalogEntry.id = chooseCatalogId(metaResult, entry.path());
    catalogEntry.sencPath = entry.path();
    catalogEntry.sourceType = metaResult.meta.sourceType;
    catalogEntry.nativeScale = metaResult.meta.nativeScale;
    catalogEntry.extent = metaResult.meta.extent;
    catalogEntry.usageBand = metaResult.meta.usageBand;
    catalogEntry.edition = metaResult.meta.edition;
    catalogEntry.update = metaResult.meta.update;
    m_entries.push_back(std::move(catalogEntry));
  }

  std::sort(m_entries.begin(), m_entries.end(), compareEntries);
  m_lastError.clear();
  return true;
}

void ChartCatalog::clear() noexcept
{
  m_entries.clear();
  m_lastError.clear();
}

const ChartCatalogEntry *ChartCatalog::findById(std::string_view id) const noexcept
{
  const auto it = std::find_if(
    m_entries.begin(),
    m_entries.end(),
    [&](const ChartCatalogEntry &entry) { return entry.id == id; });
  return it == m_entries.end() ? nullptr : &(*it);
}

}// namespace chart_view::runtime::catalog
