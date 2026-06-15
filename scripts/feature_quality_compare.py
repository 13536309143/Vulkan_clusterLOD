#!/usr/bin/env python3
"""
Build a four-column feature-preservation comparison figure:

  original | feature constraints off | proposed method | error heatmap

The heatmap visualizes improvement:
  error(no_feature, original) - error(proposed, original)

Red means the proposed method is closer to the original image; blue means worse.
All input images must be rendered with the same camera, resolution, lighting,
LOD setting, and tone mapping.

Optional separate maps:
  *_absolute_error.png: absolute pixel error of both simplified results.
  *_edge_error.png: Sobel edge/contour error of both simplified results.
"""

from __future__ import annotations

import argparse
import csv
import math
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
from PIL import Image


def read_rgb(path: Path) -> np.ndarray:
    image = Image.open(path).convert("RGB")
    return np.asarray(image, dtype=np.float32) / 255.0


def resize_like(image: np.ndarray, ref: np.ndarray) -> np.ndarray:
    if image.shape[:2] == ref.shape[:2]:
        return image
    pil = Image.fromarray(np.clip(image * 255.0, 0, 255).astype(np.uint8))
    pil = pil.resize((ref.shape[1], ref.shape[0]), Image.Resampling.LANCZOS)
    return np.asarray(pil, dtype=np.float32) / 255.0


def luminance(image: np.ndarray) -> np.ndarray:
    return image[..., 0] * 0.2126 + image[..., 1] * 0.7152 + image[..., 2] * 0.0722


def rmse(a: np.ndarray, b: np.ndarray) -> float:
    return float(np.sqrt(np.mean((a - b) ** 2)))


def psnr(a: np.ndarray, b: np.ndarray) -> float:
    mse = float(np.mean((a - b) ** 2))
    if mse <= 1e-12:
        return float("inf")
    return 10.0 * math.log10(1.0 / mse)


def ssim_global(a: np.ndarray, b: np.ndarray) -> float:
    """Small dependency-free SSIM approximation on luminance images."""
    x = luminance(a)
    y = luminance(b)
    c1 = 0.01**2
    c2 = 0.03**2

    mux = float(np.mean(x))
    muy = float(np.mean(y))
    vx = float(np.var(x))
    vy = float(np.var(y))
    cov = float(np.mean((x - mux) * (y - muy)))

    numerator = (2.0 * mux * muy + c1) * (2.0 * cov + c2)
    denominator = (mux * mux + muy * muy + c1) * (vx + vy + c2)
    return numerator / denominator if denominator > 0.0 else 1.0


def pixel_error(a: np.ndarray, b: np.ndarray) -> np.ndarray:
    return np.linalg.norm(a - b, axis=2)


def convolve3x3(image: np.ndarray, kernel: np.ndarray) -> np.ndarray:
    padded = np.pad(image, ((1, 1), (1, 1)), mode="edge")
    out = np.zeros_like(image, dtype=np.float32)
    for y in range(3):
        for x in range(3):
            out += kernel[y, x] * padded[y : y + image.shape[0], x : x + image.shape[1]]
    return out


def sobel_magnitude(image: np.ndarray) -> np.ndarray:
    gray = luminance(image).astype(np.float32)
    kx = np.array([[-1.0, 0.0, 1.0], [-2.0, 0.0, 2.0], [-1.0, 0.0, 1.0]], dtype=np.float32)
    ky = np.array([[-1.0, -2.0, -1.0], [0.0, 0.0, 0.0], [1.0, 2.0, 1.0]], dtype=np.float32)
    gx = convolve3x3(gray, kx)
    gy = convolve3x3(gray, ky)
    return np.sqrt(gx * gx + gy * gy)


def crop(image: np.ndarray, rect: tuple[int, int, int, int] | None) -> np.ndarray:
    if rect is None:
        return image
    x, y, w, h = rect
    return image[y : y + h, x : x + w]


def parse_crop(value: str | None) -> tuple[int, int, int, int] | None:
    if not value:
        return None
    parts = [int(v.strip()) for v in value.split(",")]
    if len(parts) != 4:
        raise ValueError("--crop must be x,y,w,h")
    return tuple(parts)  # type: ignore[return-value]


def save_metrics(path: Path, rows: list[dict[str, str]]) -> None:
    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)


def save_absolute_error_maps(
    path: Path,
    err_no: np.ndarray,
    err_method: np.ndarray,
    dpi: int,
) -> None:
    vmax = float(np.percentile(np.concatenate([err_no.ravel(), err_method.ravel()]), 99.0))
    vmax = max(vmax, 1e-4)

    fig, axes = plt.subplots(1, 2, figsize=(9, 4.2), constrained_layout=True)
    panels = [
        ("Feature constraints off", err_no),
        ("Proposed method", err_method),
    ]
    for ax, (title, err) in zip(axes, panels):
        image = ax.imshow(err, cmap="magma", vmin=0.0, vmax=vmax)
        ax.set_title(title, fontsize=12)
        ax.axis("off")
    cbar = fig.colorbar(image, ax=axes, fraction=0.035, pad=0.03)
    cbar.set_label("pixel error")
    fig.savefig(path, dpi=dpi)
    plt.close(fig)


