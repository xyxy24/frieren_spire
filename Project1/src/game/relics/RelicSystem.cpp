#include "game/relics/RelicSystem.hpp"

#include <algorithm>
#include <array>

namespace arcane::game::relics
{
namespace
{
constexpr std::array Definitions {
    RelicDefinition {DarkDragonHornId, "Dark Dragon Horn", "+20% damage; take +20% damage for first 15s of combat."},
    RelicDefinition {MoonGrassFlowerId, "Moon Grass Flower", "Cast Blooming Field when combat starts."},
    RelicDefinition {FlammeNotesId, "Flamme's Tactical Notes", "Show enemy attack areas for the first 8 seconds. Your first successful interrupt extends that enemy's mark by 3 seconds and refreshes Dash."},
    RelicDefinition {ObedienceScaleId, "Scale of Obedience", "The first hard control each combat lasts 40 percent longer. Each enemy's first later hard control lasts 20 percent longer; Boss resistance still applies."},
    RelicDefinition {HolyStaffId, "Holy Staff Emblem", "The first cast from each regular slot reduces the other two regular cooldowns by 0.75 seconds. Each slot triggers once per combat."},
    RelicDefinition {KraftBracerId, "Kraft's Bracer", "Basic attacks against marked, frozen, bound or sealed enemies deal 8 extra damage and reduce Dash cooldown by 0.25 seconds; 0.6 second per-target cooldown."},
    RelicDefinition {MirrorLotusRingId, "Mirror Lotus Ring", "The first regular damage spell each combat echoes after 0.35 seconds for 40 percent damage. It repeats no cost, control or relic triggers."},
    RelicDefinition {PriestScriptureId, "Priest's Scripture", "Stun, capture, bind, sleep and forced-launch duration is reduced by 40 percent. The first control each combat also grants 0.3 seconds invulnerability."},
    RelicDefinition {ElderSealFragmentId, "Elder Seal Fragment", "When one active regular spell hits multiple enemies, damage to its second target gains 12 percent and its third or later targets gain 24 percent."},
    RelicDefinition {HeroFlowerCrownId, "Hero's Flower Crown", "Excess spell healing becomes shield, up to 15 shield per combat and the global 30 percent maximum-HP shield cap."},
    RelicDefinition {PurpleGrapeHairpinId, "Purple Grape Hairpin", "Three active hits on one enemy within 2.5 seconds make the third deal 18 extra damage and reduce its slot cooldown by 0.8 seconds; 3 second target cooldown."},
    RelicDefinition {FirstClassBadgeId, "First-Class Mage Badge", "Press R on a regular reward once per act to reroll all three choices with the same damage, control and random categories."},
    RelicDefinition {OldSpellBookmarkId, "Old Spellbook Bookmark", "The equipped regular spell with the shortest base cooldown deals 12 percent more damage; the longest base cooldown is reduced by 10 percent."},
    RelicDefinition {ManaLensId, "Mana Detection Lens", "An active hit on a marked enemy reduces the longest remaining regular cooldown by 0.35 seconds; at most once per second."},
    RelicDefinition {BrokenBarrierId, "Broken Defensive Formula", "Perfect Guard deals 20 damage and 0.25 second stagger in a 96 pixel radius, then reduces the longest regular cooldown by another 0.5 seconds."},
    RelicDefinition {MimicTongueId, "Mimic Tongue", "Merchants offer one extra unowned relic, but all relic prices in that shop are increased by 10 percent and rounded up."},
    RelicDefinition {ManaSuppressionEarringId, "Mana Suppression Earring", "Your first active regular damage spell each combat deals 30 percent more damage, but starts 25 percent more cooldown."},
    RelicDefinition {SeriePageId, "Page from Serie's Grimoire", "Ultimate cooldown becomes 15.3 seconds, but all regular spell damage is reduced by 8 percent."},
    RelicDefinition {NorthernCloakId, "Northern Cloak", "After the first hit from one hostile persistent area, later ticks from that same area take 4 less damage, to a minimum of 1."},
    RelicDefinition {SteelPetalBookmarkId, "Steel Petal Bookmark", "Regular persistent fields and summons last 25 percent longer. It affects Flower Field, burning flowers, Phantom, Stone Golem and Gravity Well."},
    RelicDefinition {WarriorAxeFragmentId, "Warrior's Axe Fragment", "A basic attack hit reduces the longest remaining regular cooldown by 0.25 seconds; at most once per second."},
    RelicDefinition {GoddessTabletId, "Goddess Tablet Fragment", "When Goddess Blessing prevents control, gain 8 shield and 10 percent damage for 3 seconds; once per Blessing cast."},
    RelicDefinition {DemonCoinId, "Demon Coin", "Blood Magic deals 25 percent more final damage, but all spell healing is reduced by 25 percent."},
    RelicDefinition {OldCopperCoinId, "Old Copper Coin", "Finish a combat floor without losing actual HP to gain 10 extra gold; at most once per floor."},
    RelicDefinition {HeroSwordId, "Hero Sword", "Reduce collision damage taken by 10."},
    RelicDefinition {TrueHeroSwordId, "True Hero Sword", "Taking collision damage deals 30 damage to nearby enemies."}
};
}

const RelicDefinition* findDefinition(const std::uint32_t id) noexcept
{
    const auto found = std::find_if(Definitions.begin(), Definitions.end(), [id](const RelicDefinition& value) {
        return value.id == id;
    });
    return found == Definitions.end() ? nullptr : &*found;
}

RelicRuntime::RelicRuntime(const std::span<const std::uint32_t> relicIds)
    : ids_(relicIds.begin(), relicIds.end())
{
    darkDragonHorn_ = std::find(relicIds.begin(), relicIds.end(), DarkDragonHornId) != relicIds.end();
    moonGrassFlower_ = std::find(relicIds.begin(), relicIds.end(), MoonGrassFlowerId) != relicIds.end();
    heroSword_ = std::find(relicIds.begin(), relicIds.end(), HeroSwordId) != relicIds.end();
    trueHeroSword_ = std::find(relicIds.begin(), relicIds.end(), TrueHeroSwordId) != relicIds.end();
    if (darkDragonHorn_) vulnerableRemaining_ = 15.0F;
}

void RelicRuntime::update(const float deltaSeconds) noexcept
{
    if (deltaSeconds > 0.0F) vulnerableRemaining_ = std::max(0.0F, vulnerableRemaining_ - deltaSeconds);
}

float RelicRuntime::outgoingDamageMultiplier() const noexcept { return darkDragonHorn_ ? 1.2F : 1.0F; }
float RelicRuntime::incomingDamageMultiplier() const noexcept
{
    return darkDragonHorn_ && vulnerableRemaining_ > 0.0F ? 1.2F : 1.0F;
}
bool RelicRuntime::castsFlowerFieldOnCombatStart() const noexcept { return moonGrassFlower_; }
float RelicRuntime::vulnerableRemaining() const noexcept { return vulnerableRemaining_; }
int RelicRuntime::collisionFlatReduction() const noexcept { return heroSword_ ? 10 : 0; }
bool RelicRuntime::retaliatesOnCollision() const noexcept { return trueHeroSword_; }
bool RelicRuntime::has(const std::uint32_t id) const noexcept
{ return std::find(ids_.begin(), ids_.end(), id) != ids_.end(); }
}
