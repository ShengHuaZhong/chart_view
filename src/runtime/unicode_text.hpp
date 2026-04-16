#ifndef CHART_VIEW_RUNTIME_UNICODE_TEXT_HPP
#define CHART_VIEW_RUNTIME_UNICODE_TEXT_HPP

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

namespace chart_view::runtime::text {

struct UnicodeText
{
  std::string utf8;
  std::u32string codePoints;

  [[nodiscard]] bool empty() const noexcept { return codePoints.empty(); }
  [[nodiscard]] std::size_t size() const noexcept { return codePoints.size(); }
};

namespace detail {

inline constexpr char32_t kReplacementCodePoint = 0xFFFD;

inline bool isContinuationByte(unsigned char byte) noexcept
{
  return (byte & 0xC0U) == 0x80U;
}

inline char32_t sanitizeCodePoint(char32_t codePoint) noexcept
{
  if(codePoint > 0x10FFFFU || (codePoint >= 0xD800U && codePoint <= 0xDFFFU)) {
    return kReplacementCodePoint;
  }

  return codePoint;
}

inline void appendUtf8CodePoint(std::string &out, char32_t codePoint)
{
  const auto value = sanitizeCodePoint(codePoint);
  if(value <= 0x7FU) {
    out.push_back(static_cast<char>(value));
  } else if(value <= 0x7FFU) {
    out.push_back(static_cast<char>(0xC0U | ((value >> 6U) & 0x1FU)));
    out.push_back(static_cast<char>(0x80U | (value & 0x3FU)));
  } else if(value <= 0xFFFFU) {
    out.push_back(static_cast<char>(0xE0U | ((value >> 12U) & 0x0FU)));
    out.push_back(static_cast<char>(0x80U | ((value >> 6U) & 0x3FU)));
    out.push_back(static_cast<char>(0x80U | (value & 0x3FU)));
  } else {
    out.push_back(static_cast<char>(0xF0U | ((value >> 18U) & 0x07U)));
    out.push_back(static_cast<char>(0x80U | ((value >> 12U) & 0x3FU)));
    out.push_back(static_cast<char>(0x80U | ((value >> 6U) & 0x3FU)));
    out.push_back(static_cast<char>(0x80U | (value & 0x3FU)));
  }
}

}// namespace detail

inline UnicodeText decodeUtf8(
  std::string_view input,
  std::size_t maxCodePoints = std::numeric_limits<std::size_t>::max())
{
  UnicodeText result;
  result.utf8.reserve(input.size());
  result.codePoints.reserve((std::min)(input.size(), maxCodePoints));

  std::size_t index = 0;
  while(index < input.size() && result.codePoints.size() < maxCodePoints) {
    const auto lead = static_cast<unsigned char>(input[index]);
    char32_t codePoint = detail::kReplacementCodePoint;
    std::size_t bytesToConsume = 1;

    if(lead <= 0x7FU) {
      codePoint = static_cast<char32_t>(lead);
    } else if((lead & 0xE0U) == 0xC0U) {
      if(index + 1U < input.size()) {
        const auto b1 = static_cast<unsigned char>(input[index + 1U]);
        if(detail::isContinuationByte(b1)) {
          const auto decoded =
            static_cast<char32_t>(((lead & 0x1FU) << 6U) | (b1 & 0x3FU));
          if(decoded >= 0x80U) {
            codePoint = decoded;
            bytesToConsume = 2;
          }
        }
      }
    } else if((lead & 0xF0U) == 0xE0U) {
      if(index + 2U < input.size()) {
        const auto b1 = static_cast<unsigned char>(input[index + 1U]);
        const auto b2 = static_cast<unsigned char>(input[index + 2U]);
        if(detail::isContinuationByte(b1) && detail::isContinuationByte(b2)) {
          const auto decoded = static_cast<char32_t>(
            ((lead & 0x0FU) << 12U) | ((b1 & 0x3FU) << 6U) | (b2 & 0x3FU));
          if(decoded >= 0x800U
             && !(decoded >= 0xD800U && decoded <= 0xDFFFU)) {
            codePoint = decoded;
            bytesToConsume = 3;
          }
        }
      }
    } else if((lead & 0xF8U) == 0xF0U) {
      if(index + 3U < input.size()) {
        const auto b1 = static_cast<unsigned char>(input[index + 1U]);
        const auto b2 = static_cast<unsigned char>(input[index + 2U]);
        const auto b3 = static_cast<unsigned char>(input[index + 3U]);
        if(detail::isContinuationByte(b1)
           && detail::isContinuationByte(b2)
           && detail::isContinuationByte(b3)) {
          const auto decoded = static_cast<char32_t>(
            ((lead & 0x07U) << 18U) | ((b1 & 0x3FU) << 12U) | ((b2 & 0x3FU) << 6U)
            | (b3 & 0x3FU));
          if(decoded >= 0x10000U && decoded <= 0x10FFFFU) {
            codePoint = decoded;
            bytesToConsume = 4;
          }
        }
      }
    }

    codePoint = detail::sanitizeCodePoint(codePoint);
    result.codePoints.push_back(codePoint);
    detail::appendUtf8CodePoint(result.utf8, codePoint);
    index += bytesToConsume;
  }

  return result;
}

}// namespace chart_view::runtime::text

#endif
