from pathlib import Path
from statistics import median
from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[1]
ENEMY_ROOT = ROOT / "assets" / "enemies"
SOURCE_ROOT = ROOT / "Project1" / "assets" / "enemies" / "generated-animation-sheets"
GRID_COLUMNS = 4
GRID_ROWS = 3
ALPHA_BOUNDS_THRESHOLD = 32

# These two generated recovery poses were drawn at a visibly different intrinsic
# scale even though their transparent bounds were close to the other frames.
FRAME_SCALE_OVERRIDES = {
    ("headless_knight", 2, 3): 1.18,
    ("lugner", 2, 3): 1.20,
}

ENEMIES = (
    "aura",
    "bird_demon",
    "chaos_flower",
    "chest_mimic",
    "demon_warrior",
    "draht",
    "frost_wolf",
    "headless_knight",
    "heimon",
    "large_bird_demon",
    "linie",
    "lugner",
    "qual",
)


def frame_size(enemy: str) -> tuple[int, int]:
    with Image.open(ENEMY_ROOT / enemy / "idle.png") as image:
        return image.size


def reference_path(enemy: str, row: int) -> Path:
    names = ("idle.png", "windup.png", "attack.png")
    requested = ENEMY_ROOT / enemy / names[row]
    if requested.is_file():
        return requested
    windup = ENEMY_ROOT / enemy / "windup.png"
    return windup if windup.is_file() else ENEMY_ROOT / enemy / "idle.png"


def visible_bounds(image: Image.Image) -> tuple[int, int, int, int]:
    alpha = image.getchannel("A")
    mask = alpha.point(
        lambda value: 255 if value >= ALPHA_BOUNDS_THRESHOLD else 0
    )
    bounds = mask.getbbox()
    if bounds is None:
        raise ValueError("Animation frame has no visible pixels")
    return bounds


def centered_cell_bounds(
    sheet_size: tuple[int, int], column: int, row: int,
    columns: int = GRID_COLUMNS, rows: int = GRID_ROWS,
) -> tuple[int, int, int, int]:
    width, height = sheet_size
    cell_size = min(width / columns, height / rows)
    center_x = (column + 0.5) * width / columns
    center_y = (row + 0.5) * height / rows
    left = round(center_x - cell_size * 0.5)
    top = round(center_y - cell_size * 0.5)
    right = round(center_x + cell_size * 0.5)
    bottom = round(center_y + cell_size * 0.5)
    return left, top, right, bottom


def prepare_enemy(enemy: str) -> None:
    source = SOURCE_ROOT / f"{enemy}-alpha.png"
    destination = ENEMY_ROOT / enemy / "animation.png"
    width, height = frame_size(enemy)
    canvas_width = width * 3
    canvas_height = height * 2
    horizontal_padding = (canvas_width - width) * 0.5
    atlas = Image.new(
        "RGBA", (canvas_width * GRID_COLUMNS, canvas_height * GRID_ROWS)
    )

    with Image.open(source).convert("RGBA") as sheet:
        cells: list[list[Image.Image]] = []
        bounds_by_row: list[list[tuple[int, int, int, int]]] = []
        for row in range(GRID_ROWS):
            row_cells: list[Image.Image] = []
            row_bounds: list[tuple[int, int, int, int]] = []
            for column in range(GRID_COLUMNS):
                cell = sheet.crop(centered_cell_bounds(sheet.size, column, row))
                try:
                    bounds = visible_bounds(cell)
                except ValueError as error:
                    raise ValueError(
                        f"Empty frame {column},{row} in {source}"
                    ) from error
                row_cells.append(cell)
                row_bounds.append(bounds)
            cells.append(row_cells)
            bounds_by_row.append(row_bounds)

        with Image.open(reference_path(enemy, 0)).convert("RGBA") as idle_reference:
            idle_reference_bounds = visible_bounds(idle_reference)
            idle_reference_height = idle_reference_bounds[3] - idle_reference_bounds[1]
        source_idle_height = median(
            bounds[3] - bounds[1] for bounds in bounds_by_row[0]
        )
        shared_scale = idle_reference_height / max(source_idle_height, 1)

        for row in range(GRID_ROWS):
            with Image.open(reference_path(enemy, row)).convert("RGBA") as reference:
                reference_bounds = visible_bounds(reference)
                target_bottom = canvas_height - (height - reference_bounds[3])
            target_anchor_x = width * 0.5 + horizontal_padding

            for column in range(GRID_COLUMNS):
                cell = cells[row][column]
                bounds = bounds_by_row[row][column]
                cropped = cell.crop(bounds)
                source_anchor = cell.width * 0.5 - bounds[0]
                frame_scale = shared_scale * FRAME_SCALE_OVERRIDES.get(
                    (enemy, row, column), 1.0
                )
                scaled_width = max(1, round(cropped.width * frame_scale))
                scaled_height = max(1, round(cropped.height * frame_scale))
                frame = cropped.resize(
                    (scaled_width, scaled_height), Image.Resampling.NEAREST
                )

                anchor_scale = scaled_width / cropped.width
                left = round(target_anchor_x - source_anchor * anchor_scale)
                top = target_bottom - scaled_height
                if (left < 0 or left + scaled_width > canvas_width
                    or top < 0 or top + scaled_height > canvas_height):
                    raise ValueError(
                        f"Frame {column},{row} for {enemy} exceeds its padded cell: "
                        f"{left},{top} {scaled_width}x{scaled_height} in "
                        f"{canvas_width}x{canvas_height}"
                    )
                atlas.alpha_composite(
                    frame,
                    (column * canvas_width + left, row * canvas_height + top),
                )

    for row in range(GRID_ROWS):
        for column in range(GRID_COLUMNS):
            frame = atlas.crop((
                column * canvas_width,
                row * canvas_height,
                (column + 1) * canvas_width,
                (row + 1) * canvas_height,
            ))
            visible_bounds(frame)

    atlas.save(destination, optimize=True)
    print(f"{destination.relative_to(ROOT)}: {atlas.size} RGBA")


