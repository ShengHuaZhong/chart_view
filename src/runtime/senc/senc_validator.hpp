#ifndef CHART_VIEW_RUNTIME_SENC_SENC_VALIDATOR_HPP
#define CHART_VIEW_RUNTIME_SENC_SENC_VALIDATOR_HPP

#include "source_manifest.hpp"

#include <string>

namespace chart_view::runtime::senc {

// Reason why a SENC file needs to be rebuilt.
enum class RebuildReason : std::uint8_t
{
  kNone = 0,          // SENC is up-to-date.
  kMissing,           // No existing SENC file.
  kNameMismatch,      // Source file name changed.
  kSourceTypeMismatch,// Source format changed.
  kSizeMismatch,      // Source file size changed.
  kTimestampNewer,    // Source file was modified (newer timestamp).
  kHashMismatch,      // Source content hash changed.
  kEditionMismatch,   // Edition number changed.
  kUpdateMismatch,    // Update number changed.
};

// Result of a SENC validation check.
struct ValidationResult
{
  bool rebuildNeeded{false};
  RebuildReason reason{RebuildReason::kNone};
  std::string detail;
};

// Compares a current SourceManifest (from the live source file)
// against a stored SourceManifest (from an existing SENC file)
// to decide whether the SENC needs to be rebuilt.
class SencValidator
{
public:
  SencValidator() = default;

  // Compare current source manifest against the stored one.
  // Returns whether rebuild is needed and the reason.
  [[nodiscard]] ValidationResult validate(
    const SourceManifest &current,
    const SourceManifest &stored) const;

  // Check if a SENC is missing (no stored manifest available).
  [[nodiscard]] static ValidationResult missing();
};

}// namespace chart_view::runtime::senc

#endif
