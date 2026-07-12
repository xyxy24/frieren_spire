#pragma once

#include "game/combat/Aabb.hpp"
#include "game/math/Vec2.hpp"
#include "game/player/PlayerController.hpp"

#include <cstdint>

namespace arcane::game::ai
{
enum class EnemyAction : std::uint8_t { Chase, Windup, Active, Recovery, Dead };
enum class EnemySkill : std::uint8_t {
    Thrust, Slash, Dive, Blood, LeapingCleave, Thread, Domination, DragonClaw, BossAttack
};

struct EnemyConfig
{
    float moveSpeed {105.0F};
    float attackDistance {62.0F};
    float attackRange {72.0F};
    float windupSeconds {0.45F};
    float activeSeconds {0.12F};
    float recoverySeconds {0.55F};
    float activeDashDistance {};
    float width {48.0F};
    float height {64.0F};
    float cooldownSeconds {1.0F};
    bool hasContactDamage {true};
    bool flying {};
    EnemySkill skill {EnemySkill::Slash};
};

class EnemyController
{
public:
    static constexpr float Width = 48.0F;
    static constexpr float Height = 64.0F;

    explicit EnemyController(Vec2 spawnPosition, EnemyConfig config = {});
    void update(const Aabb& playerBounds, float deltaSeconds, const WorldBounds& worldBounds,
        float speedMultiplier = 1.0F, bool canAttack = true) noexcept;
    void markDead() noexcept;
    void interrupt() noexcept;
    void translateHorizontal(float distance, const WorldBounds& worldBounds) noexcept;
    [[nodiscard]] Vec2 position() const noexcept;
    [[nodiscard]] EnemyAction action() const noexcept;
    [[nodiscard]] float facingDirection() const noexcept;
    [[nodiscard]] std::uint64_t attackSequence() const noexcept;
    [[nodiscard]] Aabb attackBounds() const noexcept;
    [[nodiscard]] Aabb bounds() const noexcept;
    [[nodiscard]] const EnemyConfig& config() const noexcept;

private:
    void beginWindup() noexcept;
    EnemyConfig config_;
    Vec2 position_;
    EnemyAction action_ {EnemyAction::Chase};
    float stateRemaining_ {};
    float facingDirection_ {-1.0F};
    std::uint64_t attackSequence_ {};
    float activeElapsed_ {};
    float cooldownRemaining_ {};
    float groundTop_ {640.0F};
};
}
