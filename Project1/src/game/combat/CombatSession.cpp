#include "game/combat/CombatSession.hpp"

#include <algorithm>
#include <cmath>
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

ai::EnemyConfig enemyConfigFor(const EnemyArchetype archetype)
{
    switch (archetype)
    {
    case EnemyArchetype::ChestMimic:
        return {0.0F, 76.0F, 72.0F, 0.50F, 0.15F, 3.35F, 90.0F};
    case EnemyArchetype::HeadlessKnight:
        return {180.0F, 62.0F, 72.0F, 0.35F, 0.12F, 2.53F, 0.0F};
    case EnemyArchetype::Boss:
        return {125.0F, 72.0F, 90.0F, 0.55F, 0.16F, 1.30F, 20.0F};
    }
    return {};
}
}

CombatSession::CombatSession(CombatRequest request)
    : request_(std::move(request)),
      player_(request_.playerSpawn),
      playerHealth_(request_.playerMaximumHealth, request_.playerCurrentHealth),
      enemy_(request_.enemySpawn, enemyConfigFor(request_.enemyArchetype)),
      enemyHealth_(request_.enemyMaximumHealth, request_.enemyMaximumHealth),
      spells_(request_.equippedSpellIds, request_.equippedUltimateSpellId),
      relics_(request_.relicIds)
{
    if (relics_.castsFlowerFieldOnCombatStart())
    {
        flowerFieldRemaining_ = 4.0F;
        flowerFieldCenterX_ = player_.position().x + PlayerController::Width * 0.5F;
    }
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
    relics_.update(deltaSeconds);
    blessingRemaining_ = std::max(0.0F, blessingRemaining_ - deltaSeconds);
    flowerFieldRemaining_ = std::max(0.0F, flowerFieldRemaining_ - deltaSeconds);
    contactDamageCooldownRemaining_ = std::max(0.0F, contactDamageCooldownRemaining_ - deltaSeconds);
    player_.update(intent, deltaSeconds, request_.worldBounds);
    const float enemyCenter = enemy_.position().x + EnemyWidth * 0.5F;
    enemySlowed_ = flowerFieldRemaining_ > 0.0F
        && std::abs(enemyCenter - flowerFieldCenterX_) <= 360.0F;
    enemy_.update(playerBounds(), deltaSeconds, request_.worldBounds, enemySlowed_ ? 0.5F : 1.0F);
    const float playerCenter = player_.position().x + PlayerController::Width * 0.5F;
    if (flowerFieldRemaining_ > 0.0F && std::abs(playerCenter - flowerFieldCenterX_) <= 360.0F)
    {
        flowerHealingAccumulator_ += 5.0F * deltaSeconds;
        const int healing = static_cast<int>(flowerHealingAccumulator_);
        if (healing > 0)
        {
            static_cast<void>(playerHealth_.heal(healing));
            flowerHealingAccumulator_ -= static_cast<float>(healing);
        }
    }

    if (intent.attackPressed && !player_.isStunned())
    {
        attack_.tryStart();
    }

    const auto applySpellCast = [this, playerCenter](const spells::SpellCastResult& cast,
        const DamageSource damageSource)
    {
        if (!cast.cast) return;
        const float outgoingMultiplier = relics_.outgoingDamageMultiplier()
            * (blessingRemaining_ > 0.0F ? 1.2F : 1.0F);
        if (cast.effect == spells::SpellEffect::FlowerField)
        {
            flowerFieldRemaining_ = 4.0F;
            flowerFieldCenterX_ = playerCenter;
            flowerHealingAccumulator_ = 0.0F;
        }
        else if (cast.effect == spells::SpellEffect::GoddessBlessing)
        {
            blessingRemaining_ = 8.0F;
        }
        else if (cast.effect == spells::SpellEffect::BloodMagic)
        {
            const int healthCost = std::max(1, (playerHealth_.current() + 9) / 10);
            static_cast<void>(playerDamageResolver_.resolve(playerHealth_,
                {DamageSource::Event, ++selfDamageSequence_, healthCost}));
            if (cast.hit)
                static_cast<void>(enemyDamageResolver_.resolve(enemyHealth_,
                    {damageSource, cast.sequence, playerHealth_.current() / 2,
                        outgoingMultiplier}));
        }
        else if (cast.hit)
            static_cast<void>(enemyDamageResolver_.resolve(enemyHealth_,
                {damageSource, cast.sequence, cast.damage, outgoingMultiplier}));
    };

    for (std::size_t slot = 0U; slot < intent.spellPressed.size(); ++slot)
    {
        if (!intent.spellPressed[slot] || player_.isStunned()) continue;
        const auto cast = spells_.tryCast(slot, player_.position(), player_.facingDirection(), enemyBounds());
        applySpellCast(cast, SpellDamageSources[slot]);
    }

    if (intent.ultimateSpellPressed && !player_.isStunned())
        applySpellCast(spells_.tryCastUltimate(player_.position(), player_.facingDirection(), enemyBounds()),
            DamageSource::PlayerUltimateSpell);

    if (attack_.isActive()
        && intersects(attackBounds(), enemyBounds()))
    {
        static_cast<void>(enemyDamageResolver_.resolve(enemyHealth_,
            {DamageSource::PlayerBasicAttack, attack_.sequence(), AttackDamage,
                relics_.outgoingDamageMultiplier() * (blessingRemaining_ > 0.0F ? 1.2F : 1.0F)}));
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
            {DamageSource::EnemyAttack, enemy_.attackSequence(), request_.enemyAttackDamage,
                1.0F, relics_.incomingDamageMultiplier()});
        if (damage.appliedDamage > 0 && blessingRemaining_ <= 0.0F)
            player_.applyHitReaction(enemy_.facingDirection() * KnockbackSpeed, request_.enemyControlSeconds);
    }
    if (contactDamageCooldownRemaining_ <= 0.0F && intersects(playerBounds(), enemyBounds()))
    {
        const auto damage = playerDamageResolver_.resolve(playerHealth_,
            {DamageSource::EnemyContact, ++contactDamageSequence_, request_.enemyContactDamage,
                1.0F, relics_.incomingDamageMultiplier()});
        if (damage.appliedDamage > 0 && blessingRemaining_ <= 0.0F)
            player_.applyHitReaction(enemy_.facingDirection() * KnockbackSpeed, HitStunSeconds);
        contactDamageCooldownRemaining_ = 1.0F;
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
        spells_.ultimateView(),
        player_.isStunned(),
        player_.stunRemaining(),
        blessingRemaining_,
        relics_.vulnerableRemaining(),
        flowerFieldRemaining_
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
        enemy_.action() == ai::EnemyAction::Active,
        enemySlowed_
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

bool CombatSession::equipSpell(const std::size_t slot, const std::optional<std::uint32_t> id) noexcept
{
    return spells_.equip(slot, id);
}

bool CombatSession::equipUltimateSpell(const std::optional<std::uint32_t> id) noexcept
{
    return spells_.equipUltimate(id);
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
