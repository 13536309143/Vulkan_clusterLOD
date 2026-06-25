"""Build per-mesh LOD constraint classes from GLB/GLTF part analysis CSV.

The input is produced by analyze_large_glb_parts_pointnext.py or
analyze_gltf_parts_pointnext.py. Output keeps one row per original mesh and adds
size-first, shape-second, semantic/detail-aware LOD policy fields. The final
policy is a P1-P10 semantic-structural class used by the Vulkan LOD frontend and
builder.
"""

from __future__ import annotations

import argparse
import csv
import json
import math
from collections import Counter, defaultdict
from pathlib import Path


FASTENER_TYPES = {
    "screws_bolts_studs",
    "nuts",
    "washers_rings_spacers",
    "pins_rivets_keys",
}
MOTION_CRITICAL_TYPES = {
    "bearings_bushings_guides",
    "gears_pulleys_chains",
    "motors_gearmotors",
    "wheels_castors",
    "rotating_fluid_machinery",
    "springs",
}
INTERFACE_TYPES = {
    "pipe_fittings_valves_nozzles",
}
STRUCTURAL_KEY_TYPES = {
    "joints_clamps_structural_connectors",
}
BULK_STATIC_TYPES = {
    "plates_discs_shapes",
}
CONTROL_TYPES = {
    "handles_controls",
}

P10_POLICY_TABLE = {
    1: ("P1_micro_uncertain", 0.25, 0.08, 0.02, True, 0.40),
    2: ("P2_repeated_fastener", 0.38, 0.16, 0.05, True, 0.55),
    3: ("P3_large_static_bulk", 0.45, 0.22, 0.07, True, 0.65),
    4: ("P4_ordinary_low_detail", 0.55, 0.28, 0.10, True, 0.80),
    5: ("P5_balanced_visible", 0.65, 0.36, 0.16, False, 0.95),
    6: ("P6_high_detail_shape", 0.74, 0.45, 0.22, False, 1.08),
    7: ("P7_interface_fluid", 0.80, 0.52, 0.28, False, 1.18),
    8: ("P8_structural_control", 0.84, 0.58, 0.34, False, 1.28),
    9: ("P9_motion_precision", 0.92, 0.68, 0.42, False, 1.45),
    10: ("P10_critical_preserve", 1.00, 0.80, 0.55, False, 1.65),
}


def make_p10_policy(priority: int, strategy: str) -> dict:
    name, near, mid, far, allow_cull, screen_error_weight = P10_POLICY_TABLE[priority]
    return {
        "lod_priority": name,
        "lod_strategy": strategy,
        "target_ratio_near": near,
        "target_ratio_mid": mid,
        "target_ratio_far": far,
        "allow_cull": allow_cull,
        "screen_error_weight": screen_error_weight,
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Create LOD constraint classes from part analysis CSV."
    )
    parser.add_argument("--input-csv", type=Path, required=True)
    parser.add_argument("--output-csv", type=Path, required=True)
    parser.add_argument("--output-json", type=Path, default=None)
    parser.add_argument("--summary-json", type=Path, default=None)
    parser.add_argument(
        "--high-confidence",
        type=float,
        default=0.70,
        help="Prediction confidence at or above this is treated as reliable.",
    )
    parser.add_argument(
        "--low-confidence",
        type=float,
        default=0.40,
        help="Below this, semantic type is downgraded and geometry fallback is used more strongly.",
    )
    parser.add_argument(
        "--low-margin",
        type=float,
        default=0.08,
        help="If top1-top2 confidence is below this, mark semantic prediction as ambiguous.",
    )
    return parser.parse_args()


def to_float(row: dict, key: str, default: float = 0.0) -> float:
    try:
        value = row.get(key, "")
        return float(value) if value not in {"", None} else default
    except Exception:
        return default


def to_int(row: dict, key: str, default: int = 0) -> int:
    try:
        value = row.get(key, "")
        return int(float(value)) if value not in {"", None} else default
    except Exception:
        return default


