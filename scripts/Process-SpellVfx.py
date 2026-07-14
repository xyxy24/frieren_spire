#!/usr/bin/env python3
"""Normalize generated spell VFX contact sheets into transparent runtime atlases."""

from __future__ import annotations

import argparse
import json
from dataclasses import dataclass
from pathlib import Path

from PIL import Image


SAFETY_MARGIN = 2


@dataclass(frozen=True)
class VfxSource:
    columns: int
    cell_width: int
    cell_height: int
    frames_per_second: int
    loops: bool
    chroma: str = "green"
    anchor_x: int | None = None
    anchor_y: int | None = None
    align_bottom: bool = False
    detect_boundaries: bool = False
    panel_edges: tuple[int, ...] | None = None


SOURCES: dict[str, VfxSource] = {
    "cast-arcane": VfxSource(6, 64, 64, 15, False),
    "zoltraak-impact": VfxSource(8, 96, 96, 16, False),
    "ice-lance": VfxSource(6, 96, 48, 18, False),
    "stone-shot": VfxSource(6, 96, 48, 16, False),
    "homing-bolt": VfxSource(6, 64, 32, 20, False),
    "zoltraak-muzzle": VfxSource(6, 64, 64, 15, False),
    "flame-burst": VfxSource(8, 192, 128, 16, False),
    "wind-pressure": VfxSource(8, 192, 128, 16, False, "magenta"),
    "mana-strike": VfxSource(
        8,
        192,
        128,
        16,
        False,
        panel_edges=(0, 190, 398, 656, 972, 1284, 1608, 1875, 2172),
    ),
    "mana-trace-ring": VfxSource(8, 192, 192, 14, False),
    "dispel-ring": VfxSource(8, 192, 192, 16, False),
    "spatial-shatter": VfxSource(10, 192, 192, 16, False),
    "gravity-well": VfxSource(
        8, 192, 96, 12, False, anchor_x=96, anchor_y=88, align_bottom=True
    ),
    "goddess-blessing": VfxSource(
        8, 128, 128, 12, False, anchor_x=64, anchor_y=112, align_bottom=True
    ),
    "flight-aura": VfxSource(
        8, 128, 128, 12, False, anchor_x=64, anchor_y=112, align_bottom=True
    ),
    "lightning-staff": VfxSource(8, 128, 128, 12, False),
    "defensive-barrier": VfxSource(
        8, 128, 128, 14, False, anchor_x=64, anchor_y=112, align_bottom=True
    ),
    "magic-thread": VfxSource(8, 96, 96, 16, False),
    "golden-binding": VfxSource(
        8,
        96,
        96,
        14,
        False,
        "green-gold",
        panel_edges=(0, 258, 622, 822, 1122, 1378, 1628, 1918, 2172),
    ),
    "sealing-magic": VfxSource(8, 96, 96, 14, False),
    "float-slam": VfxSource(
        8, 192, 192, 14, False, anchor_x=96, anchor_y=176, align_bottom=True
    ),
    "goddess-spears": VfxSource(
        10, 192, 192, 14, False, anchor_x=96, anchor_y=176, align_bottom=True
    ),
    "destruction-lightning": VfxSource(
        10, 192, 192, 16, False, anchor_x=96, anchor_y=176, align_bottom=True
    ),
    "earth-domination": VfxSource(
        10, 192, 192, 12, False, anchor_x=96, anchor_y=176, align_bottom=True
    ),
    "phantom": VfxSource(
        10,
        128,
        96,
        12,
        False,
        anchor_x=64,
        anchor_y=92,
        align_bottom=True,
        detect_boundaries=True,
    ),
    "stone-golem": VfxSource(
        10,
        128,
        96,
        10,
        False,
        anchor_x=64,
        anchor_y=92,
        align_bottom=True,
        detect_boundaries=True,
    ),
    "mirror-array": VfxSource(
        9,
        128,
        96,
        12,
        False,
        anchor_x=64,
        anchor_y=92,
        align_bottom=True,
        detect_boundaries=True,
    ),
    "flower-field": VfxSource(
        9,
        192,
        96,
        10,
        False,
        "magenta",
        anchor_x=96,
        anchor_y=88,
        align_bottom=True,
        detect_boundaries=True,
    ),
    "blood-magic": VfxSource(
        8,
        192,
        128,
        16,
        False,
        panel_edges=(0, 181, 358, 532, 818, 1252, 1678, 1956, 2172),
    ),
    "severing-magic": VfxSource(
        10, 192, 128, 16, False, detect_boundaries=True
    ),
    "hellfire-storm": VfxSource(
        8,
        128,
        256,
        12,
        True,
        "magenta",
        anchor_x=64,
        anchor_y=248,
        align_bottom=True,
        detect_boundaries=True,
    ),
    "mimic-magic": VfxSource(
        10,
        192,
        128,
        14,
        False,
        panel_edges=(
            0,
            136,
            312,
            500,
            685,
            1003,
            1226,
            1466,
            1598,
            1788,
            1983,
        ),
    ),
}


