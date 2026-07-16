#pragma once

#include "game/run/RunTypes.hpp"

#include <array>
#include <span>
#include <string_view>
#include <vector>

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

struct SpellSynergyHint
{
    run::ContentId ownedSpellId {};
    std::string_view description;
};

[[nodiscard]] RewardOffer generateOffer(std::span<const run::ContentId> pool,
    std::span<const run::ContentId> owned, run::Seed seed, int fallbackGold = 15);
[[nodiscard]] RewardOffer generateCategorizedOffer(std::span<const run::ContentId> damagePool,
    std::span<const run::ContentId> controlPool, std::span<const run::ContentId> fullPool,
    std::span<const run::ContentId> owned, run::Seed seed, int fallbackGold = 15);
[[nodiscard]] RewardOffer generateProgressionOffer(std::span<const run::ContentId> actPool,
    std::span<const run::ContentId> unlockedPool, const run::PlayerProgress& player,
    std::uint32_t act, run::Seed seed, int fallbackGold = 15);
[[nodiscard]] std::vector<SpellSynergyHint> spellSynergyHints(
    const run::PlayerProgress& player, run::ContentId candidate);
[[nodiscard]] bool applySpellChoice(run::PlayerProgress& player, const RewardOffer& offer,
    run::ContentId choice, SpellRewardType type = SpellRewardType::Regular);
}