def percentile(sorted_values: list[float], p: float) -> float:
    if not sorted_values:
        return 0.0
    index = min(len(sorted_values) - 1, max(0, round((len(sorted_values) - 1) * p)))
    return sorted_values[index]


def make_thresholds(rows: list[dict]) -> dict:
    diagonals = sorted(to_float(row, "bbox_diagonal") for row in rows)
    faces = sorted(to_int(row, "face_count") for row in rows)
    areas = sorted(to_float(row, "surface_area") for row in rows)
    return {
        "diag_p10": percentile(diagonals, 0.10),
        "diag_p25": percentile(diagonals, 0.25),
        "diag_p50": percentile(diagonals, 0.50),
        "diag_p75": percentile(diagonals, 0.75),
        "diag_p90": percentile(diagonals, 0.90),
        "diag_p97": percentile(diagonals, 0.97),
        "face_p50": percentile(faces, 0.50),
        "face_p75": percentile(faces, 0.75),
        "face_p90": percentile(faces, 0.90),
        "face_p97": percentile(faces, 0.97),
        "area_p50": percentile(areas, 0.50),
        "area_p75": percentile(areas, 0.75),
        "area_p90": percentile(areas, 0.90),
    }


def classify_size(row: dict, thresholds: dict) -> tuple[str, int]:
    diag = to_float(row, "bbox_diagonal")
    if diag <= thresholds["diag_p10"]:
        return "S0_micro", 0
    if diag <= thresholds["diag_p25"]:
        return "S1_tiny", 1
    if diag <= thresholds["diag_p50"]:
        return "S2_small", 2
    if diag <= thresholds["diag_p75"]:
        return "S3_medium", 3
    if diag <= thresholds["diag_p90"]:
        return "S4_large", 4
    if diag <= thresholds["diag_p97"]:
        return "S5_xlarge", 5
    return "S6_huge", 6


def semantic_group(predicted_class: str, reliable: bool) -> str:
    if not reliable:
        return "semantic_uncertain"
    if predicted_class in FASTENER_TYPES:
        return "fastener_repeated"
    if predicted_class in MOTION_CRITICAL_TYPES:
        return "motion_or_precision_part"
    if predicted_class in INTERFACE_TYPES:
        return "fluid_or_interface_part"
    if predicted_class in STRUCTURAL_KEY_TYPES:
        return "structural_key_part"
    if predicted_class in BULK_STATIC_TYPES:
        return "bulk_static_part"
    if predicted_class in CONTROL_TYPES:
        return "control_or_handle"
    return "other_part"


def geometry_fallback(row: dict) -> str:
    long_side = to_float(row, "obb_size_long")
    mid_side = to_float(row, "obb_size_mid")
    short_side = to_float(row, "obb_size_short")
    elongation = to_float(row, "elongation")
    flatness = to_float(row, "flatness")
    compactness = to_float(row, "compactness")
    face_count = to_int(row, "face_count")

    if flatness < 0.015:
        return "geom_ultra_thin_sheet"
    if flatness < 0.08:
        return "geom_thin_plate"
    if elongation >= 8:
        return "geom_wire_or_rod"
    if elongation >= 3:
        return "geom_slender_bar"
    if compactness > 0.12 and flatness > 0.35:
        return "geom_compact_block"
    if abs(long_side - mid_side) / max(long_side, 1e-12) < 0.15 and flatness < 0.25:
        return "geom_ring_or_disk"
    if face_count >= 2000:
        return "geom_high_detail_irregular"
    return "geom_simple_irregular"


def detail_level(row: dict, thresholds: dict) -> tuple[str, int]:
    face_count = to_int(row, "face_count")
    area = to_float(row, "surface_area")
    diag = to_float(row, "bbox_diagonal")
    density = face_count / max(diag * diag, 1e-12)

    score = 0
    if face_count >= thresholds["face_p97"]:
        score += 3
    elif face_count >= thresholds["face_p90"]:
        score += 2
    elif face_count >= thresholds["face_p75"]:
        score += 1

    if area >= thresholds["area_p90"]:
        score += 1
    if density > 5000:
        score += 1

    if score >= 4:
        return "D4_very_high", 4
    if score == 3:
        return "D3_high", 3
    if score == 2:
        return "D2_medium", 2
    if score == 1:
        return "D1_low", 1
    return "D0_very_low", 0


