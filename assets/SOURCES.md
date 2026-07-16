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

## Enemy animation atlases

- Files: `enemies/*/animation.png`, `enemies/headless_knight/walk.png`, `enemies/aura/domination.png`; generation sources and chroma-cleaned intermediates under
  `../Project1/assets/enemies/generated-animation-sheets/`.
- Added by: project team with OpenAI built-in image generation and deterministic local cleanup,
  2026-07-15.
- Original source/license: AI-generated classroom prototype material based on the team's existing
  enemy placeholder identities; no external animation pack.
- Prompt record: `../docs/ENEMY_ANIMATION_ART_DIRECTION.md` records the shared grid constraints and
  per-enemy motion direction.
- Processing: the image-generation skill's installed chroma-key helper removes the flat green or
  magenta background. `scripts/PrepareEnemyAnimations.py` normalizes visible character height with
  nearest-neighbor sampling, preserves the ground anchor, and validates that attack/walk frames do
  not shrink below their configured size floor.
- Purpose: four-frame idle, windup, and attack presentation for all thirteen currently textured
  enemy archetypes, an eight-frame movement cycle for the Headless Knight, and an eight-frame
  telegraph/impact effect for Aura's authoritative domination area. Enemy movement animation leaves
  domain movement unchanged; domination visuals consume rather than recalculate its domain AABB.

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

## Restrained pixel spell cards

- Files: all runtime cards under `../Project1/assets/spell_cards/v2/`; generation sources under
  `../Project1/assets/spell_cards/generated-v2/`.
- Added by: project team with OpenAI built-in image generation and deterministic local cleanup,
  2026-07-15.
- Original source/license: AI-generated classroom prototype material; no external card artwork.
- Style reference: the project-owned player pixel-style reference was supplied only to match
  pixel density, hard-edge clusters, outline discipline, and the restrained in-game palette.
- Prompt record: `../docs/SPELL_CARD_ART_DIRECTION.md` stores the shared visual constraints and
  every spell's dominant composition so the set can be regenerated consistently.
- Processing: `scripts/PrepareSpellCardSamples.py` center-crops each source, downsizes it to a
  native `128x128` image with nearest-neighbor sampling, and limits it to a fixed 24-color palette.
- Purpose: a complete, restrained pixel-card identity for every active regular and Boss spell.
  `SpellCardArt` prefers these native pixel cards and retains the legacy folder only as a defensive
  fallback for missing assets.

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

## Gargoyle character and laser effect

- Files: `enemies/gargoyle/*.png`.
- Added by: project team, 2026-07-15.
- Source: team-provided classroom prototype artwork.
- Purpose: Gargoyle dormant, flying, windup and attack poses, plus its two-frame angled laser
  effect.

## Sword Demon character artwork

- Files: `enemies/sword_demon/*.png`.
- Added by: project team, 2026-07-15.
- Source: team-provided classroom prototype artwork.
- Purpose: Sword Demon's idle, slash windup, slash attack and flash-step poses.

## Three-Headed Demon form artwork

- Files: `enemies/three_headed_demon/*.png`.
- Added by: project team, 2026-07-15.
- Source: team-provided classroom prototype artwork.
- Purpose: one-, two- and three-head idle, shared windup and claw-attack poses.

## Richter character and earth-pillar artwork

- Files: `enemies/richter/*.png`.
- Added by: project team, 2026-07-15.
- Source: team-provided classroom prototype artwork.
- Purpose: Richter's idle and earth-magic windup poses, plus the three-stage summoned pillar.

## Laufen character artwork

- Files: `enemies/laufen/*.png`.
- Added by: project team, 2026-07-15.
- Source: team-provided classroom prototype artwork.
- Purpose: Laufen's idle, speed-magic windup, side-kick windup and side-kick attack poses.

## Stark copy character and slash artwork

- Files: `enemies/stark_copy/*.png`.
- Added by: project team, 2026-07-16.
- Source: team-provided classroom prototype artwork.
- Purpose: the Stark copy's idle, Sky-Splitting Strike windup/jump/landing,
  whirlwind-slash windup/attack, and vertical sword-wave effect.

## Three-act environment backgrounds and battle platforms

- Files: runtime textures under `environments/processed/`; generation sources, chroma-key
  inputs, and transparent intermediates under `environments/generated/`.
- Added by: project team with OpenAI built-in image generation and deterministic local cleanup,
  2026-07-15.
