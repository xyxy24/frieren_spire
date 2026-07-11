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
#include <vector>

namespace arcane::game
{
class CombatSession
{
public:
    static constexpr float EnemyWidth = ai::EnemyController::Width;
    static constexpr float EnemyHeight = ai::EnemyController::Height;
    static constexpr float GuardDurationSeconds = 0.60F;
    static constexpr float GuardCooldownSeconds = 2.0F;

    explicit CombatSession(CombatRequest request);

    void update(const PlayerIntent& intent, float deltaSeconds);

    [[nodiscard]] PlayerStateView playerState() const noexcept;
    [[nodiscard]] EnemyStateView enemyState() const noexcept;
    [[nodiscard]] std::vector<EnemyStateView> enemyStates() const;
    [[nodiscard]] Aabb attackBounds() const noexcept;
    [[nodiscard]] const std::optional<CombatResult>& result() const noexcept;
    [[nodiscard]] bool equipSpell(std::size_t slot, std::optional<std::uint32_t> id) noexcept;
    [[nodiscard]] bool equipUltimateSpell(std::optional<std::uint32_t> id) noexcept;

private:
    static constexpr float AttackRange = 58.0F;
    static constexpr float AttackHeight = 36.0F;
    static constexpr float AttackVerticalOffset = 14.0F;
    static constexpr int AttackDamage = 15;
    static constexpr float HitStunSeconds = 0.28F;
    static constexpr float KnockbackSpeed = 360.0F;

    [[nodiscard]] Aabb playerBounds() const noexcept;
    struct EnemyRuntime
    {
        EnemyArchetype archetype;
        ai::EnemyController controller;
        Health health;
        DamageResolver damageResolver;
        float contactCooldown {};
        std::uint64_t contactSequence {};
        std::uint64_t handledSkillSequence {};
        bool slowed {};
        float summonCooldown {};
        std::uint32_t summonCount {};
    };
    [[nodiscard]] static ai::EnemyConfig enemyConfigFor(EnemyArchetype archetype);
    [[nodiscard]] Aabb firstLivingEnemyBounds() const noexcept;
    void finish(CombatOutcome outcome) noexcept;

    CombatRequest request_;
    PlayerController player_;
    Health playerHealth_;
    std::vector<EnemyRuntime> enemies_;
    AttackState attack_;
    spells::SpellSystem spells_;
    relics::RelicRuntime relics_;
    DamageResolver playerDamageResolver_;
    float blessingRemaining_ {};
    float flowerFieldRemaining_ {};
    float flowerFieldCenterX_ {};
    float flowerHealingAccumulator_ {};
    float guardRemaining_ {};
    float guardCooldownRemaining_ {};
    std::uint64_t selfDamageSequence_ {};
    std::optional<CombatResult> result_;
};
}