def prepare_aura() -> None:
    """Keep Aura's body pixel-identical while animating only her magic accents."""
    enemy = "aura"
    destination = ENEMY_ROOT / enemy / "animation.png"
    width, height = frame_size(enemy)
    canvas_width = width * 3
    atlas = Image.new("RGBA", (canvas_width * GRID_COLUMNS, height * GRID_ROWS))

    with (
        Image.open(ENEMY_ROOT / enemy / "idle.png").convert("RGBA") as idle,
        Image.open(ENEMY_ROOT / enemy / "windup.png").convert("RGBA") as windup,
    ):
        body_left = (canvas_width - width) // 2
        particle_positions = (
            ((-17, -14), (-7, 12)),
            ((-22, -8), (-11, -19), (1, 15)),
            ((-26, -2), (-17, -17), (-5, -24), (5, 9)),
            ((-30, 5), (-23, -13), (-12, -25), (1, -19), (8, 2)),
        )
        for row in range(GRID_ROWS):
            for column in range(GRID_COLUMNS):
                # Domination has its own large effect atlas. Reusing the original
                # 72-pixel body prevents AI-redrawn poses from changing head/body scale.
                body = idle if row == 0 or (row == 2 and column >= 2) else windup
                frame = Image.new("RGBA", (canvas_width, height))
                frame.alpha_composite(body, (body_left, 0))
                draw = ImageDraw.Draw(frame)
                hand_x = body_left + 17
                hand_y = 29
                particle_count = column + (1 if row > 0 else 0)
                for offset_x, offset_y in particle_positions[column][:particle_count]:
                    x = hand_x + offset_x
                    y = hand_y + offset_y
                    color = (212, 118, 255, 230) if (x + y) % 2 else (255, 211, 92, 235)
                    draw.rectangle((x, y, x + 1, y + 1), fill=color)
                if row == 1:
                    radius = 2 + column * 2
                    draw.rectangle(
                        (hand_x - radius, hand_y - radius,
                            hand_x + radius, hand_y + radius),
                        outline=(183, 78, 244, 105 + column * 35),
                    )
                elif row == 2 and column < 2:
                    radius = 10 + column * 5
                    draw.rectangle(
                        (hand_x - radius, hand_y - radius,
                            hand_x + radius, hand_y + radius),
                        outline=(255, 223, 126, 210 - column * 55),
                    )
                atlas.alpha_composite(frame, (column * canvas_width, row * height))

    # Every row deliberately reuses a legacy body at native scale and ground line.
    for row in range(GRID_ROWS):
        for column in range(GRID_COLUMNS):
            frame = atlas.crop((column * canvas_width, row * height,
                (column + 1) * canvas_width, (row + 1) * height))
            visible_bounds(frame)

    atlas.save(destination, optimize=True)
    print(f"{destination.relative_to(ROOT)}: {atlas.size} RGBA (locked Aura body)")


