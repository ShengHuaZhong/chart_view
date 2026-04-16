#include "s57_update_application.hpp"

#include <algorithm>
#include <type_traits>

namespace chart_view::runtime::s57 {

namespace {

[[nodiscard]] std::uint64_t vectorKey(const S57SourceVectorRecord &record) noexcept
{
  return (static_cast<std::uint64_t>(record.recordName) << 32) | record.recordId;
}

[[nodiscard]] std::uint32_t updateNumberForModel(const S57SourceModel &model) noexcept
{
  if(model.sourceManifest.update != 0) {
    return model.sourceManifest.update;
  }

  return model.datasetMeta.update;
}

chart_data::Geometry buildGeometry(
  S57Primitive primitive,
  const std::vector<S57SpatialPointer> &spatialPointers,
  const std::unordered_map<std::uint64_t, S57SourceVectorRecord> &vectors)
{
  if(primitive == S57Primitive::kPoint) {
    if(!spatialPointers.empty()) {
      const auto key =
        (static_cast<std::uint64_t>(spatialPointers.front().recordName) << 32) |
        spatialPointers.front().recordId;
      if(const auto it = vectors.find(key); it != vectors.end() && !it->second.coords.empty()) {
        return chart_data::PointGeometry{it->second.coords.front()};
      }
    }

    return chart_data::PointGeometry{};
  }

  if(primitive == S57Primitive::kLine) {
    chart_data::LineGeometry line;
    for(const auto &pointer : spatialPointers) {
      const auto key = (static_cast<std::uint64_t>(pointer.recordName) << 32) | pointer.recordId;
      if(const auto it = vectors.find(key); it != vectors.end()) {
        line.vertices.insert(line.vertices.end(), it->second.coords.begin(), it->second.coords.end());
      }
    }
    return line;
  }

  chart_data::AreaGeometry area;
  for(const auto &pointer : spatialPointers) {
    const auto key = (static_cast<std::uint64_t>(pointer.recordName) << 32) | pointer.recordId;
    if(const auto it = vectors.find(key); it != vectors.end()) {
      area.exteriorRing.insert(
        area.exteriorRing.end(),
        it->second.coords.begin(),
        it->second.coords.end());
    }
  }

  return area;
}

void updateExtentFromGeometry(
  const chart_data::Geometry &geometry,
  chart_data::Extent &extent,
  bool &extentInitialized)
{
  const auto updateExtent = [&](double lon, double lat) {
    if(!extentInitialized) {
      extent.minLon = extent.maxLon = lon;
      extent.minLat = extent.maxLat = lat;
      extentInitialized = true;
    } else {
      extent.minLon = std::min(extent.minLon, lon);
      extent.maxLon = std::max(extent.maxLon, lon);
      extent.minLat = std::min(extent.minLat, lat);
      extent.maxLat = std::max(extent.maxLat, lat);
    }
  };

  std::visit(
    [&](const auto &geo) {
      using GeometryT = std::decay_t<decltype(geo)>;
      if constexpr (std::is_same_v<GeometryT, chart_data::PointGeometry>) {
        if(geo.position.lon != 0.0 || geo.position.lat != 0.0) {
          updateExtent(geo.position.lon, geo.position.lat);
        }
      } else if constexpr (std::is_same_v<GeometryT, chart_data::LineGeometry>) {
        for(const auto &vertex : geo.vertices) {
          updateExtent(vertex.lon, vertex.lat);
        }
      } else if constexpr (std::is_same_v<GeometryT, chart_data::AreaGeometry>) {
        for(const auto &vertex : geo.exteriorRing) {
          updateExtent(vertex.lon, vertex.lat);
        }
      }
    },
    geometry);
}

void rebuildFeatureGeometriesAndExtent(S57SourceModel &model)
{
  chart_data::Extent extent;
  bool extentInitialized = false;
  for(auto &feature : model.features) {
    feature.geometry = buildGeometry(feature.primitive, feature.spatialPointers, model.vectors);
    updateExtentFromGeometry(feature.geometry, extent, extentInitialized);
  }

  if(extentInitialized) {
    model.datasetMeta.extent = extent;
  }
}

std::string missingRecordError(const char *kind, std::uint32_t recordId)
{
  return std::string(kind) + " record not found for update: " + std::to_string(recordId);
}

bool applyVectorUpdate(
  std::unordered_map<std::uint64_t, S57SourceVectorRecord> &vectors,
  const S57SourceVectorRecord &update,
  std::string &error)
{
  const auto key = vectorKey(update);
  const auto operation = decodeUpdateOperation(update.updateInstruction);
  auto it = vectors.find(key);

  switch(operation) {
  case S57UpdateOperation::kBase:
  case S57UpdateOperation::kInsert:
    if(operation == S57UpdateOperation::kInsert && it != vectors.end()) {
      error = "vector record already exists for insert: " + std::to_string(update.recordId);
      return false;
    }
    vectors.insert_or_assign(key, update);
    return true;

  case S57UpdateOperation::kDelete:
    if(it == vectors.end()) {
      error = missingRecordError("vector", update.recordId);
      return false;
    }
    vectors.erase(it);
    return true;

  case S57UpdateOperation::kModify:
    if(it == vectors.end()) {
      error = missingRecordError("vector", update.recordId);
      return false;
    }
    it->second.recordVersion = update.recordVersion != 0 ? update.recordVersion : it->second.recordVersion;
    it->second.updateInstruction = update.updateInstruction;
    if(!update.coords.empty()) {
      it->second.coords = update.coords;
    }
    return true;
  }

  error = "unknown vector update operation";
  return false;
}

bool applyFeatureUpdate(
  std::vector<S57SourceFeature> &features,
  const std::unordered_map<std::uint64_t, S57SourceVectorRecord> &vectors,
  const S57SourceFeature &update,
  std::string &error)
{
  const auto operation = decodeUpdateOperation(update.updateInstruction);
  auto it = std::find_if(
    features.begin(),
    features.end(),
    [&](const S57SourceFeature &feature) {
      return feature.recordId == update.recordId;
    });

  switch(operation) {
  case S57UpdateOperation::kBase:
  case S57UpdateOperation::kInsert:
    if(operation == S57UpdateOperation::kInsert && it != features.end()) {
      error = "feature record already exists for insert: " + std::to_string(update.recordId);
      return false;
    }
    features.push_back(update);
    features.back().geometry =
      buildGeometry(features.back().primitive, features.back().spatialPointers, vectors);
    return true;

  case S57UpdateOperation::kDelete:
    if(it == features.end()) {
      error = missingRecordError("feature", update.recordId);
      return false;
    }
    features.erase(it);
    return true;

  case S57UpdateOperation::kModify:
    if(it == features.end()) {
      error = missingRecordError("feature", update.recordId);
      return false;
    }
    if(update.recordVersion != 0) {
      it->recordVersion = update.recordVersion;
    }
    it->updateInstruction = update.updateInstruction;
    if(update.primitive != S57Primitive::kNone) {
      it->primitive = update.primitive;
    }
    if(update.classCode != 0) {
      it->classCode = update.classCode;
    }
    if(!update.classAcronym.empty()) {
      it->classAcronym = update.classAcronym;
    }
    if(update.identity.has_value()) {
      it->identity = update.identity;
    }
    if(!update.spatialPointers.empty()) {
      it->spatialPointers = update.spatialPointers;
    }
    for(const auto &[key, value] : update.attributes) {
      it->attributes.insert_or_assign(key, value);
    }
    for(const auto &[key, value] : update.nationalAttributes) {
      it->nationalAttributes.insert_or_assign(key, value);
    }
    it->geometry = buildGeometry(it->primitive, it->spatialPointers, vectors);
    return true;
  }

  error = "unknown feature update operation";
  return false;
}

}// namespace

S57UpdateOperation decodeUpdateOperation(std::uint8_t raw) noexcept
{
  switch(raw) {
  case 1:
    return S57UpdateOperation::kInsert;
  case 2:
    return S57UpdateOperation::kDelete;
  case 3:
    return S57UpdateOperation::kModify;
  default:
    return S57UpdateOperation::kBase;
  }
}

S57UpdateApplicationResult applySequentialUpdates(
  S57SourceModel baseModel,
  const std::vector<S57SourceModel> &updates)
{
  S57UpdateApplicationResult result;
  result.model = std::move(baseModel);

  if(updates.empty()) {
    result.model.updateManifest.lastAppliedUpdate = result.model.datasetMeta.update;
    result.ok = true;
    return result;
  }

  std::vector<const S57SourceModel *> orderedUpdates;
  orderedUpdates.reserve(updates.size());
  for(const auto &update : updates) {
    orderedUpdates.push_back(&update);
  }

  std::sort(
    orderedUpdates.begin(),
    orderedUpdates.end(),
    [](const S57SourceModel *lhs, const S57SourceModel *rhs) {
      return updateNumberForModel(*lhs) < updateNumberForModel(*rhs);
    });

  auto expectedUpdate = result.model.datasetMeta.update + 1u;
  for(const auto *update : orderedUpdates) {
    const auto updateNumber = updateNumberForModel(*update);
    if(updateNumber == 0) {
      result.error = "update file missing update number";
      return result;
    }
    if(updateNumber != expectedUpdate) {
      result.error = "missing sequential update " + std::to_string(expectedUpdate) +
                     " before update " + std::to_string(updateNumber);
      return result;
    }

    if(update->datasetMeta.edition != 0 &&
       result.model.datasetMeta.edition != 0 &&
       update->datasetMeta.edition != result.model.datasetMeta.edition) {
      result.error = "update edition mismatch for update " + std::to_string(updateNumber);
      return result;
    }

    for(const auto &[_, vectorRecord] : update->vectors) {
      if(!applyVectorUpdate(result.model.vectors, vectorRecord, result.error)) {
        return result;
      }
    }

    for(const auto &feature : update->features) {
      if(!applyFeatureUpdate(result.model.features, result.model.vectors, feature, result.error)) {
        return result;
      }
    }

    rebuildFeatureGeometriesAndExtent(result.model);
    result.appliedUpdates.push_back(updateNumber);
    result.model.updateManifest.appliedUpdates.push_back(updateNumber);
    result.model.updateManifest.lastAppliedUpdate = updateNumber;
    result.model.datasetMeta.update = updateNumber;
    result.model.sourceManifest.update = updateNumber;
    expectedUpdate = updateNumber + 1u;
  }

  result.model.updateManifest.nextMissingUpdate = expectedUpdate;
  result.ok = true;
  return result;
}

}// namespace chart_view::runtime::s57
