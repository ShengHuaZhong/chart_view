#include "opencpn_chartsymbols_parser.hpp"

#include <QFile>
#include <QString>
#include <QXmlStreamAttributes>
#include <QXmlStreamReader>

#include <algorithm>
#include <array>
#include <cctype>
#include <string_view>
#include <utility>

namespace chart_view::runtime::portrayal {

namespace {

std::string toStdString(const QString &value)
{
  return value.trimmed().toStdString();
}

int parseInt(const QString &value, int fallback = 0)
{
  bool ok = false;
  const auto parsed = value.trimmed().toInt(&ok);
  return ok ? parsed : fallback;
}

std::uint32_t parseUInt(const QString &value, std::uint32_t fallback = 0U)
{
  bool ok = false;
  const auto parsed = value.trimmed().toUInt(&ok);
  return ok ? parsed : fallback;
}

S52PaletteId paletteFromTableName(std::string_view tableName) noexcept
{
  auto startsWithInsensitive = [&](std::string_view prefix) {
    if(tableName.size() < prefix.size()) {
      return false;
    }

    for(std::size_t index = 0; index < prefix.size(); ++index) {
      const auto lhs = static_cast<char>(std::toupper(static_cast<unsigned char>(tableName[index])));
      const auto rhs = static_cast<char>(std::toupper(static_cast<unsigned char>(prefix[index])));
      if(lhs != rhs) {
        return false;
      }
    }

    return true;
  };

  if(startsWithInsensitive("DUSK")) {
    return S52PaletteId::kDusk;
  }

  if(startsWithInsensitive("NIGHT")) {
    return S52PaletteId::kNight;
  }

  return S52PaletteId::kDay;
}

chart_data::GeometryType geometryTypeFromText(std::string_view value) noexcept
{
  if(value == "Line") {
    return chart_data::GeometryType::kLine;
  }

  if(value == "Area") {
    return chart_data::GeometryType::kArea;
  }

  return chart_data::GeometryType::kPoint;
}

int defaultDisplayPriority(chart_data::GeometryType geometryType) noexcept
{
  switch(geometryType) {
  case chart_data::GeometryType::kPoint:
    return 300;
  case chart_data::GeometryType::kLine:
    return 200;
  case chart_data::GeometryType::kArea:
    return 100;
  }

  return 0;
}

S52SourceGraphicMetrics parseMetrics(const QXmlStreamAttributes &attributes)
{
  return {
    parseInt(attributes.value("width").toString()),
    parseInt(attributes.value("height").toString()),
  };
}

void parseColorTable(QXmlStreamReader &xml, S52SourceCatalog &catalog)
{
  const auto tableName = toStdString(xml.attributes().value("name").toString());
  const auto palette = paletteFromTableName(tableName);
  std::string graphicsFile;

  while(xml.readNextStartElement()) {
    if(xml.name() == u"graphics-file") {
      graphicsFile = toStdString(xml.attributes().value("name").toString());
      xml.skipCurrentElement();
      continue;
    }

    if(xml.name() == u"color") {
      S52SourceColor color;
      color.palette = palette;
      color.token = toStdString(xml.attributes().value("name").toString());
      color.color = {
        static_cast<std::uint8_t>(parseInt(xml.attributes().value("r").toString())),
        static_cast<std::uint8_t>(parseInt(xml.attributes().value("g").toString())),
        static_cast<std::uint8_t>(parseInt(xml.attributes().value("b").toString())),
        255U,
      };
      color.tableName = tableName;
      color.graphicsFile = graphicsFile;
      catalog.colors.push_back(std::move(color));
      xml.skipCurrentElement();
      continue;
    }

    xml.skipCurrentElement();
  }
}

void parseSymbol(QXmlStreamReader &xml, S52SourceCatalog &catalog)
{
  S52SourcePointSymbol symbol;
  symbol.sourceRcid = toStdString(xml.attributes().value("RCID").toString());

  while(xml.readNextStartElement()) {
    if(xml.name() == u"name") {
      symbol.assetId = toStdString(xml.readElementText(QXmlStreamReader::SkipChildElements));
    } else if(xml.name() == u"description") {
      symbol.description = toStdString(xml.readElementText(QXmlStreamReader::SkipChildElements));
    } else if(xml.name() == u"bitmap") {
      symbol.bitmapMetrics = parseMetrics(xml.attributes());
      if(symbol.radius <= 0) {
        symbol.radius = std::max(symbol.bitmapMetrics.width, symbol.bitmapMetrics.height) / 4;
      }
      xml.skipCurrentElement();
    } else if(xml.name() == u"vector") {
      symbol.vectorMetrics = parseMetrics(xml.attributes());
      xml.skipCurrentElement();
    } else if(xml.name() == u"color-ref") {
      symbol.colorToken = toStdString(xml.readElementText(QXmlStreamReader::SkipChildElements));
    } else if(xml.name() == u"definition") {
      symbol.definition = toStdString(xml.readElementText(QXmlStreamReader::SkipChildElements));
    } else if(xml.name() == u"prefer-bitmap") {
      const auto text = xml.readElementText(QXmlStreamReader::SkipChildElements).trimmed();
      symbol.preferBitmap = text.compare("yes", Qt::CaseInsensitive) == 0 || text == "1";
    } else {
      xml.skipCurrentElement();
    }
  }

  if(symbol.radius <= 0) {
    const auto dominantSize = std::max(
      std::max(symbol.bitmapMetrics.width, symbol.bitmapMetrics.height),
      std::max(symbol.vectorMetrics.width, symbol.vectorMetrics.height));
    symbol.radius = dominantSize > 0 ? std::max(2, dominantSize / 8) : 4;
  }

  if(!symbol.assetId.empty()) {
    catalog.pointSymbols.push_back(std::move(symbol));
  }
}

void parseLineStyle(QXmlStreamReader &xml, S52SourceCatalog &catalog)
{
  S52SourceLineStyle lineStyle;
  lineStyle.sourceRcid = toStdString(xml.attributes().value("RCID").toString());

  while(xml.readNextStartElement()) {
    if(xml.name() == u"name") {
      lineStyle.assetId = toStdString(xml.readElementText(QXmlStreamReader::SkipChildElements));
    } else if(xml.name() == u"description") {
      lineStyle.description = toStdString(xml.readElementText(QXmlStreamReader::SkipChildElements));
    } else if(xml.name() == u"vector") {
      lineStyle.vectorMetrics = parseMetrics(xml.attributes());
      if(lineStyle.thickness <= 0) {
        lineStyle.thickness = std::max(1, lineStyle.vectorMetrics.height / 250);
      }
      xml.skipCurrentElement();
    } else if(xml.name() == u"color-ref") {
      lineStyle.colorToken = toStdString(xml.readElementText(QXmlStreamReader::SkipChildElements));
    } else if(xml.name() == u"HPGL") {
      lineStyle.hpgl = toStdString(xml.readElementText(QXmlStreamReader::SkipChildElements));
    } else {
      xml.skipCurrentElement();
    }
  }

  if(lineStyle.thickness <= 0) {
    lineStyle.thickness = 2;
  }

  if(!lineStyle.assetId.empty()) {
    catalog.lineStyles.push_back(std::move(lineStyle));
  }
}

void parsePattern(QXmlStreamReader &xml, S52SourceCatalog &catalog)
{
  S52SourceAreaPattern pattern;
  pattern.sourceRcid = toStdString(xml.attributes().value("RCID").toString());

  while(xml.readNextStartElement()) {
    if(xml.name() == u"name") {
      pattern.assetId = toStdString(xml.readElementText(QXmlStreamReader::SkipChildElements));
    } else if(xml.name() == u"description") {
      pattern.description = toStdString(xml.readElementText(QXmlStreamReader::SkipChildElements));
    } else if(xml.name() == u"definition") {
      pattern.definition = toStdString(xml.readElementText(QXmlStreamReader::SkipChildElements));
    } else if(xml.name() == u"filltype") {
      pattern.fillType = toStdString(xml.readElementText(QXmlStreamReader::SkipChildElements));
    } else if(xml.name() == u"spacing") {
      pattern.spacing = toStdString(xml.readElementText(QXmlStreamReader::SkipChildElements));
    } else if(xml.name() == u"bitmap") {
      pattern.bitmapMetrics = parseMetrics(xml.attributes());
      xml.skipCurrentElement();
    } else if(xml.name() == u"vector") {
      pattern.vectorMetrics = parseMetrics(xml.attributes());
      xml.skipCurrentElement();
    } else if(xml.name() == u"color-ref") {
      pattern.primaryColorToken = toStdString(xml.readElementText(QXmlStreamReader::SkipChildElements));
      pattern.fillColorToken = pattern.primaryColorToken;
    } else if(xml.name() == u"HPGL") {
      pattern.hpgl = toStdString(xml.readElementText(QXmlStreamReader::SkipChildElements));
    } else {
      xml.skipCurrentElement();
    }
  }

  if(!pattern.assetId.empty()) {
    catalog.areaPatterns.push_back(std::move(pattern));
  }
}

void parseLookup(QXmlStreamReader &xml, S52SourceCatalog &catalog)
{
  S52SourceLookupRow row;
  row.sourceLookupId = toStdString(xml.attributes().value("id").toString());
  row.sourceRcid = toStdString(xml.attributes().value("RCID").toString());
  row.objectAcronym = toStdString(xml.attributes().value("name").toString());

  while(xml.readNextStartElement()) {
    if(xml.name() == u"type") {
      row.geometryTypeText = toStdString(xml.readElementText(QXmlStreamReader::SkipChildElements));
      row.geometryType = geometryTypeFromText(row.geometryTypeText);
      row.displayPriority = defaultDisplayPriority(row.geometryType);
    } else if(xml.name() == u"disp-prio") {
      row.displayPriorityText = toStdString(xml.readElementText(QXmlStreamReader::SkipChildElements));
    } else if(xml.name() == u"radar-prio") {
      row.radarPriorityText = toStdString(xml.readElementText(QXmlStreamReader::SkipChildElements));
    } else if(xml.name() == u"table-name") {
      row.tableName = toStdString(xml.readElementText(QXmlStreamReader::SkipChildElements));
    } else if(xml.name() == u"instruction") {
      row.rawInstruction = toStdString(xml.readElementText(QXmlStreamReader::SkipChildElements));
    } else if(xml.name() == u"display-cat") {
      row.displayCategory = toStdString(xml.readElementText(QXmlStreamReader::SkipChildElements));
    } else if(xml.name() == u"comment") {
      row.comment = toStdString(xml.readElementText(QXmlStreamReader::SkipChildElements));
      row.viewGroup = parseUInt(QString::fromStdString(row.comment), 0U);
    } else if(xml.name() == u"attrib-code") {
      row.attributeCodes.push_back(toStdString(xml.readElementText(QXmlStreamReader::SkipChildElements)));
    } else {
      xml.skipCurrentElement();
    }
  }

  if(!row.objectAcronym.empty()) {
    catalog.lookupRows.push_back(std::move(row));
  }
}

OpenCpnChartsymbolsParseResult failureResult(std::string error)
{
  OpenCpnChartsymbolsParseResult result;
  result.ok = false;
  result.error = std::move(error);
  return result;
}

} // namespace

OpenCpnChartsymbolsParseResult OpenCpnChartsymbolsParser::parseBundle(const OpenCpnS52ResourceBundle &bundle)
{
  const auto chartsymbolsPath = bundle.chartsymbolsXmlPath();
  QFile file(QString::fromStdWString(chartsymbolsPath.wstring()));
  if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return failureResult("failed to open chartsymbols.xml");
  }

