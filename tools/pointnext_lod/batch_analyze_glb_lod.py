"""Batch PointNeXt + feature LOD analysis for multiple GLB models.

For each GLB, this runs:
1) PointNeXt per-mesh classification and geometry extraction.
2) Feature/LOD post-processing from the PointNeXt analysis CSV.

The output keeps one row per original mesh, so downstream LOD systems can still
address the original mesh/node identifiers.
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


DEFAULT_MODELS = ["a.glb", "b.glb", "c.glb", "o1778.glb", "o3049.glb"]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Batch analyze GLB files for LOD constraints.")
    parser.add_argument(
        "--resources-dir",
        type=Path,
        required=True,
        help="Directory containing a.glb, b.glb, c.glb, o1778.glb, o3049.glb.",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("lod_analysis_outputs"),
        help="Directory for generated CSV/JSON files.",
    )
    parser.add_argument("--pointnext-root", type=Path, required=True)
    parser.add_argument("--cfg", type=Path, required=True)
    parser.add_argument("--ckpt", type=Path, required=True)
    parser.add_argument("--classes-file", type=Path, required=True)
    parser.add_argument("--models", nargs="*", default=DEFAULT_MODELS)
    parser.add_argument("--num-points", type=int, default=8192)
    parser.add_argument("--batch-size", type=int, default=16)
    parser.add_argument("--device", choices=["cuda", "cpu"], default="cuda")
    parser.add_argument("--skip-existing", action="store_true")
    parser.add_argument(
        "--pointnext-only",
        action="store_true",
        help="Only run PointNeXt analysis, skip LOD feature post-processing.",
    )
    return parser.parse_args()


def run(cmd: list[str], cwd: Path) -> None:
    print("\n> " + " ".join(f'"{item}"' if " " in item else item for item in cmd), flush=True)
    subprocess.run(cmd, cwd=str(cwd), check=True)


def model_stem(path: Path) -> str:
    return path.stem.replace(" ", "_")


def main() -> None:
    args = parse_args()
    resources_dir = args.resources_dir.resolve()
    output_dir = args.output_dir.resolve()
    script_dir = Path(__file__).resolve().parent
    analyze_script = script_dir / "analyze_large_glb_parts_pointnext.py"
    lod_script = script_dir / "build_lod_constraints_from_analysis.py"

    missing = []
    for name in args.models:
        path = resources_dir / name
        if not path.exists():
            missing.append(str(path))
    if missing:
        raise FileNotFoundError("Missing GLB files:\n" + "\n".join(missing))

    output_dir.mkdir(parents=True, exist_ok=True)

    for name in args.models:
        glb_path = resources_dir / name
        stem = model_stem(glb_path)
        analysis_csv = output_dir / f"{stem}_pointnext_analysis.csv"
        analysis_json = output_dir / f"{stem}_pointnext_analysis.json"
        summary_csv = output_dir / f"{stem}_pointnext_class_summary.csv"
        review_csv = output_dir / f"{stem}_pointnext_review_candidates.csv"
        lod_csv = output_dir / f"{stem}_lod_constraints.csv"
        lod_summary = output_dir / f"{stem}_lod_constraints_summary.json"

        if not (args.skip_existing and analysis_csv.exists()):
            run(
                [
                    sys.executable,
                    str(analyze_script),
                    "--glb",
                    str(glb_path),
                    "--pointnext-root",
                    str(args.pointnext_root.resolve()),
                    "--cfg",
                    str(args.cfg.resolve()),
                    "--ckpt",
                    str(args.ckpt.resolve()),
                    "--classes-file",
                    str(args.classes_file.resolve()),
                    "--num-points",
                    str(args.num_points),
                    "--batch-size",
                    str(args.batch_size),
                    "--device",
                    args.device,
                    "--inference-mode",
                    "mesh",
                    "--output-csv",
                    str(analysis_csv),
                    "--output-json",
                    str(analysis_json),
                    "--summary-csv",
                    str(summary_csv),
                    "--review-csv",
                    str(review_csv),
                ],
                cwd=script_dir,
            )
        else:
            print(f"Skip existing PointNeXt output: {analysis_csv}")

        if args.pointnext_only:
            continue

        if not (args.skip_existing and lod_csv.exists()):
            run(
                [
                    sys.executable,
                    str(lod_script),
                    "--input-csv",
                    str(analysis_csv),
                    "--output-csv",
                    str(lod_csv),
                    "--summary-json",
                    str(lod_summary),
                ],
                cwd=script_dir,
            )
        else:
            print(f"Skip existing LOD output: {lod_csv}")

    print(f"\nDone. Outputs: {output_dir}")


if __name__ == "__main__":
    main()
