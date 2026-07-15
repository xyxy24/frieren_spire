from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Project1" / "assets" / "spell_cards" / "generated-v2"
OUTPUT = ROOT / "Project1" / "assets" / "spell_cards" / "v2"
SPELL_IDS = (
    1001, 1002, 1003, 1004, 1005, 1006, 1008, 1009, 1011, 1016, 1017,
    1018, 1019, 1020, 1021, 1022, 1023, 1024, 1025, 1026, 1027, 1028,
    1029, 1030, 2001, 2002, 2003, 2006, 2007, 2009, 2010, 2011, 2012,
)
NATIVE_SIZE = (128, 128)
PALETTE_COLORS = 24


def prepare_card(spell_id: int) -> None:
    source = SOURCE / f"{spell_id}-source.png"
    output = OUTPUT / f"{spell_id}.png"
    with Image.open(source).convert("RGB") as image:
        square_size = min(image.size)
        left = (image.width - square_size) // 2
        top = (image.height - square_size) // 2
        square = image.crop((left, top, left + square_size, top + square_size))
        reduced = square.resize(NATIVE_SIZE, resample=Image.Resampling.NEAREST)
        indexed = reduced.quantize(
            colors=PALETTE_COLORS,
            method=Image.Quantize.MEDIANCUT,
            dither=Image.Dither.NONE,
        )
        indexed.save(output, optimize=True)


def main() -> None:
    OUTPUT.mkdir(parents=True, exist_ok=True)
    missing = [spell_id for spell_id in SPELL_IDS if not (SOURCE / f"{spell_id}-source.png").is_file()]
    if missing:
        missing_list = ", ".join(str(spell_id) for spell_id in missing)
        raise FileNotFoundError(f"Missing generated spell-card sources: {missing_list}")

    for spell_id in SPELL_IDS:
        prepare_card(spell_id)
    for asset in sorted(OUTPUT.glob("*.png")):
        with Image.open(asset) as image:
            print(f"{asset.relative_to(ROOT)}: {image.size} {image.mode}")


if __name__ == "__main__":
    main()
