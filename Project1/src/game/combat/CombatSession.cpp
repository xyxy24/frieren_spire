#include "game/combat/CombatSession.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
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
        return EnemyConfig {0.0F, 72.0F, 72.0F, 0.5F, 72.0F / 520.0F, 0.0F, 72.0F,
            42.0F, 42.0F, 5.0F, true, false, EnemySkill::Thrust};
    case EnemyArchetype::HeadlessKnight:
        return EnemyConfig {240.0F, 42.0F, 42.0F, 0.5F, 1.0F, 0.0F, 0.0F,
            42.0F, 58.0F, 2.0F, true, false, EnemySkill::Slash};
    case EnemyArchetype::BirdDemon:
        return EnemyConfig {180.0F, 480.0F, 480.0F, 0.5F, 480.0F / 320.0F, 0.0F, 0.0F,
            42.0F, 32.0F, 6.0F, false, true, EnemySkill::Dive};
    case EnemyArchetype::Lugner:
        return EnemyConfig {160.0F, 84.0F, 84.0F, 0.5F, 1.0F, 0.0F, 0.0F,
            42.0F, 72.0F, 6.0F, false, false, EnemySkill::Blood};
    case EnemyArchetype::Linie:
        return EnemyConfig {180.0F, 72.0F, 72.0F, 0.5F, 0.9F, 0.0F, 0.0F,
            42.0F, 64.0F, 4.0F, true, false, EnemySkill::LeapingCleave};
    case EnemyArchetype::Draht:
        return EnemyConfig {160.0F, 96.0F, 96.0F, 0.5F, 1.0F, 0.0F, 0.0F,
            42.0F, 64.0F, 8.0F, false, false, EnemySkill::Thread};
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
            case EnemyArchetype::HeadlessKnight: return 75;
            case EnemyArchetype::BirdDemon: return 45;
            case EnemyArchetype::Lugner: return 100;
            case EnemyArchetype::Linie: return 120;
            case EnemyArchetype::Draht: return 80;
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
                Health(75, 75)});
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
    acidFieldRemaining_ = std::max(0.0F, acidFieldRemaining_ - deltaSeconds);
    phantomRemaining_ = std::max(0.0F, phantomRemaining_ - deltaSeconds);
    cleanseProtectionRemaining_ = std::max(0.0F, cleanseProtectionRemaining_ - deltaSeconds);
    for (auto& effect : activeSpellEffects_)
        effect.remaining = std::max(0.0F, effect.remaining - deltaSeconds);
    std::erase_if(activeSpellEffects_, [](const auto& effect) { return effect.remaining <= 0.0F; });
    for (auto& impact : pendingSpellImpacts_)
    {
        impact.delayRemaining -= deltaSeconds;
        if (impact.delayRemaining > 0.0F) continue;
        for (auto& enemy : enemies_)
            if (enemy.health.isAlive() && intersects(impact.bounds, enemy.controller.bounds()))
                static_cast<void>(enemy.damageResolver.resolve(enemy.health,
                    {impact.source, impact.sequence, impact.damage, impact.multiplier
                        * (enemy.exposedRemaining > 0.0F ? 1.25F : 1.0F)
                        * (enemy.markedRemaining > 0.0F ? 1.15F : 1.0F)}));
    }
    std::erase_if(pendingSpellImpacts_, [](const auto& impact) {
        return impact.delayRemaining <= 0.0F;
    });
    guardRemaining_ = std::max(0.0F, guardRemaining_ - deltaSeconds);
    guardCooldownRemaining_ = std::max(0.0F, guardCooldownRemaining_ - deltaSeconds);
    const bool wasDashing = player_.isDashing();
    const Vec2 beforePlayerUpdate = player_.position();
    PlayerIntent playerIntent = intent;
    if (beamRemaining_ > 0.0F)
    {
        playerIntent.moveAxis *= 0.3F;
        playerIntent.jumpPressed = false;
        playerIntent.dashPressed = false;
    }
    player_.update(playerIntent, deltaSeconds, request_.worldBounds);
    if (intent.dashPressed && !wasDashing && player_.isDashing())
    {
        const float left = player_.facingDirection() > 0.0F
            ? beforePlayerUpdate.x : beforePlayerUpdate.x - PlayerController::DashDistance;
        activeSpellEffects_.push_back({1010U,
            {left, beforePlayerUpdate.y, PlayerController::DashDistance, PlayerController::Height},
            PlayerController::DashDurationSeconds, PlayerController::DashDurationSeconds});
    }
    if (teaChannelRemaining_ > 0.0F)
    {
        const float previous = teaChannelRemaining_;
        teaChannelRemaining_ = std::max(0.0F, teaChannelRemaining_ - deltaSeconds);
        if (std::abs(player_.position().x - teaStartX_) > 8.0F || player_.isStunned())
            teaChannelRemaining_ = 0.0F;
        else if (previous > 0.0F && teaChannelRemaining_ <= 0.0F)
            static_cast<void>(playerHealth_.heal(18));
    }
    if (intent.guardPressed && !player_.isStunned() && !player_.isDashing()
        && guardCooldownRemaining_ <= 0.0F)
    {
        guardRemaining_ = GuardDurationSeconds;
        guardCooldownRemaining_ = GuardCooldownSeconds;
        activeSpellEffects_.push_back({1007U, playerBounds(),
            GuardDurationSeconds, GuardDurationSeconds});
    }
    const float playerCenter = player_.position().x + PlayerController::Width * 0.5F;
    const bool canAct = !player_.isStunned() && !player_.isDashing()
        && guardRemaining_ <= 0.0F && beamRemaining_ <= 0.0F;

    for (std::size_t enemyIndex = 0U; enemyIndex < enemies_.size(); ++enemyIndex)
    {
        auto& enemy = enemies_[enemyIndex];
        if (!enemy.health.isAlive()) continue;
        enemy.frostSlowRemaining = std::max(0.0F, enemy.frostSlowRemaining - deltaSeconds);
        enemy.controlRemaining = std::max(0.0F, enemy.controlRemaining - deltaSeconds);
        enemy.exposedRemaining = std::max(0.0F, enemy.exposedRemaining - deltaSeconds);
        enemy.markedRemaining = std::max(0.0F, enemy.markedRemaining - deltaSeconds);
        if (acidFieldRemaining_ > 0.0F && intersects(acidFieldBounds_, enemy.controller.bounds()))
            enemy.exposedRemaining = std::max(enemy.exposedRemaining, 0.1F);
        if (enemy.burnRemaining > 0.0F)
        {
            const float activeDelta = std::min(deltaSeconds, enemy.burnRemaining);
            enemy.burnRemaining -= activeDelta;
            enemy.burnTickAccumulator += activeDelta;
            while (enemy.burnTickAccumulator >= 1.0F && enemy.health.isAlive())
            {
                enemy.burnTickAccumulator -= 1.0F;
                ++enemy.burnTick;
                static_cast<void>(enemy.damageResolver.resolve(enemy.health,
                    {enemy.burnSource, enemy.burnSequenceBase + enemy.burnTick, 4}));
            }
        }
        if (!enemy.health.isAlive()) continue;
        enemy.contactCooldown = std::max(0.0F, enemy.contactCooldown - deltaSeconds);
        const auto bounds = enemy.controller.bounds();
        const bool flowerSlowed = flowerFieldRemaining_ > 0.0F
            && std::abs(bounds.left + bounds.width * 0.5F - flowerFieldCenterX_) <= 360.0F;
        enemy.slowed = flowerSlowed || enemy.frostSlowRemaining > 0.0F;
        if (enemy.archetype == EnemyArchetype::RedMirrorDragon)
        {
            enemy.breathCooldown = std::max(0.0F, enemy.breathCooldown - deltaSeconds);
            if (enemy.breathWindup > 0.0F)
            {
                enemy.breathWindup = std::max(0.0F, enemy.breathWindup - deltaSeconds);
                if (enemy.breathWindup <= 0.0F)
                {
                    enemy.breathRemaining = 1.5F;
                    enemy.breathTickAccumulator = 0.0F;
                }
                continue;
            }
            if (enemy.breathRemaining > 0.0F)
            {
                const float activeDelta = std::min(deltaSeconds, enemy.breathRemaining);
                enemy.breathRemaining -= activeDelta;
                enemy.breathTickAccumulator += activeDelta;
                const auto dragon = enemy.controller.bounds();
                const float left = enemy.controller.facingDirection() > 0.0F
                    ? dragon.left + dragon.width : dragon.left - 160.0F;
                const Aabb flames {left, request_.worldBounds.groundTop - 64.0F, 160.0F, 64.0F};
                while (enemy.breathTickAccumulator >= 0.5F)
                {
                    enemy.breathTickAccumulator -= 0.5F;
                    ++enemy.breathSequence;
                    const std::uint64_t sequence = (1ULL << 63U)
                        | (static_cast<std::uint64_t>(enemyIndex + 1U) << 32U)
                        | enemy.breathSequence;
                    if (intersects(playerBounds(), flames))
                        static_cast<void>(playerDamageResolver_.resolve(playerHealth_,
                            {DamageSource::EnemyAttack, sequence, 15, 1.0F,
                                relics_.incomingDamageMultiplier()
                                    * (cleanseProtectionRemaining_ > 0.0F ? 0.8F : 1.0F), 0,
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
        if (enemy.controlRemaining <= 0.0F)
        {
            const Aabb target = phantomRemaining_ > 0.0F ? phantomBounds_ : playerBounds();
            const float speedMultiplier = flowerSlowed ? 0.5F
                : (enemy.frostSlowRemaining > 0.0F ? 0.6F : 1.0F);
            enemy.controller.update(target, deltaSeconds, request_.worldBounds, speedMultiplier);
        }
        if (phantomRemaining_ > 0.0F && enemy.controller.action() == ai::EnemyAction::Active
            && intersects(phantomBounds_, enemy.controller.attackBounds()))
            phantomRemaining_ = 0.0F;
    }

    if (hellfireRemaining_ > 0.0F)
    {
        const float activeDelta = std::min(deltaSeconds, hellfireRemaining_);
        hellfireRemaining_ -= activeDelta;
        hellfireTickAccumulator_ += activeDelta;
        const float fieldCenter = hellfireBounds_.left + hellfireBounds_.width * 0.5F;
        for (auto& enemy : enemies_)
            if (enemy.health.isAlive() && intersects(hellfireBounds_, enemy.controller.bounds()))
            {
                const auto bounds = enemy.controller.bounds();
                const float enemyCenter = bounds.left + bounds.width * 0.5F;
                enemy.controller.translateHorizontal((fieldCenter > enemyCenter ? 1.0F : -1.0F)
                    * 24.0F * activeDelta, request_.worldBounds);
            }
        while (hellfireTickAccumulator_ >= 0.5F)
        {
            hellfireTickAccumulator_ -= 0.5F;
            ++hellfireTick_;
            const int tickDamage = hellfireTick_ <= 4U ? 15 : 14;
            for (auto& enemy : enemies_)
                if (enemy.health.isAlive() && intersects(hellfireBounds_, enemy.controller.bounds()))
                    static_cast<void>(enemy.damageResolver.resolve(enemy.health,
                        {DamageSource::PlayerUltimateSpell, hellfireSequenceBase_ + hellfireTick_,
                            tickDamage, hellfireMultiplier_}));
        }
    }
    if (beamRemaining_ > 0.0F)
    {
        const float activeDelta = std::min(deltaSeconds, beamRemaining_);
        beamRemaining_ -= activeDelta;
        beamTickAccumulator_ += activeDelta;
        while (beamTickAccumulator_ >= 0.25F)
        {
            beamTickAccumulator_ -= 0.25F;
            ++beamTick_;
            for (auto& enemy : enemies_)
                if (enemy.health.isAlive() && intersects(beamBounds_, enemy.controller.bounds()))
                    static_cast<void>(enemy.damageResolver.resolve(enemy.health,
                        {DamageSource::PlayerUltimateSpell, beamSequenceBase_ + beamTick_,
                            15, beamMultiplier_}));
        }
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
            Health(75, 75)});
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
                    {source, sequence, damage, multiplier
                        * (enemy.exposedRemaining > 0.0F ? 1.25F : 1.0F)
                        * (enemy.markedRemaining > 0.0F ? 1.15F : 1.0F)}));
    };
    const auto applyCast = [&](const spells::SpellCastResult& cast, const DamageSource source,
        const spells::SpellDefinition* definition)
    {
        if (!cast.cast || !definition) return;
        const auto visualDuration = [](const spells::SpellEffect effect) {
            switch (effect)
            {
            case spells::SpellEffect::FlowerField: return 4.0F;
            case spells::SpellEffect::GoddessBlessing: return 6.0F;
            case spells::SpellEffect::FlameBurst: return 3.0F;
            case spells::SpellEffect::Phantom: return 3.0F;
            case spells::SpellEffect::SourGrape: return 5.0F;
            case spells::SpellEffect::HotTea: return 1.0F;
            case spells::SpellEffect::ManaTrace: return 6.0F;
            case spells::SpellEffect::GoddessSpears: return 0.6F;
            case spells::SpellEffect::DestructionLightning: return 0.8F;
            case spells::SpellEffect::HellfireStorm: return 3.0F;
            case spells::SpellEffect::JudgmentBeam: return 2.0F;
            case spells::SpellEffect::EarthPillars: return 3.0F;
            case spells::SpellEffect::MirrorArray: return 8.0F;
            default: return 0.35F;
            }
        };
        const float duration = visualDuration(cast.effect);
        activeSpellEffects_.push_back({cast.spellId, cast.effectBounds, duration, duration});
        const float multiplier = relics_.outgoingDamageMultiplier()
            * (blessingRemaining_ > 0.0F ? 1.2F : 1.0F);
        if (cast.effect == spells::SpellEffect::FlowerField)
        {
            flowerFieldRemaining_ = 4.0F;
            flowerFieldCenterX_ = playerCenter;
            flowerHealingAccumulator_ = 0.0F;
        }
        else if (cast.effect == spells::SpellEffect::GoddessBlessing) blessingRemaining_ = 6.0F;
        else if (cast.effect == spells::SpellEffect::BloodMagic)
        {
            const int cost = std::max(1, (playerHealth_.current() + 9) / 10);
            static_cast<void>(playerDamageResolver_.resolve(playerHealth_,
                {DamageSource::Event, ++selfDamageSequence_, cost}));
            damageEnemies(source, cast.sequence, playerHealth_.current() / 2, multiplier, cast.effectBounds);
        }
        else if (cast.effect == spells::SpellEffect::FrostLance)
        {
            damageEnemies(source, cast.sequence, cast.damage, multiplier, cast.effectBounds);
            for (auto& enemy : enemies_)
                if (enemy.health.isAlive() && intersects(cast.effectBounds, enemy.controller.bounds()))
                    enemy.frostSlowRemaining = 2.5F;
        }
        else if (cast.effect == spells::SpellEffect::FlameBurst)
        {
            damageEnemies(source, cast.sequence, cast.damage, multiplier, cast.effectBounds);
            for (auto& enemy : enemies_)
                if (enemy.health.isAlive() && intersects(cast.effectBounds, enemy.controller.bounds()))
                {
                    enemy.burnRemaining = 3.0F;
                    enemy.burnTickAccumulator = 0.0F;
                    enemy.burnSequenceBase = cast.sequence * 16U;
                    enemy.burnTick = 0U;
                    enemy.burnSource = source;
                }
        }
        else if (cast.effect == spells::SpellEffect::MagicThread)
        {
            damageEnemies(source, cast.sequence, cast.damage, multiplier, cast.effectBounds);
            for (auto& enemy : enemies_)
                if (enemy.health.isAlive() && intersects(cast.effectBounds, enemy.controller.bounds()))
                {
                    const float direction = playerCenter < enemy.controller.position().x ? -1.0F : 1.0F;
                    enemy.controller.translateHorizontal(direction * 126.0F, request_.worldBounds);
                    enemy.controlRemaining = enemy.archetype == EnemyArchetype::Boss ? 0.4F : 1.2F;
                }
        }
        else if (cast.effect == spells::SpellEffect::StoneShot)
        {
            damageEnemies(source, cast.sequence, cast.damage, multiplier, cast.effectBounds);
            for (auto& enemy : enemies_)
                if (enemy.health.isAlive() && intersects(cast.effectBounds, enemy.controller.bounds()))
                    enemy.controller.translateHorizontal(player_.facingDirection() * 80.0F,
                        request_.worldBounds);
        }
        else if (cast.effect == spells::SpellEffect::Phantom)
        {
            phantomRemaining_ = 3.0F;
            phantomBounds_ = {player_.position().x, player_.position().y,
                PlayerController::Width, PlayerController::Height};
            activeSpellEffects_.back().bounds = phantomBounds_;
        }
        else if (cast.effect == spells::SpellEffect::SourGrape)
        {
            acidFieldRemaining_ = 5.0F;
            acidFieldBounds_ = cast.effectBounds;
        }
        else if (cast.effect == spells::SpellEffect::Cleaning)
        {
            static_cast<void>(playerHealth_.heal(10));
            cleanseProtectionRemaining_ = 1.0F;
        }
        else if (cast.effect == spells::SpellEffect::HotTea)
        {
            teaChannelRemaining_ = 1.0F;
            teaStartX_ = player_.position().x;
        }
        else if (cast.effect == spells::SpellEffect::BirdCapture)
        {
            const auto distanceToPlayer = [playerCenter](const auto& enemy) {
                if (!enemy.health.isAlive()) return std::numeric_limits<float>::max();
                const auto bounds = enemy.controller.bounds();
                return std::abs(bounds.left + bounds.width * 0.5F - playerCenter);
            };
            auto found = std::min_element(enemies_.begin(), enemies_.end(), [&](const auto& left,
                const auto& right) { return distanceToPlayer(left) < distanceToPlayer(right); });
            if (found != enemies_.end()
                && !intersects(cast.effectBounds, found->controller.bounds())) found = enemies_.end();
            if (found != enemies_.end())
            {
                static_cast<void>(found->damageResolver.resolve(found->health,
                    {source, cast.sequence, cast.damage, multiplier}));
                const bool boss = found->archetype == EnemyArchetype::Aura
                    || found->archetype == EnemyArchetype::RedMirrorDragon
                    || found->archetype == EnemyArchetype::Boss;
                found->controlRemaining = boss ? 0.7F : 2.5F;
            }
        }
        else if (cast.effect == spells::SpellEffect::ManaTrace)
        {
            const auto distanceToPlayer = [playerCenter](const auto& enemy) {
                if (!enemy.health.isAlive()) return std::numeric_limits<float>::max();
                const auto bounds = enemy.controller.bounds();
                return std::abs(bounds.left + bounds.width * 0.5F - playerCenter);
            };
            auto found = std::min_element(enemies_.begin(), enemies_.end(), [&](const auto& left,
                const auto& right) { return distanceToPlayer(left) < distanceToPlayer(right); });
            if (found != enemies_.end()
                && !intersects(cast.effectBounds, found->controller.bounds())) found = enemies_.end();
            if (found != enemies_.end()) found->markedRemaining = 6.0F;
        }
        else if (cast.effect == spells::SpellEffect::BossZoltraak)
        {
            for (auto& enemy : enemies_)
            {
                if (!enemy.health.isAlive() || !intersects(cast.effectBounds, enemy.controller.bounds()))
                    continue;
                const bool demon = enemy.archetype == EnemyArchetype::Lugner
                    || enemy.archetype == EnemyArchetype::Linie
                    || enemy.archetype == EnemyArchetype::Draht
                    || enemy.archetype == EnemyArchetype::Aura;
                static_cast<void>(enemy.damageResolver.resolve(enemy.health,
                    {source, cast.sequence, cast.damage, multiplier * (demon ? 1.3F : 1.0F)}));
            }
        }
        else if (cast.effect == spells::SpellEffect::GoddessSpears)
        {
            bool fullHit = false;
            for (auto& enemy : enemies_)
            {
                if (!enemy.health.isAlive() || !intersects(cast.effectBounds, enemy.controller.bounds()))
                    continue;
                std::uint32_t landedSpears = 0U;
                for (std::uint64_t spear = 1U; spear <= 3U && enemy.health.isAlive(); ++spear)
                {
                    static_cast<void>(enemy.damageResolver.resolve(enemy.health,
                        {source, cast.sequence * 4U + spear, cast.damage, multiplier}));
                    ++landedSpears;
                }
                fullHit = fullHit || landedSpears == 3U;
            }
            if (fullHit) static_cast<void>(playerHealth_.heal(12));
        }
        else if (cast.effect == spells::SpellEffect::SeveringSlash)
        {
            for (auto& enemy : enemies_)
                if (enemy.health.isAlive() && intersects(cast.effectBounds, enemy.controller.bounds()))
                {
                    const bool executeRange = enemy.health.current() * 5 <= enemy.health.maximum();
                    static_cast<void>(enemy.damageResolver.resolve(enemy.health,
                        {source, cast.sequence, cast.damage,
                            multiplier * (executeRange ? 1.3F : 1.0F)}));
                }
        }
        else if (cast.effect == spells::SpellEffect::Mimic)
        {
            const auto found = std::find_if(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
                return enemy.health.isAlive();
            });
            if (found != enemies_.end())
            {
                int copiedDamage = 20;
                switch (found->controller.config().skill)
                {
                case ai::EnemySkill::Thread: copiedDamage = 10; break;
                case ai::EnemySkill::DragonClaw: copiedDamage = 25; break;
                case ai::EnemySkill::LeapingCleave: copiedDamage = 25; break;
                case ai::EnemySkill::Blood: copiedDamage = 30; break;
                case ai::EnemySkill::Domination: copiedDamage = 20; break;
                default: break;
                }
                damageEnemies(source, cast.sequence, static_cast<int>(std::lround(copiedDamage * 1.2F)),
                    multiplier, cast.effectBounds);
            }
        }
        else if (cast.effect == spells::SpellEffect::DestructionLightning)
        {
            Aabb impact = cast.effectBounds;
            const bool hitsMarked = std::any_of(enemies_.begin(), enemies_.end(), [&](const auto& enemy) {
                return enemy.health.isAlive() && enemy.markedRemaining > 0.0F
                    && intersects(impact, enemy.controller.bounds());
            });
            if (hitsMarked)
            {
                const float growX = impact.width * 0.2F;
                const float growY = impact.height * 0.2F;
                impact = {impact.left - growX, impact.top - growY,
                    impact.width * 1.4F, impact.height * 1.4F};
                activeSpellEffects_.back().bounds = impact;
            }
            pendingSpellImpacts_.push_back({cast.spellId, impact, 0.8F, cast.damage,
                cast.sequence, source, multiplier});
        }
        else if (cast.effect == spells::SpellEffect::HellfireStorm)
        {
            hellfireRemaining_ = 3.0F;
            hellfireTickAccumulator_ = 0.0F;
            hellfireBounds_ = cast.effectBounds;
            hellfireSequenceBase_ = cast.sequence * 16U;
            hellfireTick_ = 0U;
            hellfireMultiplier_ = multiplier;
        }
        else if (cast.effect == spells::SpellEffect::JudgmentBeam)
        {
            beamRemaining_ = 2.0F;
            beamTickAccumulator_ = 0.0F;
            beamBounds_ = cast.effectBounds;
            beamSequenceBase_ = cast.sequence * 16U;
            beamTick_ = 0U;
            beamMultiplier_ = multiplier;
        }
        else if (cast.effect == spells::SpellEffect::EarthPillars)
        {
            const float center = cast.effectBounds.left + cast.effectBounds.width * 0.5F;
            std::vector<std::uint32_t> hitsPerEnemy(enemies_.size(), 0U);
            for (std::uint64_t pillarIndex = 0U; pillarIndex < 3U; ++pillarIndex)
            {
                const Aabb pillar {center - 88.0F + static_cast<float>(pillarIndex) * 64.0F,
                    request_.worldBounds.groundTop - 160.0F, 48.0F, 160.0F};
                activeSpellEffects_.push_back({cast.spellId, pillar, 3.0F, 3.0F});
                for (std::size_t enemyIndex = 0U; enemyIndex < enemies_.size(); ++enemyIndex)
                {
                    auto& enemy = enemies_[enemyIndex];
                    if (enemy.health.isAlive() && intersects(pillar, enemy.controller.bounds()))
                    {
                        static_cast<void>(enemy.damageResolver.resolve(enemy.health,
                            {source, cast.sequence * 4U + pillarIndex + 1U,
                                hitsPerEnemy[enemyIndex] == 0U ? 35 : 25, multiplier}));
                        ++hitsPerEnemy[enemyIndex];
                    }
                }
            }
        }
        else if (cast.effect == spells::SpellEffect::MirrorArray) mirrorCopies_ = 2U;
        else damageEnemies(source, cast.sequence, cast.damage, multiplier, cast.effectBounds);
    };
    for (std::size_t slot = 0U; slot < intent.spellPressed.size(); ++slot)
    {
        if (!intent.spellPressed[slot] || !canAct) continue;
        const auto id = spells_.view()[slot].id;
        const auto cast = spells_.tryCast(slot, player_.position(), player_.facingDirection(),
            firstLivingEnemyBounds());
        applyCast(cast, SpellDamageSources[slot], id ? spells::findDefinition(*id) : nullptr);
        int mirrorBaseDamage = cast.damage;
        if (cast.effect == spells::SpellEffect::BloodMagic && cast.cast)
            mirrorBaseDamage = playerHealth_.current() / 2;
        if (cast.cast && mirrorCopies_ > 0U && mirrorBaseDamage > 0)
        {
            const float mirrorMultiplier = relics_.outgoingDamageMultiplier()
                * (blessingRemaining_ > 0.0F ? 1.2F : 1.0F);
            for (std::uint32_t copy = 0U; copy < mirrorCopies_; ++copy)
                damageEnemies(SpellDamageSources[slot], cast.sequence * 16U + copy + 8U,
                    static_cast<int>(std::lround(mirrorBaseDamage * 0.35F)),
                    mirrorMultiplier, cast.effectBounds);
            std::erase_if(activeSpellEffects_, [](const auto& effect) {
                return effect.spellId == 2012U;
            });
            activeSpellEffects_.push_back({2012U, cast.effectBounds, 0.5F, 0.5F});
            mirrorCopies_ = 0U;
        }
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
                    1.0F, relics_.incomingDamageMultiplier()
                        * (cleanseProtectionRemaining_ > 0.0F ? 0.8F : 1.0F), 0,
                    player_.isDashing() || guardRemaining_ > 0.0F});
            if (result.appliedDamage > 0) beamRemaining_ = 0.0F;
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
            const auto contactResult = playerDamageResolver_.resolve(playerHealth_,
                {DamageSource::EnemyContact, contactSequence, contactDamage,
                    1.0F, relics_.incomingDamageMultiplier()
                        * (cleanseProtectionRemaining_ > 0.0F ? 0.8F : 1.0F), 0,
                    player_.isDashing() || guardRemaining_ > 0.0F});
            if (contactResult.appliedDamage > 0) beamRemaining_ = 0.0F;
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
            && skill != ai::EnemySkill::Thread && skill != ai::EnemySkill::Domination)
        {
            const auto area = enemy.controller.attackBounds();
            if (area.width > 0.0F && area.height > 0.0F) skillBounds = area;
        }
        if (enemy.archetype == EnemyArchetype::RedMirrorDragon
            && (enemy.breathWindup > 0.0F || enemy.breathRemaining > 0.0F))
        {
            const float left = enemy.controller.facingDirection() > 0.0F
                ? bounds.left + bounds.width : bounds.left - 160.0F;
            skillBounds = Aabb {left, request_.worldBounds.groundTop - 64.0F, 160.0F, 64.0F};
        }
        const bool windingUp = enemy.controller.action() == ai::EnemyAction::Windup
            || enemy.breathWindup > 0.0F;
        const bool active = enemy.controller.action() == ai::EnemyAction::Active
            || enemy.breathRemaining > 0.0F;
        views.push_back({enemy.archetype, enemy.controller.position(), bounds.width, bounds.height,
            enemy.health.current(), enemy.health.maximum(), enemy.health.isAlive(),
            windingUp, active, enemy.slowed, skillBounds, enemy.controller.facingDirection()});
    }
    return views;
}