def confidence_state(row: dict, high: float, low: float, margin_threshold: float) -> tuple[str, bool, float]:
    confidence = to_float(row, "confidence")
    second = to_float(row, "second_confidence")
    margin = confidence - second
    if confidence >= high and margin >= margin_threshold:
        return "C2_high", True, margin
    if confidence < low or margin < margin_threshold:
        return "C0_low_or_ambiguous", False, margin
    return "C1_medium", True, margin


def lod_policy(
    row: dict,
    size_rank: int,
    detail_rank: int,
    reliable: bool,
    semantic: str,
    fallback: str,
) -> dict:
    shape = row.get("shape_hint", "")
    predicted_class = row.get("predicted_class", "")

    is_micro = size_rank <= 1
    is_small = size_rank <= 2
    is_large = size_rank >= 4
    is_xlarge = size_rank >= 5
    is_huge = size_rank >= 6
    is_thin_or_plate = shape == "flat_plate" or fallback in {"geom_ultra_thin_sheet", "geom_thin_plate"}
    is_simple_bulk_shape = fallback in {
        "geom_ultra_thin_sheet",
        "geom_thin_plate",
        "geom_compact_block",
        "geom_simple_irregular",
    }
    is_bulk_static = (
        semantic in {"bulk_static_part", "other_part", "semantic_uncertain"}
        and is_xlarge
        and (is_simple_bulk_shape or is_thin_or_plate or is_huge)
    )
    is_motion_key = semantic == "motion_or_precision_part" and reliable
    is_structural_key = semantic == "structural_key_part" and reliable
    is_control_key = semantic == "control_or_handle" and reliable
    is_interface_key = semantic == "fluid_or_interface_part" and reliable
    is_fastener = semantic == "fastener_repeated"

    # Oversized static masses such as buildings, foundations, covers, walls and slabs
    # should become cheap quickly. Size alone must not imply preservation.
    if is_bulk_static:
        if detail_rank >= 4 and not is_thin_or_plate:
            return {
                "lod_priority": "P3_standard",
                "lod_strategy": "large_static_bulk_balanced",
                "target_ratio_near": 0.62,
                "target_ratio_mid": 0.34,
                "target_ratio_far": 0.14,
                "allow_cull": False,
                "screen_error_weight": 0.90,
            }
        return {
            "lod_priority": "P2_aggressive",
            "lod_strategy": "large_static_bulk_fast_simplify",
            "target_ratio_near": 0.45,
            "target_ratio_mid": 0.20,
            "target_ratio_far": 0.06,
            "allow_cull": True,
            "screen_error_weight": 0.65,
        }

    # Repeated fasteners are usually numerous and should collapse quickly unless
    # they are the only visible source of shape complexity.
    if is_fastener:
        if is_micro or not reliable:
            return {
                "lod_priority": "P1_micro_or_uncertain",
                "lod_strategy": "fastener_micro_cull_far",
                "target_ratio_near": 0.30,
                "target_ratio_mid": 0.10,
                "target_ratio_far": 0.02,
                "allow_cull": True,
                "screen_error_weight": 0.45,
            }
        return {
            "lod_priority": "P2_aggressive",
            "lod_strategy": "fastener_repeated_aggressive",
            "target_ratio_near": 0.45,
            "target_ratio_mid": 0.20,
            "target_ratio_far": 0.06,
            "allow_cull": True,
            "screen_error_weight": 0.65,
        }

    # Preserve parts whose function depends on visible mechanical motion,
    # precision alignment, or contact surfaces.
    if is_motion_key:
        if predicted_class in {"gears_pulleys_chains", "motors_gearmotors", "bearings_bushings_guides", "springs"} or detail_rank >= 2 or is_large:
            return {
                "lod_priority": "P5_preserve",
                "lod_strategy": "preserve_motion_or_precision_key",
                "target_ratio_near": 1.00,
                "target_ratio_mid": 0.75,
                "target_ratio_far": 0.45,
                "allow_cull": False,
                "screen_error_weight": 1.50,
            }
        return {
            "lod_priority": "P4_conservative",
            "lod_strategy": "protect_motion_part",
            "target_ratio_near": 0.85,
            "target_ratio_mid": 0.55,
            "target_ratio_far": 0.30,
            "allow_cull": False,
            "screen_error_weight": 1.20,
        }

    # Structural connectors are key when they are joints, clamps or brackets.
    # Plain large plates remain bulk static and are handled above.
    if is_structural_key:
        if detail_rank >= 3 or fallback in {"geom_wire_or_rod", "geom_slender_bar", "geom_high_detail_irregular"}:
            return {
                "lod_priority": "P5_preserve",
                "lod_strategy": "preserve_structural_connector_key",
                "target_ratio_near": 0.95,
                "target_ratio_mid": 0.68,
                "target_ratio_far": 0.40,
                "allow_cull": False,
                "screen_error_weight": 1.40,
            }
        return {
            "lod_priority": "P4_conservative",
            "lod_strategy": "protect_structural_connector",
            "target_ratio_near": 0.80,
            "target_ratio_mid": 0.50,
            "target_ratio_far": 0.26,
            "allow_cull": False,
            "screen_error_weight": 1.15,
        }

    if is_control_key:
        if is_large or detail_rank >= 3:
            return {
                "lod_priority": "P4_conservative",
                "lod_strategy": "protect_control_surface",
                "target_ratio_near": 0.80,
                "target_ratio_mid": 0.50,
                "target_ratio_far": 0.26,
                "allow_cull": False,
                "screen_error_weight": 1.15,
            }
        return {
            "lod_priority": "P3_standard",
            "lod_strategy": "control_surface_balanced",
            "target_ratio_near": 0.65,
            "target_ratio_mid": 0.36,
            "target_ratio_far": 0.16,
            "allow_cull": False,
            "screen_error_weight": 0.95,
        }

    if is_interface_key:
        if detail_rank >= 3 or is_large:
            return {
                "lod_priority": "P4_conservative",
                "lod_strategy": "protect_interface_part",
                "target_ratio_near": 0.80,
                "target_ratio_mid": 0.50,
                "target_ratio_far": 0.26,
                "allow_cull": False,
                "screen_error_weight": 1.15,
            }
        return {
            "lod_priority": "P3_standard",
            "lod_strategy": "interface_part_balanced",
            "target_ratio_near": 0.65,
            "target_ratio_mid": 0.36,
            "target_ratio_far": 0.16,
            "allow_cull": False,
            "screen_error_weight": 0.95,
        }

    if not reliable:
        if is_small or is_thin_or_plate:
            return {
                "lod_priority": "P1_micro_or_uncertain",
                "lod_strategy": "uncertain_small_or_sheet_cull_far",
                "target_ratio_near": 0.30,
                "target_ratio_mid": 0.10,
                "target_ratio_far": 0.02,
                "allow_cull": True,
                "screen_error_weight": 0.45,
            }
        if is_xlarge:
            return {
                "lod_priority": "P2_aggressive",
                "lod_strategy": "uncertain_large_fast_simplify",
                "target_ratio_near": 0.45,
                "target_ratio_mid": 0.20,
                "target_ratio_far": 0.06,
                "allow_cull": True,
                "screen_error_weight": 0.65,
            }

    if detail_rank >= 4 and not is_thin_or_plate:
        return {
            "lod_priority": "P4_conservative",
            "lod_strategy": "protect_high_detail_shape",
            "target_ratio_near": 0.80,
            "target_ratio_mid": 0.50,
            "target_ratio_far": 0.26,
            "allow_cull": False,
            "screen_error_weight": 1.15,
        }
    if detail_rank >= 2 and not is_micro:
        return {
            "lod_priority": "P3_standard",
            "lod_strategy": "balanced_visible_part",
            "target_ratio_near": 0.65,
            "target_ratio_mid": 0.36,
            "target_ratio_far": 0.16,
            "allow_cull": False,
            "screen_error_weight": 0.95,
        }
    if size_rank >= 2:
        return {
            "lod_priority": "P2_aggressive",
            "lod_strategy": "ordinary_part_fast_simplify",
            "target_ratio_near": 0.45,
            "target_ratio_mid": 0.20,
            "target_ratio_far": 0.06,
            "allow_cull": True,
            "screen_error_weight": 0.65,
        }
    return {
        "lod_priority": "P1_micro_or_uncertain",
        "lod_strategy": "micro_or_trivial_cull_far",
        "target_ratio_near": 0.30,
        "target_ratio_mid": 0.10,
        "target_ratio_far": 0.02,
        "allow_cull": True,
        "screen_error_weight": 0.45,
    }


