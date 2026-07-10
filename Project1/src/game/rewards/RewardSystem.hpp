#pragma once

#include "game/run/RunTypes.hpp"

#include <array>
#include <span>

namespace arcane::game::rewards
{
struct RewardOffer
{
    std::array<run::ContentId, 3> candidates {};
    run::Seed seed {};
};

[[nodiscard]] RewardOffer generateOffer(std::span<const run::ContentId> pool,
    std::span<const run::ContentId> owned, run::Seed seed);
[[nodiscard]] bool applySpellChoice(run::PlayerProgress& player, const RewardOffer& offer,
    run::ContentId choice);
}
