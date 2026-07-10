#include "game/combat/AttackState.hpp"

#include <algorithm>

namespace arcane::game
{
void AttackState::update(const float deltaSeconds) noexcept
{
    if (deltaSeconds <= 0.0F)
    {
        return;
    }

    cooldownRemaining_ = std::max(0.0F, cooldownRemaining_ - deltaSeconds);
    activeRemaining_ = std::max(0.0F, activeRemaining_ - deltaSeconds);
}

bool AttackState::tryStart() noexcept
{
    if (cooldownRemaining_ > 0.0F)
    {
        return false;
    }

    cooldownRemaining_ = CooldownSeconds;
    activeRemaining_ = ActiveSeconds;
    ++sequence_;
    return true;
}

bool AttackState::isActive() const noexcept
{
    return activeRemaining_ > 0.0F;
}

float AttackState::cooldownRemaining() const noexcept
{
    return cooldownRemaining_;
}

std::uint64_t AttackState::sequence() const noexcept
{
    return sequence_;
}
}