def is_chroma(red: int, green: int, blue: int, chroma: str) -> bool:
    """Remove the generated green plate and green-contaminated edge pixels.

    Cyan spell pixels are preserved because their blue channel is close to or
    greater than green. Ochre and gold pixels are preserved because red is high.
    """
    if chroma == "magenta":
        return (
            red >= 96
            and blue >= 96
            and red >= green + 45
            and blue >= green + 36
        )
    base_green = green >= 96 and green >= red + 45 and green >= blue + 36
    if chroma == "green-gold":
        # Gold keeps red above green. This stricter variant removes lime spill
        # left by the generated Golden Binding source without erasing gold.
        return base_green or (
            green >= 80 and green >= red + 6 and green >= blue + 30
        )
    return base_green


def remove_chroma(image: Image.Image, chroma: str) -> Image.Image:
    rgba = image.convert("RGBA")
    cleaned: list[tuple[int, int, int, int]] = []
    for red, green, blue, alpha in rgba.getdata():
        if is_chroma(red, green, blue, chroma):
            cleaned.append((0, 0, 0, 0))
        else:
            cleaned.append((red, green, blue, alpha))
    rgba.putdata(cleaned)
    return rgba


def detected_panel_edges(cleaned: Image.Image, columns: int) -> list[int]:
    """Find real frame gaps near the expected equally spaced boundaries.

    Generated sheets sometimes distribute key poses unevenly. Searching only
    near each expected boundary avoids mistaking interior gaps, such as the
    space between two mirrors, for a frame separator.
    """
    alpha = cleaned.getchannel("A")
    column_has_pixels = [
        alpha.crop((x, 0, x + 1, alpha.height)).getbbox() is not None
        for x in range(alpha.width)
    ]
    gaps: list[tuple[int, int]] = []
    start: int | None = None
    for x, occupied in enumerate([*column_has_pixels, True]):
        if not occupied and start is None:
            start = x
        elif occupied and start is not None:
            if x - start >= 3:
                gaps.append((start, x))
            start = None

    nominal_width = cleaned.width / columns
    edges = [0]
    for index in range(1, columns):
        expected = nominal_width * index
        search_radius = nominal_width * 0.5
        candidates = []
        for left, right in gaps:
            midpoint = (left + right) / 2.0
            if abs(midpoint - expected) <= search_radius:
                width = right - left
                score = width - abs(midpoint - expected) * 0.2
                candidates.append(
                    (score, width, -abs(midpoint - expected), midpoint)
                )
        if candidates:
            boundary = round(max(candidates)[3])
        else:
            boundary = round(expected)
        minimum = edges[-1] + round(nominal_width * 0.4)
        maximum = round(cleaned.width - (columns - index) * nominal_width * 0.4)
        edges.append(max(minimum, min(boundary, maximum)))
    edges.append(cleaned.width)
    return edges


