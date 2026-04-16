#ifndef CHART_VIEW_RUNTIME_SENC_SENC_WRITER_HPP
#define CHART_VIEW_RUNTIME_SENC_SENC_WRITER_HPP

#include "senc_types.hpp"
#include "source_manifest.hpp"
#include "../chart_data/feature_chart_dataset.hpp"
#include "../s57/s57_source_model.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace chart_view::runtime::senc {

// Writes a structurally valid SENC v1 file from a FeatureChartDataset.
//
// Layout:
//   [FileHeader]                      (16 bytes)
//   [SectionDesc] x sectionCount      (16 bytes each)
//   [section payload 0]
//   [section payload 1]
//   ...
//
// All sections listed in SectionType are emitted in order.
// Empty / unused sections have size == 0 with a valid descriptor.
class SencWriter
{
public:
  SencWriter() = default;

  void setFormatVersion(std::uint32_t formatVersion) { m_formatVersion = formatVersion; }

  // Set an explicit source manifest to encode into the SENC file.
  // If not set, a minimal manifest is derived from dataset metadata.
  void setSourceManifest(SourceManifest manifest) { m_manifest = std::move(manifest); }

  void setS57SourceModel(s57::S57SourceModel sourceModel)
  {
    m_s57SourceModel = std::move(sourceModel);
  }

  // Serialize dataset to SENC v1 binary blob.
  [[nodiscard]] std::vector<std::uint8_t> write(const chart_data::FeatureChartDataset &dataset) const;

  // Convenience: serialize and write to file. Returns true on success.
  [[nodiscard]] bool writeToFile(const chart_data::FeatureChartDataset &dataset,
                                 const std::string &path) const;

private:
  // Build section payloads. Index matches SectionType ordinal.
  [[nodiscard]] std::vector<std::vector<std::uint8_t>> buildSectionPayloads(
    const chart_data::FeatureChartDataset &dataset) const;

  // Encode dataset metadata into SourceManifest payload.
  [[nodiscard]] std::vector<std::uint8_t> encodeSourceManifest(
    const chart_data::FeatureChartDataset &dataset) const;

  // Encode dataset metadata into DatasetMeta payload.
  [[nodiscard]] std::vector<std::uint8_t> encodeDatasetMeta(
    const chart_data::FeatureChartDataset &dataset) const;

  // Encode features into FeatureTable payload.
  [[nodiscard]] std::vector<std::uint8_t> encodeFeatureTable(
    const chart_data::FeatureChartDataset &dataset) const;

  // Encode geometries into GeometryBlob payload.
  [[nodiscard]] std::vector<std::uint8_t> encodeGeometryBlob(
    const chart_data::FeatureChartDataset &dataset) const;

  // Encode attributes into AttributeBlob payload.
  [[nodiscard]] std::vector<std::uint8_t> encodeAttributeBlob(
    const chart_data::FeatureChartDataset &dataset) const;

  [[nodiscard]] std::vector<std::uint8_t> encodeS57SemanticManifestV2() const;

  [[nodiscard]] std::vector<std::uint8_t> encodeS57FeatureSemanticsV2() const;

  [[nodiscard]] std::vector<std::uint8_t> encodeS57VectorRecordsV2() const;

  [[nodiscard]] SourceManifest manifestForWrite(const chart_data::FeatureChartDataset &dataset) const;

  [[nodiscard]] bool shouldWriteV2Semantics() const noexcept;

  std::uint32_t m_formatVersion{kSencFormatVersionV1};
  std::optional<SourceManifest> m_manifest;
  std::optional<s57::S57SourceModel> m_s57SourceModel;
};

}// namespace chart_view::runtime::senc

#endif
