#ifndef CHART_VIEW_RUNTIME_SENC_SENC_READER_HPP
#define CHART_VIEW_RUNTIME_SENC_SENC_READER_HPP

#include "senc_types.hpp"
#include "source_manifest.hpp"
#include "../chart_data/feature_chart_dataset.hpp"
#include "../s57/s57_source_model.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace chart_view::runtime::senc {

// Result of a SENC read operation.
struct SencReadResult
{
  bool ok{false};
  std::string error;
  chart_data::FeatureChartDataset dataset;
  std::optional<SourceManifest> manifest;
  std::optional<s57::S57SourceModel> sourceModel;
};

struct SencCatalogMetaReadResult
{
  bool ok{false};
  std::string error;
  chart_data::DatasetMeta meta;
  std::optional<SourceManifest> manifest;
};

// Reads SENC v1 files written by SencWriter.
// Validates header magic/version, section table integrity, and CRC checksums.
class SencReader
{
public:
  SencReader() = default;

  // Read from in-memory blob.
  [[nodiscard]] SencReadResult read(std::span<const std::uint8_t> blob) const;

  // Read from file path.
  [[nodiscard]] SencReadResult readFromFile(const std::string &path) const;

  // Read only catalog-level metadata from a SENC blob without decoding geometry.
  [[nodiscard]] SencCatalogMetaReadResult readCatalogMeta(std::span<const std::uint8_t> blob) const;

  // Read only catalog-level metadata from a SENC file without decoding geometry.
  [[nodiscard]] SencCatalogMetaReadResult readCatalogMetaFromFile(const std::string &path) const;

private:
  // Validate file header. Returns error string (empty on success).
  [[nodiscard]] std::string validateHeader(const FileHeader &fh, std::size_t blobSize) const;

  // Validate section table. Returns error string (empty on success).
  [[nodiscard]] std::string validateSectionTable(
    const std::vector<SectionDesc> &descs,
    const FileHeader &fh,
    std::span<const std::uint8_t> blob) const;

  // Decode section payloads into dataset and manifest.
  [[nodiscard]] std::string decodeSections(
    const FileHeader &fh,
    const std::vector<SectionDesc> &descs,
    std::span<const std::uint8_t> blob,
    chart_data::FeatureChartDataset &out,
    std::optional<SourceManifest> &manifestOut,
    std::optional<s57::S57SourceModel> &sourceModelOut) const;

  [[nodiscard]] std::string decodeCatalogSections(
    const std::vector<SectionDesc> &descs,
    std::span<const std::uint8_t> blob,
    chart_data::DatasetMeta &metaOut,
    std::optional<SourceManifest> &manifestOut) const;

  // Individual section decoders.
  [[nodiscard]] std::string decodeSourceManifest(
    std::span<const std::uint8_t> payload,
    SourceManifest &manifestOut) const;

  [[nodiscard]] std::string decodeDatasetMeta(
    std::span<const std::uint8_t> payload,
    chart_data::FeatureChartDataset &out) const;

  [[nodiscard]] std::string decodeFeatureTable(
    std::span<const std::uint8_t> payload,
    chart_data::FeatureChartDataset &out) const;

  [[nodiscard]] std::string decodeGeometryBlob(
    std::span<const std::uint8_t> payload,
    chart_data::FeatureChartDataset &out) const;

  [[nodiscard]] std::string decodeAttributeBlob(
    std::span<const std::uint8_t> payload,
    chart_data::FeatureChartDataset &out) const;

  [[nodiscard]] std::string decodeS57SemanticManifestV2(
    std::span<const std::uint8_t> payload,
    s57::S57SourceModel &out) const;

  [[nodiscard]] std::string decodeS57FeatureSemanticsV2(
    std::span<const std::uint8_t> payload,
    s57::S57SourceModel &out) const;

  [[nodiscard]] std::string decodeS57VectorRecordsV2(
    std::span<const std::uint8_t> payload,
    s57::S57SourceModel &out) const;
};

}// namespace chart_view::runtime::senc

#endif