def refine_to_p10_policy(
    row: dict,
    size_rank: int,
    detail_rank: int,
    reliable: bool,
    semantic: str,
    fallback: str,
    base_policy: dict,
) -> dict:
    shape = row.get("shape_hint", "")
    predicted_class = row.get("predicted_class", "")

    is_micro = size_rank <= 1
    is_small = size_rank <= 2
    is_large = size_rank >= 4
    is_xlarge = size_rank >= 5
    is_huge = size_rank >= 6
    is_thin_or_plate = shape == "flat_plate" or fallback in {"geom_ultra_thin_sheet", "geom_thin_plate"}
    is_simple_bulk_shape = fallback in {
        "geom_ultra_thin_sheet",
        "geom_thin_plate",
        "geom_compact_block",
        "geom_simple_irregular",
    }
    is_bulk_static = (
        semantic in {"bulk_static_part", "other_part", "semantic_uncertain"}
        and is_xlarge
        and (is_simple_bulk_shape or is_thin_or_plate or is_huge)
    )
    is_fastener = semantic == "fastener_repeated"
    is_motion_key = semantic == "motion_or_precision_part" and reliable
    is_structural_key = semantic == "structural_key_part" and reliable
    is_control_key = semantic == "control_or_handle" and reliable
    is_interface_key = semantic == "fluid_or_interface_part" and reliable

    if is_fastener:
        if is_micro or not reliable:
            return make_p10_policy(1, "micro_or_uncertain_fastener_cull_far")
        return make_p10_policy(2, "repeated_fastener_aggressive")

    if is_bulk_static:
        if detail_rank >= 4 and not is_thin_or_plate:
            return make_p10_policy(4, "large_static_high_detail_low_detail_constraint")
        return make_p10_policy(3, "large_static_bulk_fast_simplify")

    if is_motion_key:
        if predicted_class in {"gears_pulleys_chains", "motors_gearmotors", "bearings_bushings_guides", "springs"} or detail_rank >= 3:
            return make_p10_policy(10, "critical_motion_or_precision_preserve")
        return make_p10_policy(9, "motion_precision_protect")

    if is_structural_key:
        if detail_rank >= 3 or fallback in {"geom_wire_or_rod", "geom_slender_bar", "geom_high_detail_irregular"}:
            return make_p10_policy(8, "structural_connector_key")
        return make_p10_policy(7, "structural_interface_connector")

    if is_interface_key:
        return make_p10_policy(7, "fluid_or_mounting_interface")

    if is_control_key:
        if is_large or detail_rank >= 3:
            return make_p10_policy(8, "structural_control_surface")
        return make_p10_policy(6, "visible_control_shape")

    if not reliable:
        if is_small or is_thin_or_plate:
            return make_p10_policy(1, "uncertain_micro_or_sheet")
        if is_xlarge:
            return make_p10_policy(3, "uncertain_large_static_fast_simplify")
        return make_p10_policy(4, "uncertain_ordinary_low_detail")

    if detail_rank >= 4 and not is_thin_or_plate:
        return make_p10_policy(6, "high_detail_shape")
    if detail_rank >= 2 and not is_micro:
        return make_p10_policy(5, "balanced_visible_part")
    if size_rank >= 2:
        return make_p10_policy(4, "ordinary_low_detail_fast_simplify")

    return make_p10_policy(1, "micro_or_trivial_cull_far")


