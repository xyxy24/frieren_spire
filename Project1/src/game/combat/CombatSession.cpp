#include "game/combat/CombatSession.hpp"

#include <algorithm>
#include <utility>

namespace arcane::game
{
namespace
{
constexpr std::array SpellDamageSources {
    DamageSource::PlayerSpell0,
    DamageSource::PlayerSpell1,
    DamageSource::PlayerSpell2
};
}

CombatSession::CombatSession(CombatRequest request)
    : request_(std::move(request)),
      player_(request_.playerSpawn),
      playerHealth_(request_.playerMaximumHealth, request_.playerCurrentHealth),
      enemy_(request_.enemySpawn),
      enemyHealth_(request_.enemyMaximumHealth, request_.enemyMaximumHealth),
      spells_(request_.equippedSpellIds)
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
    spells_.update(deltaSeconds);
    player_.update(intent, deltaSeconds, request_.worldBounds);
    enemy_.update(playerBounds(), deltaSeconds, request_.worldBounds);

    if (intent.attackPressed && !player_.isStunned())
    {
        attack_.tryStart();
    }

    for (std::size_t slot = 0U; slot < intent.spellPressed.size(); ++slot)
    {
        if (!intent.spellPressed[slot] || player_.isStunned()) continue;
        const auto cast = spells_.tryCast(slot, player_.position(), player_.facingDirection(), enemyBounds());
        if (cast.hit)
            static_cast<void>(enemyDamageResolver_.resolve(enemyHealth_,
                {SpellDamageSources[slot], cast.sequence, cast.damage}));
    }

    if (attack_.isActive()
        && intersects(attackBounds(), enemyBounds()))
    {
        static_cast<void>(enemyDamageResolver_.resolve(enemyHealth_,
            {DamageSource::PlayerBasicAttack, attack_.sequence(), AttackDamage}));
    }

    if (!enemyHealth_.isAlive())
    {
        enemy_.markDead();
        finish(CombatOutcome::Victory);
        return;
    }

    if (enemy_.action() == ai::EnemyAction::Active
        && intersects(playerBounds(), enemy_.attackBounds()))
    {
        const auto damage = playerDamageResolver_.resolve(playerHealth_,
            {DamageSource::EnemyAttack, enemy_.attackSequence(), request_.enemyContactDamage});
        if (damage.appliedDamage > 0)
            player_.applyHitReaction(enemy_.facingDirection() * KnockbackSpeed, HitStunSeconds);
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
        attack_.cooldownRemaining(),
        spells_.view(),
        player_.isStunned(),
        player_.stunRemaining()
    };
}

EnemyStateView CombatSession::enemyState() const noexcept
{
    return {
        enemy_.position(),
        enemyHealth_.current(),
        enemyHealth_.maximum(),
        enemyHealth_.isAlive(),
        enemy_.action() == ai::EnemyAction::Windup,
        enemy_.action() == ai::EnemyAction::Active
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
    const auto position = enemy_.position();
    return {position.x, position.y, EnemyWidth, EnemyHeight};
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
