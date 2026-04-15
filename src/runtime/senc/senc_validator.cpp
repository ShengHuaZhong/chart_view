#include "senc_validator.hpp"

namespace chart_view::runtime::senc {

ValidationResult SencValidator::missing()
{
  return {true, RebuildReason::kMissing, "no existing SENC file"};
}

ValidationResult SencValidator::validate(
  const SourceManifest &current,
  const SourceManifest &stored) const
{
  // Check name.
  if (current.name != stored.name)
    return {true, RebuildReason::kNameMismatch,
            "source name changed: \"" + stored.name + "\" -> \"" + current.name + "\""};

  // Check source type.
  if (current.sourceType != stored.sourceType)
    return {true, RebuildReason::kSourceTypeMismatch, "source type changed"};

  // Check edition.
  if (current.edition != stored.edition)
    return {true, RebuildReason::kEditionMismatch,
            "edition changed: " + std::to_string(stored.edition)
              + " -> " + std::to_string(current.edition)};

  // Check update.
  if (current.update != stored.update)
    return {true, RebuildReason::kUpdateMismatch,
            "update changed: " + std::to_string(stored.update)
              + " -> " + std::to_string(current.update)};

  // Check file size (if both are known).
  if (current.sourceSize != 0 && stored.sourceSize != 0
      && current.sourceSize != stored.sourceSize)
    return {true, RebuildReason::kSizeMismatch, "source file size changed"};

  // Check timestamp (if both are known, current must not be newer).
  if (current.sourceTimestamp != 0 && stored.sourceTimestamp != 0
      && current.sourceTimestamp > stored.sourceTimestamp)
    return {true, RebuildReason::kTimestampNewer, "source file is newer"};

  // Check hash (if both are known).
  if (current.sourceHash != 0 && stored.sourceHash != 0
      && current.sourceHash != stored.sourceHash)
    return {true, RebuildReason::kHashMismatch, "source content hash changed"};

  return {false, RebuildReason::kNone, {}};
}

}// namespace chart_view::runtime::senc
