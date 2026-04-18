#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import json
import sys
import xml.etree.ElementTree as ET
from collections import Counter, defaultdict
from pathlib import Path
from typing import Dict, List, Sequence


REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_BASELINE = REPO_ROOT / "tests" / "data" / "reference" / "phase6b_s52_resource_snapshot_inventory.reference.json"
DEFAULT_SNAPSHOT = REPO_ROOT / "vendor" / "opencpn_s57data" / "Release_5.14.0" / "s57data" / "chartsymbols.xml"
DEFAULT_JSON_OUT = REPO_ROOT / "docs" / "generated" / "phase6d_lookuprow_cause_bucket_inventory.json"
DEFAULT_CSV_OUT = REPO_ROOT / "docs" / "generated" / "phase6d_lookuprow_cause_bucket_inventory.csv"


BUCKET_DEFINITIONS = {
    "snapshot_asset_available_but_not_reached": (
        "The required asset is defined in the pinned vendored snapshot, but the current "
        "compiler/loader/precedence path does not reach it."
    ),
    "supplemental_upstream_candidate": (
        "The pinned snapshot does not define the required asset, but the row looks like a "
        "normal asset-family gap that task 106 should attempt to source from GPL-compatible "
        "supplemental OpenCPN resources."
    ),
    "manual_overlay_residual": (
        "The pinned snapshot does not define the required asset and the asset is already "
        "called out in the Phase 6D plan as a likely manual-overlay residual for task 107."
    ),
    "parser_or_compiler_spillover": (
        "The row looks malformed, combined, or compiler-spillover-driven and should be "
        "cleaned up in parser/compiler closure work rather than by adding assets first."
    ),
    "harness_only_partial": (
        "The row is already compiled, but its only remaining reason is missing repository-owned "
        "graphical/reference evidence."
    ),
    "internal_or_meta_row_only": (
        "The row belongs to an internal/meta family and must be tracked separately from ordinary "
        "S57 rows."
    ),
    "inventory_false_positive_or_misclassified": (
        "The row could not be confidently assigned to another bucket and needs explicit audit "
        "review before closure."
    ),
}


MANUAL_OVERLAY_ASSETS = {
    "FLTHAZ02",
    "BOYSPH79",
    "ESSARE01",
    "PSSARE01",
}

SPILLOVER_ASSET_PATTERNS = (
    "TESOBJNAM",
    "TXOBJNAM",
    "DRFSTA",
)

SPILLOVER_FAMILIES = {
    "NEWOBJ",
    "_SLGTO",
}


def normalize_identifier(value: str) -> str:
    return "".join(ch for ch in value.upper() if ch.isalnum())


def is_internal_or_meta_family(object_acronym: str) -> bool:
    upper = object_acronym.upper()
    return upper.startswith("$") or upper == "######"


def build_row_id(row: dict) -> str:
    return "|".join(
        [
            row.get("objectAcronym", ""),
            row.get("geometryType", ""),
            row.get("tableName", ""),
            row.get("sourceLookupId", ""),
            row.get("sourceRcid", ""),
        ]
    )


def asset_reason_entries(row: dict) -> List[tuple[str, str]]:
    entries: List[tuple[str, str]] = []
    for reason in row.get("reasons", []):
        if reason.startswith("compiler_missing_point_asset:"):
            entries.append(("point", reason.split(":", 1)[1]))
        elif reason.startswith("compiler_missing_line_asset:"):
            entries.append(("line", reason.split(":", 1)[1]))
        elif reason.startswith("compiler_missing_area_asset:"):
            entries.append(("area", reason.split(":", 1)[1]))
        elif reason.startswith("compiler_missing_text_asset:"):
            entries.append(("text", reason.split(":", 1)[1]))
    return entries


def representative_asset_or_rule(row: dict) -> str:
    entries = asset_reason_entries(row)
    if entries:
        return entries[0][1]
    for field in ("pointAssets", "lineAssets", "areaAssets", "textAssets", "conditionalTokens"):
        values = row.get(field, [])
        if values:
            return values[0]
    return row.get("ruleId", "") or row.get("sourceRcid", "")


def load_definition_sets(snapshot_xml: Path) -> Dict[str, set[str]]:
    tree = ET.parse(snapshot_xml)
    root = tree.getroot()
    definition_sets = {
        "point": set(),
        "line": set(),
        "area": set(),
        "text": set(),
    }

    symbols = root.find("symbols")
    if symbols is not None:
        for symbol in list(symbols):
            name = symbol.findtext("name")
            if name:
                definition_sets["point"].add(normalize_identifier(name))

    line_styles = root.find("line-styles")
    if line_styles is not None:
        for line_style in list(line_styles):
            name = line_style.findtext("name")
            if name:
                definition_sets["line"].add(normalize_identifier(name))

    patterns = root.find("patterns")
    if patterns is not None:
        for pattern in list(patterns):
            name = pattern.findtext("name")
            if name:
                definition_sets["area"].add(normalize_identifier(name))

    return definition_sets