def extract_panels(source: Image.Image, config: VfxSource) -> list[Image.Image]:
    cleaned = remove_chroma(source, config.chroma)
    if config.panel_edges is not None:
        edges = list(config.panel_edges)
        if (
            len(edges) != config.columns + 1
            or edges[0] != 0
            or edges[-1] != source.width
        ):
            raise ValueError(
                f"reviewed panel edges do not match source width for {source.width}px sheet"
            )
    elif config.detect_boundaries:
        edges = detected_panel_edges(cleaned, config.columns)
    else:
        edges = [
            round(index * source.width / config.columns)
            for index in range(config.columns + 1)
        ]
    panels: list[Image.Image] = []
    for index in range(config.columns):
        panel = cleaned.crop((edges[index], 0, edges[index + 1], source.height))
        bounds = panel.getchannel("A").getbbox()
        if bounds is None:
            raise ValueError(f"frame {index} contains no foreground pixels")
        panels.append(panel.crop(bounds))
    return panels


def normalize_panels(panels: list[Image.Image], config: VfxSource) -> list[Image.Image]:
    maximum_width = max(panel.width for panel in panels)
    maximum_height = max(panel.height for panel in panels)
    available_width = config.cell_width - SAFETY_MARGIN * 2
    available_height = config.cell_height - SAFETY_MARGIN * 2
    scale = min(available_width / maximum_width, available_height / maximum_height)

    frames: list[Image.Image] = []
    for panel in panels:
        width = max(1, round(panel.width * scale))
        height = max(1, round(panel.height * scale))
        resized = panel.resize((width, height), Image.Resampling.NEAREST)
        frame = Image.new("RGBA", (config.cell_width, config.cell_height), (0, 0, 0, 0))
        resized_bounds = resized.getchannel("A").getbbox()
        if resized_bounds is None:
            raise ValueError("normalized frame contains no foreground pixels")
        destination_y = (
            config.cell_height - SAFETY_MARGIN - resized_bounds[3]
            if config.align_bottom
            else (config.cell_height - height) // 2
        )
        destination = ((config.cell_width - width) // 2, destination_y)
        frame.alpha_composite(resized, destination)
        frames.append(frame)
    return frames


def process(source_root: Path, output_root: Path, name: str) -> dict[str, object]:
    config = SOURCES[name]
    source_path = source_root / f"{name}-source.png"
    source = Image.open(source_path).convert("RGBA")
    frames = normalize_panels(extract_panels(source, config), config)

    frame_root = output_root / "frames" / name
    frame_root.mkdir(parents=True, exist_ok=True)
    for stale_frame in frame_root.glob("frame-*.png"):
        stale_frame.unlink()
    for index, frame in enumerate(frames):
        frame.save(frame_root / f"frame-{index:02d}.png")

    sheet = Image.new(
        "RGBA", (config.cell_width * len(frames), config.cell_height), (0, 0, 0, 0)
    )
    for index, frame in enumerate(frames):
        sheet.alpha_composite(frame, (config.cell_width * index, 0))
    sheet_path = output_root / f"{name}.png"
    sheet.save(sheet_path)

    return {
        "name": name,
        "source": source_path.as_posix(),
        "sheet": sheet_path.as_posix(),
        "frameCount": len(frames),
        "frameWidth": config.cell_width,
        "frameHeight": config.cell_height,
        "anchorX": config.anchor_x
        if config.anchor_x is not None
        else config.cell_width // 2,
        "anchorY": config.anchor_y
        if config.anchor_y is not None
        else config.cell_height // 2,
        "framesPerSecond": config.frames_per_second,
        "loops": config.loops,
        "alignBottom": config.align_bottom,
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--source-root", type=Path, default=Path("Project1/assets/spells/generated")
    )
    parser.add_argument(
        "--output-root", type=Path, default=Path("Project1/assets/spells/processed")
    )
    parser.add_argument("--animation", choices=["all", *SOURCES], default="all")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    names = list(SOURCES) if args.animation == "all" else [args.animation]
    args.output_root.mkdir(parents=True, exist_ok=True)
    manifest = [process(args.source_root, args.output_root, name) for name in names]
    manifest_path = args.output_root / "manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(f"Processed {sum(item['frameCount'] for item in manifest)} spell VFX frames")
    print(f"Manifest: {manifest_path}")


if __name__ == "__main__":
    main()
