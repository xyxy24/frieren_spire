#pragma once

#include "game/contracts/PlayerIntent.hpp"
#include "game/math/Vec2.hpp"

namespace arcane::game
{
struct WorldBounds
{
    float left {0.0F};
    float right {1280.0F};
    float groundTop {640.0F};
};

class PlayerController
{
public:
    static constexpr float Width = 42.0F;
    static constexpr float Height = 64.0F;
    static constexpr float DashDistance = 150.0F;
    static constexpr float DashInvulnerabilitySeconds = 0.20F;
    static constexpr float DashCooldownSeconds = 1.20F;

    explicit PlayerController(Vec2 spawnPosition = {160.0F, 576.0F});

    void update(const PlayerIntent& intent, float deltaSeconds, const WorldBounds& bounds);

    [[nodiscard]] Vec2 position() const noexcept;
    [[nodiscard]] Vec2 velocity() const noexcept;
    [[nodiscard]] bool isGrounded() const noexcept;
    [[nodiscard]] float facingDirection() const noexcept;
    void applyHitReaction(float horizontalVelocity, float stunSeconds) noexcept;
    void applyLaunch(float upwardSpeed, float stunSeconds) noexcept;
    [[nodiscard]] bool isStunned() const noexcept;
    [[nodiscard]] float stunRemaining() const noexcept;
    [[nodiscard]] bool isDashing() const noexcept;
    [[nodiscard]] float dashRemaining() const noexcept;
    [[nodiscard]] float dashCooldownRemaining() const noexcept;

private:
    static constexpr float MoveSpeed = 260.0F;
    static constexpr float JumpSpeed = 650.0F;
    static constexpr float Gravity = 1600.0F;
    static constexpr float GroundTolerance = 0.01F;

    Vec2 position_;
    Vec2 velocity_;
    bool grounded_ {false};
    float facingDirection_ {1.0F};
    float stunRemaining_ {};
    float dashRemaining_ {};
    float dashCooldownRemaining_ {};
};
}
