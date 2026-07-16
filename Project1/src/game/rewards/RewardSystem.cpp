#include "game/rewards/RewardSystem.hpp"

#include "game/progression/ProgressionSystem.hpp"
#include "game/run/DeterministicRng.hpp"
#include "game/spells/SpellSystem.hpp"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace arcane::game::rewards
{
namespace
{
struct SpellSynergyDefinition
{
    run::ContentId first {};
    run::ContentId second {};
    std::string_view description;
};

constexpr std::array SpellSynergies {
    SpellSynergyDefinition {1001U, 1006U,
        "Cast Blooming Field, then Flame Burst inside it: the healing field becomes a 2 second fire field dealing 8 damage each second."},
    SpellSynergyDefinition {1005U, 1006U,
        "Use Frost Lance to slow or freeze, then Flame Burst: it deals 10 extra thermal damage before clearing Slow, Chill and Freeze."},
    SpellSynergyDefinition {1005U, 1021U,
        "Apply Chill and hit with Frost Lance again to Freeze, then cast Float and Slam: its slam deals 12 extra damage to the frozen target."},
    SpellSynergyDefinition {1008U, 1009U,
        "Magic Thread pulls and binds an enemy into position; follow with Stone Shot toward a wall to trigger its 14 wall-collision damage."},
    SpellSynergyDefinition {1009U, 1020U,
        "Cast Golden Binding first, then hit the bound target with Stone Shot: the binding is consumed for 16 extra damage."},
    SpellSynergyDefinition {1011U, 1017U,
        "Let Phantom receive an attack to mark the attacker, then use Multi Zoltraak: the third beam deals 10 extra damage and all beams gain the mark bonus."},
    SpellSynergyDefinition {1011U, 1024U,
        "Let Phantom draw an enemy attack, then use Spatial Shatter during that attack: a successful interrupt raises its damage from 26 to 38 and grants brief invulnerability."},
    SpellSynergyDefinition {1011U, 1025U,
        "Let Phantom receive an attack to mark that enemy, then use Sealing Magic: the seal lasts 1 extra second, or 2 extra seconds at Rank III."},
    SpellSynergyDefinition {1011U, 2007U,
        "Let Phantom mark its attacker, then target it with Destruction Lightning: the delayed lightning damage area becomes 40 percent larger."},
    SpellSynergyDefinition {1004U, 1016U,
        "Cast Mana Trace first: the marked target takes 12 percent more Zoltraak damage; Rank III Zoltraak also deals 8 additional damage."},
    SpellSynergyDefinition {1016U, 1017U,
        "Cast Mana Trace first, then Multi Zoltraak: the marked target takes 12 percent more damage and the third beam deals another 10 damage."},
    SpellSynergyDefinition {1016U, 1025U,
        "Cast Mana Trace before Sealing Magic: the marked target is sealed 1 extra second, or 2 extra seconds at Rank III."},
    SpellSynergyDefinition {1016U, 2007U,
        "Mark the target with Mana Trace before Destruction Lightning: the delayed lightning damage area becomes 40 percent larger."},
    SpellSynergyDefinition {1004U, 1018U,
        "Successfully dispel an active enemy attack to mark it for 4 seconds, then cast Zoltraak for 12 percent more damage; Rank III adds another 8 damage."},
    SpellSynergyDefinition {1017U, 1018U,
        "Successfully dispel an active enemy attack to mark it, then cast Multi Zoltraak: all beams gain the mark bonus and the third deals another 10 damage."},
    SpellSynergyDefinition {1018U, 1025U,
        "Dispelling Magic interrupts and marks an attacking enemy; follow with Sealing Magic to keep its special attacks disabled 1 second longer."},
    SpellSynergyDefinition {1018U, 2007U,
        "Successfully dispel an active attack to mark that enemy, then target it with Destruction Lightning for a 40 percent larger damage area."},
    SpellSynergyDefinition {1020U, 2003U,
        "Cast Golden Binding first, then Severing Magic: the slash consumes the binding for 16 additional damage."},
    SpellSynergyDefinition {1001U, 1030U,
        "Place Gravity Well inside Blooming Field: the pull keeps several enemies in the slowing field and groups them for its Flame ignition."},
    SpellSynergyDefinition {1006U, 1030U,
        "Use Gravity Well to gather enemies, then place Flame Burst over the center so its direct hit and burn catch the whole group."},
    SpellSynergyDefinition {1021U, 1030U,
        "Use Gravity Well to gather enemies into one point, then target its center with Float and Slam to hit the entire group."},
    SpellSynergyDefinition {1024U, 1030U,
        "Pull enemies around yourself with Gravity Well, then use Spatial Shatter while their attacks are active to interrupt several at once."},
    SpellSynergyDefinition {1030U, 2011U,
        "Gather enemies at Gravity Well's center, then cast Earth Dominion there so the eruption and stone pillars hit the grouped targets."}
};

[[nodiscard]] bool isDamagingRegularSpell(const run::ContentId id) noexcept
{
    const auto* spell = spells::findDefinition(id);
    if (!spell || spell->tier != spells::SpellTier::Regular) return false;
    return spell->damage > 0 || spell->effect == spells::SpellEffect::BloodMagic
        || spell->effect == spells::SpellEffect::MultiZoltraak
        || spell->effect == spells::SpellEffect::ManaStrike
        || spell->effect == spells::SpellEffect::SpatialShatter
        || spell->effect == spells::SpellEffect::HomingVolley
        || spell->effect == spells::SpellEffect::WindPressure
        || spell->effect == spells::SpellEffect::GravityWell;
}

[[nodiscard]] bool hasExplicitSynergy(
    const run::ContentId first, const run::ContentId second) noexcept
{
    return std::any_of(SpellSynergies.begin(), SpellSynergies.end(), [&](const auto& synergy) {
        return (synergy.first == first && synergy.second == second)
            || (synergy.first == second && synergy.second == first);
    });
}

[[nodiscard]] bool isSynergy(const run::ContentId equipped, const run::ContentId candidate) noexcept
{
    return hasExplicitSynergy(equipped, candidate)
        || (equipped == 1002U && isDamagingRegularSpell(candidate))
        || (candidate == 1002U && isDamagingRegularSpell(equipped))
        || (equipped == 1023U && isDamagingRegularSpell(candidate))
        || (candidate == 1023U && isDamagingRegularSpell(equipped));
}

[[nodiscard]] bool owns(const run::PlayerProgress& player, const run::ContentId id) noexcept
{
    return std::find(player.learnedSpells.begin(), player.learnedSpells.end(), id)
        != player.learnedSpells.end();
}
}

std::vector<SpellSynergyHint> spellSynergyHints(
    const run::PlayerProgress& player, const run::ContentId candidate)
{
    std::vector<SpellSynergyHint> hints;
    hints.reserve(3U);
    const auto add = [&](const run::ContentId ownedId, const std::string_view description) {
        if (hints.size() < 3U && ownedId != candidate)
            hints.push_back({ownedId, description});
    };
    const auto inspect = [&](const run::ContentId ownedId) {
        for (const auto& synergy : SpellSynergies)
            if ((synergy.first == candidate && synergy.second == ownedId)
                || (synergy.second == candidate && synergy.first == ownedId))
            {
                add(ownedId, synergy.description);
                return;
            }
        if (ownedId == 1002U && isDamagingRegularSpell(candidate))
            add(ownedId, "Cast Goddess Blessing before this damaging spell: it deals 15 percent more damage during the 5 second blessing window.");
        else if (candidate == 1002U && isDamagingRegularSpell(ownedId))
            add(ownedId, "Cast Goddess Blessing, then use this owned damaging spell during its 5 second window: the spell deals 15 percent more damage.");
        else if (ownedId == 1023U && isDamagingRegularSpell(candidate))
            add(ownedId, "Cast Flight Magic first, then use this as the first damaging spell while flying: it deals 15 percent more damage.");
        else if (candidate == 1023U && isDamagingRegularSpell(ownedId))
            add(ownedId, "Cast Flight Magic, then use this owned spell as the first damaging spell while flying: it deals 15 percent more damage.");
        else if (ownedId == 2012U && isDamagingRegularSpell(candidate))
            add(ownedId, "Cast Mirror Array first: this regular damaging spell is repeated by both mirrors at 45 percent power each, adding 90 percent total damage.");
        else if (candidate == 2012U && isDamagingRegularSpell(ownedId))
            add(ownedId, "Cast Mirror Array, then use this owned regular damaging spell: both mirrors repeat it at 45 percent power, adding 90 percent total damage.");
    };
    for (const auto id : player.learnedSpells)
    {
        inspect(id);
        if (hints.size() >= 3U) return hints;
    }
    for (const auto id : player.learnedBossSpells)
    {
        inspect(id);
        if (hints.size() >= 3U) return hints;
    }
    return hints;
}

RewardOffer generateOffer(const std::span<const run::ContentId> pool,
    const std::span<const run::ContentId> owned, const run::Seed seed, const int fallbackGold)
{
    if (fallbackGold < 0) throw std::invalid_argument("fallback gold cannot be negative");
    std::vector<run::ContentId> eligible;
    for (const auto id : pool)
    {
        if (std::find(owned.begin(), owned.end(), id) == owned.end()
            && std::find(eligible.begin(), eligible.end(), id) == eligible.end())
        {
            eligible.push_back(id);
        }
    }
    if (eligible.size() < 3U)
    {
        return {{{}}, seed, RewardKind::GoldFallback, fallbackGold};
    }

    run::DeterministicRng rng(seed);
    for (std::size_t index = eligible.size() - 1U; index > 0U; --index)
    {
        const auto swapIndex = rng.index(static_cast<std::uint32_t>(index + 1U));
        std::swap(eligible[index], eligible[swapIndex]);
    }
    return {{{eligible[0], eligible[1], eligible[2]}}, seed, RewardKind::SpellChoice, 0};
}

RewardOffer generateCategorizedOffer(const std::span<const run::ContentId> damagePool,
    const std::span<const run::ContentId> controlPool, const std::span<const run::ContentId> fullPool,
    const std::span<const run::ContentId> owned, const run::Seed seed, const int fallbackGold)
{
    if (fallbackGold < 0) throw std::invalid_argument("fallback gold cannot be negative");
    run::DeterministicRng rng(seed);
    std::array<run::ContentId, 3> selected {};
    const auto choose = [&](const std::span<const run::ContentId> pool, const std::size_t count) {
        std::vector<run::ContentId> eligible;
        for (const auto id : pool)
            if (std::find(owned.begin(), owned.end(), id) == owned.end()
                && std::find(selected.begin(), selected.begin() + static_cast<std::ptrdiff_t>(count), id)
                    == selected.begin() + static_cast<std::ptrdiff_t>(count)
                && std::find(eligible.begin(), eligible.end(), id) == eligible.end())
                eligible.push_back(id);
        return eligible.empty() ? run::ContentId {} : eligible[rng.index(
            static_cast<std::uint32_t>(eligible.size()))];
    };
    selected[0] = choose(damagePool, 0U);
    selected[1] = choose(controlPool, 1U);
    selected[2] = choose(fullPool, 2U);
    if (selected[0] == 0U || selected[1] == 0U || selected[2] == 0U)
        return generateOffer(fullPool, owned, seed, fallbackGold);
    return {selected, seed, RewardKind::SpellChoice, 0};
}

RewardOffer generateProgressionOffer(const std::span<const run::ContentId> actPool,
    const std::span<const run::ContentId> unlockedPool, const run::PlayerProgress& player,
    const std::uint32_t act, const run::Seed seed, const int fallbackGold)
{
    if (fallbackGold < 0) throw std::invalid_argument("fallback gold cannot be negative");
    run::DeterministicRng rng(seed);
    std::vector<run::ContentId> selected;
    selected.reserve(3U);

    const auto addRandom = [&](const std::span<const run::ContentId> pool, const auto& predicate) {
        std::vector<run::ContentId> eligible;
        for (const auto id : pool)
            if (std::find(selected.begin(), selected.end(), id) == selected.end()
                && std::find(eligible.begin(), eligible.end(), id) == eligible.end()
                && predicate(id)) eligible.push_back(id);
        if (eligible.empty()) return false;
        selected.push_back(eligible[rng.index(static_cast<std::uint32_t>(eligible.size()))]);
        return true;
    };

    const auto addUpgradeableEquipped = [&] {
        std::array<run::ContentId, 3> equipped {};
        std::size_t count = 0U;
        for (const auto id : player.equippedSpells)
            if (id && progression::canUpgradeSpell(player, *id, act)
                && std::find(selected.begin(), selected.end(), *id) == selected.end())
                equipped[count++] = *id;
        if (count == 0U) return false;
        selected.push_back(equipped[rng.index(static_cast<std::uint32_t>(count))]);
        return true;
    };

    if (act <= 1U)
    {
        while (selected.size() < 3U
            && addRandom(actPool, [&](const auto id) { return !owns(player, id); })) {}
    }
    else
    {
        static_cast<void>(addRandom(actPool, [&](const auto id) { return !owns(player, id); }));
        static_cast<void>(addUpgradeableEquipped());
        static_cast<void>(addRandom(unlockedPool, [&](const auto id) {
            if (owns(player, id)) return false;
            return std::any_of(player.equippedSpells.begin(), player.equippedSpells.end(),
                [&](const auto equipped) { return equipped && isSynergy(*equipped, id); });
        }));
        while (selected.size() < 3U && addUpgradeableEquipped()) {}
        while (selected.size() < 3U
            && addRandom(actPool, [&](const auto id) { return !owns(player, id); })) {}
        while (selected.size() < 3U
            && addRandom(unlockedPool, [&](const auto id) { return !owns(player, id); })) {}
        while (selected.size() < 3U
            && addRandom(player.learnedSpells, [&](const auto id) {
                return progression::canUpgradeSpell(player, id, act);
            })) {}
    }

    if (selected.size() < 3U)
        return {{{}}, seed, RewardKind::GoldFallback, fallbackGold};
    return {{{selected[0], selected[1], selected[2]}}, seed, RewardKind::SpellChoice, 0};
}

bool applySpellChoice(run::PlayerProgress& player, const RewardOffer& offer,
    const run::ContentId choice, const SpellRewardType type)
{
    if (offer.kind != RewardKind::SpellChoice) return false;
    auto& learned = type == SpellRewardType::Boss
        ? player.learnedBossSpells
        : player.learnedSpells;
    if (std::find(offer.candidates.begin(), offer.candidates.end(), choice) == offer.candidates.end()
        || std::find(learned.begin(), learned.end(), choice) != learned.end())
    {
        return false;
    }
    learned.push_back(choice);
    if (type == SpellRewardType::Regular)
        progression::registerLearnedSpell(player, choice);
    if (type == SpellRewardType::Boss) player.ultimateSpellUnlocked = true;
    return true;
}
}
