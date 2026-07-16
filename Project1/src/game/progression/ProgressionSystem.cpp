#include "game/progression/ProgressionSystem.hpp"

#include <algorithm>
#include <numeric>

namespace arcane::game::progression
{
namespace
{
constexpr std::array Definitions {
    BreakthroughDefinition {BreakthroughType::Power, "Mana Amplification",
        "Regular spell damage and damaging secondary effects gain 10 percent.",
        "Regular spell damage bonus increases from 10 to 17 percent."},
    BreakthroughDefinition {BreakthroughType::Haste, "Rapid Formula",
        "Regular spell cooldown recovery speed increases by 10 percent.",
        "Regular spell cooldown recovery bonus increases from 10 to 17 percent."},
    BreakthroughDefinition {BreakthroughType::Defense, "Defensive Formula",
        "Gain 15 maximum HP and begin each combat with 8 shield.",
        "Gain 10 more maximum HP and begin each combat with 14 shield."}
};

[[nodiscard]] std::size_t breakthroughIndex(const BreakthroughType type) noexcept
{
    return static_cast<std::size_t>(type);
}
}

const std::array<BreakthroughDefinition, 3>& breakthroughDefinitions() noexcept
{
    return Definitions;
}

std::uint8_t spellRank(
    const run::PlayerProgress& player, const run::ContentId spellId) noexcept
{
    const auto found = std::find_if(player.spellMasteries.begin(), player.spellMasteries.end(),
        [spellId](const auto& mastery) { return mastery.spellId == spellId; });
    if (found != player.spellMasteries.end()) return std::clamp<std::uint8_t>(found->rank, 1U, 3U);
    return std::find(player.learnedSpells.begin(), player.learnedSpells.end(), spellId)
        != player.learnedSpells.end() ? 1U : 0U;
}

std::uint8_t maximumSpellRank(const std::uint32_t act) noexcept
{
    return static_cast<std::uint8_t>(std::clamp<std::uint32_t>(act, 1U, 3U));
}

bool canUpgradeSpell(const run::PlayerProgress& player,
    const run::ContentId spellId, const std::uint32_t act) noexcept
{
    const auto rank = spellRank(player, spellId);
    return rank > 0U && rank < maximumSpellRank(act);
}

bool upgradeSpell(run::PlayerProgress& player,
    const run::ContentId spellId, const std::uint32_t act) noexcept
{
    if (!canUpgradeSpell(player, spellId, act)) return false;
    auto found = std::find_if(player.spellMasteries.begin(), player.spellMasteries.end(),
        [spellId](const auto& mastery) { return mastery.spellId == spellId; });
    if (found == player.spellMasteries.end())
    {
        player.spellMasteries.push_back({spellId, 2U});
        return true;
    }
    ++found->rank;
    return true;
}

void registerLearnedSpell(run::PlayerProgress& player, const run::ContentId spellId)
{
    if (std::find_if(player.spellMasteries.begin(), player.spellMasteries.end(),
            [spellId](const auto& mastery) { return mastery.spellId == spellId; })
        == player.spellMasteries.end())
    {
        player.spellMasteries.push_back({spellId, 1U});
    }
}

bool applyBreakthrough(run::PlayerProgress& player, const BreakthroughType type) noexcept
{
    const std::size_t index = breakthroughIndex(type);
    const unsigned int total = std::accumulate(player.breakthroughRanks.begin(),
        player.breakthroughRanks.end(), 0U);
    if (index >= player.breakthroughRanks.size() || player.breakthroughRanks[index] >= 2U
        || total >= 2U)
        return false;
    const auto previous = player.breakthroughRanks[index]++;
    if (type == BreakthroughType::Defense)
    {
        const int healthGain = previous == 0U ? 15 : 10;
        player.maxHp += healthGain;
        player.currentHp = std::min(player.maxHp, player.currentHp + healthGain);
    }
    return true;
}

float regularDamageMultiplier(const run::PlayerProgress& player) noexcept
{
    const auto rank = player.breakthroughRanks[breakthroughIndex(BreakthroughType::Power)];
    return rank == 0U ? 1.0F : (rank == 1U ? 1.10F : 1.17F);
}

float regularCooldownMultiplier(const run::PlayerProgress& player) noexcept
{
    const auto rank = player.breakthroughRanks[breakthroughIndex(BreakthroughType::Haste)];
    const float recovery = rank == 0U ? 1.0F : (rank == 1U ? 1.10F : 1.17F);
    return 1.0F / recovery;
}

int startingShield(const run::PlayerProgress& player) noexcept
{
    const auto rank = player.breakthroughRanks[breakthroughIndex(BreakthroughType::Defense)];
    return rank == 0U ? 0 : (rank == 1U ? 8 : 14);
}

float masteryPowerMultiplier(const std::uint8_t rank) noexcept
{
    return rank >= 3U ? 1.30F : (rank >= 2U ? 1.15F : 1.0F);
}

float masteryCooldownMultiplier(const std::uint8_t rank) noexcept
{
    return rank >= 3U ? 0.85F : (rank >= 2U ? 0.92F : 1.0F);
}

float masteryAreaMultiplier(const std::uint8_t rank) noexcept
{
    return rank >= 3U ? 1.16F : (rank >= 2U ? 1.08F : 1.0F);
}

std::string_view rankThreeSignature(const run::ContentId spellId) noexcept
{
    switch (spellId)
    {
    case 1001U: return "Igniting the field begins with a 16 damage area burst.";
    case 1002U: return "Casting also grants 10 shield, up to the normal shield cap.";
    case 1003U: return "Current-HP casting cost is reduced from 12 to 8 percent.";
    case 1004U: return "A marked target takes 8 additional damage.";
    case 1005U: return "Hitting an already frozen target shatters it for 18 area damage.";
    case 1006U: return "Thermal shock becomes a 20 damage area explosion.";
    case 1008U: return "The pull distance and hard-control duration gain another 25 percent.";
    case 1009U: return "A wall collision deals 24 instead of 14 additional damage.";
    case 1011U: return "When the Phantom expires it deals 18 damage around itself.";
    case 1016U: return "The scan marks the two nearest enemies instead of one.";
    case 1017U: return "Fire a fourth 10 damage beam after the normal three beams.";
    case 1018U: return "A successful dispel reduces the longest cooldown by 2.5 seconds.";
    case 1019U: return "A post-Dash hit immediately refreshes normal Dash.";
    case 1020U: return "Consuming the binding refunds 30 percent of Golden Binding cooldown.";
    case 1021U: return "The slam creates an additional 18 damage shockwave.";
    case 1022U: return "The golem survives one extra intercepted attack.";
    case 1023U: return "The first damaging spell while flying gains 30 instead of 15 percent.";
    case 1024U: return "A successful interrupt also reduces the longest cooldown by 1.5 seconds.";
    case 1025U: return "Sealing a marked target lasts 2 additional seconds instead of 1.";
    case 1026U: return "Gain four charged attacks; the final blast deals 20 damage.";
    case 1027U: return "Fire a fourth homing bolt for 10 damage.";
    case 1028U: return "Barrier break damage increases from 12 to 20.";
    case 1029U: return "A wall collision deals 18 instead of 10 additional damage.";
    case 1030U: return "Gravity Well pulls 35 percent faster and ticks for 8 damage.";
    default: return {};
    }
}
}