def is_spillover_asset(asset_id: str) -> bool:
    upper = asset_id.upper()
    if any(pattern in upper for pattern in SPILLOVER_ASSET_PATTERNS):
        return True
    return False


def bucket_for_row(row: dict, definition_sets: Dict[str, set[str]]) -> tuple[str, str]:
    object_acronym = row.get("objectAcronym", "")
    reasons: List[str] = list(row.get("reasons", []))
    asset_entries = asset_reason_entries(row)
    normalized_assets = [(kind, normalize_identifier(asset_id), asset_id) for kind, asset_id in asset_entries]

    if is_internal_or_meta_family(object_acronym):
        return (
            "internal_or_meta_row_only",
            "object acronym is internal/meta and still carries degraded status",
        )

    if reasons == ["scene_harness_not_covered"]:
        return (
            "harness_only_partial",
            "only remaining degraded reason is missing repository-owned harness/reference evidence",
        )

    if "compiler_missing_lookup_row" in reasons:
        return (
            "parser_or_compiler_spillover",
            "ordinary row still lacks a compiled lookup row and must be closed in compiler/parser work",
        )

    if object_acronym.upper() in SPILLOVER_FAMILIES or any(is_spillover_asset(asset_id) for _, _, asset_id in normalized_assets):
        return (
            "parser_or_compiler_spillover",
            "asset id or family looks like spillover/malformed parser output rather than a clean symbol gap",
        )

    for kind, normalized_asset, raw_asset in normalized_assets:
        if normalized_asset and normalized_asset in definition_sets.get(kind, set()):
            return (
                "snapshot_asset_available_but_not_reached",
                f"{raw_asset} is defined in the pinned snapshot but the current compiler path did not reach it",
            )

    for _, normalized_asset, raw_asset in normalized_assets:
        if normalized_identifier_for_manual(raw_asset) in MANUAL_OVERLAY_ASSETS:
            return (
                "manual_overlay_residual",
                f"{raw_asset} is already identified as a manual-overlay residual in the Phase 6D plan",
            )

    if normalized_assets:
        return (
            "supplemental_upstream_candidate",
            "missing asset is not defined in the pinned snapshot and is a task-106 supplemental-upstream candidate",
        )

    return (
        "inventory_false_positive_or_misclassified",
        "row still appears degraded but does not match a known cause bucket",
    )


def normalized_identifier_for_manual(value: str) -> str:
    return "".join(ch for ch in value.upper() if ch.isalnum())


def likely_owning_task(bucket: str) -> str:
    return {
        "snapshot_asset_available_but_not_reached": "108-s52-all-lookuprow-closure",
        "supplemental_upstream_candidate": "106-s52-upstream-supplemental-opencpn-assets",
        "manual_overlay_residual": "107-s52-manual-overlay-asset-pack",
        "parser_or_compiler_spillover": "108-s52-all-lookuprow-closure",
        "harness_only_partial": "109-s52-harness-and-standalone-proof",
        "internal_or_meta_row_only": "108-s52-all-lookuprow-closure",
        "inventory_false_positive_or_misclassified": "110-phase6d-all-lookuprow-verification",
    }[bucket]


def ordinary_or_meta(row: dict) -> str:
    return "internal_meta" if is_internal_or_meta_family(row.get("objectAcronym", "")) else "ordinary_s57"


def sorted_counts(counter: Counter) -> List[dict]:
    items = sorted(counter.items(), key=lambda item: (-item[1], item[0]))
    return [{"name": name, "count": count} for name, count in items]


def build_bucket_summary(bucket_rows: Sequence[dict], total_backlog: int) -> dict:
    family_counter = Counter(row["family"] for row in bucket_rows)
    ordinary = sum(1 for row in bucket_rows if row["ordinaryOrMeta"] == "ordinary_s57")
    internal = len(bucket_rows) - ordinary
    sample_rows = [
        {
            "rowId": row["rowId"],
            "family": row["family"],
            "sourceRcid": row["sourceRcid"],
            "tableName": row["tableName"],
            "representativeAssetOrRule": row["representativeAssetOrRule"],
            "notes": row["notes"],
        }
        for row in sorted(bucket_rows, key=lambda item: (item["family"], item["sourceRcid"], item["rowId"]))[:10]
    ]
    return {
        "count": len(bucket_rows),
        "ordinaryRows": ordinary,
        "internalMetaRows": internal,
        "shareOfBacklog": (len(bucket_rows) / total_backlog) if total_backlog else 0.0,
        "topFamilies": sorted_counts(family_counter)[:10],
        "sampleRows": sample_rows,
    }


def ensure_parent(path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)


