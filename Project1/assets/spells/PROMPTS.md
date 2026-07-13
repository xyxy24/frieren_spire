# Spell VFX generation prompts

All assets use the player reference only for pixel density and palette discipline. Every prompt
also requires one horizontal chronological row, a flat `#00FF00` background, clearly different
frames, generous padding, no character, no UI, no text, no shadows, and no green in the effect.

## `cast-arcane-source.png`

Six centered frames: tiny cyan spark; incomplete hexagonal arcs; expanding concentric hexagons;
aligned runes; precise white-cyan center flash; collapse into forward rectangular particles.
Limited cyan, white, and deep-blue 16-bit pixel palette.

## `zoltraak-impact-source.png`

Eight centered frames: thin beam entering from the left; small contact ring; expanding ring;
center penetration; peak concentric hexagonal impact; four broken ring segments; outward dimming
fragments; final fading pixels. No fire, smoke, or spherical explosion.

## `ice-lance-source.png`

Six right-facing frames: compact ice shard; extended faceted lance; two travel poses with changing
crystal highlights and stepped tail; cracked tip; forward burst into hexagonal ice fragments.

## `stone-shot-source.png`

Six right-facing frames: square stones gather around a magic core; compress into an irregular
pointed rock; two rotating travel poses shedding chunks; deep split; directional stone fan and
compact dust ring. Limited ochre, brown, gray, and white palette.

## `homing-bolt-source.png`

Six right-facing frames: cyan diamond core; short tail; upward course correction; downward course
correction; stronger gold-centered bolt; compact cyan-gold star impact. Limited cyan, white, deep
blue, and a small gold accent.

## `zoltraak-muzzle-source.png`

Six centered frames: cyan particles gather; a hexagonal ring opens; concentric rings align; the
center compresses; a narrow beam erupts right while the rings recoil; the ring breaks into blue
rectangular sparks.

## `flame-burst-source.png`

Eight right-facing frames: yellow-white core compression; short orange cone; layered expanding
flame tongues; peak wide forward impact; rising embers; low fading sparks. It must never become a
round explosion or smoke cloud.

## `wind-pressure-source.png`

Eight right-facing frames on a magenta key: curved mint strokes gather; hollow spiral forms;
separated wind arcs expand into a wide pressure cone; peak hollow blast; arcs break into rightward
fragments; two speed lines fade.

## `mana-strike-source.png`

Eight right-facing frames: violet point; angular block compression; short thrust; heavy wedge
impact with speed cuts; white-purple knockback peak; square fragments; thin fading forward line.

## `mana-trace-ring-source.png`

Eight centered frames: tiny cyan hexagonal reticle; progressively wider concentric scan rings;
maximum-radius pulse with four target brackets; broken outer pixels; persistent small crosshair.
The center stays transparent and the radius change is the dominant motion.

## `dispel-ring-source.png`

Eight centered frames: wide broken gold rune circle; four visibly smaller inward steps; compact
closed seal; sharp four-point cancellation flash; square gold stars fading. The motion is an
implosion, not an explosion or shield.

## `spatial-shatter-source.png`

Ten centered frames: tiny violet square crack; expanding jagged grid fractures and displaced
tiles; wide hollow white-purple peak; a few fragments reverse inward; sparse blue-violet cracks
fade. No portal, smoke, or solid disc.

## `gravity-well-source.png`

Eight bottom-centered ground-effect frames: faint shallow ellipse; concentric purple rings and
rectangular particles move inward; compact blue-black core reaches a bright pulse; the field
collapses into a few inward-falling pixels. Low side-view silhouette, never a photographic black
hole or tall effect that hides enemies.

## `goddess-blessing-source.png`

Eight bottom-centered frames around an empty body space: three feather sparks rise; warm-gold
feathers orbit while a triangular sigil forms overhead; two stable orbit poses; a hollow wing
outline flashes on successful control resistance; square gold particles dissipate.

## `flight-aura-source.png`

Eight bottom-centered frames around an empty body space: one then two cyan wind rings form under
the feet; downward particles establish lift; two shifted stable loop poses retain one bright
upper-right bonus star; the star shoots forward when consumed; broken rings fade.

## `lightning-staff-source.png`

Eight centered state frames: spark; exactly one orb; exactly two separated orbs; exactly three
orbs; rotated three-orb pose; exactly two remaining orbs; exactly one remaining orb; zero-orb
hollow radial discharge. Orb counts are a gameplay-readable state contract, not decoration.

