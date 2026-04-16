#ifndef CHART_VIEW_RUNTIME_S57_S57_ATTRIBUTE_CODEC_HPP
#define CHART_VIEW_RUNTIME_S57_S57_ATTRIBUTE_CODEC_HPP

#include "../chart_data/feature.hpp"

#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

namespace chart_view::runtime::s57 {

namespace detail {

inline std::string trimCopy(std::string_view value)
{
  std::size_t first = 0;
  while(first < value.size() && std::isspace(static_cast<unsigned char>(value[first])) != 0) {
    ++first;
  }

  std::size_t last = value.size();
  while(last > first && std::isspace(static_cast<unsigned char>(value[last - 1])) != 0) {
    --last;
  }

  return std::string(value.substr(first, last - first));
}

inline bool tryParseInt64(std::string_view text, std::int64_t &value) noexcept
{
  const auto *begin = text.data();
  const auto *end = begin + text.size();
  const auto [ptr, ec] = std::from_chars(begin, end, value);
  return ec == std::errc() && ptr == end;
}

inline bool tryParseDouble(std::string_view text, double &value) noexcept
{
  std::string scratch(text);
  char *end = nullptr;
  value = std::strtod(scratch.c_str(), &end);
  return end == scratch.c_str() + scratch.size() && std::isfinite(value);
}

inline std::vector<std::string> splitListTokens(std::string_view text)
{
  std::vector<std::string> tokens;
  std::size_t start = 0;
  while(start <= text.size()) {
    const auto separator = text.find_first_of(",;", start);
    const auto token = trimCopy(
      separator == std::string_view::npos ? text.substr(start) : text.substr(start, separator - start));
    if(!token.empty()) {
      tokens.push_back(token);
    }
    if(separator == std::string_view::npos) {
      break;
    }
    start = separator + 1;
  }

  return tokens;
}

inline std::string attributeValueToString(const chart_data::AttributeValue &value)
{
  return std::visit(
    [](const auto &typedValue) -> std::string {
      using T = std::decay_t<decltype(typedValue)>;
      if constexpr(std::is_same_v<T, std::int64_t>) {
        return std::to_string(typedValue);
      } else if constexpr(std::is_same_v<T, double>) {
        return std::to_string(typedValue);
      } else if constexpr(std::is_same_v<T, std::string>) {
        return typedValue;
      } else {
        std::string joined;
        for(std::size_t i = 0; i < typedValue.size(); ++i) {
          if(i != 0) {
            joined += ",";
          }
          if constexpr(std::is_same_v<typename T::value_type, std::string>) {
            joined += typedValue[i];
          } else {
            joined += std::to_string(typedValue[i]);
          }
        }
        return joined;
      }
    },
    value);
}

inline chart_data::AttributeStringList attributeValueToStringList(const chart_data::AttributeValue &value)
{
  return std::visit(
    [](const auto &typedValue) -> chart_data::AttributeStringList {
      using T = std::decay_t<decltype(typedValue)>;
      if constexpr(std::is_same_v<T, std::string>) {
        return {typedValue};
      } else if constexpr(std::is_same_v<T, chart_data::AttributeStringList>) {
        return typedValue;
      } else if constexpr(std::is_same_v<T, std::int64_t>) {
        return {std::to_string(typedValue)};
      } else if constexpr(std::is_same_v<T, double>) {
        return {std::to_string(typedValue)};
      } else {
        chart_data::AttributeStringList result;
        result.reserve(typedValue.size());
        for(const auto &entry : typedValue) {
          result.push_back(std::to_string(entry));
        }
        return result;
      }
    },
    value);
}

inline bool attributeValueIsIntegralFamily(const chart_data::AttributeValue &value) noexcept
{
  return std::holds_alternative<std::int64_t>(value)
      || std::holds_alternative<chart_data::AttributeIntList>(value);
}

inline bool attributeValueIsNumericFamily(const chart_data::AttributeValue &value) noexcept
{
  return attributeValueIsIntegralFamily(value)
      || std::holds_alternative<double>(value)
      || std::holds_alternative<chart_data::AttributeDoubleList>(value);
}

inline chart_data::AttributeIntList attributeValueToIntList(const chart_data::AttributeValue &value)
{
  return std::visit(
    [](const auto &typedValue) -> chart_data::AttributeIntList {
      using T = std::decay_t<decltype(typedValue)>;
      if constexpr(std::is_same_v<T, std::int64_t>) {
        return {typedValue};
      } else if constexpr(std::is_same_v<T, chart_data::AttributeIntList>) {
        return typedValue;
      } else {
        return {};
      }
    },
    value);
}

inline chart_data::AttributeDoubleList attributeValueToDoubleList(const chart_data::AttributeValue &value)
{
  return std::visit(
    [](const auto &typedValue) -> chart_data::AttributeDoubleList {
      using T = std::decay_t<decltype(typedValue)>;
      if constexpr(std::is_same_v<T, std::int64_t>) {
        return {static_cast<double>(typedValue)};
      } else if constexpr(std::is_same_v<T, double>) {
        return {typedValue};
      } else if constexpr(std::is_same_v<T, chart_data::AttributeIntList>) {
        chart_data::AttributeDoubleList result;
        result.reserve(typedValue.size());
        for(const auto entry : typedValue) {
          result.push_back(static_cast<double>(entry));
        }
        return result;
      } else if constexpr(std::is_same_v<T, chart_data::AttributeDoubleList>) {
        return typedValue;
      } else {
        return {};
      }
    },
    value);
}

} // namespace detail

inline chart_data::AttributeValue decodeS57AttributeValue(std::string value)
{
  const auto tokens = detail::splitListTokens(value);
  if(tokens.size() > 1U) {
    bool allIntegral = true;
    bool allNumeric = true;
    chart_data::AttributeIntList integralValues;
    chart_data::AttributeDoubleList numericValues;
    chart_data::AttributeStringList stringValues;
    integralValues.reserve(tokens.size());
    numericValues.reserve(tokens.size());
    stringValues.reserve(tokens.size());

    for(const auto &token : tokens) {
      std::int64_t integralValue = 0;
      double numericValue = 0.0;
      if(detail::tryParseInt64(token, integralValue)) {
        integralValues.push_back(integralValue);
        numericValues.push_back(static_cast<double>(integralValue));
        stringValues.push_back(token);
        continue;
      }

      allIntegral = false;
      if(detail::tryParseDouble(token, numericValue)) {
        numericValues.push_back(numericValue);
        stringValues.push_back(token);
        continue;
      }

      allNumeric = false;
      stringValues.push_back(token);
    }

    if(allIntegral) {
      return integralValues;
    }
    if(allNumeric) {
      return numericValues;
    }
    return stringValues;
  }

  const auto trimmed = detail::trimCopy(value);
  std::int64_t integralValue = 0;
  if(detail::tryParseInt64(trimmed, integralValue)) {
    return integralValue;
  }

  double numericValue = 0.0;
  if(detail::tryParseDouble(trimmed, numericValue)) {
    return numericValue;
  }

  return trimmed;
}

inline void mergeS57AttributeValue(
  std::unordered_map<std::string, chart_data::AttributeValue> &attributes,
  std::string key,
  chart_data::AttributeValue value)
{
  const auto it = attributes.find(key);
  if(it == attributes.end()) {
    attributes.insert_or_assign(std::move(key), std::move(value));
    return;
  }

  if(detail::attributeValueIsIntegralFamily(it->second)
     && detail::attributeValueIsIntegralFamily(value)) {
    auto merged = detail::attributeValueToIntList(it->second);
    const auto incoming = detail::attributeValueToIntList(value);
    merged.insert(merged.end(), incoming.begin(), incoming.end());
    it->second = std::move(merged);
    return;
  }

  if(detail::attributeValueIsNumericFamily(it->second)
     && detail::attributeValueIsNumericFamily(value)) {
    auto merged = detail::attributeValueToDoubleList(it->second);
    const auto incoming = detail::attributeValueToDoubleList(value);
    merged.insert(merged.end(), incoming.begin(), incoming.end());
    it->second = std::move(merged);
    return;
  }

  auto merged = detail::attributeValueToStringList(it->second);
  const auto incoming = detail::attributeValueToStringList(value);
  merged.insert(merged.end(), incoming.begin(), incoming.end());
  it->second = std::move(merged);
}

} // namespace chart_view::runtime::s57

#endif
