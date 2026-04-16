#include <catch2/catch_test_macros.hpp>

#include "unicode_text.hpp"

namespace {

std::string utf8Harbor()
{
  return std::string("\xE6\xB8\xAF\xE5\x8F\xA3");
}

std::string utf8HarborA()
{
  return utf8Harbor() + "A";
}

}// namespace

TEST_CASE("Unicode text decodes UTF-8 by code point without splitting multibyte labels", "[text][unicode]")
{
  const auto decoded = chart_view::runtime::text::decodeUtf8(utf8HarborA(), 2);

  REQUIRE(decoded.utf8 == utf8Harbor());
  REQUIRE(decoded.codePoints.size() == 2);
  REQUIRE(decoded.codePoints[0] == 0x6E2FU);
  REQUIRE(decoded.codePoints[1] == 0x53E3U);
}

TEST_CASE("Unicode text replaces malformed UTF-8 with replacement code points", "[text][unicode]")
{
  const std::string malformed("\xE6\xB8");
  const auto decoded = chart_view::runtime::text::decodeUtf8(malformed);

  REQUIRE(decoded.codePoints.size() == 2);
  REQUIRE(decoded.codePoints[0] == 0xFFFDU);
  REQUIRE(decoded.codePoints[1] == 0xFFFDU);
}