def prepare_headless_walk() -> None:
    enemy = "headless_knight"
    source = SOURCE_ROOT / f"{enemy}-walk-alpha.png"
    destination = ENEMY_ROOT / enemy / "walk.png"
    width, height = frame_size(enemy)
    canvas_width = width * 3
    frame_count = 8
    atlas = Image.new("RGBA", (canvas_width * frame_count, height))

    with Image.open(ENEMY_ROOT / enemy / "idle.png").convert("RGBA") as reference:
        reference_bounds = visible_bounds(reference)
        target_height = reference_bounds[3] - reference_bounds[1]
        target_bottom = reference_bounds[3]

    with Image.open(source).convert("RGBA") as sheet:
        for frame_index in range(frame_count):
            row, column = divmod(frame_index, 4)
            cell = sheet.crop(centered_cell_bounds(
                sheet.size, column, row, columns=4, rows=2
            ))
            bounds = visible_bounds(cell)
            cropped = cell.crop(bounds)
            scale = target_height / max(cropped.height, 1)
            scaled_width = max(1, round(cropped.width * scale))
            scaled_height = max(1, round(cropped.height * scale))
            source_anchor = cell.width * 0.5 - bounds[0]
            scaled_anchor = source_anchor * scale
            right_extent = (cropped.width - source_anchor) * scale
            target_anchor = canvas_width * 0.5
            fit_scale = min(
                1.0,
                target_bottom / scaled_height,
                target_anchor / max(scaled_anchor, 1.0),
                (canvas_width - target_anchor) / max(right_extent, 1.0),
            )
            scaled_width = max(1, round(scaled_width * fit_scale))
            scaled_height = max(1, round(scaled_height * fit_scale))
            frame = cropped.resize(
                (scaled_width, scaled_height), Image.Resampling.NEAREST
            )
            anchor_scale = scaled_width / cropped.width
            left = round(target_anchor - source_anchor * anchor_scale)
            top = target_bottom - scaled_height
            atlas.alpha_composite(frame, (frame_index * canvas_width + left, top))

    for frame_index in range(frame_count):
        frame = atlas.crop((
            frame_index * canvas_width, 0,
            (frame_index + 1) * canvas_width, height,
        ))
        bounds = visible_bounds(frame)
        visible_height = bounds[3] - bounds[1]
        if visible_height < round(target_height * 0.95):
            raise ValueError(
                f"Walk frame {frame_index} shrank below the size floor: "
                f"{visible_height}/{target_height}"
            )

    atlas.save(destination, optimize=True)
    print(f"{destination.relative_to(ROOT)}: {atlas.size} RGBA")


def prepare_aura_domination() -> None:
    source = SOURCE_ROOT / "aura-domination-alpha.png"
    destination = ENEMY_ROOT / "aura" / "domination.png"
    frame_width = 280
    frame_height = 120
    frame_count = 8
    padding = 4
    atlas = Image.new("RGBA", (frame_width * frame_count, frame_height))

    with Image.open(source).convert("RGBA") as sheet:
        for frame_index in range(frame_count):
            row, column = divmod(frame_index, 4)
            cell = sheet.crop(centered_cell_bounds(
                sheet.size, column, row, columns=4, rows=2
            ))
            bounds = visible_bounds(cell)
            cropped = cell.crop(bounds)
            scale = min(
                (frame_width - padding * 2) / cropped.width,
                (frame_height - padding * 2) / cropped.height,
            )
            size = (
                max(1, round(cropped.width * scale)),
                max(1, round(cropped.height * scale)),
            )
            frame = cropped.resize(size, Image.Resampling.NEAREST)
            left = frame_index * frame_width + (frame_width - frame.width) // 2
            top = (frame_height - frame.height) // 2
            atlas.alpha_composite(frame, (left, top))

    for frame_index in range(frame_count):
        frame = atlas.crop((
            frame_index * frame_width, 0,
            (frame_index + 1) * frame_width, frame_height,
        ))
        visible_bounds(frame)

    atlas.save(destination, optimize=True)
    print(f"{destination.relative_to(ROOT)}: {atlas.size} RGBA")


def main() -> None:
    missing = [enemy for enemy in ENEMIES
        if not (SOURCE_ROOT / f"{enemy}-alpha.png").is_file()]
    if missing:
        raise FileNotFoundError(
            "Missing cleaned enemy animation sheets: " + ", ".join(missing)
        )
    for enemy in ENEMIES:
        if enemy == "aura":
            prepare_aura()
        else:
            prepare_enemy(enemy)
    walk_source = SOURCE_ROOT / "headless_knight-walk-alpha.png"
    if not walk_source.is_file():
        raise FileNotFoundError(f"Missing cleaned walk sheet: {walk_source}")
    prepare_headless_walk()
    domination_source = SOURCE_ROOT / "aura-domination-alpha.png"
    if not domination_source.is_file():
        raise FileNotFoundError(
            f"Missing cleaned domination sheet: {domination_source}"
        )
    prepare_aura_domination()


if __name__ == "__main__":
    main()
