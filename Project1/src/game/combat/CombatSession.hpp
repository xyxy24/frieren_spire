#pragma once

#include "game/combat/Aabb.hpp"
#include "game/combat/AttackState.hpp"
#include "game/combat/CombatContracts.hpp"
#include "game/combat/Health.hpp"
#include "game/combat/DamageResolver.hpp"
#include "game/contracts/PlayerIntent.hpp"
#include "game/player/PlayerController.hpp"
#include "game/spells/SpellSystem.hpp"
#include "game/ai/EnemyController.hpp"
#include "game/relics/RelicSystem.hpp"

#include <cstdint>
#include <optional>

namespace arcane::game
{
class CombatSession
{
public:
    static constexpr float EnemyWidth = ai::EnemyController::Width;
    static constexpr float EnemyHeight = ai::EnemyController::Height;

    explicit CombatSession(CombatRequest request);

    void update(const PlayerIntent& intent, float deltaSeconds);

    [[nodiscard]] PlayerStateView playerState() const noexcept;
    [[nodiscard]] EnemyStateView enemyState() const noexcept;
    [[nodiscard]] Aabb attackBounds() const noexcept;
    [[nodiscard]] const std::optional<CombatResult>& result() const noexcept;

private:
    static constexpr float AttackRange = 58.0F;
    static constexpr float AttackHeight = 36.0F;
    static constexpr float AttackVerticalOffset = 14.0F;
    static constexpr int AttackDamage = 15;
    static constexpr float HitStunSeconds = 0.28F;
    static constexpr float KnockbackSpeed = 360.0F;

    [[nodiscard]] Aabb playerBounds() const noexcept;
    [[nodiscard]] Aabb enemyBounds() const noexcept;
    void finish(CombatOutcome outcome) noexcept;

    CombatRequest request_;
    PlayerController player_;
    Health playerHealth_;
    ai::EnemyController enemy_;
    Health enemyHealth_;
    AttackState attack_;
    spells::SpellSystem spells_;
    relics::RelicRuntime relics_;
    DamageResolver playerDamageResolver_;
    DamageResolver enemyDamageResolver_;
    float blessingRemaining_ {};
    float flowerFieldRemaining_ {};
    float flowerFieldCenterX_ {};
    float flowerHealingAccumulator_ {};
    std::uint64_t selfDamageSequence_ {};
    bool enemySlowed_ {};
    float contactDamageCooldownRemaining_ {};
    std::uint64_t contactDamageSequence_ {};
    std::optional<CombatResult> result_;
};
}
