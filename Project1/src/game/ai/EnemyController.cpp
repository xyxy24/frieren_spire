#include "game/ai/EnemyController.hpp"

#include <algorithm>
#include <cmath>

namespace arcane::game::ai
{
EnemyController::EnemyController(const Vec2 spawnPosition, const EnemyConfig config)
    : config_(config), position_(spawnPosition), cooldownRemaining_(config.cooldownSeconds * 0.5F) {}

void EnemyController::update(const Aabb& playerBounds, const float deltaSeconds,
    const WorldBounds& worldBounds, const float speedMultiplier, const bool canAttack) noexcept
{
    if (deltaSeconds <= 0.0F || action_ == EnemyAction::Dead) return;
    groundTop_ = worldBounds.groundTop;
    cooldownRemaining_ = std::max(0.0F, cooldownRemaining_ - deltaSeconds);
    const float playerCenter = playerBounds.left + playerBounds.width * 0.5F;
    const float enemyCenter = position_.x + config_.width * 0.5F;
    const float horizontalDelta = playerCenter - enemyCenter;
    if (action_ == EnemyAction::Chase && horizontalDelta != 0.0F)
        facingDirection_ = horizontalDelta > 0.0F ? 1.0F : -1.0F;

    switch (action_)
    {
    case EnemyAction::Chase:
    {
        const bool displacementSkill = config_.activeDashDistance > 0.0F
            || config_.skill == EnemySkill::Dive
            || config_.skill == EnemySkill::LeapingCleave
            || config_.skill == EnemySkill::Swoop;
        const float triggerDistance = config_.skill == EnemySkill::Swoop
            ? 144.0F
            : config_.width * 0.5F + config_.attackDistance
                + (displacementSkill ? config_.activeDashDistance + playerBounds.width * 0.5F : 0.0F);
        if (canAttack && cooldownRemaining_ <= 0.0F && std::abs(horizontalDelta) <= triggerDistance)
        {
            if (config_.skill == EnemySkill::Swoop)
            {
                swoopTargetX_ = playerCenter + facingDirection_ * 64.0F;
                swoopAscending_ = false;
            }
            beginWindup();
        }
        else
        {
            const float desiredGap = config_.hasContactDamage ? 20.0F : 42.0F;
            const float desiredCenterDistance = config_.width * 0.5F
                + playerBounds.width * 0.5F + desiredGap;
            const float maximumTravel = std::max(0.0F,
                std::abs(horizontalDelta) - desiredCenterDistance);
            const float travel = std::min(config_.moveSpeed * std::max(0.0F, speedMultiplier)
                * deltaSeconds, maximumTravel);
            position_.x += facingDirection_ * travel;
            position_.x = std::clamp(position_.x, worldBounds.left, worldBounds.right - config_.width);
        }
        break;
    }
    case EnemyAction::Windup:
        stateRemaining_ -= deltaSeconds;
        if (stateRemaining_ <= 0.0F)
        {
            action_ = EnemyAction::Active;
            stateRemaining_ = config_.activeSeconds;
            activeElapsed_ = 0.0F;
            ++attackSequence_;
        }
        break;
    case EnemyAction::Active:
        activeElapsed_ += deltaSeconds;
        if (config_.skill == EnemySkill::Thrust)
            position_.x += facingDirection_ * 520.0F * deltaSeconds;
        else if (config_.skill == EnemySkill::Dive)
        {
            position_.x += facingDirection_ * 320.0F * deltaSeconds;
            const float progress = std::clamp(activeElapsed_ / std::max(config_.activeSeconds, 0.01F), 0.0F, 1.0F);
            const float airY = worldBounds.groundTop - 132.0F - config_.height;
            const float groundY = worldBounds.groundTop - config_.height;
            position_.y = progress < 0.5F
                ? airY + (groundY - airY) * (progress * 2.0F)
                : groundY + (airY - groundY) * ((progress - 0.5F) * 2.0F);
        }
        else if (config_.skill == EnemySkill::LeapingCleave)
        {
            constexpr float LeapSeconds = 0.9F;
            const float movingSeconds = std::min(deltaSeconds,
                std::max(0.0F, LeapSeconds - (activeElapsed_ - deltaSeconds)));
            position_.x += facingDirection_ * 160.0F * movingSeconds;
            const float progress = std::clamp(activeElapsed_ / LeapSeconds, 0.0F, 1.0F);
            position_.y = worldBounds.groundTop - config_.height - 162.0F * 4.0F * progress * (1.0F - progress);
        }
        else if (config_.skill == EnemySkill::Swoop)
        {
            const float airY = worldBounds.groundTop - 144.0F - config_.height;
            const float groundY = worldBounds.groundTop - config_.height;
            if (!swoopAscending_)
            {
                const float dx = swoopTargetX_ - (position_.x + config_.width * 0.5F);
                const float dy = groundY - position_.y;
                const float distance = std::sqrt(dx * dx + dy * dy);
                if (distance <= 200.0F * deltaSeconds || distance <= 0.01F)
                {
                    position_.x += dx;
                    position_.y = groundY;
                    swoopAscending_ = true;
                }
                else
                {
                    position_.x += dx / distance * 200.0F * deltaSeconds;
                    position_.y += dy / distance * 200.0F * deltaSeconds;
                }
            }
            else
            {
                const float playerDelta = playerCenter - (position_.x + config_.width * 0.5F);
                if (playerDelta != 0.0F)
                    facingDirection_ = playerDelta > 0.0F ? 1.0F : -1.0F;
                position_.x += std::clamp(playerDelta, -config_.moveSpeed * deltaSeconds,
                    config_.moveSpeed * deltaSeconds);
                position_.y = std::max(airY, position_.y - 100.0F * deltaSeconds);
                if (position_.y <= airY) stateRemaining_ = 0.0F;
            }
        }
        position_.x = std::clamp(position_.x, worldBounds.left, worldBounds.right - config_.width);
        stateRemaining_ -= deltaSeconds;
        if (stateRemaining_ <= 0.0F)
        {
            action_ = EnemyAction::Chase;
            cooldownRemaining_ = config_.cooldownSeconds;
        }
        break;
    case EnemyAction::Recovery:
        action_ = EnemyAction::Chase;
        break;
    case EnemyAction::Dead:
        break;
    }
}

void EnemyController::markDead() noexcept { action_ = EnemyAction::Dead; }
void EnemyController::interrupt() noexcept
{
    if (action_ == EnemyAction::Dead) return;
    action_ = EnemyAction::Chase;
    stateRemaining_ = 0.0F;
    activeElapsed_ = 0.0F;
    cooldownRemaining_ = config_.cooldownSeconds;
}
void EnemyController::translateHorizontal(const float distance, const WorldBounds& worldBounds) noexcept
{
    position_.x = std::clamp(position_.x + distance,
        worldBounds.left, worldBounds.right - config_.width);
}
void EnemyController::setPosition(const Vec2 position, const WorldBounds& worldBounds) noexcept
{
    position_ = position;
    position_.x = std::clamp(position_.x, worldBounds.left, worldBounds.right - config_.width);
    position_.y = std::min(position_.y, worldBounds.groundTop - config_.height);
}
void EnemyController::forceAttackToward(const float targetCenterX) noexcept
{
    if (action_ == EnemyAction::Dead) return;
    const float center = position_.x + config_.width * 0.5F;
    if (targetCenterX != center) facingDirection_ = targetCenterX > center ? 1.0F : -1.0F;
    action_ = EnemyAction::Active;
    stateRemaining_ = config_.activeSeconds;
    activeElapsed_ = 0.0F;
    ++attackSequence_;
}
Vec2 EnemyController::position() const noexcept { return position_; }
EnemyAction EnemyController::action() const noexcept { return action_; }
float EnemyController::facingDirection() const noexcept { return facingDirection_; }
std::uint64_t EnemyController::attackSequence() const noexcept { return attackSequence_; }
float EnemyController::activeProgress() const noexcept
{
    return config_.activeSeconds > 0.0F
        ? std::clamp(activeElapsed_ / config_.activeSeconds, 0.0F, 1.0F) : 0.0F;
}
bool EnemyController::isSwoopAscending() const noexcept
{
    return config_.skill == EnemySkill::Swoop && action_ == EnemyAction::Active
        && swoopAscending_;
}

Aabb EnemyController::attackBounds() const noexcept
{
    if (config_.skill == EnemySkill::Thrust || config_.skill == EnemySkill::Dive)
        return bounds();
    if (config_.skill == EnemySkill::LeapingCleave)
    {
        if (activeElapsed_ < 0.9F) return {};
        return {position_.x - config_.attackRange, groundTop_ - 24.0F,
            config_.width + config_.attackRange * 2.0F, 24.0F};
    }
    if (config_.skill == EnemySkill::Domination)
    {
        constexpr float EffectHeight = 180.0F;
        const float left = facingDirection_ > 0.0F
            ? position_.x + config_.width : position_.x - config_.attackRange;
        return {left, groundTop_ - EffectHeight, config_.attackRange, EffectHeight};
    }
    if (config_.skill == EnemySkill::Blood)
    {
        const float left = facingDirection_ > 0.0F
            ? position_.x + config_.width : position_.x - config_.attackRange;
        return {left, position_.y - 18.0F, config_.attackRange, 54.0F};
    }
    if (config_.skill == EnemySkill::KillingMagic)
    {
        const float left = facingDirection_ > 0.0F
            ? position_.x + config_.width : position_.x - config_.attackRange;
        constexpr float EffectHeight = 34.0F;
        constexpr float EffectVerticalOffset = -10.0F;
        return {left, position_.y + (config_.height - EffectHeight) * 0.5F
                + EffectVerticalOffset,
            config_.attackRange, EffectHeight};
    }
    if (config_.skill == EnemySkill::FogAttack)
    {
        const float left = facingDirection_ > 0.0F
            ? position_.x + config_.width : position_.x - config_.attackRange;
        constexpr float EffectHeight = 32.0F;
        return {left, position_.y + (config_.height - EffectHeight) * 0.5F,
            config_.attackRange, EffectHeight};
    }
    const float left = facingDirection_ > 0.0F
        ? position_.x + config_.width : position_.x - config_.attackRange;
    return {left, position_.y, config_.attackRange, config_.height};
}

Aabb EnemyController::bounds() const noexcept
{
    return {position_.x, position_.y, config_.width, config_.height};
}

const EnemyConfig& EnemyController::config() const noexcept { return config_; }

void EnemyController::beginWindup() noexcept
{
    action_ = EnemyAction::Windup;
    stateRemaining_ = config_.windupSeconds;
}
}