def classify_row(row: dict, thresholds: dict, args: argparse.Namespace) -> dict:
    size_class, size_rank = classify_size(row, thresholds)
    confidence_class, reliable, margin = confidence_state(
        row, args.high_confidence, args.low_confidence, args.low_margin
    )
    semantic = semantic_group(row.get("predicted_class", ""), reliable)
    fallback = geometry_fallback(row)
    detail_class, detail_rank = detail_level(row, thresholds)
    base_policy = lod_policy(row, size_rank, detail_rank, reliable, semantic, fallback)
    policy = refine_to_p10_policy(row, size_rank, detail_rank, reliable, semantic, fallback, base_policy)

    small_low_conf_refined = size_rank <= 2 and not reliable
    if small_low_conf_refined:
        semantic = f"{semantic}:{fallback}"

    return {
        **row,
        "size_class": size_class,
        "detail_class": detail_class,
        "confidence_class": confidence_class,
        "confidence_margin": margin,
        "semantic_group": semantic,
        "geometry_fallback_class": fallback,
        "small_low_conf_refined": small_low_conf_refined,
        **policy,
    }


def summarize(rows: list[dict], thresholds: dict) -> dict:
    return {
        "num_rows": len(rows),
        "thresholds": thresholds,
        "size_class_counts": Counter(row["size_class"] for row in rows),
        "detail_class_counts": Counter(row["detail_class"] for row in rows),
        "confidence_class_counts": Counter(row["confidence_class"] for row in rows),
        "semantic_group_counts": Counter(row["semantic_group"] for row in rows),
        "geometry_fallback_counts": Counter(row["geometry_fallback_class"] for row in rows),
        "lod_priority_counts": Counter(row["lod_priority"] for row in rows),
        "lod_strategy_counts": Counter(row["lod_strategy"] for row in rows),
        "predicted_class_counts": Counter(row["predicted_class"] for row in rows),
    }


