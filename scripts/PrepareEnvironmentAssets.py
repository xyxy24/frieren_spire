from pathlib import Path

from PIL import Image, ImageOps


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets" / "environments" / "generated"
OUTPUT = ROOT / "assets" / "environments" / "processed"


def prepare_background(act: int) -> None:
    source = SOURCE / f"act{act}-background-source.png"
    output = OUTPUT / f"act{act}-background.png"
    with Image.open(source).convert("RGB") as image:
        prepared = ImageOps.fit(
            image,
            (1280, 720),
            method=Image.Resampling.NEAREST,
            centering=(0.5, 0.5),
        )
        prepared.save(output, optimize=True)


def prepare_platform(act: int) -> None:
    source = SOURCE / f"act{act}-platform-alpha.png"
    output = OUTPUT / f"act{act}-platform.png"
    with Image.open(source).convert("RGBA") as image:
        alpha = image.getchannel("A")
        bounds = alpha.getbbox()
        if bounds is None:
            raise RuntimeError(f"act {act} platform contains no opaque pixels")
        left, top, right, bottom = bounds
        padding = 4
        cropped = image.crop((
            max(0, left - padding),
            max(0, top - padding),
            min(image.width, right + padding),
            min(image.height, bottom + padding),
        ))
        target_width = 1024
        target_height = max(64, round(cropped.height * target_width / cropped.width))
        prepared = cropped.resize(
            (target_width, target_height),
            resample=Image.Resampling.NEAREST,
        )
        prepared.save(output, optimize=True)

        corners = [
            prepared.getpixel((0, 0))[3],
            prepared.getpixel((prepared.width - 1, 0))[3],
            prepared.getpixel((0, prepared.height - 1))[3],
            prepared.getpixel((prepared.width - 1, prepared.height - 1))[3],
        ]
        if any(alpha_value != 0 for alpha_value in corners):
            raise RuntimeError(f"act {act} platform has non-transparent corners")


def main() -> None:
    OUTPUT.mkdir(parents=True, exist_ok=True)
    for act in range(1, 4):
        prepare_background(act)
        prepare_platform(act)
    for asset in sorted(OUTPUT.glob("*.png")):
        with Image.open(asset) as image:
            print(f"{asset.relative_to(ROOT)}: {image.size} {image.mode}")


if __name__ == "__main__":
    main()
