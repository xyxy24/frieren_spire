#include "game/combat/CombatSession.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace arcane::game
{
namespace
{
constexpr std::array SpellDamageSources {
    DamageSource::PlayerSpell0, DamageSource::PlayerSpell1, DamageSource::PlayerSpell2
};
}

ai::EnemyConfig CombatSession::enemyConfigFor(const EnemyArchetype archetype)
{
    using ai::EnemyConfig;
    using ai::EnemySkill;
    switch (archetype)
    {
    case EnemyArchetype::ChestMimic:
        return EnemyConfig {0.0F, 64.0F, 64.0F, 0.5F, 64.0F / 520.0F, 0.0F, 64.0F,
            42.0F, 42.0F, 5.0F, true, false, EnemySkill::Thrust};
    case EnemyArchetype::HeadlessKnight:
        return EnemyConfig {240.0F, 42.0F, 42.0F, 0.5F, 1.0F, 0.0F, 0.0F,
            42.0F, 58.0F, 2.0F, true, false, EnemySkill::Slash};
    case EnemyArchetype::BirdDemon:
        return EnemyConfig {180.0F, 480.0F, 480.0F, 0.5F, 480.0F / 320.0F, 0.0F, 0.0F,
            42.0F, 32.0F, 6.0F, false, true, EnemySkill::Dive};
    case EnemyArchetype::Lugner:
        return EnemyConfig {160.0F, 72.0F, 72.0F, 0.5F, 1.0F, 0.0F, 0.0F,
            42.0F, 72.0F, 7.0F, false, false, EnemySkill::Blood};
    case EnemyArchetype::Linie:
        return EnemyConfig {180.0F, 72.0F, 72.0F, 0.5F, 0.9F, 0.0F, 0.0F,
            42.0F, 64.0F, 5.0F, true, false, EnemySkill::LeapingCleave};
    case EnemyArchetype::Draht:
        return EnemyConfig {160.0F, 72.0F, 72.0F, 0.5F, 1.0F, 0.0F, 0.0F,
            42.0F, 64.0F, 9.0F, false, false, EnemySkill::Thread};
    case EnemyArchetype::Aura:
        return EnemyConfig {120.0F, 96.0F, 96.0F, 0.5F, 1.0F, 0.0F, 0.0F,
            42.0F, 64.0F, 10.0F, false, false, EnemySkill::Domination};
    case EnemyArchetype::RedMirrorDragon:
        return EnemyConfig {96.0F, 72.0F, 72.0F, 0.5F, 1.0F, 0.0F, 0.0F,
            128.0F, 84.0F, 7.0F, true, false, EnemySkill::DragonClaw};
    case EnemyArchetype::Boss:
        return EnemyConfig {125.0F, 72.0F, 90.0F, 0.55F, 0.16F, 0.0F, 20.0F,
            48.0F, 64.0F, 1.3F, true, false, EnemySkill::BossAttack};
    }
    return {};
}

CombatSession::CombatSession(CombatRequest request)
    : request_(std::move(request)), player_(request_.playerSpawn),
      playerHealth_(request_.playerMaximumHealth, request_.playerCurrentHealth),
      spells_(request_.equippedSpellIds, request_.equippedUltimateSpellId), relics_(request_.relicIds)
{
    auto spawns = request_.enemies;
    if (spawns.empty()) spawns.push_back({request_.enemyArchetype, request_.enemySpawn});
    enemies_.reserve(spawns.size());
    for (const auto& spawn : spawns)
    {
        const auto config = enemyConfigFor(spawn.archetype);
        Vec2 position = spawn.position;
        if (!request_.enemies.empty() || spawn.archetype == EnemyArchetype::Aura
            || spawn.archetype == EnemyArchetype::RedMirrorDragon)
            position.y = request_.worldBounds.groundTop - config.height;
        if (config.flying) position.y = request_.worldBounds.groundTop - 132.0F - config.height;
        const int health = request_.enemies.empty() ? request_.enemyMaximumHealth : [&] {
            switch (spawn.archetype)
            {
            case EnemyArchetype::ChestMimic: return 75;
            case EnemyArchetype::HeadlessKnight: return 60;
            case EnemyArchetype::BirdDemon: return 45;
            case EnemyArchetype::Lugner: return 100;
            case EnemyArchetype::Linie: return 100;
            case EnemyArchetype::Draht: return 75;
            case EnemyArchetype::Aura: return 225;
            case EnemyArchetype::RedMirrorDragon: return 300;
            case EnemyArchetype::Boss: return request_.enemyMaximumHealth;
            }
            return 1;
        }();
        enemies_.push_back({spawn.archetype, ai::EnemyController(position, config), Health(health, health),
            {}, 0.0F, 0U, 0U, false, spawn.archetype == EnemyArchetype::Aura ? 12.0F : 0.0F, 0U});
        if (spawn.archetype == EnemyArchetype::RedMirrorDragon)
            enemies_.back().breathCooldown = 6.0F;
    }
    const auto aura = std::find_if(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
        return enemy.archetype == EnemyArchetype::Aura;
    });
    if (aura != enemies_.end())
    {
        const float auraX = aura->controller.position().x;
        for (const float offset : std::array {-110.0F, 110.0F})
        {
            const auto config = enemyConfigFor(EnemyArchetype::HeadlessKnight);
            const Vec2 position {std::clamp(auraX + offset, request_.worldBounds.left,
                request_.worldBounds.right - config.width), request_.worldBounds.groundTop - config.height};
            enemies_.push_back({EnemyArchetype::HeadlessKnight, ai::EnemyController(position, config),
                Health(60, 60)});
        }
    }
    if (relics_.castsFlowerFieldOnCombatStart())
    {
        flowerFieldRemaining_ = 4.0F;
        flowerFieldCenterX_ = player_.position().x + PlayerController::Width * 0.5F;
    }
    if (!playerHealth_.isAlive()) finish(CombatOutcome::Defeat);
}

