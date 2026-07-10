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
    const float moveAxis = stunRemaining_ > 0.0F ? 0.0F : std::clamp(intent.moveAxis, -1.0F, 1.0F);
    if (stunRemaining_ <= 0.0F) velocity_.x = moveAxis * MoveSpeed;

    if (moveAxis > 0.0F)
    {
        facingDirection_ = 1.0F;
    }
    else if (moveAxis < 0.0F)
    {
        facingDirection_ = -1.0F;
    }

    if (intent.jumpPressed && grounded_ && stunRemaining_ <= 0.0F)
    {
        velocity_.y = -JumpSpeed;
        grounded_ = false;
    }

    if (!grounded_)
    {
        velocity_.y += Gravity * deltaSeconds;
    }

    position_.x += velocity_.x * deltaSeconds;
    position_.y += velocity_.y * deltaSeconds;

    position_.x = std::clamp(position_.x, bounds.left, bounds.right - Width);

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

bool PlayerController::isStunned() const noexcept { return stunRemaining_ > 0.0F; }
float PlayerController::stunRemaining() const noexcept { return stunRemaining_; }
}
