#ifndef CHART_VIEW_RUNTIME_S57_ISO8211_HPP
#define CHART_VIEW_RUNTIME_S57_ISO8211_HPP

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace chart_view::runtime::s57::iso8211 {

// ISO 8211 leader (first 24 bytes of every record).
struct Leader
{
  std::uint32_t recordLength{0};
  char interchangeLevel{'0'};
  char leaderIdentifier{' '};
  char inlineCodeExtension{' '};
  char versionNumber{'0'};
  char applicationIndicator{' '};
  std::uint32_t fieldControlLength{0};
  std::uint32_t baseAddressOfFieldArea{0};
  char extCharSetIndicator[3]{' ', ' ', ' '};
  std::uint32_t sizeOfFieldLength{0};
  std::uint32_t sizeOfFieldPosition{0};
  std::uint32_t sizeOfFieldTag{0};
};

// A single directory entry in a record.
struct DirectoryEntry
{
  std::string tag;
  std::uint32_t fieldLength{0};
  std::uint32_t fieldPosition{0};
};

// A single subfield value.
struct SubfieldValue
{
  std::string tag;
  std::string data;  // raw bytes (may be binary)
  bool isBinary{false};
};

// A single field within a record.
struct Field
{
  std::string tag;
  std::vector<std::uint8_t> data; // raw field data
};

// Field definition from the DDR.
struct FieldDefinition
{
  std::string tag;
  std::string name;
  std::string arrayDescriptor;
  std::string formatControls;
  std::vector<std::pair<std::string, char>> subfields; // (label, type: A/I/R/B/b)
};

// A parsed ISO 8211 record.
struct Record
{
  Leader leader;
  std::vector<DirectoryEntry> directory;
  std::vector<Field> fields;
};

// Parsed ISO 8211 module (file).
struct Module
{
  Record ddr; // Data Descriptive Record
  std::vector<Record> dataRecords;
  std::vector<FieldDefinition> fieldDefinitions;
};

// Parse an ISO 8211 file from a byte buffer.
// Returns empty module with error info on failure.
struct ParseResult
{
  bool ok{false};
  std::string error;
  Module module;
};

[[nodiscard]] ParseResult parse(std::span<const std::uint8_t> data);

}// namespace chart_view::runtime::s57::iso8211

#endif