void CombatSession::update(const PlayerIntent& intent, const float deltaSeconds)
{
    if (deltaSeconds <= 0.0F) return;
    if (result_)
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
    guardRemaining_ = std::max(0.0F, guardRemaining_ - deltaSeconds);
    guardCooldownRemaining_ = std::max(0.0F, guardCooldownRemaining_ - deltaSeconds);
    player_.update(intent, deltaSeconds, request_.worldBounds);
    if (intent.guardPressed && !player_.isStunned() && !player_.isDashing()
        && guardCooldownRemaining_ <= 0.0F)
    {
        guardRemaining_ = GuardDurationSeconds;
        guardCooldownRemaining_ = GuardCooldownSeconds;
    }
    const float playerCenter = player_.position().x + PlayerController::Width * 0.5F;
    const bool canAct = !player_.isStunned() && !player_.isDashing() && guardRemaining_ <= 0.0F;

    for (std::size_t enemyIndex = 0U; enemyIndex < enemies_.size(); ++enemyIndex)
    {
        auto& enemy = enemies_[enemyIndex];
        if (!enemy.health.isAlive()) continue;
        enemy.contactCooldown = std::max(0.0F, enemy.contactCooldown - deltaSeconds);
        const auto bounds = enemy.controller.bounds();
        enemy.slowed = flowerFieldRemaining_ > 0.0F
            && std::abs(bounds.left + bounds.width * 0.5F - flowerFieldCenterX_) <= 360.0F;
        if (enemy.archetype == EnemyArchetype::RedMirrorDragon)
        {
            enemy.breathCooldown = std::max(0.0F, enemy.breathCooldown - deltaSeconds);
            if (enemy.breathWindup > 0.0F)
            {
                enemy.breathWindup = std::max(0.0F, enemy.breathWindup - deltaSeconds);
                if (enemy.breathWindup <= 0.0F)
                {
                    enemy.breathRemaining = 3.0F;
                    enemy.breathTickAccumulator = 0.0F;
                }
                continue;
            }
            if (enemy.breathRemaining > 0.0F)
            {
                enemy.breathRemaining = std::max(0.0F, enemy.breathRemaining - deltaSeconds);
                enemy.breathTickAccumulator += deltaSeconds;
                const auto dragon = enemy.controller.bounds();
                const float left = enemy.controller.facingDirection() > 0.0F
                    ? dragon.left + dragon.width : dragon.left - 128.0F;
                const Aabb flames {left, request_.worldBounds.groundTop - 84.0F, 128.0F, 84.0F};
                while (enemy.breathTickAccumulator >= 1.0F)
                {
                    enemy.breathTickAccumulator -= 1.0F;
                    ++enemy.breathSequence;
                    const std::uint64_t sequence = (1ULL << 63U)
                        | (static_cast<std::uint64_t>(enemyIndex + 1U) << 32U)
                        | enemy.breathSequence;
                    if (intersects(playerBounds(), flames))
                        static_cast<void>(playerDamageResolver_.resolve(playerHealth_,
                            {DamageSource::EnemyAttack, sequence, 15, 1.0F,
                                relics_.incomingDamageMultiplier(), 0,
                                player_.isDashing() || guardRemaining_ > 0.0F}));
                }
                if (enemy.breathRemaining <= 0.0F) enemy.breathCooldown = 12.0F;
                continue;
            }
            if (enemy.breathCooldown <= 0.0F
                && enemy.controller.action() == ai::EnemyAction::Chase)
            {
                enemy.breathWindup = 1.0F;
                continue;
            }
        }
        enemy.controller.update(playerBounds(), deltaSeconds, request_.worldBounds, enemy.slowed ? 0.5F : 1.0F);
    }

    std::vector<Vec2> summonedKnights;
    for (auto& enemy : enemies_)
    {
        if (enemy.archetype != EnemyArchetype::Aura || !enemy.health.isAlive()) continue;
        enemy.summonCooldown = std::max(0.0F, enemy.summonCooldown - deltaSeconds);
        if (enemy.summonCooldown <= 0.0F && enemy.controller.action() == ai::EnemyAction::Chase)
        {
            const float auraX = enemy.controller.position().x;
            summonedKnights.push_back({auraX - 110.0F, 0.0F});
            summonedKnights.push_back({auraX + 110.0F, 0.0F});
            enemy.summonCooldown = 12.0F;
            ++enemy.summonCount;
        }
    }
    for (auto position : summonedKnights)
    {
        const auto config = enemyConfigFor(EnemyArchetype::HeadlessKnight);
        position.x = std::clamp(position.x, request_.worldBounds.left,
            request_.worldBounds.right - config.width);
        position.y = request_.worldBounds.groundTop - config.height;
        enemies_.push_back({EnemyArchetype::HeadlessKnight, ai::EnemyController(position, config),
            Health(60, 60)});
    }

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
    if (intent.attackPressed && canAct) attack_.tryStart();

    const auto damageEnemies = [this](const DamageSource source, const std::uint64_t sequence,
        const int damage, const float multiplier, const Aabb& area)
    {
        for (auto& enemy : enemies_)
            if (enemy.health.isAlive() && intersects(area, enemy.controller.bounds()))
                static_cast<void>(enemy.damageResolver.resolve(enemy.health,
                    {source, sequence, damage, multiplier}));
    };
    const auto castArea = [this](const spells::SpellDefinition& definition) {
        const auto position = player_.position();
        const float left = player_.facingDirection() > 0.0F
            ? position.x + PlayerController::Width : position.x - definition.range;
        return Aabb {left, position.y + (PlayerController::Height - definition.height) * 0.5F,
            definition.range, definition.height};
    };
    const auto applyCast = [&](const spells::SpellCastResult& cast, const DamageSource source,
        const spells::SpellDefinition* definition)
    {
        if (!cast.cast || !definition) return;
        const float multiplier = relics_.outgoingDamageMultiplier()
            * (blessingRemaining_ > 0.0F ? 1.2F : 1.0F);
        if (cast.effect == spells::SpellEffect::FlowerField)
        {
            flowerFieldRemaining_ = 4.0F;
            flowerFieldCenterX_ = playerCenter;
            flowerHealingAccumulator_ = 0.0F;
        }
        else if (cast.effect == spells::SpellEffect::GoddessBlessing) blessingRemaining_ = 8.0F;
        else if (cast.effect == spells::SpellEffect::BloodMagic)
        {
            const int cost = std::max(1, (playerHealth_.current() + 9) / 10);
            static_cast<void>(playerDamageResolver_.resolve(playerHealth_,
                {DamageSource::Event, ++selfDamageSequence_, cost}));
            damageEnemies(source, cast.sequence, playerHealth_.current() / 2, multiplier, castArea(*definition));
        }
        else damageEnemies(source, cast.sequence, cast.damage, multiplier, castArea(*definition));
    };
    for (std::size_t slot = 0U; slot < intent.spellPressed.size(); ++slot)
    {
        if (!intent.spellPressed[slot] || !canAct) continue;
        const auto id = spells_.view()[slot].id;
        applyCast(spells_.tryCast(slot, player_.position(), player_.facingDirection(), firstLivingEnemyBounds()),
            SpellDamageSources[slot], id ? spells::findDefinition(*id) : nullptr);
    }
    if (intent.ultimateSpellPressed && canAct)
    {
        const auto id = spells_.ultimateView().id;
        applyCast(spells_.tryCastUltimate(player_.position(), player_.facingDirection(), firstLivingEnemyBounds()),
            DamageSource::PlayerUltimateSpell, id ? spells::findDefinition(*id) : nullptr);
    }
    if (attack_.isActive())
        damageEnemies(DamageSource::PlayerBasicAttack, attack_.sequence(), AttackDamage,
            relics_.outgoingDamageMultiplier() * (blessingRemaining_ > 0.0F ? 1.2F : 1.0F), attackBounds());

    for (std::size_t enemyIndex = 0U; enemyIndex < enemies_.size(); ++enemyIndex)
    {
        auto& enemy = enemies_[enemyIndex];
        if (!enemy.health.isAlive())
        {
            enemy.controller.markDead();
            continue;
        }
        const auto& config = enemy.controller.config();
        if (enemy.controller.action() == ai::EnemyAction::Active
            && enemy.handledSkillSequence != enemy.controller.attackSequence())
        {
            enemy.handledSkillSequence = enemy.controller.attackSequence();
            if (config.skill == ai::EnemySkill::Blood)
                static_cast<void>(enemy.damageResolver.resolve(enemy.health,
                    {DamageSource::Event, enemy.handledSkillSequence, 10}));
        }
        if (enemy.controller.action() == ai::EnemyAction::Active
            && intersects(playerBounds(), enemy.controller.attackBounds()))
        {
            int damage = 20;
            if (config.skill == ai::EnemySkill::Thread) damage = 10;
            if (config.skill == ai::EnemySkill::Domination) damage = 0;
            if (config.skill == ai::EnemySkill::DragonClaw) damage = 25;
            if (config.skill == ai::EnemySkill::Blood)
                damage = (enemy.health.maximum() - enemy.health.current()) / 2;
            if (request_.enemies.empty() && enemy.archetype != EnemyArchetype::Aura
                && enemy.archetype != EnemyArchetype::RedMirrorDragon)
                damage = request_.enemyAttackDamage;
            const std::uint64_t skillSequence = (static_cast<std::uint64_t>(enemyIndex + 1U) << 32U)
                | enemy.controller.attackSequence();
            const auto result = playerDamageResolver_.resolve(playerHealth_,
                {DamageSource::EnemyAttack, skillSequence, damage,
                    1.0F, relics_.incomingDamageMultiplier(), 0,
                    player_.isDashing() || guardRemaining_ > 0.0F});
            const bool dominationHit = config.skill == ai::EnemySkill::Domination
                && !player_.isDashing() && guardRemaining_ <= 0.0F;
            if ((result.appliedDamage > 0 || dominationHit) && blessingRemaining_ <= 0.0F)
            {
                if (config.skill == ai::EnemySkill::Thread) player_.applyLaunch(540.0F, 0.28F);
                else if (config.skill == ai::EnemySkill::Domination)
                    player_.applyHitReaction(0.0F, 1.5F);
                else if (config.skill == ai::EnemySkill::Thrust)
                    player_.applyHitReaction(0.0F, 1.0F);
                else player_.applyHitReaction(enemy.controller.facingDirection() * KnockbackSpeed,
                    HitStunSeconds);
            }
        }
        if (config.hasContactDamage && enemy.contactCooldown <= 0.0F
            && intersects(playerBounds(), enemy.controller.bounds()))
        {
            const std::uint64_t contactSequence = (static_cast<std::uint64_t>(enemyIndex + 1U) << 32U)
                | ++enemy.contactSequence;
            const bool legacyDamage = request_.enemies.empty()
                && enemy.archetype != EnemyArchetype::Aura
                && enemy.archetype != EnemyArchetype::RedMirrorDragon;
            const int contactDamage = legacyDamage ? request_.enemyContactDamage : 15;
            static_cast<void>(playerDamageResolver_.resolve(playerHealth_,
                {DamageSource::EnemyContact, contactSequence, contactDamage,
                    1.0F, relics_.incomingDamageMultiplier(), 0,
                    player_.isDashing() || guardRemaining_ > 0.0F}));
            enemy.contactCooldown = 1.0F;
        }
    }
    if (std::none_of(enemies_.begin(), enemies_.end(), [](const auto& enemy) { return enemy.health.isAlive(); }))
        finish(CombatOutcome::Victory);
    else if (!playerHealth_.isAlive()) finish(CombatOutcome::Defeat);
}

