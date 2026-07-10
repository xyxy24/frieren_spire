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

bool applySpellChoice(run::PlayerProgress& player, const RewardOffer& offer, const run::ContentId choice)
{
    if (offer.kind != RewardKind::SpellChoice) return false;
    if (std::find(offer.candidates.begin(), offer.candidates.end(), choice) == offer.candidates.end()
        || std::find(player.learnedSpells.begin(), player.learnedSpells.end(), choice)
            != player.learnedSpells.end())
    {
        return false;
    }
    player.learnedSpells.push_back(choice);
    return true;
}
}
