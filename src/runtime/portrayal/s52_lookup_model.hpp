#ifndef CHART_VIEW_RUNTIME_PORTRAYAL_S52_LOOKUP_MODEL_HPP
#define CHART_VIEW_RUNTIME_PORTRAYAL_S52_LOOKUP_MODEL_HPP

#include "../chart_data/feature.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace chart_view::runtime::portrayal {

enum class S52InstructionType
{
  kPointSymbol,
  kLineStyle,
  kAreaPattern,
  kTextLabel,
};

struct S52Instruction
{
  S52InstructionType type{S52InstructionType::kPointSymbol};
  std::string assetId;
  std::string styleKey;
};

struct S52LookupResult
{
  std::string lookupKey;
  std::vector<S52Instruction> instructions;
  bool suppressed{false};
};

class S52LookupModel
{
public:
  [[nodiscard]] static std::optional<S52LookupResult> lookup(const chart_data::Feature &feature)
  {
    struct Entry
    {
      std::string_view acronym;
      S52InstructionType type;
      std::string_view assetId;
      std::string_view styleKey;
    };

    constexpr std::array<Entry, 12> kPointEntries{{
      {"SOUNDG", S52InstructionType::kPointSymbol, "SOUNDG01", "point/sounding"},
      {"BOYSPP", S52InstructionType::kPointSymbol, "BOYSPP01", "point/buoy"},
      {"BOYLAT", S52InstructionType::kPointSymbol, "BOYSPP01", "point/buoy"},
      {"BOYSAW", S52InstructionType::kPointSymbol, "BOYSPP01", "point/buoy"},
      {"BCNSPP", S52InstructionType::kPointSymbol, "BCNSPP01", "point/beacon"},
      {"BCNLAT", S52InstructionType::kPointSymbol, "BCNSPP01", "point/beacon"},
      {"BCNSAW", S52InstructionType::kPointSymbol, "BCNSPP01", "point/beacon"},
      {"WRECKS", S52InstructionType::kPointSymbol, "DANGER01", "point/danger"},
      {"UWTROC", S52InstructionType::kPointSymbol, "DANGER01", "point/danger"},
      {"LIGHTS", S52InstructionType::kPointSymbol, "LNDMRK01", "point/landmark"},
      {"LNDMRK", S52InstructionType::kPointSymbol, "LNDMRK01", "point/landmark"},
      {"PILPNT", S52InstructionType::kPointSymbol, "LNDMRK01", "point/landmark"},
    }};

    constexpr std::array<Entry, 5> kLineEntries{{
      {"DEPCNT", S52InstructionType::kLineStyle, "DEPCN01", "line/depth_contour"},
      {"COALNE", S52InstructionType::kLineStyle, "COALNE01", "line/coastline"},
      {"FAIRWY", S52InstructionType::kLineStyle, "FAIRWY01", "line/channel"},
      {"CANALS", S52InstructionType::kLineStyle, "FAIRWY01", "line/channel"},
      {"RIVERS", S52InstructionType::kLineStyle, "FAIRWY01", "line/channel"},
    }};

    constexpr std::array<Entry, 5> kAreaEntries{{
      {"DEPARE", S52InstructionType::kAreaPattern, "DEPARE01", "area/depth"},
      {"DRGARE", S52InstructionType::kAreaPattern, "DEPARE01", "area/depth"},
      {"LNDARE", S52InstructionType::kAreaPattern, "LNDARE01", "area/land"},
      {"RESARE", S52InstructionType::kAreaPattern, "RESARE01", "area/restricted"},
      {"UNSARE", S52InstructionType::kAreaPattern, "RESARE01", "area/restricted"},
    }};

    const auto normalizedAcronym = normalizeAcronym(feature.classAcronym);
    if(normalizedAcronym.empty()) {
      return std::nullopt;
    }

    const auto geometryType = chart_data::geometryType(feature.geometry);
    const auto *entry = geometryType == chart_data::GeometryType::kPoint
                          ? findEntry(normalizedAcronym, kPointEntries)
                          : geometryType == chart_data::GeometryType::kLine
                              ? findEntry(normalizedAcronym, kLineEntries)
                              : findEntry(normalizedAcronym, kAreaEntries);
    if(entry == nullptr) {
      return std::nullopt;
    }

    S52LookupResult result;
    result.lookupKey = normalizedAcronym;
    result.instructions.push_back(
      {entry->type, std::string(entry->assetId), std::string(entry->styleKey)});

    if(hasNonEmptyStringAttribute(feature, "OBJNAM") || hasNonEmptyStringAttribute(feature, "NOBJNM")) {
      result.instructions.push_back({S52InstructionType::kTextLabel, "TEXT01", "text/default"});
    }

    return result;
  }

private:
  static std::string normalizeAcronym(std::string_view value)
  {
    std::string normalized(value);
    std::transform(
      normalized.begin(),
      normalized.end(),
      normalized.begin(),
      [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
    return normalized;
  }

  static bool hasNonEmptyStringAttribute(const chart_data::Feature &feature, std::string_view key)
  {
    const auto it = feature.attributes.find(std::string(key));
    if(it == feature.attributes.end()) {
      return false;
    }

    const auto *value = std::get_if<std::string>(&it->second);
    return value != nullptr && !value->empty();
  }

  template<typename T, std::size_t N>
  static const T *findEntry(std::string_view acronym, const std::array<T, N> &entries)
  {
    const auto it = std::find_if(
      entries.begin(),
      entries.end(),
      [&](const auto &entry) { return entry.acronym == acronym; });
    return it == entries.end() ? nullptr : &(*it);
  }
};

}// namespace chart_view::runtime::portrayal

#endif
