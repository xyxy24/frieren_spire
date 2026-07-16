#pragma once

#include "game/run/RunTypes.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace arcane::game::progression
{
enum class BreakthroughType : std::uint8_t { Power, Haste, Defense };

struct BreakthroughDefinition
{
    BreakthroughType type {BreakthroughType::Power};
    std::string_view name;
    std::string_view firstDescription;
    std::string_view secondDescription;
};

[[nodiscard]] const std::array<BreakthroughDefinition, 3>& breakthroughDefinitions() noexcept;
[[nodiscard]] std::uint8_t spellRank(
    const run::PlayerProgress& player, run::ContentId spellId) noexcept;
[[nodiscard]] std::uint8_t maximumSpellRank(std::uint32_t act) noexcept;
[[nodiscard]] bool canUpgradeSpell(const run::PlayerProgress& player,
    run::ContentId spellId, std::uint32_t act) noexcept;
[[nodiscard]] bool upgradeSpell(run::PlayerProgress& player,
    run::ContentId spellId, std::uint32_t act) noexcept;
void registerLearnedSpell(run::PlayerProgress& player, run::ContentId spellId);

[[nodiscard]] bool applyBreakthrough(
    run::PlayerProgress& player, BreakthroughType type) noexcept;
[[nodiscard]] float regularDamageMultiplier(const run::PlayerProgress& player) noexcept;
[[nodiscard]] float regularCooldownMultiplier(const run::PlayerProgress& player) noexcept;
[[nodiscard]] int startingShield(const run::PlayerProgress& player) noexcept;

[[nodiscard]] float masteryPowerMultiplier(std::uint8_t rank) noexcept;
[[nodiscard]] float masteryCooldownMultiplier(std::uint8_t rank) noexcept;
[[nodiscard]] float masteryAreaMultiplier(std::uint8_t rank) noexcept;
[[nodiscard]] std::string_view rankThreeSignature(run::ContentId spellId) noexcept;
}