def counter_to_dict(obj):
    if isinstance(obj, Counter):
        return dict(obj)
    if isinstance(obj, dict):
        return {key: counter_to_dict(value) for key, value in obj.items()}
    return obj


def main() -> None:
    args = parse_args()
    rows = list(csv.DictReader(args.input_csv.open(encoding="utf-8-sig", newline="")))
    if not rows:
        raise ValueError(f"No rows in {args.input_csv}")

    thresholds = make_thresholds(rows)
    output_rows = [classify_row(row, thresholds, args) for row in rows]

    args.output_csv.parent.mkdir(parents=True, exist_ok=True)
    with args.output_csv.open("w", encoding="utf-8-sig", newline="") as file:
        writer = csv.DictWriter(file, fieldnames=list(output_rows[0].keys()))
        writer.writeheader()
        writer.writerows(output_rows)

    summary = summarize(output_rows, thresholds)
    if args.output_json:
        args.output_json.write_text(json.dumps(output_rows, ensure_ascii=False, indent=2), encoding="utf-8")
    if args.summary_json:
        args.summary_json.write_text(json.dumps(counter_to_dict(summary), ensure_ascii=False, indent=2), encoding="utf-8")

    print(f"Wrote: {args.output_csv.resolve()}")
    if args.output_json:
        print(f"Wrote: {args.output_json.resolve()}")
    if args.summary_json:
        print(f"Wrote: {args.summary_json.resolve()}")
    print("LOD priority counts:")
    for key, value in summary["lod_priority_counts"].most_common():
        print(f"  {key}: {value}")
    print("LOD strategy counts:")
    for key, value in summary["lod_strategy_counts"].most_common():
        print(f"  {key}: {value}")


if __name__ == "__main__":
    main()