def save_edge_error_maps(
    path: Path,
    original: np.ndarray,
    nofeature: np.ndarray,
    method: np.ndarray,
    dpi: int,
) -> None:
    edge_ref = sobel_magnitude(original)
    edge_no = sobel_magnitude(nofeature)
    edge_method = sobel_magnitude(method)
    err_no = np.abs(edge_no - edge_ref)
    err_method = np.abs(edge_method - edge_ref)

    edge_vmax = max(float(np.percentile(edge_ref, 99.0)), 1e-4)
    err_vmax = float(np.percentile(np.concatenate([err_no.ravel(), err_method.ravel()]), 99.0))
    err_vmax = max(err_vmax, 1e-4)

    fig, axes = plt.subplots(1, 3, figsize=(12, 4.2), constrained_layout=True)
    axes[0].imshow(edge_ref, cmap="gray", vmin=0.0, vmax=edge_vmax)
    axes[0].set_title("Original edge magnitude", fontsize=12)
    axes[0].axis("off")

    panels = [
        ("Edge error: constraints off", err_no),
        ("Edge error: proposed method", err_method),
    ]
    for ax, (title, err) in zip(axes[1:], panels):
        image = ax.imshow(err, cmap="inferno", vmin=0.0, vmax=err_vmax)
        ax.set_title(title, fontsize=12)
        ax.axis("off")
    cbar = fig.colorbar(image, ax=axes[1:], fraction=0.035, pad=0.03)
    cbar.set_label("edge error")
    fig.savefig(path, dpi=dpi)
    plt.close(fig)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--original", required=True, type=Path)
    parser.add_argument("--nofeature", required=True, type=Path)
    parser.add_argument("--method", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument("--metrics", type=Path)
    parser.add_argument("--crop", help="Optional ROI: x,y,w,h")
    parser.add_argument("--dpi", type=int, default=180)
    parser.add_argument("--extra-maps", action="store_true", help="Save separate absolute-error and edge-error maps next to --out")
    parser.add_argument("--skip-metrics", action="store_true", help="Do not write a metrics CSV")
    args = parser.parse_args()

    rect = parse_crop(args.crop)
    original = read_rgb(args.original)
    nofeature = resize_like(read_rgb(args.nofeature), original)
    method = resize_like(read_rgb(args.method), original)

    original = crop(original, rect)
    nofeature = crop(nofeature, rect)
    method = crop(method, rect)

    err_no = pixel_error(nofeature, original)
    err_method = pixel_error(method, original)
    improvement = err_no - err_method

    vmax = float(np.percentile(np.abs(improvement), 99.0))
    vmax = max(vmax, 1e-4)

    metrics_rows = [
        {
            "method": "feature_constraints_off",
            "rmse": f"{rmse(nofeature, original):.6f}",
            "psnr_db": f"{psnr(nofeature, original):.3f}",
            "ssim": f"{ssim_global(nofeature, original):.6f}",
            "mean_pixel_error": f"{float(np.mean(err_no)):.6f}",
            "p95_pixel_error": f"{float(np.percentile(err_no, 95.0)):.6f}",
        },
        {
            "method": "proposed",
            "rmse": f"{rmse(method, original):.6f}",
            "psnr_db": f"{psnr(method, original):.3f}",
            "ssim": f"{ssim_global(method, original):.6f}",
            "mean_pixel_error": f"{float(np.mean(err_method)):.6f}",
            "p95_pixel_error": f"{float(np.percentile(err_method, 95.0)):.6f}",
        },
    ]

    fig, axes = plt.subplots(1, 4, figsize=(16, 4.5), constrained_layout=True)
    panels = [
        ("Original", original),
        ("Feature constraints off", nofeature),
        ("Proposed method", method),
    ]

    for ax, (title, image) in zip(axes[:3], panels):
        ax.imshow(np.clip(image, 0.0, 1.0))
        ax.set_title(title, fontsize=12)
        ax.axis("off")

    heat = axes[3].imshow(improvement, cmap="coolwarm", vmin=-vmax, vmax=vmax)
    axes[3].set_title("Error improvement heatmap", fontsize=12)
    axes[3].axis("off")
    cbar = fig.colorbar(heat, ax=axes[3], fraction=0.046, pad=0.04)
    cbar.set_label("err(off)-err(proposed)")

    args.out.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(args.out, dpi=args.dpi)
    plt.close(fig)

    if args.extra_maps:
        save_absolute_error_maps(args.out.with_name(f"{args.out.stem}_absolute_error.png"), err_no, err_method, args.dpi)
        save_edge_error_maps(args.out.with_name(f"{args.out.stem}_edge_error.png"), original, nofeature, method, args.dpi)

    metrics_path = args.metrics or args.out.with_suffix(".metrics.csv")
    if not args.skip_metrics:
        save_metrics(metrics_path, metrics_rows)

    print(f"saved comparison: {args.out}")
    if args.extra_maps:
        print(f"saved absolute error map: {args.out.with_name(f'{args.out.stem}_absolute_error.png')}")
        print(f"saved edge error map: {args.out.with_name(f'{args.out.stem}_edge_error.png')}")
    if not args.skip_metrics:
        print(f"saved metrics: {metrics_path}")
    for row in metrics_rows:
        print(row)


if __name__ == "__main__":
    main()
