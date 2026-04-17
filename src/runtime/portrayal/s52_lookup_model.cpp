#include "s52_lookup_model.hpp"

#include "s52_source_catalog_compiler.hpp"

#include <algorithm>
#include <cctype>

namespace chart_view::runtime::portrayal {

namespace {

const S52CompiledCatalog &compiledCatalog()
{
  static const auto catalog = S52SourceCatalogCompiler::compilePreferred();
  return catalog;
}

std::string normalizeAcronymValue(std::string_view value)
{
  std::string normalized(value);
  std::transform(
    normalized.begin(),
    normalized.end(),
    normalized.begin(),
    [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
  return normalized;
}

bool hasNonEmptyTextAttribute(const chart_data::Feature &feature, std::string_view key)
{
  const auto it = feature.attributes.find(std::string(key));
  if(it == feature.attributes.end()) {
    return false;
  }

  if(const auto *value = std::get_if<std::string>(&it->second)) {
    return !value->empty();
  }

  if(const auto *values = std::get_if<chart_data::AttributeStringList>(&it->second)) {
    return !values->empty() && std::any_of(values->begin(), values->end(), [](const auto &entry) {
      return !entry.empty();
    });
  }

  return false;
}

std::string preferredTextAttributeKey(const chart_data::Feature &feature)
{
  if(hasNonEmptyTextAttribute(feature, "NOBJNM")) {
    return "NOBJNM";
  }
  if(hasNonEmptyTextAttribute(feature, "OBJNAM")) {
    return "OBJNAM";
  }
  return {};
}

const S52CompiledLookupRow *findCompiledRow(const chart_data::Feature &feature)
{
  const auto normalizedAcronym = normalizeAcronymValue(feature.classAcronym);
  if(normalizedAcronym.empty()) {
    return nullptr;
  }

  const auto geometryType = chart_data::geometryType(feature.geometry);
  const auto &catalog = compiledCatalog();
  const auto it = std::find_if(
    catalog.lookupRows.begin(),
    catalog.lookupRows.end(),
    [&](const auto &row) {
      return row.objectAcronym == normalizedAcronym && row.geometryType == geometryType
          && !row.instructions.empty();
    });
  return it == catalog.lookupRows.end() ? nullptr : &(*it);
}

} // namespace

std::optional<S52LookupResult> S52LookupModel::lookup(const chart_data::Feature &feature)
{
  const auto *row = findCompiledRow(feature);
  if(row == nullptr) {
    return std::nullopt;
  }

  S52LookupResult result;
  result.lookupKey = row->objectAcronym;
  result.ruleId = row->ruleId;
  result.displayCategory = row->displayCategory;
  result.displayPriority = row->displayPriority;
  result.viewGroup = row->viewGroup;
  result.instructions = row->instructions;

  if(const auto attributeKey = preferredTextAttributeKey(feature); !attributeKey.empty()) {
    result.instructions.push_back(S52TextInstruction{"TEXT01", "text/default", attributeKey});
  }

  return result;
}

std::string S52LookupModel::normalizeAcronym(std::string_view value)
{
  return normalizeAcronymValue(value);
}

} // namespace chart_view::runtime::portrayal