PlayerStateView CombatSession::playerState() const noexcept
{
    return {player_.position(), playerHealth_.current(), playerHealth_.maximum(), player_.isGrounded(),
        player_.facingDirection(), attack_.isActive(), attack_.cooldownRemaining(), spells_.view(),
        spells_.ultimateView(), player_.dashRemaining(), player_.dashCooldownRemaining(),
        guardRemaining_ > 0.0F, guardRemaining_, guardCooldownRemaining_,
        player_.isStunned(), player_.stunRemaining(), blessingRemaining_,
        relics_.vulnerableRemaining(), flowerFieldRemaining_};
}

std::vector<EnemyStateView> CombatSession::enemyStates() const
{
    std::vector<EnemyStateView> views;
    views.reserve(enemies_.size());
    for (const auto& enemy : enemies_)
    {
        const auto bounds = enemy.controller.bounds();
        std::optional<Aabb> skillBounds;
        const auto skill = enemy.controller.config().skill;
        if ((enemy.controller.action() == ai::EnemyAction::Windup
                || enemy.controller.action() == ai::EnemyAction::Active)
            && skill != ai::EnemySkill::Thrust && skill != ai::EnemySkill::Dive
            && skill != ai::EnemySkill::Thread)
        {
            const auto area = enemy.controller.attackBounds();
            if (area.width > 0.0F && area.height > 0.0F) skillBounds = area;
        }
        if (enemy.archetype == EnemyArchetype::RedMirrorDragon
            && (enemy.breathWindup > 0.0F || enemy.breathRemaining > 0.0F))
        {
            const float left = enemy.controller.facingDirection() > 0.0F
                ? bounds.left + bounds.width : bounds.left - 128.0F;
            skillBounds = Aabb {left, request_.worldBounds.groundTop - 84.0F, 128.0F, 84.0F};
        }
        const bool windingUp = enemy.controller.action() == ai::EnemyAction::Windup
            || enemy.breathWindup > 0.0F;
        const bool active = enemy.controller.action() == ai::EnemyAction::Active
            || enemy.breathRemaining > 0.0F;
        views.push_back({enemy.archetype, enemy.controller.position(), bounds.width, bounds.height,
            enemy.health.current(), enemy.health.maximum(), enemy.health.isAlive(),
            windingUp, active, enemy.slowed, skillBounds});
    }
    return views;
}

