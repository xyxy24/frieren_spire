#pragma once

#include "game/combat/Health.hpp"

#include <array>
#include <cstdint>

namespace arcane::game
{
enum class DamageSource : std::uint8_t
{
    PlayerBasicAttack,
    PlayerSpell0,
    PlayerSpell1,
    PlayerSpell2,
    PlayerUltimateSpell,
    EnemyAttack,
    EnemyContact,
    Event,
    Count
};

struct DamageRequest
{
    DamageSource source {DamageSource::PlayerBasicAttack};
    std::uint64_t sequence {};
    int baseDamage {};
    float sourceMultiplier {1.0F};
    float targetMultiplier {1.0F};
    int flatReduction {};
    bool blocked {};
};

struct DamageResult
{
    DamageSource source {DamageSource::PlayerBasicAttack};
    std::uint64_t sequence {};
    int baseDamage {};
    int resolvedDamage {};
    int appliedDamage {};
    int healthBefore {};
    int healthAfter {};
    bool killed {};
    bool blocked {};
    bool duplicate {};
};

class DamageResolver
{
public:
    [[nodiscard]] DamageResult resolve(Health& target, const DamageRequest& request) noexcept;

private:
    struct SequenceState
    {
        bool initialized {};
        std::uint64_t value {};
    };
    std::array<SequenceState, static_cast<std::size_t>(DamageSource::Count)> lastSequences_;
};
}
