#ifndef CHART_VIEW_RUNTIME_S57_S57_SOURCE_MODEL_HPP
#define CHART_VIEW_RUNTIME_S57_S57_SOURCE_MODEL_HPP

#include "../chart_data/dataset_meta.hpp"
#include "../chart_data/feature.hpp"
#include "../senc/source_manifest.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace chart_view::runtime::s57 {

enum class S57Primitive : std::uint8_t
{
  kPoint = 1,
  kLine = 2,
  kArea = 3,
  kNone = 255
};

struct S57FeatureIdentity
{
  std::uint16_t agency{0};
  std::uint32_t featureId{0};
  std::uint16_t featureSubdivision{0};

  [[nodiscard]] bool isValid() const noexcept
  {
    return agency != 0 || featureId != 0 || featureSubdivision != 0;
  }
};

struct S57SourceVectorRecord
{
  std::uint8_t recordName{0};
  std::uint32_t recordId{0};
  std::uint16_t recordVersion{0};
  std::uint8_t updateInstruction{0};
  std::vector<chart_data::Coordinate> coords;
};

struct S57SpatialPointer
{
  std::uint8_t recordName{0};
  std::uint32_t recordId{0};
  std::uint8_t orientation{0};
  std::uint8_t usage{0};
  std::uint8_t mask{0};
};

using S57AttributeMap = std::unordered_map<std::string, chart_data::AttributeValue>;

struct S57SourceFeature
{
  std::uint32_t recordId{0};
  std::uint16_t recordVersion{0};
  std::uint8_t updateInstruction{0};
  S57Primitive primitive{S57Primitive::kNone};
  std::uint32_t classCode{0};
  std::string classAcronym;
  std::optional<S57FeatureIdentity> identity;
  S57AttributeMap attributes;
  S57AttributeMap nationalAttributes;
  std::vector<S57SpatialPointer> spatialPointers;
  chart_data::Geometry geometry;
};

struct S57UpdateFile
{
  std::string path;
  std::string name;
  std::uint32_t updateNumber{0};
  std::uint64_t sourceSize{0};
  std::int64_t sourceTimestamp{0};
};

struct S57UpdateManifest
{
  std::string basePath;
  std::string baseName;
  std::uint32_t edition{0};
  std::uint32_t baseUpdate{0};
  std::vector<S57UpdateFile> availableUpdates;
  std::uint32_t highestContiguousUpdate{0};
  std::uint32_t nextMissingUpdate{0};

  [[nodiscard]] bool hasPendingUpdates() const noexcept
  {
    return !availableUpdates.empty();
  }
};

struct S57SourceModel
{
  std::string sourcePath;
  std::string sourceName;
  std::string declaredDatasetName;
  chart_data::DatasetMeta datasetMeta;
  double coordinateMultiplier{1.0e-7};
  std::unordered_map<std::uint64_t, S57SourceVectorRecord> vectors;
  std::vector<S57SourceFeature> features;
  senc::SourceManifest sourceManifest;
  S57UpdateManifest updateManifest;

  [[nodiscard]] bool empty() const noexcept
  {
    return vectors.empty() && features.empty();
  }
};

[[nodiscard]] S57UpdateManifest buildUpdateManifestForBasePath(
  const std::string &basePath,
  std::uint32_t edition = 0,
  std::uint32_t baseUpdate = 0);

[[nodiscard]] senc::SourceManifest buildSourceManifestForPath(
  const std::string &path,
  const std::string &sourceName,
  std::uint32_t edition = 0,
  std::uint32_t update = 0);

[[nodiscard]] senc::SourceManifest buildSourceManifestForBuffer(
  const std::string &sourceName,
  std::span<const std::uint8_t> data,
  std::uint32_t edition = 0,
  std::uint32_t update = 0);

}// namespace chart_view::runtime::s57

#endif