## `defensive-barrier-source.png`

Eight bottom-centered frames around an empty body space: lower hexagonal panel; shield panels
assemble upward; stable hollow barrier; right-side panels flare for a directional hit; all panels
crack outward around a compact shock ring; blue shards fade.

## `magic-thread-source.png`

Eight centered frames: violet needle-head launches right with a stepped thread; the thread bends
around an empty target; exactly three thin loops tighten at the waist; all loops stretch left for
the pull; angular purple segments snap and fade. Thin flexible line language, never a chain.

## `golden-binding-source.png`

Eight centered frames: rigid gold wedge launches; upper and lower rune nails embed around an empty
target; three heavy angular rings close inward; compact cage and lock diamond stabilize; the
combo-consumed state fractures rings and nails into square gold pieces.

## `sealing-magic-source.png`

Eight centered frames: white-gold paper talisman launches; four corner brackets assemble around
an empty target; a rectangular crossing frame closes; the stable state dims to dark gold with a
small lower lock; interrupt pulse flashes inward; only the lock and corner pixels remain.

## `float-slam-source.png`

Eight bottom-centered frames: ground ellipse and upward arrows; particles suspend an empty target;
waist ring holds at maximum height; arrows and particles reverse downward; energy compresses;
wide low slam impact; cracked ellipse and debris remain.

## `goddess-spears-source.png`

Ten bottom-centered frames: triangular three-point warning; exactly three narrow divine spears are
shown and then strike separate landing points sequentially; the third strike completes a hollow
triangle; three healing motes rise and move toward the player side before fading.

## `destruction-lightning-source.png`

Ten bottom-centered frames: four warning-only poses progressively close three blue-violet rings;
a narrow filament descends only afterward; one thick jagged bolt strikes; low shock ring, ground
electric scars, broken ring, and sparse violet sparks dissipate.

## `earth-domination-source.png`

Ten bottom-centered frames: square fault telegraph cracks; three different stone pillars rise
left-to-right; the active trio pulses with pale fractures; tops break into square chunks; all
pillars collapse into debris and three cracked stumps.

## `phantom-source.png`

Ten foot-anchored frames: lavender outline particles assemble upward into a hollow long-haired
mage decoy; three idle poses shift hair, robe edge, and one-pixel glitches; a right-side hit adds
an analysis diamond; glass-like pieces fracture and collapse inward; only the mark remains.

## `stone-golem-source.png`

Ten foot-anchored state frames: stone blocks assemble into a compact golem; two idle weight shifts;
crossed arms block a hit; one ending shatters the chest and body; a separate intact ending raises
one arm, slams right, and recovers. Shatter and timeout attack remain visibly distinct.

## `mirror-array-source.png`

Nine accepted foot-anchored frames with exactly two mirrors: silver shards assemble two standing frames;
dim mage silhouettes and blocky surface bands idle; two stars show the armed state; both mirrors
flash inward, crack, break into silver-blue shards, and leave two base glints.

## `flower-field-source.png`

Nine accepted bottom-centered frames on a magenta key: cyan ellipse and sprouts grow into a low flower field;
three loop poses shift heads and petals without changing footprint; an orange spark converts the
same field into two burning poses; charred stems and a broken orange ellipse remain.

## `blood-magic-source.png`

Eight forward frames: red pixels first move inward into a compressed blood diamond to show HP
payment; a jagged crimson blade then extrudes right, reaches a long saw-tooth peak, fractures into
dark-red rectangular cuts, and fades. No gore or conventional sword arc.

## `severing-magic-source.png`

Ten forward frames: a one-pixel white cut line pauses before opening into an offset black-violet
spatial slit; the peak retains a hollow black center and displaced square chunks; edges break,
realign, close into a violet scar, and disappear.

## `hellfire-storm-source.png`

Eight bottom-centered seamless loop frames on a magenta key: a broad cracked base erupts into a
tall infernal flame pillar with a pale white-hot core, orange body, crimson outer flame, dark-purple
smoke pockets, and square embers. Adjacent poses visibly compress, fork, surge, collapse, and return
to the first pose. Runtime places several equal-aspect pillars with independent phase offsets.

## `mimic-magic-source.png`

Ten centered frames: violet mirror assembles and scans an abstract silhouette; hollow forward-box
and circular projections demonstrate runtime-selected geometry; copied impact flashes; fallback
mirror cracks, breaks into shards, and leaves a small copy-rune diamond.
