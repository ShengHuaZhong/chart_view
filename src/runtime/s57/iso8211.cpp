#include "iso8211.hpp"

#include <algorithm>
#include <charconv>
#include <cstring>

namespace chart_view::runtime::s57::iso8211 {

namespace {

// Parse an ASCII integer from a fixed-width field.
std::uint32_t parseAsciiInt(const std::uint8_t *data, std::uint32_t width)
{
  // Skip leading spaces.
  std::uint32_t val = 0;
  for (std::uint32_t i = 0; i < width; ++i) {
    char c = static_cast<char>(data[i]);
    if (c >= '0' && c <= '9')
      val = val * 10 + static_cast<std::uint32_t>(c - '0');
  }
  return val;
}

// Parse leader from first 24 bytes.
Leader parseLeader(const std::uint8_t *data)
{
  Leader l;
  l.recordLength = parseAsciiInt(data, 5);
  l.interchangeLevel = static_cast<char>(data[5]);
  l.leaderIdentifier = static_cast<char>(data[6]);
  l.inlineCodeExtension = static_cast<char>(data[7]);
  l.versionNumber = static_cast<char>(data[8]);
  l.applicationIndicator = static_cast<char>(data[9]);
  l.fieldControlLength = parseAsciiInt(data + 10, 2);
  l.baseAddressOfFieldArea = parseAsciiInt(data + 12, 5);
  l.extCharSetIndicator[0] = static_cast<char>(data[17]);
  l.extCharSetIndicator[1] = static_cast<char>(data[18]);
  l.extCharSetIndicator[2] = static_cast<char>(data[19]);
  l.sizeOfFieldLength = parseAsciiInt(data + 20, 1);
  l.sizeOfFieldPosition = parseAsciiInt(data + 21, 1);
  // data[22] is reserved
  l.sizeOfFieldTag = parseAsciiInt(data + 23, 1);
  return l;
}

// Parse directory entries from a record.
std::vector<DirectoryEntry> parseDirectory(
  const std::uint8_t *data,
  std::uint32_t dirStart,
  std::uint32_t baseAddr,
  const Leader &leader)
{
  std::vector<DirectoryEntry> entries;
  const std::uint32_t entrySize = leader.sizeOfFieldTag + leader.sizeOfFieldLength + leader.sizeOfFieldPosition;

  std::uint32_t pos = dirStart;
  while (pos + entrySize <= baseAddr) {
    // Field terminator check.
    if (data[pos] == 0x1E) break;

    DirectoryEntry entry;
    entry.tag.assign(reinterpret_cast<const char *>(data + pos), leader.sizeOfFieldTag);
    pos += leader.sizeOfFieldTag;

    entry.fieldLength = parseAsciiInt(data + pos, leader.sizeOfFieldLength);
    pos += leader.sizeOfFieldLength;

    entry.fieldPosition = parseAsciiInt(data + pos, leader.sizeOfFieldPosition);
    pos += leader.sizeOfFieldPosition;

    entries.push_back(std::move(entry));
  }

  return entries;
}

// Parse a single record from the buffer at offset.
// Returns the record and advances offset past it.
bool parseRecord(
  std::span<const std::uint8_t> data,
  std::size_t &offset,
  Record &rec)
{
  if (offset + 24 > data.size()) return false;

  rec.leader = parseLeader(data.data() + offset);
  if (rec.leader.recordLength == 0) return false;
  if (offset + rec.leader.recordLength > data.size()) return false;

  const std::uint8_t *recordBase = data.data() + offset;

  // Parse directory (starts at byte 24, ends at baseAddressOfFieldArea - 1).
  rec.directory = parseDirectory(recordBase, 24, rec.leader.baseAddressOfFieldArea, rec.leader);

  // Parse fields.
  rec.fields.reserve(rec.directory.size());
  for (const auto &dirEntry : rec.directory) {
    Field f;
    f.tag = dirEntry.tag;

    std::uint32_t fieldStart = rec.leader.baseAddressOfFieldArea + dirEntry.fieldPosition;
    std::uint32_t fieldLen = dirEntry.fieldLength;

    if (fieldStart + fieldLen > rec.leader.recordLength) {
      // Clamp to record boundary.
      fieldLen = rec.leader.recordLength - fieldStart;
    }

    f.data.assign(recordBase + fieldStart, recordBase + fieldStart + fieldLen);

    rec.fields.push_back(std::move(f));
  }

  offset += rec.leader.recordLength;
  return true;
}

// Parse field definitions from the DDR.
std::vector<FieldDefinition> parseDDRFieldDefs(const Record &ddr)
{
  std::vector<FieldDefinition> defs;

  for (std::size_t i = 0; i < ddr.fields.size(); ++i) {
    const auto &field = ddr.fields[i];
    if (field.tag == "0001") continue; // file control field

    FieldDefinition def;
    def.tag = field.tag;

    if (field.data.empty()) {
      defs.push_back(std::move(def));
      continue;
    }

    // Field data format:
    // [field controls] (fieldControlLength bytes)
    // [field name] 0x1F [array descriptor] 0x1F [format controls] 0x1E
    const auto &d = field.data;
    std::size_t pos = 0;

    // Skip field controls (typically 2 bytes for DR fields, variable for DDR).
    if (ddr.leader.fieldControlLength > 0 && d.size() > ddr.leader.fieldControlLength) {
      pos = ddr.leader.fieldControlLength;
    }

    // Find field name (up to first 0x1F unit terminator).
    std::size_t nameStart = pos;
    while (pos < d.size() && d[pos] != 0x1F && d[pos] != 0x1E) ++pos;
    def.name.assign(reinterpret_cast<const char *>(d.data() + nameStart), pos - nameStart);

    // Array descriptor.
    if (pos < d.size() && d[pos] == 0x1F) {
      ++pos;
      std::size_t adStart = pos;
      while (pos < d.size() && d[pos] != 0x1F && d[pos] != 0x1E) ++pos;
      def.arrayDescriptor.assign(reinterpret_cast<const char *>(d.data() + adStart), pos - adStart);
    }

    // Format controls.
    if (pos < d.size() && d[pos] == 0x1F) {
      ++pos;
      std::size_t fcStart = pos;
      while (pos < d.size() && d[pos] != 0x1E) ++pos;
      def.formatControls.assign(reinterpret_cast<const char *>(d.data() + fcStart), pos - fcStart);
    }

    // Parse subfield labels from array descriptor.
    // Format: "LABEL1!LABEL2!LABEL3" or "*LABEL1!LABEL2" (repeating).
    if (!def.arrayDescriptor.empty()) {
      std::string ad = def.arrayDescriptor;
      // Remove leading '*' for repeating fields.
      if (ad[0] == '*') ad = ad.substr(1);

      std::size_t sp = 0;
      while (sp < ad.size()) {
        std::size_t sep = ad.find('!', sp);
        if (sep == std::string::npos) sep = ad.size();
        std::string label = ad.substr(sp, sep - sp);
        if (!label.empty()) {
          def.subfields.emplace_back(std::move(label), 'A'); // default type
        }
        sp = sep + 1;
      }
    }

    // Try to assign types from format controls.
    // Format: "(A(xx),I(xx),R(xx),B(xx),b(xx),...)"
    if (!def.formatControls.empty() && !def.subfields.empty()) {
      std::string fc = def.formatControls;
      // Strip outer parens.
      if (fc.front() == '(' && fc.back() == ')') {
        fc = fc.substr(1, fc.size() - 2);
      }

      // Parse comma-separated format items.
      std::size_t sfIdx = 0;
      std::size_t fp = 0;
      while (fp < fc.size() && sfIdx < def.subfields.size()) {
        char typeChar = fc[fp];
        if (typeChar == 'A' || typeChar == 'I' || typeChar == 'R' ||
            typeChar == 'B' || typeChar == 'b') {
          def.subfields[sfIdx].second = typeChar;
          ++sfIdx;
        }
        // Skip to next comma.
        std::size_t nextComma = fc.find(',', fp);
        if (nextComma == std::string::npos) break;
        fp = nextComma + 1;
      }
    }

    defs.push_back(std::move(def));
  }

  return defs;
}

}// namespace

ParseResult parse(std::span<const std::uint8_t> data)
{
  ParseResult result;

  if (data.size() < 24) {
    result.error = "file too small for ISO 8211 leader";
    return result;
  }

  // Parse DDR (first record).
  std::size_t offset = 0;
  if (!parseRecord(data, offset, result.module.ddr)) {
    result.error = "failed to parse DDR";
    return result;
  }

  if (result.module.ddr.leader.leaderIdentifier != 'L') {
    result.error = "first record is not a DDR (expected 'L')";
    return result;
  }

  // Parse field definitions from DDR.
  result.module.fieldDefinitions = parseDDRFieldDefs(result.module.ddr);

  // Parse data records.
  while (offset < data.size()) {
    Record rec;
    if (!parseRecord(data, offset, rec)) break;
    result.module.dataRecords.push_back(std::move(rec));
  }

  result.ok = true;
  return result;
}

}// namespace chart_view::runtime::s57::iso8211
