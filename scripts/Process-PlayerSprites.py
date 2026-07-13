#!/usr/bin/env python3
"""Convert AI key-pose contact sheets into normalized transparent sprite sheets."""

from __future__ import annotations

import argparse
import json
from dataclasses import dataclass
from pathlib import Path

from PIL import Image


CELL_WIDTH = 128
CELL_HEIGHT = 96
ANCHOR_X = 64
BASELINE_Y = 92
HORIZONTAL_MARGIN = 2


@dataclass(frozen=True)
class AnimationSource:
    columns: int
    rows: int
    scale: float
    frames_per_second: int
    loops: bool


SOURCES: dict[str, AnimationSource] = {
    "idle": AnimationSource(2, 2, 0.16, 6, True),
    "run": AnimationSource(3, 2, 0.22, 10, True),
    "jump": AnimationSource(4, 2, 0.24, 10, False),
    "attack": AnimationSource(3, 2, 0.21, 12, False),
    "cast": AnimationSource(3, 2, 0.22, 10, False),
    "dash": AnimationSource(2, 2, 0.24, 22, False),
    "shade-dash": AnimationSource(3, 2, 0.24, 33, False),
    "hit": AnimationSource(2, 2, 0.17, 10, False),
    "defeat": AnimationSource(3, 2, 0.22, 8, False),
    "pickup": AnimationSource(2, 2, 0.17, 8, False),
    "interact": AnimationSource(2, 2, 0.17, 8, False),
}


def is_chroma(pixel: tuple[int, int, int, int]) -> bool:
    red, green, blue, _ = pixel
    return green >= 180 and green - red >= 70 and green - blue >= 70


def remove_chroma(image: Image.Image) -> Image.Image:
    rgba = image.convert("RGBA")
    pixels = list(rgba.getdata())
    rgba.putdata([(red, green, blue, 0 if is_chroma((red, green, blue, alpha)) else alpha)
                  for red, green, blue, alpha in pixels])
    return rgba


def alpha_bounds(image: Image.Image) -> tuple[int, int, int, int]:
    alpha = image.getchannel("A")
    bounds = alpha.getbbox()
    if bounds is None:
        raise ValueError("frame contains no foreground pixels after chroma removal")
    return bounds


def horizontal_anchor(image: Image.Image, bounds: tuple[int, int, int, int]) -> float:
    """Estimate the body root from dense pixels near the bottom of the pose.

    The median resists a thin staff tip while following boots and grounded limbs.
    """
    left, top, right, bottom = bounds
    band_top = max(top, bottom - max(24, round((bottom - top) * 0.24)))
    alpha = image.getchannel("A")
    samples: list[int] = []
    for y in range(band_top, bottom):
        for x in range(left, right):
            if alpha.getpixel((x, y)) >= 128:
                samples.append(x)
    if not samples:
        return (left + right) * 0.5
    samples.sort()
    return float(samples[len(samples) // 2])


def normalized_frame(panel: Image.Image, scale: float) -> Image.Image:
    transparent = remove_chroma(panel)
    bounds = alpha_bounds(transparent)
    source_anchor_x = horizontal_anchor(transparent, bounds)
    cropped = transparent.crop(bounds)
    width = max(1, round(cropped.width * scale))
    height = max(1, round(cropped.height * scale))
    resized = cropped.resize((width, height), Image.Resampling.NEAREST)

    anchor_in_crop = (source_anchor_x - bounds[0]) * scale
    destination_x = round(ANCHOR_X - anchor_in_crop)
    destination_y = BASELINE_Y - height
    available_width = CELL_WIDTH - 2 * HORIZONTAL_MARGIN
    if width > available_width:
        raise ValueError(
            "normalized frame exceeds horizontal cell after reserving the "
            f"{HORIZONTAL_MARGIN}px margin: width={width}"
        )
    destination_x = max(
        HORIZONTAL_MARGIN,
        min(destination_x, CELL_WIDTH - HORIZONTAL_MARGIN - width),
    )
    if height > CELL_HEIGHT:
        raise ValueError(
            f"normalized frame exceeds vertical cell: height={height}"
        )
    destination_y = max(0, min(destination_y, CELL_HEIGHT - height))

    frame = Image.new("RGBA", (CELL_WIDTH, CELL_HEIGHT), (0, 0, 0, 0))
    frame.alpha_composite(resized, (destination_x, destination_y))
    return frame


def panel_bounds(
    width: int, height: int, columns: int, rows: int, index: int
) -> tuple[int, int, int, int]:
    column = index % columns
    row = index // columns
    return (
        round(column * width / columns),
        round(row * height / rows),
        round((column + 1) * width / columns),
        round((row + 1) * height / rows),
    )


def process_animation(source_root: Path, output_root: Path, name: str) -> dict[str, object]:
    config = SOURCES[name]
    source_path = source_root / f"frieren-{name}-keyposes.png"
    if not source_path.is_file():
        raise FileNotFoundError(source_path)

    source = Image.open(source_path).convert("RGBA")
    frame_count = config.columns * config.rows
    frames: list[Image.Image] = []
    frame_root = output_root / "frames" / name
    frame_root.mkdir(parents=True, exist_ok=True)

    for index in range(frame_count):
        panel = source.crop(panel_bounds(
            source.width, source.height, config.columns, config.rows, index
        ))
        frame = normalized_frame(panel, config.scale)
        frame.save(frame_root / f"frame-{index:02d}.png")
        frames.append(frame)

    sheet = Image.new("RGBA", (CELL_WIDTH * frame_count, CELL_HEIGHT), (0, 0, 0, 0))
    for index, frame in enumerate(frames):
        sheet.alpha_composite(frame, (CELL_WIDTH * index, 0))
    sheet_path = output_root / f"frieren-{name}.png"
    sheet.save(sheet_path)

    return {
        "name": name,
        "source": source_path.as_posix(),
        "sheet": sheet_path.as_posix(),
        "frameCount": frame_count,
        "frameWidth": CELL_WIDTH,
        "frameHeight": CELL_HEIGHT,
        "anchorX": ANCHOR_X,
        "baselineY": BASELINE_Y,
        "framesPerSecond": config.frames_per_second,
        "loops": config.loops,
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--source-root",
        type=Path,
        default=Path("Project1/assets/player/generated"),
    )
    parser.add_argument(
        "--output-root",
        type=Path,
        default=Path("Project1/assets/player/processed"),
    )
    parser.add_argument(
        "--animation",
        choices=["all", *SOURCES.keys()],
        default="all",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    names = list(SOURCES) if args.animation == "all" else [args.animation]
    args.output_root.mkdir(parents=True, exist_ok=True)
    manifest = [process_animation(args.source_root, args.output_root, name) for name in names]
    manifest_path = args.output_root / "manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(f"Processed {sum(item['frameCount'] for item in manifest)} frames")
    print(f"Manifest: {manifest_path}")


if __name__ == "__main__":
    main()
