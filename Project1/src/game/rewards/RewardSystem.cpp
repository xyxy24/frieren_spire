#include "game/rewards/RewardSystem.hpp"

#include "game/run/DeterministicRng.hpp"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace arcane::game::rewards
{
RewardOffer generateOffer(const std::span<const run::ContentId> pool,
    const std::span<const run::ContentId> owned, const run::Seed seed)
{
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
        throw std::invalid_argument("reward pool requires three distinct unowned spells");
    }

    run::DeterministicRng rng(seed);
    for (std::size_t index = eligible.size() - 1U; index > 0U; --index)
    {
        const auto swapIndex = rng.index(static_cast<std::uint32_t>(index + 1U));
        std::swap(eligible[index], eligible[swapIndex]);
    }
    return {{{eligible[0], eligible[1], eligible[2]}}, seed};
}

bool applySpellChoice(run::PlayerProgress& player, const RewardOffer& offer, const run::ContentId choice)
{
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
