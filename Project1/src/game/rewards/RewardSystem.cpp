#include "game/rewards/RewardSystem.hpp"

#include "game/run/DeterministicRng.hpp"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace arcane::game::rewards
{
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
    if (type == SpellRewardType::Boss) player.ultimateSpellUnlocked = true;
    return true;
}
}
