# Spell VFX asset pipeline

`generated/` contains AI-generated chroma-key source sheets. They are production source material,
not runtime textures. `processed/` contains transparent, nearest-neighbor runtime atlases and
individual frames.

Rebuild the processed assets from the repository root:

```powershell
python .\scripts\Process-SpellVfx.py
```

The current production set contains the shared arcane casting effect, Zoltraak muzzle and impact,
Frost Lance projectile, Stone Shot projectile, Homing Volley bolt, Flame Burst, Wind Pressure,
Mana Strike, Mana Trace ring, Dispel ring, Spatial Shatter, and Gravity Well. Their intended
animation roles, palette rules, and later batches are defined in `docs/MAGIC_VFX_DESIGN.md`.
The self-aura set adds Goddess Blessing, Flight Magic, Lightning Staff charge states, and the
Defensive Barrier activation/hit/break sequence.
The control set adds Magic Thread, Golden Binding, and Sealing Magic, each combining its launch,
stable target state, and break or interrupt feedback in one eight-frame atlas.
The target-drop set adds Float and Slam, Goddess Three Spears, Destruction Lightning, and Earth
Domination, including pre-damage telegraphs and bottom-aligned ground impacts.
The summon set adds Phantom, Stone Golem, and Mirror Array with assembly, stable idle states, and
distinct hit, attack, or destruction endings.
The unique set adds Blooming Field with ignition conversion, Blood Magic, Severing Magic,
Hellfire Storm, and Mimic Magic. Together with the shared atlases, every active regular and Boss
spell now has at least one core visual resource.

`processed/manifest.json` is the presentation-side metadata contract. Most atlases use a centered
anchor; ground effects such as Gravity Well declare a bottom-center anchor explicitly.
Self auras also use a bottom-body anchor where appropriate so the effect remains stable while
the player sprite changes animation.

Generated poses are not guaranteed to be evenly spaced across a contact sheet. All 32 current
sources have been checked against their cleaned alpha bounds; every sheet whose equal-width or
automatic boundary intersects foreground pixels now uses reviewed `panel_edges` in
`Process-SpellVfx.py`. Dividing those sheets into equal columns can cut one pose across adjacent
runtime frames. After processing, every foreground alpha bound must remain inside its frame cell
rather than touching a left or right cell edge. Earth Dominion is the sole source whose two
destruction poses contain overlapping stray particles, so its reviewed seam uses the lowest-density
two-pixel column between the intact pillar bodies.

AI outputs remain source key poses. Keep gameplay AABBs authoritative and do not infer collision,
damage timing, or projectile speed from transparent texture bounds. Presentation size is an
independent per-spell choice: an atlas may extend beyond, repeat across, or occupy only part of an
authoritative AABB. Hellfire Storm demonstrates this by tiling several uniformly scaled pillars
across the field instead of stretching one sprite into the damage rectangle. Every active spell now
selects an explicit runtime layout: fixed center, caster baseline, target baseline, forward endpoint,
field row, tether, beam, triple beam, homing volley, or multi-pillar storm. No runtime spell atlas is
scaled independently on the X and Y axes.

Spell-card illustrations are intentionally independent assets under `assets/spell_cards/`; combat
atlases must not be cropped or reused as card faces. This separation allows either presentation to
be replaced without silently changing the other.