  QXmlStreamReader xml(&file);
  if(!xml.readNextStartElement()) {
    return failureResult("chartsymbols.xml does not contain a root element");
  }

  if(xml.name() != u"chartsymbols") {
    return failureResult("unexpected chartsymbols.xml root element");
  }

  OpenCpnChartsymbolsParseResult result;
  while(xml.readNextStartElement()) {
    if(xml.name() == u"color-tables") {
      while(xml.readNextStartElement()) {
        if(xml.name() == u"color-table") {
          parseColorTable(xml, result.catalog);
        } else {
          xml.skipCurrentElement();
        }
      }
    } else if(xml.name() == u"lookups") {
      while(xml.readNextStartElement()) {
        if(xml.name() == u"lookup") {
          parseLookup(xml, result.catalog);
        } else {
          xml.skipCurrentElement();
        }
      }
    } else if(xml.name() == u"line-styles") {
      while(xml.readNextStartElement()) {
        if(xml.name() == u"line-style") {
          parseLineStyle(xml, result.catalog);
        } else {
          xml.skipCurrentElement();
        }
      }
    } else if(xml.name() == u"patterns") {
      while(xml.readNextStartElement()) {
        if(xml.name() == u"pattern") {
          parsePattern(xml, result.catalog);
        } else {
          xml.skipCurrentElement();
        }
      }
    } else if(xml.name() == u"symbols") {
      while(xml.readNextStartElement()) {
        if(xml.name() == u"symbol") {
          parseSymbol(xml, result.catalog);
        } else {
          xml.skipCurrentElement();
        }
      }
    } else {
      xml.skipCurrentElement();
    }
  }

  if(xml.hasError()) {
    return failureResult(("xml parse error: " + xml.errorString()).toStdString());
  }

  result.ok = true;
  return result;
}

} // namespace chart_view::runtime::portrayal
