#include "game/player/PlayerController.hpp"

#include <algorithm>

namespace arcane::game
{
PlayerController::PlayerController(const Vec2 spawnPosition)
    : position_(spawnPosition)
{
}

void PlayerController::update(const PlayerIntent& intent, const float deltaSeconds, const WorldBounds& bounds)
{
    if (deltaSeconds <= 0.0F)
    {
        return;
    }

    const float groundPosition = bounds.groundTop - Height;
    const bool touchingGround = position_.y >= groundPosition - GroundTolerance && velocity_.y >= 0.0F;

    if (touchingGround)
    {
        position_.y = groundPosition;
        velocity_.y = 0.0F;
        grounded_ = true;
    }
    else
    {
        grounded_ = false;
    }

    stunRemaining_ = std::max(0.0F, stunRemaining_ - deltaSeconds);
    dashCooldownRemaining_ = std::max(0.0F, dashCooldownRemaining_ - deltaSeconds);
    shadowDashChargeRemaining_ = std::max(0.0F, shadowDashChargeRemaining_ - deltaSeconds);
    flightRemaining_ = std::max(0.0F, flightRemaining_ - deltaSeconds);
    const float moveAxis = stunRemaining_ > 0.0F ? 0.0F : std::clamp(intent.moveAxis, -1.0F, 1.0F);

    if (dashRemaining_ <= 0.0F && moveAxis > 0.0F)
    {
        facingDirection_ = 1.0F;
    }
    else if (dashRemaining_ <= 0.0F && moveAxis < 0.0F)
    {
        facingDirection_ = -1.0F;
    }

    bool consumedByDash = dashRemaining_ > 0.0F;
    if (intent.dashPressed && stunRemaining_ <= 0.0F && dashRemaining_ <= 0.0F
        && dashCooldownRemaining_ <= 0.0F)
    {
        dashRemaining_ = DashDurationSeconds;
        dashCooldownRemaining_ = DashCooldownSeconds;
        shadowDashActive_ = shadowDashChargeRemaining_ <= 0.0F;
        if (shadowDashActive_) shadowDashChargeRemaining_ = ShadowDashChargeSeconds;
        dashDirection_ = facingDirection_;
        preDashVerticalVelocity_ = velocity_.y;
        consumedByDash = true;
    }

    float simulationSeconds = deltaSeconds;
    if (dashRemaining_ > 0.0F)
    {
        const float dashSeconds = std::min(simulationSeconds, dashRemaining_);
        const float dashSpeed = DashDistance / DashDurationSeconds;
        const float unclampedX = position_.x + dashDirection_ * dashSpeed * dashSeconds;
        position_.x = std::clamp(unclampedX, bounds.left, bounds.right - Width);
        dashRemaining_ = std::max(0.0F, dashRemaining_ - dashSeconds);
        simulationSeconds -= dashSeconds;
        velocity_.x = dashDirection_ * dashSpeed;
        velocity_.y = 0.0F;
        if (position_.x != unclampedX) dashRemaining_ = 0.0F;
        if (dashRemaining_ > 0.0F) return;
        velocity_.x = 0.0F;
        velocity_.y = preDashVerticalVelocity_;
        if (simulationSeconds <= 0.0F) return;
    }

    if (stunRemaining_ <= 0.0F) velocity_.x = (consumedByDash ? 0.0F : moveAxis) * MoveSpeed;

    if (flightRemaining_ > 0.0F && stunRemaining_ <= 0.0F)
    {
        velocity_.y = std::clamp(intent.verticalMoveAxis, -1.0F, 1.0F) * MoveSpeed;
        grounded_ = false;
    }
    else if (intent.jumpPressed && !consumedByDash && grounded_ && stunRemaining_ <= 0.0F)
    {
        velocity_.y = -JumpSpeed;
        grounded_ = false;
    }

    if (!grounded_ && flightRemaining_ <= 0.0F)
    {
        velocity_.y += Gravity * simulationSeconds;
    }

    position_.x += velocity_.x * simulationSeconds;
    position_.y += velocity_.y * simulationSeconds;

    position_.x = std::clamp(position_.x, bounds.left, bounds.right - Width);
    if (flightRemaining_ > 0.0F)
        position_.y = std::clamp(position_.y, 48.0F, groundPosition);

    if (position_.y >= groundPosition && velocity_.y >= 0.0F)
    {
        position_.y = groundPosition;
        velocity_.y = 0.0F;
        grounded_ = true;
    }
}

Vec2 PlayerController::position() const noexcept
{
    return position_;
}

Vec2 PlayerController::velocity() const noexcept
{
    return velocity_;
}

bool PlayerController::isGrounded() const noexcept
{
    return grounded_;
}

float PlayerController::facingDirection() const noexcept
{
    return facingDirection_;
}

void PlayerController::applyHitReaction(const float horizontalVelocity, const float stunSeconds) noexcept
{
    velocity_.x = horizontalVelocity;
    stunRemaining_ = std::max(stunRemaining_, std::max(0.0F, stunSeconds));
}

void PlayerController::applyLaunch(const float upwardSpeed, const float stunSeconds) noexcept
{
    velocity_.y = -std::abs(upwardSpeed);
    grounded_ = false;
    stunRemaining_ = std::max(stunRemaining_, std::max(0.0F, stunSeconds));
}

bool PlayerController::isStunned() const noexcept { return stunRemaining_ > 0.0F; }
float PlayerController::stunRemaining() const noexcept { return stunRemaining_; }
bool PlayerController::isDashing() const noexcept { return dashRemaining_ > 0.0F; }
bool PlayerController::isShadowDashing() const noexcept
{ return dashRemaining_ > 0.0F && shadowDashActive_; }
float PlayerController::dashRemaining() const noexcept { return dashRemaining_; }
float PlayerController::dashCooldownRemaining() const noexcept { return dashCooldownRemaining_; }
float PlayerController::shadowDashChargeRemaining() const noexcept
{ return shadowDashChargeRemaining_; }
void PlayerController::grantFlight(const float seconds) noexcept
{ flightRemaining_ = std::max(flightRemaining_, std::max(0.0F, seconds)); }
void PlayerController::translateHorizontal(const float distance, const WorldBounds& bounds) noexcept
{ position_.x = std::clamp(position_.x + distance, bounds.left, bounds.right - Width); }
void PlayerController::setHorizontalPosition(const float position, const WorldBounds& bounds) noexcept
{
    position_.x = std::clamp(position, bounds.left, bounds.right - Width);
    velocity_.x = 0.0F;
}
void PlayerController::settleOnGround(const WorldBounds& bounds) noexcept
{
    position_.y = bounds.groundTop - Height;
    velocity_ = {};
    grounded_ = true;
    stunRemaining_ = 0.0F;
    dashRemaining_ = 0.0F;
    shadowDashActive_ = false;
    preDashVerticalVelocity_ = 0.0F;
    flightRemaining_ = 0.0F;
}
float PlayerController::flightRemaining() const noexcept { return flightRemaining_; }
void PlayerController::reduceDashCooldown(const float seconds) noexcept
{ dashCooldownRemaining_ = std::max(0.0F, dashCooldownRemaining_ - std::max(0.0F, seconds)); }
void PlayerController::refreshDash() noexcept { dashCooldownRemaining_ = 0.0F; }
}
