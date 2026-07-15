# Enemy Animation Art Direction

## Runtime Format

Each supported enemy owns an `animation.png` atlas with four columns and three rows. The rows are
idle, windup, and attack. Every row contains four chronological frames. Frames preserve the legacy
enemy texture's visible size and bottom-center anchor, but use a triple-width transparent canvas so
weapons and attack trails do not force the character itself to shrink. Presentation changes cannot
alter the authoritative collision box.

Idle playback uses the sequence `0, 1, 2, 3, 2, 1` at 4 FPS. Reversing through the middle frames
avoids a hard frame-three-to-frame-zero snap while retaining all four generated poses.

The Headless Knight additionally owns an eight-column `walk.png` atlas played at 10 FPS. Its frames
form a complete heavy-armour gait cycle with alternating planted feet. `EnemyAnimator` selects it
only when adjacent read-only snapshots show actual horizontal movement; a stationary Chase state
returns to idle instead of advancing the walk cycle.

Aura additionally owns an eight-column `domination.png` effect atlas: six escalating warning frames
build violet chains, balance scales and command rings; frame seven is the judgment flash and frame
eight dissipates. The View scales each selected frame to `EnemyStateView::skillEffectBounds`, so the
effect cannot disagree with or enlarge the `420 x 180` authoritative area.

## Shared Generation Prompt

Use case: stylized-concept. Create a production-ready low-resolution pixel enemy animation sheet
with exactly four equal columns and three equal rows. Row one is a four-frame idle loop, row two is
a four-frame attack windup, and row three is a four-frame attack with impact, follow-through, and
recovery. Treat the existing idle, windup, and attack PNG files as identity and pose references.
Preserve the exact species, face, horns, hair, costume, equipment, palette, outline weight, pixel
density, scale, and side-facing direction. Use one full-body enemy per cell with the same
bottom-center anchor and at least 15 percent empty padding. Every body part, weapon, wing, tail,
effect, and motion arc must remain inside its own invisible cell. Adjacent frames must visibly
change. Use a perfectly flat chroma-key background with no cell borders, shadows, gradients,
texture, text, numbers, watermark, extra characters, cropping, or antialiasing.

Green characters use `#ff00ff`; other characters use `#00ff00`.

## Motion Notes

| Enemy | Idle | Windup | Attack |
| --- | --- | --- | --- |
| Headless Knight | Armor weight and cloak breathing | Lower stance and draw sword back | Slash/thrust, extension, follow-through |
| Chest Mimic | Lid tremor and box weight shift | Lid opens and jaw tenses | Snap, lunge, recoil |
| Bird Demon | Wingbeat, body bob, talon flex | Wings draw back and torso pitches | Dive and claw rake |
| Frost Wolf | Cold breath, ears and tails sway | Shoulders lower and paws brace | Pounce, claw/bite, landing |
| Chaos Flower | Petal pulse and root sway | Bloom contracts and vines coil | Vine lash and bloom snap |
| Demon Warrior | Hair and coat hem breathing | Sword draws behind the hip | Fast sword slash/thrust |
| Large Bird Demon | Heavy wingbeat and talon flex | Wings fold and claws prepare | Forceful swoop and rake |
| Draht | Restrained breathing | Finger raises and thread gathers | Thread lash and controlled recovery |
| Linie | Hair, coat, and axe weight shift | Knees bend and axe draws back | Leaping axe cleave and landing |
| Lugner | Long coat breathing | Hand gathers blood magic | Blood lash, recoil, recovery |
| Qual | Mane and feathered cloak settle | Casting hand rises | Killing-magic release and recoil |
| Heimon | Mantle and staff ornament sway | Staff rotates and fog gathers | Staff sweep and fog strike |
| Aura | Hair, skirt, and scale chains sway | Scale lifts and domination gathers | Command gesture and violet release |

## Processing

Built-in image generation sources and chroma-cleaned intermediates are stored under
`Project1/assets/enemies/generated-animation-sheets/`. The installed image-generation chroma-key
helper removes the flat background. `scripts/PrepareEnemyAnimations.py` then crops the centered
four-by-three grid, applies nearest-neighbor normalization to the matching legacy pose, and writes
the runtime atlas into each `assets/enemies/<enemy>/animation.png` directory. Each generated frame
is scaled independently by visible character height: an attack or windup frame may never render
shorter than 90 percent of the matching enemy's idle height. The source cell center and the legacy
ground line provide a stable anchor, while the triple-width canvas absorbs horizontal weapons and
trails. This prevents broad attack poses from shrinking the character or moving a stationary enemy.
