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

## Draht placeholder

- Files: `enemies/draht/idle.png`, `windup.png`, `attack.png`
- Added by: project team member, 2026-07-13
- Original source/license: not yet recorded; classroom prototype use only until the team fills this in.
- Purpose: placeholder idle, thread-magic windup, and attack-active presentation for Draht. Thread magic intentionally has no separate effect texture.

## Aura character and dialogue portraits

- Files: `enemies/aura/initial.png`, `idle.png`, `windup.png`, `die.png`, `portraits/aura/*.png`, and `portraits/frieren/idle.png`
- Added by: project team member, 2026-07-13
- Original source/license: not yet recorded; classroom prototype use only until the team fills this in.
- Purpose: Aura's intro, idle, domination-windup and defeated presentation, plus dialogue portraits for Aura and Frieren.

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

## Spell registration presentation reference

- Files: no external binary asset is copied; the reference is reimplemented procedurally in
  `../Project1/src/presentation/views/SpellAcquisitionView.cpp`.
- Added by: project team with Codex-assisted local code and resource analysis, 2026-07-15.
- Original reference: the spell-acquisition sequence shown in Bilibili video `BV178411U7uA`
  and its matching implementation in the team's local classroom copy of `Alice in Cradle
  0.29a`: `EfSpellRegisteration`, the magic-device path in `NelTreasureBoxDrawer`, and the
  `magic_device_opening` / `magic_device_closing` particle definitions.
- Purpose: classroom-only study reference for the spell reward acquisition presentation; no
  executable code, texture, audio, or packed Unity resource from the reference game is shipped.

## Animated spellbook loot

- Files: `loot/magic-book.png` and generation sources under
  `../Project1/assets/loot/generated/`.
- Added by: project team with OpenAI built-in image generation and deterministic local cleanup,
  2026-07-15.
- Original source/license: AI-generated classroom prototype material; no external asset pack.
- Style reference: the project-owned player pixel-style sheet was supplied only to match outline,
  pixel density, shading, and the established gold/cyan palette.
- Processing: the flat chroma background was removed locally, the 4-by-2 contact sheet was split,
  every frame received one shared nearest-neighbor scale, and the eight `128×96` frames were
  combined into the transparent runtime atlas `loot/magic-book.png`.
- Purpose: looping airborne spellbook reward with distinct hover, page-turn, rune, and glow frames.

## Revolte character and dialogue portraits

- Files: `enemies/revolte/*.png` and `portraits/revolte/*.png`.
- Added by: project team, 2026-07-15.
- Source: team-provided classroom prototype artwork.
- Purpose: Revolte's idle, upper/lower slash, heavy strike, parry, dash and defeat poses, plus
  the three dialogue-stage portraits used before battle, at phase two and after defeat.

## Denken character and tornado effects

- Files: `enemies/denken/*.png`.
- Added by: project team, 2026-07-15.
- Source: team-provided classroom prototype artwork.
- Purpose: Denken's idle, tornado summon and cremation-magic poses, plus the normal and evolved
  tornado effects.
