#include "game/combat/CombatSession.hpp"

#include <algorithm>
#include <utility>

namespace arcane::game
{
CombatSession::CombatSession(CombatRequest request)
    : request_(std::move(request)),
      player_(request_.playerSpawn),
      playerHealth_(request_.playerMaximumHealth, request_.playerCurrentHealth),
      enemyPosition_(request_.enemySpawn),
      enemyHealth_(request_.enemyMaximumHealth, request_.enemyMaximumHealth)
{
    if (!playerHealth_.isAlive())
    {
        finish(CombatOutcome::Defeat);
    }
}

void CombatSession::update(const PlayerIntent& intent, const float deltaSeconds)
{
    if (deltaSeconds <= 0.0F)
    {
        return;
    }

    if (result_.has_value())
    {
        if (result_->outcome == CombatOutcome::Victory)
        {
            attack_.update(deltaSeconds);
            player_.update(intent, deltaSeconds, request_.worldBounds);
        }
        return;
    }

    attack_.update(deltaSeconds);
    contactDamageCooldownRemaining_ = std::max(0.0F, contactDamageCooldownRemaining_ - deltaSeconds);
    player_.update(intent, deltaSeconds, request_.worldBounds);

    if (intent.attackPressed)
    {
        attack_.tryStart();
    }

    if (attack_.isActive()
        && lastHitAttackSequence_ != attack_.sequence()
        && intersects(attackBounds(), enemyBounds()))
    {
        enemyHealth_.damage(AttackDamage);
        lastHitAttackSequence_ = attack_.sequence();
    }

    if (!enemyHealth_.isAlive())
    {
        finish(CombatOutcome::Victory);
        return;
    }

    if (contactDamageCooldownRemaining_ <= 0.0F && intersects(playerBounds(), enemyBounds()))
    {
        playerHealth_.damage(request_.enemyContactDamage);
        contactDamageCooldownRemaining_ = ContactDamageCooldown;
    }

    if (!playerHealth_.isAlive())
    {
        finish(CombatOutcome::Defeat);
    }
}

PlayerStateView CombatSession::playerState() const noexcept
{
    return {
        player_.position(),
        playerHealth_.current(),
        playerHealth_.maximum(),
        player_.isGrounded(),
        player_.facingDirection(),
        attack_.isActive(),
        attack_.cooldownRemaining()
    };
}

EnemyStateView CombatSession::enemyState() const noexcept
{
    return {
        enemyPosition_,
        enemyHealth_.current(),
        enemyHealth_.maximum(),
        enemyHealth_.isAlive()
    };
}

Aabb CombatSession::attackBounds() const noexcept
{
    const Vec2 position = player_.position();
    const float left = player_.facingDirection() > 0.0F
        ? position.x + PlayerController::Width
        : position.x - AttackRange;

    return {left, position.y + AttackVerticalOffset, AttackRange, AttackHeight};
}

const std::optional<CombatResult>& CombatSession::result() const noexcept
{
    return result_;
}

Aabb CombatSession::playerBounds() const noexcept
{
    const Vec2 position = player_.position();
    return {position.x, position.y, PlayerController::Width, PlayerController::Height};
}

Aabb CombatSession::enemyBounds() const noexcept
{
    return {enemyPosition_.x, enemyPosition_.y, EnemyWidth, EnemyHeight};
}

void CombatSession::finish(const CombatOutcome outcome) noexcept
{
    result_ = CombatResult {
        outcome,
        request_.encounterId,
        outcome == CombatOutcome::Victory ? 1 : 0,
        outcome == CombatOutcome::Victory ? request_.goldReward : 0,
        playerHealth_.current()
    };
}
}
