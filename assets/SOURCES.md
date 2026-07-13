# Asset Sources

## Headless Knight placeholder

- Files: `enemies/headless_knight/idle.png`, `windup.png`, `attack.png`
- Added by: project team member, 2026-07-12
- Original source/license: not yet recorded; classroom prototype use only until the team fills this in.
- Purpose: placeholder idle, attack windup, and attack-active presentation for the Headless Knight enemy.

## Chest Mimic placeholder

- Files: `enemies/chest_mimic/idle.png`, `windup.png`, `attack.png`
- Added by: project team member, 2026-07-12
- Original source/license: not yet recorded; classroom prototype use only until the team fills this in.
- Purpose: placeholder idle, attack windup, and attack-active presentation for the Chest Mimic enemy.

## Bird Demon placeholder

- Files: `enemies/bird_demon/idle.png`, `windup.png`, `attack.png`
- Added by: project team member, 2026-07-12
- Original source/license: not yet recorded; classroom prototype use only until the team fills this in.
- Purpose: placeholder idle, dive windup, and dive-active presentation for the Bird Demon enemy.

## Lugner and blood-magic placeholders

- Files: `enemies/lugner/idle.png`, `windup.png`, `attack.png`, `skill1.png`, `skill2.png`, `skill3.png`
- Added by: project team member, 2026-07-13
- Original source/license: not yet recorded; classroom prototype use only until the team fills this in.
- Purpose: placeholder character states and a three-frame blood-magic effect for Lugner.

## Linie and leaping-cleave placeholders

- Files: `enemies/linie/idle.png`, `windup.png`, `jump.png`, `attack.png`, `skill1.png`, `skill2.png`
- Added by: project team member, 2026-07-13
- Original source/license: not yet recorded; classroom prototype use only until the team fills this in.
- Purpose: placeholder character states, airborne pose, and two-frame grounded cleave effect for Linie.

## Player pixel-animation placeholders

- Files: `../Project1/assets/player/generated/*.png` and derived
  `../Project1/assets/player/processed/*.png`.
- Added by: project team with AI-assisted generation and deterministic cleanup, 2026-07-13.
- Original source/license: classroom-only reference-based placeholder artwork; the visual design
  references Frieren and is not intended for a commercial release.
- Processing: `scripts/Process-PlayerSprites.py` removes the green background, splits key poses,
  applies nearest-neighbor scaling, and normalizes every frame to the same gameplay anchor.
- Purpose: player movement, attack, spell, dash, hit, interaction, and defeat presentation for
  the course prototype.

## Spell pixel-VFX placeholders

- Files: `../Project1/assets/spells/generated/*.png` and derived
  `../Project1/assets/spells/processed/*.png`.
- Added by: project team with AI-assisted generation and deterministic cleanup, 2026-07-13.
- Original source/license: AI-generated classroom prototype material; no external asset pack.
- Style reference: the project-owned player pixel-style reference was supplied only to match
  pixel density and palette discipline; generated spell sheets contain no copied character art.
- Processing: `scripts/Process-SpellVfx.py` removes chroma contamination, splits chronological
  frames, applies one shared scale per animation, and exports transparent runtime atlases.
- Purpose: shared casting, beam, projectile, forward-burst, scan, dispel, spatial-counter,
  gravity-field, self-aura, charge-state, shield, tether, binding, sealing, target-telegraph,
  ground-impact, decoy, golem, mirror-summon, field-conversion, blood, spatial-slash, firestorm,
  and mimic VFX for the course build.