def write_csv(path: Path, rows: Sequence[dict]) -> None:
    ensure_parent(path)
    fieldnames = [
        "rowId",
        "family",
        "status",
        "ordinaryOrMeta",
        "bucket",
        "likelyOwningTask",
        "representativeAssetOrRule",
        "notes",
        "sourceRcid",
        "sourceLookupId",
        "geometryType",
        "tableName",
        "ruleId",
        "reasons",
    ]
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        for row in rows:
            writer.writerow(
                {
                    "rowId": row["rowId"],
                    "family": row["family"],
                    "status": row["status"],
                    "ordinaryOrMeta": row["ordinaryOrMeta"],
                    "bucket": row["bucket"],
                    "likelyOwningTask": row["likelyOwningTask"],
                    "representativeAssetOrRule": row["representativeAssetOrRule"],
                    "notes": row["notes"],
                    "sourceRcid": row["sourceRcid"],
                    "sourceLookupId": row["sourceLookupId"],
                    "geometryType": row["geometryType"],
                    "tableName": row["tableName"],
                    "ruleId": row["ruleId"],
                    "reasons": ";".join(row["reasons"]),
                }
            )


def main(argv: Sequence[str]) -> int:
    parser = argparse.ArgumentParser(description="Audit remaining lookup-row cause buckets from the committed inventory baseline.")
    parser.add_argument("--baseline", type=Path, default=DEFAULT_BASELINE)
    parser.add_argument("--snapshot", type=Path, default=DEFAULT_SNAPSHOT)
    parser.add_argument("--json-out", type=Path, default=DEFAULT_JSON_OUT)
    parser.add_argument("--csv-out", type=Path, default=DEFAULT_CSV_OUT)
    args = parser.parse_args(argv)

    with args.baseline.open("r", encoding="utf-8") as handle:
        baseline = json.load(handle)

    definition_sets = load_definition_sets(args.snapshot)
    degraded_source_rows = baseline.get("degradedRows", [])

    rows: List[dict] = []
    bucket_counter = Counter()
    bucket_rows: Dict[str, List[dict]] = defaultdict(list)
    ordinary_counter = Counter()
    internal_counter = Counter()

    for source_row in degraded_source_rows:
        bucket, note = bucket_for_row(source_row, definition_sets)
        family = source_row.get("objectAcronym", "").upper()
        row = {
            "rowId": build_row_id(source_row),
            "family": family,
            "status": source_row.get("status", ""),
            "ordinaryOrMeta": ordinary_or_meta(source_row),
            "bucket": bucket,
            "likelyOwningTask": likely_owning_task(bucket),
            "representativeAssetOrRule": representative_asset_or_rule(source_row),
            "notes": note,
            "sourceLookupId": source_row.get("sourceLookupId", ""),
            "sourceRcid": source_row.get("sourceRcid", ""),
            "geometryType": source_row.get("geometryType", ""),
            "tableName": source_row.get("tableName", ""),
            "ruleId": source_row.get("ruleId", ""),
            "reasons": list(source_row.get("reasons", [])),
        }
        rows.append(row)
        bucket_counter[bucket] += 1
        bucket_rows[bucket].append(row)
        if row["ordinaryOrMeta"] == "ordinary_s57":
            ordinary_counter[bucket] += 1
        else:
            internal_counter[bucket] += 1

    total_backlog = len(rows)
    ordinary_backlog = sum(1 for row in rows if row["ordinaryOrMeta"] == "ordinary_s57")
    internal_backlog = total_backlog - ordinary_backlog

    summary = {
        "baselineSource": str(args.baseline.resolve()),
        "snapshotSource": str(args.snapshot.resolve()),
        "lookupRowsTotal": baseline.get("snapshot", {}).get("lookupRowsTotal", 0),
        "currentCommittedBacklogRows": total_backlog,
        "ordinaryS57BacklogRows": ordinary_backlog,
        "internalMetaBacklogRows": internal_backlog,
        "rowCoverage": baseline.get("rowCoverage", {}),
        "old3049StillValid": total_backlog == 3049,
        "bucketCounts": dict(bucket_counter),
        "bucketCountsOrdinary": dict(ordinary_counter),
        "bucketCountsInternalMeta": dict(internal_counter),
    }

    bucket_summaries = {
        bucket: build_bucket_summary(bucket_rows[bucket], total_backlog)
        for bucket in BUCKET_DEFINITIONS
    }

    output = {
        "baseline": summary,
        "bucketDefinitions": BUCKET_DEFINITIONS,
        "bucketSummaries": bucket_summaries,
        "rows": rows,
    }

    ensure_parent(args.json_out)
    with args.json_out.open("w", encoding="utf-8") as handle:
        json.dump(output, handle, indent=2, ensure_ascii=False)
        handle.write("\n")

    write_csv(args.csv_out, rows)

    print(json.dumps(summary, indent=2, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
