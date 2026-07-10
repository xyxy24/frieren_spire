#include "game/combat/DamageResolver.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace arcane::game
{
DamageResult DamageResolver::resolve(Health& target, const DamageRequest& request) noexcept
{
    DamageResult result;
    result.source = request.source;
    result.sequence = request.sequence;
    result.baseDamage = request.baseDamage;
    result.healthBefore = target.current();
    result.healthAfter = result.healthBefore;

    const auto sourceIndex = static_cast<std::size_t>(request.source);
    if (sourceIndex >= lastSequences_.size())
    {
        result.blocked = true;
        return result;
    }
    auto& last = lastSequences_[sourceIndex];
    if (last.initialized && last.value == request.sequence)
    {
        result.duplicate = true;
        return result;
    }
    last = {true, request.sequence};

    if (request.blocked || request.baseDamage <= 0 || request.sourceMultiplier <= 0.0F
        || request.targetMultiplier <= 0.0F || !target.isAlive())
    {
        result.blocked = request.blocked;
        return result;
    }

    const double scaled = static_cast<double>(request.baseDamage)
        * static_cast<double>(request.sourceMultiplier) * static_cast<double>(request.targetMultiplier);
    const double reduced = std::max(0.0, scaled - static_cast<double>(std::max(0, request.flatReduction)));
    const double capped = std::min(reduced, static_cast<double>(std::numeric_limits<int>::max()));
    result.resolvedDamage = static_cast<int>(std::lround(capped));
    result.appliedDamage = target.damage(result.resolvedDamage);
    result.healthAfter = target.current();
    result.killed = result.healthBefore > 0 && result.healthAfter == 0;
    return result;
}
}