EnemyStateView CombatSession::enemyState() const noexcept
{
    if (enemies_.empty()) return {};
    const auto& enemy = enemies_.front();
    const auto bounds = enemy.controller.bounds();
    std::optional<Aabb> skillBounds;
    const auto skill = enemy.controller.config().skill;
    if ((enemy.controller.action() == ai::EnemyAction::Windup
            || enemy.controller.action() == ai::EnemyAction::Active)
        && skill != ai::EnemySkill::Thrust && skill != ai::EnemySkill::Dive
        && skill != ai::EnemySkill::Thread)
    {
        const auto area = enemy.controller.attackBounds();
        if (area.width > 0.0F && area.height > 0.0F) skillBounds = area;
    }
    if (enemy.archetype == EnemyArchetype::RedMirrorDragon
        && (enemy.breathWindup > 0.0F || enemy.breathRemaining > 0.0F))
    {
        const float left = enemy.controller.facingDirection() > 0.0F
            ? bounds.left + bounds.width : bounds.left - 128.0F;
        skillBounds = Aabb {left, request_.worldBounds.groundTop - 84.0F, 128.0F, 84.0F};
    }
    return {enemy.archetype, enemy.controller.position(), bounds.width, bounds.height,
        enemy.health.current(), enemy.health.maximum(), enemy.health.isAlive(),
        enemy.controller.action() == ai::EnemyAction::Windup || enemy.breathWindup > 0.0F,
        enemy.controller.action() == ai::EnemyAction::Active || enemy.breathRemaining > 0.0F,
        enemy.slowed, skillBounds};
}