- Original source/license: AI-generated classroom prototype material; no external environment
  asset pack or game texture was copied.
- Art direction: Act 1 depicts Aura's occupied twilight city with red banners and purple thread
  magic; Act 2 depicts a snowy northern fortress and mountain pass; Act 3 depicts the First-Class
  Mage Exam hall with teal runes, pale stone, and restrained gold trim.
- Processing: `scripts/PrepareEnvironmentAssets.py` crops and normalizes the generated images with
  nearest-neighbor scaling. Platform chroma keys were removed with the image-generation skill's
  `remove_chroma_key.py` helper before normalization.
- Purpose: full-screen scene backgrounds and transparent walkable platform artwork. Runtime
  artwork is presentation-only; collision continues to come from deterministic `ArenaLayout`
  rectangles.

## Three-act staircase exits

- Files: runtime textures `props/staircase-act1.png`, `props/staircase-act2.png`, and
  `props/staircase-act3.png`. Large generation sources and transparent intermediates are retained
  only in the author workspace and are not runtime dependencies or repository assets.
- Added by: project team with OpenAI built-in image generation and deterministic local cleanup,
  2026-07-16.
- Original source/license: AI-generated classroom prototype material; no external asset pack.
- Art direction: Act 1 uses occupied gothic masonry, chains, burgundy banners, and Aura's scale
  crest; Act 2 uses a snowbound northern fortress stair with ice and blue braziers; Act 3 uses
  pale examination-hall stone, teal-and-gold runes, and floating mana crystals.
- Processing: flat green/magenta chroma backgrounds were removed with the image-generation
  skill helper, then each source was cropped and normalized to the same transparent `320x400`
  bottom-centered runtime canvas.
- Purpose: replace the geometric staircase placeholder while preserving the authoritative
  `TowerSessionConfig::staircaseBounds` interaction rectangle.

## Aura soul-guillotine effect

- Files: `enemies/aura/soul-guillotine-frame.png` and
  `enemies/aura/soul-guillotine-blade.png`.
- Added by: project team with OpenAI built-in image generation and deterministic local cleanup,
  2026-07-16.
- Original source/license: AI-generated classroom prototype material; no external game texture
  or asset pack was copied.
- Frame prompt: a tall dark-fantasy pixel-art guillotine perimeter with an ornate top beam, two
  thin side rails and short corner chains; the entire center is explicitly empty, with no blade,
  sword, slab, character, text or scenery, on flat `#00ff00` chroma green.
- Blade prompt: one isolated broad and thin front-facing execution blade, roughly four times
  wider than tall, with a slanted cutting edge and restrained violet engraving; no handle, sword
  body, frame, rails, chains, character, text or scenery, on flat `#00ff00` chroma green.
- Processing: the image-generation skill's chroma-key helper removed the green background. The
  visible frame was normalized to a transparent `192x420` runtime canvas and the blade to a
  transparent `128x48` canvas; runtime code keeps the frame fixed and moves only the blade.
- Purpose: provide the polished surface art for Aura's Soul Guillotine while keeping its
  authoritative range, warning time, hit timing and full-height drop in gameplay code.

## Frieren copy

- Files: `enemies/frieren_copy/*.png`.
- Added by: project team, 2026-07-16.
- Original source/license: team-provided classroom prototype artwork.
- Purpose: idle, cast and attack poses plus two-frame beam/lightning effects and a repeating
  ground-fire tile for the final Boss encounter's Frieren copy.

## Fern copy

- Files: `enemies/fern_copy/*.png`.
- Added by: project team, 2026-07-16.
- Original source/license: team-provided classroom prototype artwork.
- Purpose: idle, windup and attack poses for the final Boss encounter's Fern copy; her beam
  reuses the Frieren copy's two-frame effect.

## Water Mirror Demon

- Files: `enemies/water_mirror_demon/*.png`, `portraits/water_mirror_demon/*.png`, and
  `portraits/frieren_copy/avatar.png`.
- Added by: project team, 2026-07-16.
- Original source/license: team-provided classroom prototype artwork.
- Purpose: idle/death art and dialogue portraits for the final Boss encounter.

## Half-Century Meteor Shower event

- Files: `events/half_century_meteor_shower/5101.png` through `5103.png` and
  `npcs/meteor_observer.png`.
- Added by: project team, 2026-07-16.
- Original source/license: team-provided classroom prototype artwork.
- Purpose: the three event-choice cards and the spatial NPC for the Half-Century Meteor Shower.
