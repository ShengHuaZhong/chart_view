#include "s52_instruction_string_parser.hpp"

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace chart_view::runtime::portrayal {

namespace {

std::string trimCopy(std::string_view value)
{
  while(!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
    value.remove_prefix(1);
  }

  while(!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
    value.remove_suffix(1);
  }

  return std::string(value);
}

std::string upperAscii(std::string_view value)
{
  std::string normalized;
  normalized.reserve(value.size());
  for(const auto ch : value) {
    normalized.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
  }
  return normalized;
}

std::string normalizeInstructionToken(std::string_view value)
{
  std::string normalized;
  normalized.reserve(value.size());
  for(const auto ch : value) {
    if(std::isalnum(static_cast<unsigned char>(ch)) != 0) {
      normalized.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
    } else if(ch == '_' || ch == '-' || ch == '.') {
      normalized.push_back('_');
    }
  }
  return normalized;
}

std::vector<std::string> splitTopLevel(std::string_view value, char delimiter)
{
  std::vector<std::string> parts;
  std::size_t start = 0;
  int parenDepth = 0;
  bool inQuote = false;

  for(std::size_t index = 0; index < value.size(); ++index) {
    const auto ch = value[index];
    if(ch == '\'') {
      inQuote = !inQuote;
      continue;
    }

    if(inQuote) {
      continue;
    }

    if(ch == '(') {
      ++parenDepth;
      continue;
    }

    if(ch == ')') {
      parenDepth = std::max(parenDepth - 1, 0);
      continue;
    }

    if(ch == delimiter && parenDepth == 0) {
      parts.push_back(trimCopy(value.substr(start, index - start)));
      start = index + 1;
    }
  }

  if(start <= value.size()) {
    parts.push_back(trimCopy(value.substr(start)));
  }

  std::erase_if(parts, [](const std::string &part) { return part.empty(); });
  return parts;
}

std::vector<std::string> splitArguments(std::string_view value)
{
  std::vector<std::string> args;
  std::size_t start = 0;
  int parenDepth = 0;
  bool inQuote = false;

  for(std::size_t index = 0; index < value.size(); ++index) {
    const auto ch = value[index];
    if(ch == '\'') {
      inQuote = !inQuote;
      continue;
    }

    if(inQuote) {
      continue;
    }

    if(ch == '(') {
      ++parenDepth;
      continue;
    }

    if(ch == ')') {
      parenDepth = std::max(parenDepth - 1, 0);
      continue;
    }

    if(ch == ',' && parenDepth == 0) {
      args.push_back(trimCopy(value.substr(start, index - start)));
      start = index + 1;
    }
  }

  if(start <= value.size()) {
    args.push_back(trimCopy(value.substr(start)));
  }

  for(auto &arg : args) {
    if(arg.size() >= 2 && arg.front() == '\'' && arg.back() == '\'') {
      arg = arg.substr(1, arg.size() - 2);
    }
  }

  return args;
}

std::string makeSyntheticLineAssetId(const std::vector<std::string> &args)
{
  std::string assetId = "LS";
  for(const auto &arg : args) {
    const auto normalizedArg = normalizeInstructionToken(arg);
    if(normalizedArg.empty()) {
      continue;
    }

    assetId += "_";
    assetId += normalizedArg;
  }

  return assetId;
}

std::string normalizeAttributeKey(std::string_view value)
{
  auto normalized = upperAscii(trimCopy(value));
  normalized.erase(
    std::remove_if(
      normalized.begin(),
      normalized.end(),
      [](unsigned char ch) {
        return !(std::isalnum(ch) != 0 || ch == '_' || ch == '$');
      }),
    normalized.end());
  return normalized;
}

std::optional<S52SourceLookupInstruction> parseStatement(std::string_view statement)
{
  const auto open = statement.find('(');
  const auto close = statement.rfind(')');
  if(open == std::string_view::npos || close == std::string_view::npos || close <= open) {
    return std::nullopt;
  }

  const auto opcode = upperAscii(trimCopy(statement.substr(0, open)));
  const auto args = splitArguments(statement.substr(open + 1, close - open - 1));

  if(opcode == "SY") {
    if(args.empty()) {
      return std::nullopt;
    }
    return S52SourceLookupInstruction{
      S52InstructionType::kPointSymbol,
      args.front(),
      {},
      {}};
  }

  if(opcode == "LS") {
    if(args.empty()) {
      return std::nullopt;
    }
    return S52SourceLookupInstruction{
      S52InstructionType::kLineStyle,
      makeSyntheticLineAssetId(args),
      {},
      {}};
  }

  if(opcode == "LC") {
    if(args.empty()) {
      return std::nullopt;
    }
    return S52SourceLookupInstruction{
      S52InstructionType::kLineStyle,
      args.front(),
      {},
      {}};
  }

  if(opcode == "AP") {
    if(args.empty()) {
      return std::nullopt;
    }
    return S52SourceLookupInstruction{
      S52InstructionType::kAreaPattern,
      args.front(),
      {},
      {}};
  }

  if(opcode == "TE") {
    const auto attributeKey =
      args.size() > 1 ? normalizeAttributeKey(args[1]) : std::string("OBJNAM");
    return S52SourceLookupInstruction{
      S52InstructionType::kTextLabel,
      "TEXT01",
      "text/default",
      attributeKey};
  }

  if(opcode == "TX") {
    const auto attributeKey =
      args.empty() ? std::string("OBJNAM") : normalizeAttributeKey(args.front());
    return S52SourceLookupInstruction{
      S52InstructionType::kTextLabel,
      "TEXT01",
      "text/default",
      attributeKey};
  }

  if(opcode == "CS") {
    if(args.empty()) {
      return std::nullopt;
    }
    const auto conditionId = normalizeInstructionToken(args.front());
    if(conditionId.empty()) {
      return std::nullopt;
    }

    return S52SourceLookupInstruction{
      S52InstructionType::kConditional,
      conditionId,
      conditionId,
      {}};
  }

  return std::nullopt;
}

} // namespace

S52InstructionStringParseResult S52InstructionStringParser::parse(std::string_view rawInstruction)
{
  S52InstructionStringParseResult result;
  for(const auto &statement : splitTopLevel(rawInstruction, ';')) {
    if(statement.empty()) {
      continue;
    }

    if(const auto parsed = parseStatement(statement); parsed.has_value()) {
      result.instructions.push_back(*parsed);
    } else {
      result.unsupportedStatements.push_back(statement);
    }
  }

  return result;
}

} // namespace chart_view::runtime::portrayal
