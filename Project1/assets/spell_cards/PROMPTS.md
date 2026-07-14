# Independent Spell Card Artwork

Every active learnable spell owns one square PNG named `<spell-id>.png`. These images are standalone
card illustrations and must not be cropped from combat VFX atlases.

## Shared generation prompt

Use case: stylized-concept
Asset type: square spell-card illustration for a dark fantasy pixel-art action roguelike
Style/medium: polished 32-bit pixel art, crisp deliberate clusters, dramatic modern game lighting,
high contrast silhouette that remains readable at 66x66 pixels
Composition: one centered iconic spell phenomenon, full square composition, strong foreground and
simple dark arcane background, no separate card border because SFML draws the frame
Constraints: no words, letters, numbers, logos, watermark, UI, character portrait, screenshot,
photorealism, anime cel screenshot, blur, shallow depth of field, or combat hitbox visualization
Series reference rule: official anime images may inform only the spell's palette, material, and
general visual language. Create a new composition and do not reproduce a frame or character.

## Per-card subjects

| ID | File | Primary subject and palette |
|---:|---|---|
| 1001 | `1001.png` | moonlit field of white and blue flowers blooming from a luminous green magic circle |
| 1002 | `1002.png` | serene golden goddess halo protecting a small silver staff sigil, white-gold rays |
| 1003 | `1003.png` | crimson blood ribbons forming a predatory spear around a dark red core |
| 1004 | `1004.png` | white-blue Zoltraak bolt crossing a six-petal black magic circle |
| 1005 | `1005.png` | long crystalline frost lance with frozen runes and cyan shards |
| 1006 | `1006.png` | compact orange-red flame blossom exploding outward, blue-hot center |
| 1008 | `1008.png` | taut violet mana threads weaving a binding web around an empty dark core |
| 1009 | `1009.png` | spinning stone bullet surrounded by broken slate and ochre wind trails |
| 1011 | `1011.png` | translucent lavender phantom mage silhouette splitting into afterimages |
| 1016 | `1016.png` | cyan scanning ring revealing glowing mana footprints and a single marked eye |
| 1017 | `1017.png` | three parallel white-blue Zoltraak beams bursting from layered six-petal circles |
| 1018 | `1018.png` | enemy spell glyph unraveling into clean white motes inside a violet dispel ring |
| 1019 | `1019.png` | close-range turquoise mana fist impacting a fractured arcane shield |
| 1020 | `1020.png` | brilliant golden binding chains knotting around a black floating crystal |
| 1021 | `1021.png` | levitating enemy-shaped stone silhouette slammed downward by a violet gravity seal |
| 1022 | `1022.png` | broad rune-carved stone golem awakening from a circular earth seal |
| 1023 | `1023.png` | silver feather and small staff rising through pale blue wind rings and clouds |
| 1024 | `1024.png` | space cracking like violet glass around a white central starburst |
| 1025 | `1025.png` | layered blue-violet sealing talismans locking a red hostile magic core |
| 1026 | `1026.png` | ornate staff crowned by three electric blue charges and branching lightning |
| 1027 | `1027.png` | three pearl-white homing orbs curving toward one red target rune |
| 1028 | `1028.png` | translucent cyan hexagonal barrier dome with a bright protected center |
| 1029 | `1029.png` | wide crescent wall of teal wind blasting stone fragments backward |
| 1030 | `1030.png` | black-violet gravity sphere bending stars and pulling debris into an accretion ring |
| 2001 | `2001.png` | colossal demon-killing Zoltraak, white beam with black core and six-petal circle |
| 2002 | `2002.png` | three monumental golden goddess spears descending from a cathedral halo |
| 2003 | `2003.png` | invisible severing line revealed by a clean crimson-white split through steel and stone |
| 2006 | `2006.png` | mirrored prism copying three differently colored enemy spell silhouettes |
| 2007 | `2007.png` | enormous magenta-blue lightning bolt annihilating crystal and stone below |
| 2009 | `2009.png` | towering infernal fire vortex with red-black flames and ember crown |
| 2010 | `2010.png` | divine judgment beam raining as a fan of sharp gold-white rays |
| 2011 | `2011.png` | three gigantic rune-carved earth pillars erupting around a glowing fault line |
| 2012 | `2012.png` | twin silver arcane mirrors repeating a central cyan spell into two echoes |

## Visual references

- [Official anime magic gallery](https://frieren-anime.jp/special/magic/) — spell palette,
  material, and staging reference. Temporary downloaded stills are kept only under ignored `tmp/`
  during generation and are not shipped with the game.
- [Zoltraak overview and gallery](https://frieren.fandom.com/wiki/Zoltraak) — six-petal casting
  circle and the black/white distinction between variants.
- [Pixel Card UI design reference](https://indigolay.itch.io/pixelcardui) — reference for keeping
  the illustration window separate from UI framing. No files from this paid pack are included.
