# Spell Card Art Direction

## Runtime Goal

Spell cards must remain readable in the 58-by-58-pixel loadout slots while also supporting the
larger reward and learned-spell views. Each card therefore uses one dominant silhouette instead
of a miniature scene. The presentation layer supplies the rarity frame; the texture supplies only
the illustration and its dark grimoire-paper backdrop.

## Shared Generation Prompt

Use case: stylized-concept. Asset type: 128-by-128 game spell-card illustration. Create authentic
low-resolution pixel art with hard-edged clusters, no antialiasing, and no painterly brushwork.
Use a muted charcoal/navy worn-grimoire-paper background, one large readable spell symbol,
deliberate negative space, and at most six principal colors plus their shades. The silhouette must
remain recognizable at 58 by 58 pixels. Use the project player reference only for pixel density,
outline discipline, and palette restraint. Do not draw a frame because the UI supplies it. No
characters, text, numbers, watermark, scenery, cathedral, circular mandala, radial flower,
excessive particles, bloom, lens flare, fog, soft glow, gradients, or tiny filigree.

Boss cards may add restrained ivory and antique-gold accents for rarity, but must preserve the
same low-color, single-subject composition.

## Per-Spell Subject Prompts

| ID | Spell | Dominant card subject |
| --- | --- | --- |
| 1001 | Blooming Field | A five-petal silver flower growing from a flat green rune patch, with one cyan root and one healing dew drop. |
| 1002 | Goddess Blessing | Angular ivory protective wing plates around a tiny gold staff core, with three descending gold motes. |
| 1003 | Blood Magic | A huge crimson blood drop cut by a black staff shard, transforming into a forward red damage wedge. |
| 1004 | Zoltraak | A narrow diagonal cyan-white beam punching through a broken dark square. |
| 1005 | Frost Lance | One faceted ice lance with only a few large ice chips. |
| 1006 | Flame Burst | One orange-red fire projectile with a blue-white core, short flame tail, and two square embers. |
| 1008 | Magic Thread | Two violet magic threads passing through a metal clasp and pulling a dark target diamond inward. |
| 1009 | Stone Shot | A heavy three-facet stone projectile smashing a wall corner with a short gravel trail. |
| 1011 | Phantom | An empty pale-violet hooded cloak decoy, one half-cell echo, and a small analysis diamond. |
| 1016 | Mana Trace | A narrow cyan scan wedge crossing a dark target diamond, flanked by four short measurement ticks. |
| 1017 | Multi Zoltraak | Three parallel cyan-white beams cutting offset dark blocks; the third beam carries a gold mark notch. |
| 1018 | Dispelling Magic | A hostile red angular spell sigil cleanly severed by a cyan-white slash, releasing one blue analysis shard. |
| 1019 | Mana Strike | A compact cyan-white mana fist wedge following a violet dash trail into a dark stone block. |
| 1020 | Golden Binding | A violet prism locked by two angular gold bands. |
| 1021 | Float and Slam | A dark stone effigy suspended over a pale-blue line, with a thick down arrow and ground crack below. |
| 1022 | Stone Golem | A squat square-headed stone golem bust blocking with a cracked shield arm and one cyan forehead rune. |
| 1023 | Flight Magic | Angular pale-cyan mana wings lifting a tiny gold staff core through arrow-shaped negative space. |
| 1024 | Spatial Shatter | A dark square plane torn by four large violet-white fractures, edged by a short cyan protective shell. |
| 1025 | Sealing Magic | A hostile dark-red glyph trapped in a square rune cage and locked by a pale-blue seal bar. |
| 1026 | Lightning Staff | A short dark-red staff with a gold ring and three blue-white charge nodes, the third visibly stronger. |
| 1027 | Homing Volley | Exactly three small cyan bolts following short curves toward one dark target diamond. |
| 1028 | Defensive Barrier | A thick pale-blue hexagonal shield slab with one crack and two square counter-shock fragments. |
| 1029 | Wind Pressure | A broad pale-cyan wind wedge and three sharp wind lines driving a dark block into a cracked wall. |
| 1030 | Gravity Well | A small deep-violet gravity core pulling exactly four large stones inward along short straight paths. |
| 2001 | Demon-Killing Zoltraak | A massive black-white beam with a gold cutting edge piercing a simplified horned demon mask. |
| 2002 | Goddess Three Spears | Exactly three ivory-gold divine spears falling in parallel above one tiny cyan healing star-drop. |
| 2003 | Severing Magic | One enormous black-violet cut severing a gold binding clasp, with a single red low-health notch. |
| 2006 | Mimic Magic | One irregular silver mimic mirror split between a claw silhouette and a beam silhouette. |
| 2007 | Destruction Lightning | One thick vertical blue-white bolt striking a square target held by four antique-gold corner marks. |
| 2009 | Hellfire Storm | A horizontal deep-orange fire torrent with a dark-violet core pulling three large fragments inward. |
| 2010 | Judgment Beam | A sustained ivory beam fired from a compact gold focusing prism, with readable charge, body, and taper. |
| 2011 | Earth Dominion | Three staggered blocky stone pillars erupting upward, the central pillar split by one gold fracture. |
| 2012 | Mirror Array | Exactly two inward-tilted silver mirrors splitting one incoming cyan bolt into two smaller reflected beams. |

## Processing

The built-in image generator creates one source image per spell under
`Project1/assets/spell_cards/generated-v2/`. `scripts/PrepareSpellCardSamples.py` center-crops each
source, resizes it to the native 128-by-128 runtime size with nearest-neighbor sampling, and
quantizes it to a fixed 24-color palette without dithering. Runtime files are written under
`Project1/assets/spell_cards/v2/`.