std::vector<SpellEffectView> CombatSession::spellEffects() const
{
    std::vector<SpellEffectView> views;
    views.reserve(activeSpellEffects_.size());
    for (const auto& effect : activeSpellEffects_)
        views.push_back({effect.spellId, effect.bounds, effect.remaining, effect.duration});
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
        && skill != ai::EnemySkill::Thread && skill != ai::EnemySkill::Domination)
    {
        const auto area = enemy.controller.attackBounds();
        if (area.width > 0.0F && area.height > 0.0F) skillBounds = area;
    }
    if (enemy.archetype == EnemyArchetype::RedMirrorDragon
        && (enemy.breathWindup > 0.0F || enemy.breathRemaining > 0.0F))
    {
        const float left = enemy.controller.facingDirection() > 0.0F
            ? bounds.left + bounds.width : bounds.left - 160.0F;
        skillBounds = Aabb {left, request_.worldBounds.groundTop - 64.0F, 160.0F, 64.0F};
    }
    return {enemy.archetype, enemy.controller.position(), bounds.width, bounds.height,
        enemy.health.current(), enemy.health.maximum(), enemy.health.isAlive(),
        enemy.controller.action() == ai::EnemyAction::Windup || enemy.breathWindup > 0.0F,
        enemy.controller.action() == ai::EnemyAction::Active || enemy.breathRemaining > 0.0F,
        enemy.slowed, skillBounds, enemy.controller.facingDirection()};
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
