# SENC v1 Contract

SENC v1 is the required internal cache format for Phase 1.

## Mandatory sections
- FileHeader
- SourceManifest
- DatasetMeta
- FeatureTable
- GeometryBlob
- AttributeBlob
- SpatialIndex
- RenderCache
- PickIndex
- StringTable

## Meaning of “complete SENC” in this project
A complete SENC v1 must support:
- cache invalidation checks
- fast metadata load
- visible-object spatial filtering
- render-bucket consumption by runtime renderer
- minimal pick/query support
