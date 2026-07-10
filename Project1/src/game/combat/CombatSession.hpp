#pragma once

#include "game/combat/Aabb.hpp"
#include "game/combat/AttackState.hpp"
#include "game/combat/CombatContracts.hpp"
#include "game/combat/Health.hpp"
#include "game/contracts/PlayerIntent.hpp"
#include "game/player/PlayerController.hpp"
#include "game/spells/SpellSystem.hpp"

#include <cstdint>
#include <optional>

namespace arcane::game
{
class CombatSession
{
public:
    static constexpr float EnemyWidth = 48.0F;
    static constexpr float EnemyHeight = 64.0F;

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
    static constexpr int AttackDamage = 25;
    static constexpr float ContactDamageCooldown = 1.0F;

    [[nodiscard]] Aabb playerBounds() const noexcept;
    [[nodiscard]] Aabb enemyBounds() const noexcept;
    void finish(CombatOutcome outcome) noexcept;

    CombatRequest request_;
    PlayerController player_;
    Health playerHealth_;
    Vec2 enemyPosition_;
    Health enemyHealth_;
    AttackState attack_;
    spells::SpellSystem spells_;
    std::uint64_t lastHitAttackSequence_ {0};
    float contactDamageCooldownRemaining_ {0.0F};
    std::optional<CombatResult> result_;
};
}
