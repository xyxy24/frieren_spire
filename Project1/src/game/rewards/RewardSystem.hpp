#pragma once

#include "game/run/RunTypes.hpp"

#include <array>
#include <span>

namespace arcane::game::rewards
{
enum class RewardKind : std::uint8_t { SpellChoice, GoldFallback };
enum class SpellRewardType : std::uint8_t { Regular, Boss };

struct RewardOffer
{
    std::array<run::ContentId, 3> candidates {};
    run::Seed seed {};
    RewardKind kind {RewardKind::SpellChoice};
    int fallbackGold {};
};

[[nodiscard]] RewardOffer generateOffer(std::span<const run::ContentId> pool,
    std::span<const run::ContentId> owned, run::Seed seed, int fallbackGold = 15);
[[nodiscard]] RewardOffer generateCategorizedOffer(std::span<const run::ContentId> damagePool,
    std::span<const run::ContentId> controlPool, std::span<const run::ContentId> fullPool,
    std::span<const run::ContentId> owned, run::Seed seed, int fallbackGold = 15);
[[nodiscard]] bool applySpellChoice(run::PlayerProgress& player, const RewardOffer& offer,
    run::ContentId choice, SpellRewardType type = SpellRewardType::Regular);
}
