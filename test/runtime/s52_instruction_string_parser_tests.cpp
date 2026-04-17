#include <catch2/catch_test_macros.hpp>

#include "portrayal/s52_instruction_string_parser.hpp"

namespace {
using chart_view::runtime::portrayal::S52InstructionStringParser;
using chart_view::runtime::portrayal::S52InstructionType;
}

TEST_CASE("S52InstructionStringParser parses mixed symbol, line, conditional, and text statements",
          "[portrayal][s52][instruction_string]")
{
  const auto result = S52InstructionStringParser::parse(
    "SY(ACHARE02);LS(DASH,2,CHMGF);CS(RESTRN01);TE('%s','OBJNAM',3,1,2,'15110',1,0,CHBLK,29)");

  REQUIRE(result.unsupportedStatements.empty());
  REQUIRE(result.instructions.size() == 4);
  REQUIRE(result.instructions[0].type == S52InstructionType::kPointSymbol);
  REQUIRE(result.instructions[0].assetId == "ACHARE02");
  REQUIRE(result.instructions[1].type == S52InstructionType::kLineStyle);
  REQUIRE(result.instructions[1].assetId == "LS_DASH_2_CHMGF");
  REQUIRE(result.instructions[2].type == S52InstructionType::kConditional);
  REQUIRE(result.instructions[2].assetId == "RESTRN01");
  REQUIRE(result.instructions[3].type == S52InstructionType::kTextLabel);
  REQUIRE(result.instructions[3].assetId == "TEXT01");
  REQUIRE(result.instructions[3].styleKey == "text/default");
  REQUIRE(result.instructions[3].attributeKey == "OBJNAM");
}

TEST_CASE("S52InstructionStringParser extracts area, line-complex, and TX attribute instructions",
          "[portrayal][s52][instruction_string]")
{
  const auto result = S52InstructionStringParser::parse(
    "AC(NODTA);AP(PRTSUR01);LC(NAVARE51);TX(NOBJNM,2,1,2,'14106',-1,-1,CHBLK,21)");

  REQUIRE(result.instructions.size() == 3);
  REQUIRE(result.unsupportedStatements.size() == 1);
  REQUIRE(result.unsupportedStatements.front() == "AC(NODTA)");
  REQUIRE(result.instructions[0].type == S52InstructionType::kAreaPattern);
  REQUIRE(result.instructions[0].assetId == "PRTSUR01");
  REQUIRE(result.instructions[1].type == S52InstructionType::kLineStyle);
  REQUIRE(result.instructions[1].assetId == "NAVARE51");
  REQUIRE(result.instructions[2].type == S52InstructionType::kTextLabel);
  REQUIRE(result.instructions[2].attributeKey == "NOBJNM");
}