Aabb CombatSession::attackBounds() const noexcept
{
    const auto position = player_.position();
    const float left = player_.facingDirection() > 0.0F
        ? position.x + PlayerController::Width : position.x - AttackRange;
    return {left, position.y + AttackVerticalOffset, AttackRange, AttackHeight};
}

const std::optional<CombatResult>& CombatSession::result() const noexcept { return result_; }
bool CombatSession::equipSpell(const std::size_t slot, const std::optional<std::uint32_t> id) noexcept
{ return spells_.equip(slot, id); }
bool CombatSession::equipUltimateSpell(const std::optional<std::uint32_t> id) noexcept
{ return spells_.equipUltimate(id); }
Aabb CombatSession::playerBounds() const noexcept
{ const auto p = player_.position(); return {p.x, p.y, PlayerController::Width, PlayerController::Height}; }
Aabb CombatSession::firstLivingEnemyBounds() const noexcept
{
    const auto found = std::find_if(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
        return enemy.health.isAlive();
    });
    return found == enemies_.end() ? Aabb {} : found->controller.bounds();
}
void CombatSession::finish(const CombatOutcome outcome) noexcept
{
    result_ = {outcome, request_.encounterId,
        outcome == CombatOutcome::Victory ? static_cast<int>(enemies_.size()) : 0,
        outcome == CombatOutcome::Victory ? request_.goldReward : 0, playerHealth_.current()};
}
}
