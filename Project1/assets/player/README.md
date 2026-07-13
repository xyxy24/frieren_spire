# Player sprite asset pipeline

`generated/` contains the reviewed AI key-pose contact sheets and the canonical pixel-style
reference. These files are source material and are not loaded by the game.

Run this command from the repository root to rebuild normalized assets:

```powershell
python .\scripts\Process-PlayerSprites.py
```

The script writes transparent, nearest-neighbor sprite sheets to `processed/`. Every output
frame is `128x96`, uses a bottom-center gameplay anchor at `(64, 92)`, and is also exported as
an individual PNG under `processed/frames/<animation>/` for Aseprite cleanup. A two-pixel
transparent horizontal safety margin prevents neighboring frames from bleeding when sampled.

## Animation inventory

| Animation | Frames | Suggested FPS | Loop | Intended state |
| --- | ---: | ---: | :---: | --- |
| `idle` | 4 | 6 | Yes | Grounded with no movement input |
| `run` | 6 | 10 | Yes | Grounded horizontal movement |
| `jump` | 8 | 10 | No | Takeoff through landing; gameplay remains physics-driven |
| `attack` | 6 | 12 | No | Basic staff attack |
| `cast` | 6 | 10 | No | Regular or Boss-spell cast |
| `dash` | 4 | 22 | No | Normal dash; paced to its authoritative 0.18-second duration |
| `shade-dash` | 6 | 33 | No | Charged shadow dash; paced to its authoritative 0.18-second duration |
| `hit` | 4 | 10 | No | Damage reaction |
| `defeat` | 6 | 8 | No | Player defeat |
| `pickup` | 4 | 8 | No | Loot interaction feedback |
| `interact` | 4 | 8 | No | Staircase, merchant, or event interaction |

The same character art faces right in every runtime sheet. Render left-facing movement by
mirroring the sprite horizontally around the `(64, 92)` anchor; do not maintain duplicate
left-facing textures. Animation must remain presentation-only: combat timing, dash duration,
invulnerability, and hitboxes continue to come from gameplay-domain state.

The current files are key poses, not final hand-authored in-betweens. Preserve the generated
sources when manually improving frames so the pipeline can be rerun and compared.
