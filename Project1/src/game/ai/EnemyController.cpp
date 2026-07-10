#include "game/ai/EnemyController.hpp"

#include <algorithm>
#include <cmath>

namespace arcane::game::ai
{
EnemyController::EnemyController(const Vec2 spawnPosition, const EnemyConfig config)
    : config_(config), position_(spawnPosition) {}

void EnemyController::update(const Aabb& playerBounds, const float deltaSeconds,
    const WorldBounds& worldBounds, const float speedMultiplier) noexcept
{
    if (deltaSeconds <= 0.0F || action_ == EnemyAction::Dead) return;
    const float playerCenter = playerBounds.left + playerBounds.width * 0.5F;
    const float enemyCenter = position_.x + Width * 0.5F;
    const float horizontalDelta = playerCenter - enemyCenter;
    if (horizontalDelta != 0.0F) facingDirection_ = horizontalDelta > 0.0F ? 1.0F : -1.0F;

    switch (action_)
    {
    case EnemyAction::Chase:
        if (std::abs(horizontalDelta) <= config_.attackDistance)
        {
            beginWindup();
        }
        else
        {
            const float maximumTravel = std::max(0.0F, std::abs(horizontalDelta) - config_.attackDistance);
            const float travel = std::min(config_.moveSpeed * std::max(0.0F, speedMultiplier)
                * deltaSeconds, maximumTravel);
            position_.x += facingDirection_ * travel;
            position_.x = std::clamp(position_.x, worldBounds.left, worldBounds.right - Width);
        }
        break;
    case EnemyAction::Windup:
        stateRemaining_ -= deltaSeconds;
        if (stateRemaining_ <= 0.0F)
        {
            action_ = EnemyAction::Active;
            stateRemaining_ = config_.activeSeconds;
            position_.x = std::clamp(position_.x + facingDirection_ * config_.activeDashDistance,
                worldBounds.left, worldBounds.right - Width);
            ++attackSequence_;
        }
        break;
    case EnemyAction::Active:
        stateRemaining_ -= deltaSeconds;
        if (stateRemaining_ <= 0.0F)
        {
            action_ = EnemyAction::Recovery;
            stateRemaining_ = config_.recoverySeconds;
        }
        break;
    case EnemyAction::Recovery:
        stateRemaining_ -= deltaSeconds;
        if (stateRemaining_ <= 0.0F) action_ = EnemyAction::Chase;
        break;
    case EnemyAction::Dead:
        break;
    }
}

void EnemyController::markDead() noexcept { action_ = EnemyAction::Dead; }
Vec2 EnemyController::position() const noexcept { return position_; }
EnemyAction EnemyController::action() const noexcept { return action_; }
float EnemyController::facingDirection() const noexcept { return facingDirection_; }
std::uint64_t EnemyController::attackSequence() const noexcept { return attackSequence_; }

Aabb EnemyController::attackBounds() const noexcept
{
    const float left = facingDirection_ > 0.0F ? position_.x : position_.x - config_.attackRange;
    return {left, position_.y + 10.0F, config_.attackRange + Width, Height - 20.0F};
}

void EnemyController::beginWindup() noexcept
{
    action_ = EnemyAction::Windup;
    stateRemaining_ = config_.windupSeconds;
}
}
