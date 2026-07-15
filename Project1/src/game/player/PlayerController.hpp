#pragma once

#include "game/combat/Aabb.hpp"
#include "game/contracts/PlayerIntent.hpp"
#include "game/math/Vec2.hpp"

#include <span>

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
    static constexpr float DashDurationSeconds = 0.18F;
    static constexpr float DashCooldownSeconds = 0.60F;
    static constexpr float ShadowDashChargeSeconds = 1.50F;

    explicit PlayerController(Vec2 spawnPosition = {160.0F, 576.0F});

    void update(const PlayerIntent& intent, float deltaSeconds, const WorldBounds& bounds,
        std::span<const Aabb> oneWayPlatforms = {});

    [[nodiscard]] Vec2 position() const noexcept;
    [[nodiscard]] Vec2 velocity() const noexcept;
    [[nodiscard]] bool isGrounded() const noexcept;
    [[nodiscard]] float facingDirection() const noexcept;
    void applyHitReaction(float horizontalVelocity, float stunSeconds) noexcept;
    void applyLaunch(float upwardSpeed, float stunSeconds) noexcept;
    void grantFlight(float seconds) noexcept;
    void translateHorizontal(float distance, const WorldBounds& bounds) noexcept;
    void setHorizontalPosition(float position, const WorldBounds& bounds) noexcept;
    void settleOnGround(const WorldBounds& bounds) noexcept;
    [[nodiscard]] bool isStunned() const noexcept;
    [[nodiscard]] float stunRemaining() const noexcept;
    [[nodiscard]] bool isDashing() const noexcept;
    [[nodiscard]] bool isShadowDashing() const noexcept;
    [[nodiscard]] float dashRemaining() const noexcept;
    [[nodiscard]] float dashCooldownRemaining() const noexcept;
    [[nodiscard]] float shadowDashChargeRemaining() const noexcept;
    [[nodiscard]] float flightRemaining() const noexcept;
    void reduceDashCooldown(float seconds) noexcept;
    void refreshDash() noexcept;

private:
    static constexpr float MoveSpeed = 260.0F;
    static constexpr float JumpSpeed = 650.0F;
    static constexpr float DropThroughSpeed = 80.0F;
    static constexpr float Gravity = 1600.0F;
    static constexpr float GroundTolerance = 0.01F;

    Vec2 position_;
    Vec2 velocity_;
    bool grounded_ {false};
    float facingDirection_ {1.0F};
    float stunRemaining_ {};
    float dashRemaining_ {};
    float dashCooldownRemaining_ {};
    float shadowDashChargeRemaining_ {};
    bool shadowDashActive_ {};
    float dashDirection_ {1.0F};
    float preDashVerticalVelocity_ {};
    float flightRemaining_ {};
};
}
