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

    const float moveAxis = std::clamp(intent.moveAxis, -1.0F, 1.0F);
    velocity_.x = moveAxis * MoveSpeed;

    if (intent.jumpPressed && grounded_)
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
}

