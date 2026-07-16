from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
SOURCE_ROOT = ROOT / "Project1" / "assets" / "props" / "generated"
DESTINATION_ROOT = ROOT / "assets" / "props"
ACTS = ("act1", "act2", "act3")
CANVAS_SIZE = (320, 400)
PADDING = 10
ALPHA_THRESHOLD = 16


def visible_bounds(image: Image.Image) -> tuple[int, int, int, int]:
    alpha = image.getchannel("A")
    mask = alpha.point(lambda value: 255 if value >= ALPHA_THRESHOLD else 0)
    bounds = mask.getbbox()
    if bounds is None:
        raise ValueError("staircase source has no visible pixels")
    return bounds


def prepare(act: str) -> None:
    source = SOURCE_ROOT / f"staircase-{act}-alpha.png"
    destination = DESTINATION_ROOT / f"staircase-{act}.png"
    with Image.open(source).convert("RGBA") as image:
        cropped = image.crop(visible_bounds(image))
        scale = min(
            (CANVAS_SIZE[0] - PADDING * 2) / cropped.width,
            (CANVAS_SIZE[1] - PADDING * 2) / cropped.height,
        )
        size = (
            max(1, round(cropped.width * scale)),
            max(1, round(cropped.height * scale)),
        )
        staircase = cropped.resize(size, Image.Resampling.LANCZOS)
        canvas = Image.new("RGBA", CANVAS_SIZE)
        left = (CANVAS_SIZE[0] - staircase.width) // 2
        top = CANVAS_SIZE[1] - PADDING - staircase.height
        canvas.alpha_composite(staircase, (left, top))

    if canvas.getpixel((0, 0))[3] != 0 or canvas.getpixel(
        (CANVAS_SIZE[0] - 1, CANVAS_SIZE[1] - 1)
    )[3] != 0:
        raise ValueError(f"{act} staircase does not have transparent corners")
    visible_bounds(canvas)
    canvas.save(destination, optimize=True)
    print(f"{destination.relative_to(ROOT)}: {canvas.size} RGBA")


def main() -> None:
    missing = [act for act in ACTS
        if not (SOURCE_ROOT / f"staircase-{act}-alpha.png").is_file()]
    if missing:
        raise FileNotFoundError("Missing staircase sources: " + ", ".join(missing))
    DESTINATION_ROOT.mkdir(parents=True, exist_ok=True)
    for act in ACTS:
        prepare(act)


if __name__ == "__main__":
    main()
