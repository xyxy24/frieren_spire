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
    case EnemyArchetype::ChaosFlower:
        return EnemyConfig {0.0F, 84.0F, 84.0F, 0.5F, 1.0F, 0.0F, 0.0F,
            64.0F, 84.0F, 5.0F, true, false, EnemySkill::LeafBlade};
    case EnemyArchetype::FrostWolf:
        return EnemyConfig {240.0F, 64.0F, 64.0F, 0.5F, 1.0F, 0.0F, 0.0F,
            64.0F, 42.0F, 4.0F, true, false, EnemySkill::WolfClaw};
    case EnemyArchetype::Qual:
        return EnemyConfig {120.0F, 96.0F, 96.0F, 0.5F, 1.0F, 0.0F, 0.0F,
            64.0F, 96.0F, 3.0F, true, false, EnemySkill::KillingMagic};
    case EnemyArchetype::Laufen:
        return EnemyConfig {180.0F, 64.0F, 64.0F, 0.5F, 1.0F, 0.0F, 0.0F,
            42.0F, 64.0F, 3.0F, false, false, EnemySkill::SideKick};
    case EnemyArchetype::Richter:
        return EnemyConfig {120.0F, 84.0F, 84.0F, 0.5F, 1.0F, 0.0F, 0.0F,
            42.0F, 72.0F, 7.0F, false, false, EnemySkill::EarthMagic};
    case EnemyArchetype::Denken:
        return EnemyConfig {100.0F, 120.0F, 120.0F, 0.5F, 1.0F, 0.0F, 0.0F,
            42.0F, 64.0F, 9.0F, false, false, EnemySkill::TornadoMagic};
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
    if (relics_.has(relics::FlammeNotesId)) tacticalNotesRemaining_ = 8.0F;
    if (relics_.has(relics::SeriePageId)) spells_.setUltimateCooldown(15.3F);
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
            case EnemyArchetype::ChaosFlower: return 125;
            case EnemyArchetype::FrostWolf: return 120;
            case EnemyArchetype::Qual: return 150;
            case EnemyArchetype::Laufen: return 160;
            case EnemyArchetype::Richter: return 150;
            case EnemyArchetype::Denken: return 135;
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
        if (spawn.archetype == EnemyArchetype::ChaosFlower)
            enemies_.back().specialCooldown = 3.5F;
        if (spawn.archetype == EnemyArchetype::FrostWolf)
            enemies_.back().specialCooldown = 3.0F;
        if (spawn.archetype == EnemyArchetype::Laufen)
            enemies_.back().specialCooldown = 2.5F;
        if (spawn.archetype == EnemyArchetype::Richter)
            enemies_.back().specialCooldown = 3.5F;
        if (spawn.archetype == EnemyArchetype::Denken)
            enemies_.back().specialCooldown = 4.5F;
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
        flowerFieldRemaining_ = relics_.has(relics::SteelPetalBookmarkId) ? 5.0F : 4.0F;
        flowerFieldCenterX_ = player_.position().x + PlayerController::Width * 0.5F;
    }
    if (!playerHealth_.isAlive()) finish(CombatOutcome::Defeat);
}

DamageResult CombatSession::resolvePlayerDamage(DamageRequest request) noexcept
{
    const int beforeBarrier = barrierShield_;
    const int beforePersistent = persistentShield_;
    const int beforeShield = beforeBarrier + beforePersistent;
    if (beforeShield > 0) request.flatReduction += beforeShield;
    const int incoming = std::max(0, static_cast<int>(std::lround(
        static_cast<float>(request.baseDamage) * request.sourceMultiplier * request.targetMultiplier))
        - (request.flatReduction - beforeShield));
    const int absorbed = request.blocked ? 0 : std::min(beforeShield, incoming);
    const auto result = playerDamageResolver_.resolve(playerHealth_, request);
    if (result.appliedDamage > 0) actualHpLost_ = true;
    const int barrierAbsorbed = std::min(beforeBarrier, absorbed);
    barrierShield_ -= barrierAbsorbed;
    persistentShield_ -= absorbed - barrierAbsorbed;
    if (beforeBarrier > 0 && barrierShield_ <= 0)
    {
        barrierShield_ = 0;
        barrierRemaining_ = 0.0F;
        const auto player = playerBounds();
        const Aabb burst {player.left - 80.0F, player.top - 48.0F,
            player.width + 160.0F, player.height + 96.0F};
        for (auto& enemy : enemies_)
            if (enemy.health.isAlive() && intersects(burst, enemy.controller.bounds()))
                static_cast<void>(enemy.damageResolver.resolve(enemy.health,
                    {DamageSource::Event, ++environmentalSequence_, 12}));
    }
    return result;
}

int CombatSession::healPlayer(int amount) noexcept
{
    if (amount <= 0) return 0;
    if (relics_.has(relics::DemonCoinId)) amount = std::max(1, amount * 3 / 4);
    const int healed = playerHealth_.heal(amount);
    const int excess = amount - healed;
    if (excess > 0 && relics_.has(relics::HeroFlowerCrownId) && flowerCrownConverted_ < 15)
    {
        const int shieldCap = playerHealth_.maximum() * 3 / 10;
        const int converted = std::min({excess, 15 - flowerCrownConverted_,
            std::max(0, shieldCap - persistentShield_ - barrierShield_)});
        persistentShield_ += converted;
        flowerCrownConverted_ += converted;
    }
    return healed;
}

float CombatSession::relicControlMultiplier(EnemyRuntime& enemy) noexcept
{
    if (!relics_.has(relics::ObedienceScaleId)) return 1.0F;
    if (!globalControlRelicUsed_)
    {
        globalControlRelicUsed_ = true;
        enemy.controlRelicTriggered = true;
        return 1.40F;
    }
    if (!enemy.controlRelicTriggered)
    {
        enemy.controlRelicTriggered = true;
        return 1.20F;
    }
    return 1.0F;
}

float CombatSession::playerControlDuration(const float seconds) noexcept
{
    if (!relics_.has(relics::PriestScriptureId)) return seconds;
    if (!firstIncomingControlUsed_)
    {
        spellInvulnerableRemaining_ = std::max(spellInvulnerableRemaining_, 0.3F);
        firstIncomingControlUsed_ = true;
    }
    return seconds * 0.60F;
}

void CombatSession::onBlessingPreventedControl() noexcept
{
    if (!relics_.has(relics::GoddessTabletId) || goddessTabletTriggered_) return;
    const int cap = playerHealth_.maximum() * 3 / 10;
    persistentShield_ += std::min(8, std::max(0, cap - persistentShield_ - barrierShield_));
    goddessTabletDamageRemaining_ = 3.0F;
    goddessTabletTriggered_ = true;
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
    tacticalNotesRemaining_ = std::max(0.0F, tacticalNotesRemaining_ - deltaSeconds);
    manaLensCooldown_ = std::max(0.0F, manaLensCooldown_ - deltaSeconds);
    warriorAxeCooldown_ = std::max(0.0F, warriorAxeCooldown_ - deltaSeconds);
    goddessTabletDamageRemaining_ = std::max(0.0F, goddessTabletDamageRemaining_ - deltaSeconds);
    blessingRemaining_ = std::max(0.0F, blessingRemaining_ - deltaSeconds);
    flowerFieldRemaining_ = std::max(0.0F, flowerFieldRemaining_ - deltaSeconds);
    if (burningFlowerRemaining_ > 0.0F)
    {
        const float activeDelta = std::min(deltaSeconds, burningFlowerRemaining_);
        burningFlowerRemaining_ -= activeDelta;
        burningFlowerAccumulator_ += activeDelta;
        while (burningFlowerAccumulator_ >= 1.0F)
        {
            burningFlowerAccumulator_ -= 1.0F;
            ++burningFlowerTick_;
            const Aabb field {flowerFieldCenterX_ - 300.0F,
                request_.worldBounds.groundTop - 300.0F, 600.0F, 300.0F};
            for (auto& enemy : enemies_)
                if (enemy.health.isAlive() && intersects(field, enemy.controller.bounds()))
                    static_cast<void>(enemy.damageResolver.resolve(enemy.health,
                        {burningFlowerSource_, burningFlowerSequenceBase_ + burningFlowerTick_,
                            8, burningFlowerMultiplier_}));
        }
    }
    acidFieldRemaining_ = std::max(0.0F, acidFieldRemaining_ - deltaSeconds);
    phantomRemaining_ = std::max(0.0F, phantomRemaining_ - deltaSeconds);
    cleanseProtectionRemaining_ = std::max(0.0F, cleanseProtectionRemaining_ - deltaSeconds);
    postDashComboRemaining_ = std::max(0.0F, postDashComboRemaining_ - deltaSeconds);
    spellInvulnerableRemaining_ = std::max(0.0F, spellInvulnerableRemaining_ - deltaSeconds);
    lightningStaffRemaining_ = std::max(0.0F, lightningStaffRemaining_ - deltaSeconds);
    if (lightningStaffRemaining_ <= 0.0F) lightningStaffCharges_ = 0U;
    barrierRemaining_ = std::max(0.0F, barrierRemaining_ - deltaSeconds);
    if (barrierRemaining_ <= 0.0F) barrierShield_ = 0;
    if (gravityWellRemaining_ > 0.0F)
    {
        const float activeDelta = std::min(deltaSeconds, gravityWellRemaining_);
        gravityWellRemaining_ -= activeDelta;
        gravityWellTickAccumulator_ += activeDelta;
        const float centerX = gravityWellBounds_.left + gravityWellBounds_.width * 0.5F;
        for (auto& enemy : enemies_)
        {
            if (!enemy.health.isAlive() || !intersects(gravityWellBounds_, enemy.controller.bounds())) continue;
            const float enemyCenter = enemy.controller.bounds().left + enemy.controller.bounds().width * 0.5F;
            enemy.controller.translateHorizontal((centerX > enemyCenter ? 1.0F : -1.0F)
                * 90.0F * activeDelta, request_.worldBounds);
        }
        while (gravityWellTickAccumulator_ >= 1.0F)
        {
            gravityWellTickAccumulator_ -= 1.0F;
            ++gravityWellTick_;
            for (auto& enemy : enemies_)
                if (enemy.health.isAlive() && intersects(gravityWellBounds_, enemy.controller.bounds()))
                    static_cast<void>(enemy.damageResolver.resolve(enemy.health,
                        {gravityWellSource_, gravityWellSequenceBase_ + gravityWellTick_,
                            6, gravityWellMultiplier_ * (enemy.markedRemaining > 0.0F ? 1.12F : 1.0F)}));
        }
    }
    const float previousGolemRemaining = golemRemaining_;
    golemRemaining_ = std::max(0.0F, golemRemaining_ - deltaSeconds);
    if (previousGolemRemaining > 0.0F && golemRemaining_ <= 0.0F)
    {
        const auto found = std::find_if(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
            return enemy.health.isAlive();
        });
        if (found != enemies_.end())
            static_cast<void>(found->damageResolver.resolve(found->health,
                {golemSource_, golemSequence_ * 16U + 1U, 20, golemMultiplier_}));
    }
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
                        * (enemy.markedRemaining > 0.0F ? 1.12F : 1.0F)}));
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
    for (auto& pillar : activePillars_)
    {
        pillar.remaining = std::max(0.0F, pillar.remaining - deltaSeconds);
        if (pillar.remaining > 0.0F && intersects(playerBounds(), pillar.bounds))
            player_.setHorizontalPosition(beforePlayerUpdate.x, request_.worldBounds);
    }
    std::erase_if(activePillars_, [](const auto& pillar) { return pillar.remaining <= 0.0F; });
    for (auto& tornado : activeTornadoes_)
    {
        const float activeDelta = std::min(deltaSeconds, tornado.remaining);
        tornado.remaining -= activeDelta;
        tornado.evolutionRemaining -= activeDelta;
        const float tornadoCenter = tornado.bounds.left + tornado.bounds.width * 0.5F;
        const float currentPlayerCenter = player_.position().x + PlayerController::Width * 0.5F;
        tornado.bounds.left += std::clamp(currentPlayerCenter - tornadoCenter,
            -160.0F * activeDelta, 160.0F * activeDelta);
        const float movedCenter = tornado.bounds.left + tornado.bounds.width * 0.5F;
        player_.translateHorizontal((movedCenter > currentPlayerCenter ? 1.0F : -1.0F)
            * 60.0F * activeDelta, request_.worldBounds);
        if (intersects(playerBounds(), tornado.bounds))
        {
            if (!tornado.launched)
            {
                if (blessingRemaining_ > 0.0F) onBlessingPreventedControl();
                else player_.applyLaunch(600.0F, playerControlDuration(0.28F));
                tornado.launched = true;
            }
            tornado.tickAccumulator += activeDelta;
            while (tornado.tickAccumulator >= 0.5F)
            {
                tornado.tickAccumulator -= 0.5F;
                const bool laterTick = (tornado.sequence & 0xffffffffULL) > 0U;
                static_cast<void>(resolvePlayerDamage(
                    {DamageSource::EnemyAttack, ++tornado.sequence,
                        tornado.evolutionRemaining > 0.0F ? 10 : 20, 1.0F,
                        relics_.incomingDamageMultiplier(),
                        laterTick && relics_.has(relics::NorthernCloakId) ? 4 : 0,
                        player_.isDashing() || guardRemaining_ > 0.0F
                            || spellInvulnerableRemaining_ > 0.0F}));
            }
        }
        else tornado.tickAccumulator = 0.0F;
    }
    std::erase_if(activeTornadoes_, [](const auto& tornado) { return tornado.remaining <= 0.0F; });
    if (player_.flightRemaining() <= 0.0F) flightBoostAvailable_ = false;
    if (wasDashing && !player_.isDashing()) postDashComboRemaining_ = 0.4F;
    if (intent.dashPressed && !wasDashing && player_.isDashing())
    {
        const float left = player_.facingDirection() > 0.0F
            ? beforePlayerUpdate.x : beforePlayerUpdate.x - PlayerController::DashDistance;
        activeSpellEffects_.push_back({1010U,
            {left, beforePlayerUpdate.y, PlayerController::DashDistance, PlayerController::Height},
            PlayerController::DashDurationSeconds, PlayerController::DashDurationSeconds});
        const Aabb dashPath {left, beforePlayerUpdate.y,
            PlayerController::DashDistance, PlayerController::Height};
        for (auto& enemy : enemies_)
            if (enemy.health.isAlive() && intersects(dashPath, enemy.controller.bounds()))
                enemy.markedRemaining = std::max(enemy.markedRemaining, 2.0F);
    }
    if (teaChannelRemaining_ > 0.0F)
    {
        const float previous = teaChannelRemaining_;
        teaChannelRemaining_ = std::max(0.0F, teaChannelRemaining_ - deltaSeconds);
        if (std::abs(player_.position().x - teaStartX_) > 8.0F || player_.isStunned())
            teaChannelRemaining_ = 0.0F;
        else if (previous > 0.0F && teaChannelRemaining_ <= 0.0F)
            static_cast<void>(healPlayer(18));
    }
    if (intent.guardPressed && !player_.isStunned() && !player_.isDashing()
        && player_.flightRemaining() <= 0.0F
        && guardCooldownRemaining_ <= 0.0F)
    {
        guardRemaining_ = GuardDurationSeconds;
        guardCooldownRemaining_ = GuardCooldownSeconds;
        perfectGuardConsumed_ = false;
        activeSpellEffects_.push_back({1007U, playerBounds(),
            GuardDurationSeconds, GuardDurationSeconds});
    }
    const float playerCenter = player_.position().x + PlayerController::Width * 0.5F;
    const bool canAct = !player_.isStunned() && !player_.isDashing()
        && guardRemaining_ <= 0.0F && beamRemaining_ <= 0.0F;
    const auto handleGuard = [this]() {
        if (guardRemaining_ > GuardDurationSeconds - PerfectGuardSeconds
            && !perfectGuardConsumed_)
        {
            spells_.reduceLongestRegularCooldown(1.0F);
            if (relics_.has(relics::BrokenBarrierId))
            {
                spells_.reduceLongestRegularCooldown(0.5F);
                const auto player = playerBounds();
                const Aabb burst {player.left - 96.0F, player.top - 64.0F,
                    player.width + 192.0F, player.height + 128.0F};
                for (auto& enemy : enemies_)
                    if (enemy.health.isAlive() && intersects(burst, enemy.controller.bounds()))
                    {
                        static_cast<void>(enemy.damageResolver.resolve(enemy.health,
                            {DamageSource::Event, ++environmentalSequence_, 20}));
                        const bool boss = enemy.archetype == EnemyArchetype::Aura
                            || enemy.archetype == EnemyArchetype::RedMirrorDragon
                            || enemy.archetype == EnemyArchetype::Boss;
                        if (!boss) enemy.controlRemaining = std::max(enemy.controlRemaining, 0.25F);
                    }
            }
            perfectGuardConsumed_ = true;
        }
    };

    for (std::size_t enemyIndex = 0U; enemyIndex < enemies_.size(); ++enemyIndex)
    {
        auto& enemy = enemies_[enemyIndex];
        if (!enemy.health.isAlive()) continue;
        enemy.frostSlowRemaining = std::max(0.0F, enemy.frostSlowRemaining - deltaSeconds);
        enemy.controlRemaining = std::max(0.0F, enemy.controlRemaining - deltaSeconds);
        enemy.exposedRemaining = std::max(0.0F, enemy.exposedRemaining - deltaSeconds);
        enemy.markedRemaining = std::max(0.0F, enemy.markedRemaining - deltaSeconds);
        enemy.frozenRemaining = std::max(0.0F, enemy.frozenRemaining - deltaSeconds);
        enemy.goldenBindRemaining = std::max(0.0F, enemy.goldenBindRemaining - deltaSeconds);
        enemy.skillSealRemaining = std::max(0.0F, enemy.skillSealRemaining - deltaSeconds);
        enemy.relicComboWindow = std::max(0.0F, enemy.relicComboWindow - deltaSeconds);
        enemy.relicComboCooldown = std::max(0.0F, enemy.relicComboCooldown - deltaSeconds);
        enemy.kraftCooldown = std::max(0.0F, enemy.kraftCooldown - deltaSeconds);
        if (enemy.relicComboWindow <= 0.0F) enemy.relicComboHits = 0U;
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
                    {enemy.burnSource, enemy.burnSequenceBase + enemy.burnTick, 4,
                        enemy.burnMultiplier}));
            }
        }
        if (!enemy.health.isAlive()) continue;
        enemy.contactCooldown = std::max(0.0F, enemy.contactCooldown - deltaSeconds);
        const auto bounds = enemy.controller.bounds();
        const bool flowerSlowed = flowerFieldRemaining_ > 0.0F
            && std::abs(bounds.left + bounds.width * 0.5F - flowerFieldCenterX_) <= 300.0F;
        enemy.slowed = flowerSlowed || enemy.frostSlowRemaining > 0.0F;
        if (enemy.archetype == EnemyArchetype::ChaosFlower)
        {
            enemy.specialCooldown = std::max(0.0F, enemy.specialCooldown - deltaSeconds);
            if (enemy.specialWindup > 0.0F)
            {
                enemy.specialWindup = std::max(0.0F, enemy.specialWindup - deltaSeconds);
                if (enemy.specialWindup <= 0.0F)
                {
                    const auto flower = enemy.controller.bounds();
                    const Aabb curse {flower.left + flower.width * 0.5F - 160.0F,
                        flower.top + flower.height * 0.5F - 160.0F, 320.0F, 320.0F};
                    if (intersects(playerBounds(), curse) && blessingRemaining_ <= 0.0F
                        && !player_.isDashing() && guardRemaining_ <= 0.0F
                        && spellInvulnerableRemaining_ <= 0.0F)
                    {
                        sleepStacks_ = std::min<std::uint32_t>(5U, sleepStacks_ + 1U);
                        player_.applyHitReaction(0.0F,
                            playerControlDuration(static_cast<float>(sleepStacks_) * 0.5F));
                    }
                    else if (intersects(playerBounds(), curse) && blessingRemaining_ > 0.0F)
                        onBlessingPreventedControl();
                    enemy.specialCooldown = 7.0F;
                }
                continue;
            }
            if (enemy.specialCooldown <= 0.0F
                && enemy.controller.action() == ai::EnemyAction::Chase)
            {
                enemy.specialWindup = 0.5F;
                continue;
            }
        }
        if (enemy.archetype == EnemyArchetype::FrostWolf)
        {
            enemy.specialCooldown = std::max(0.0F, enemy.specialCooldown - deltaSeconds);
            if (enemy.specialWindup > 0.0F)
            {
                enemy.specialWindup = std::max(0.0F, enemy.specialWindup - deltaSeconds);
                if (enemy.specialWindup <= 0.0F)
                {
                    enemy.specialActive = 0.4F;
                    enemy.specialElapsed = 0.0F;
                    enemy.specialDirection = enemy.controller.facingDirection();
                }
                continue;
            }
            if (enemy.specialActive > 0.0F)
            {
                const float activeDelta = std::min(deltaSeconds, enemy.specialActive);
                enemy.specialActive -= activeDelta;
                enemy.specialElapsed += activeDelta;
                auto position = enemy.controller.position();
                position.x += enemy.specialDirection * 240.0F * activeDelta;
                const float progress = std::clamp(enemy.specialElapsed / 0.4F, 0.0F, 1.0F);
                position.y = request_.worldBounds.groundTop - bounds.height
                    - 32.0F * 4.0F * progress * (1.0F - progress);
                enemy.controller.setPosition(position, request_.worldBounds);
                if (enemy.specialActive <= 0.0F) enemy.specialCooldown = 6.0F;
                continue;
            }
            if (enemy.specialCooldown <= 0.0F
                && enemy.controller.action() == ai::EnemyAction::Chase)
            {
                enemy.specialWindup = 0.5F;
                continue;
            }
        }
        if (enemy.archetype == EnemyArchetype::Laufen)
        {
            enemy.specialCooldown = std::max(0.0F, enemy.specialCooldown - deltaSeconds);
            if (enemy.specialWindup > 0.0F)
            {
                enemy.specialWindup = std::max(0.0F, enemy.specialWindup - deltaSeconds);
                if (enemy.specialWindup <= 0.0F)
                {
                    const auto playerArea = playerBounds();
                    const float playerFacing = player_.facingDirection();
                    const float behindX = playerFacing > 0.0F
                        ? playerArea.left - 24.0F - bounds.width
                        : playerArea.left + playerArea.width + 24.0F;
                    const bool behindFits = behindX >= request_.worldBounds.left
                        && behindX + bounds.width <= request_.worldBounds.right;
                    const float frontX = playerFacing > 0.0F
                        ? playerArea.left + playerArea.width + 48.0F
                        : playerArea.left - 48.0F - bounds.width;
                    enemy.controller.setPosition({behindFits ? behindX : frontX,
                        request_.worldBounds.groundTop - bounds.height}, request_.worldBounds);
                    enemy.controller.forceAttackToward(
                        playerArea.left + playerArea.width * 0.5F);
                    enemy.specialCooldown = 5.0F;
                }
                continue;
            }
            if (enemy.specialCooldown <= 0.0F
                && enemy.controller.action() == ai::EnemyAction::Chase)
            {
                enemy.specialWindup = 0.5F;
                continue;
            }
        }
        if (enemy.archetype == EnemyArchetype::Richter)
        {
            enemy.specialCooldown = std::max(0.0F, enemy.specialCooldown - deltaSeconds);
            if (enemy.specialWindup > 0.0F)
            {
                enemy.specialWindup = std::max(0.0F, enemy.specialWindup - deltaSeconds);
                if (enemy.specialWindup <= 0.0F)
                {
                    const bool occupied = std::any_of(enemies_.begin(), enemies_.end(),
                        [&](const auto& other) {
                            return &other != &enemy && other.health.isAlive()
                                && intersects(other.controller.bounds(), enemy.specialTargetBounds);
                        });
                    if (occupied) enemy.specialCooldown = 2.0F;
                    else
                    {
                        activePillars_.push_back({enemy.specialTargetBounds, 4.0F});
                        if (intersects(playerBounds(), enemy.specialTargetBounds))
                        {
                            static_cast<void>(resolvePlayerDamage(
                                {DamageSource::EnemyAttack, ++environmentalSequence_, 25, 1.0F,
                                    relics_.incomingDamageMultiplier()}));
                            if (blessingRemaining_ > 0.0F) onBlessingPreventedControl();
                            else player_.applyLaunch(650.0F, playerControlDuration(0.28F));
                        }
                        enemy.specialCooldown = 7.0F;
                    }
                }
                continue;
            }
            if (enemy.specialCooldown <= 0.0F
                && enemy.controller.action() == ai::EnemyAction::Chase)
            {
                const float center = player_.position().x + PlayerController::Width * 0.5F;
                enemy.specialTargetBounds = {center - 32.0F,
                    request_.worldBounds.groundTop - 108.0F, 64.0F, 108.0F};
                enemy.specialWindup = 0.5F;
                continue;
            }
        }
        if (enemy.archetype == EnemyArchetype::Denken)
        {
            enemy.specialCooldown = std::max(0.0F, enemy.specialCooldown - deltaSeconds);
            if (enemy.specialWindup > 0.0F)
            {
                enemy.specialWindup = std::max(0.0F, enemy.specialWindup - deltaSeconds);
                if (enemy.specialWindup <= 0.0F)
                {
                    const auto caster = enemy.controller.bounds();
                    const float left = enemy.specialDirection > 0.0F
                        ? caster.left + caster.width : caster.left - 72.0F;
                    activeTornadoes_.push_back({{left,
                        request_.worldBounds.groundTop - 128.0F, 72.0F, 128.0F},
                        7.0F, 3.0F, 0.0F, (++environmentalSequence_) << 32U, false});
                    enemy.specialCooldown = 9.0F;
                }
                continue;
            }
            if (enemy.specialCooldown <= 0.0F
                && enemy.controller.action() == ai::EnemyAction::Chase)
            {
                enemy.specialDirection = enemy.controller.facingDirection();
                enemy.specialWindup = 0.5F;
                continue;
            }
        }
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
                    {
                        if (guardRemaining_ > 0.0F) handleGuard();
                        static_cast<void>(resolvePlayerDamage(
                            {DamageSource::EnemyAttack, sequence, 15, 1.0F,
                                relics_.incomingDamageMultiplier()
                                    * (cleanseProtectionRemaining_ > 0.0F ? 0.8F : 1.0F),
                                enemy.breathRemaining < 1.0F
                                    && relics_.has(relics::NorthernCloakId) ? 4 : 0,
                                player_.isDashing() || guardRemaining_ > 0.0F
                                    || spellInvulnerableRemaining_ > 0.0F}));
                    }
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
        if (enemy.controlRemaining <= 0.0F && enemy.frozenRemaining <= 0.0F)
        {
            const Aabb target = phantomRemaining_ > 0.0F ? phantomBounds_
                : (golemRemaining_ > 0.0F ? golemBounds_ : playerBounds());
            const float speedMultiplier = flowerSlowed ? 0.65F
                : (enemy.frostSlowRemaining > 0.0F ? 0.65F : 1.0F);
            const bool manualSkill = enemy.archetype == EnemyArchetype::Richter
                || enemy.archetype == EnemyArchetype::Denken;
            enemy.controller.update(target, deltaSeconds, request_.worldBounds, speedMultiplier,
                enemy.skillSealRemaining <= 0.0F && !manualSkill);
        }
        if (phantomRemaining_ > 0.0F && enemy.controller.action() == ai::EnemyAction::Active
            && intersects(phantomBounds_, enemy.controller.attackBounds()))
        {
            phantomRemaining_ = 0.0F;
            enemy.markedRemaining = std::max(enemy.markedRemaining, 3.0F);
            spells_.reduceLongestRegularCooldown(1.0F);
            std::erase_if(activeSpellEffects_, [](const auto& effect) { return effect.spellId == 1011U; });
        }
        if (golemRemaining_ > 0.0F && enemy.controller.action() == ai::EnemyAction::Active
            && intersects(golemBounds_, enemy.controller.attackBounds()))
        {
            golemRemaining_ = 0.0F;
            const Aabb blast {golemBounds_.left - 49.0F, golemBounds_.top - 35.0F, 140.0F, 140.0F};
            for (auto& target : enemies_)
                if (target.health.isAlive() && intersects(blast, target.controller.bounds()))
                    static_cast<void>(target.damageResolver.resolve(target.health,
                        {golemSource_, golemSequence_ * 16U + 2U, 24, golemMultiplier_}));
            activeSpellEffects_.push_back({1022U, blast, 0.5F, 0.5F});
        }
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

    if (flowerFieldRemaining_ > 0.0F && std::abs(playerCenter - flowerFieldCenterX_) <= 300.0F)
    {
        flowerHealingAccumulator_ += 3.0F * deltaSeconds;
        const int healing = static_cast<int>(flowerHealingAccumulator_);
        if (healing > 0)
        {
            static_cast<void>(healPlayer(healing));
            flowerHealingAccumulator_ -= static_cast<float>(healing);
        }
    }
    if (intent.attackPressed && canAct) attack_.tryStart();

    const auto regularSlotForSource = [](const DamageSource source) -> std::optional<std::size_t> {
        if (source == DamageSource::PlayerSpell0) return 0U;
        if (source == DamageSource::PlayerSpell1) return 1U;
        if (source == DamageSource::PlayerSpell2) return 2U;
        return std::nullopt;
    };
    const auto onActiveHit = [this, &regularSlotForSource](EnemyRuntime& enemy,
        const DamageSource source, const std::uint64_t sequence)
    {
        const auto slot = regularSlotForSource(source);
        if (slot && relics_.has(relics::ManaLensId) && enemy.markedRemaining > 0.0F
            && manaLensCooldown_ <= 0.0F)
        {
            spells_.reduceLongestRegularCooldown(0.35F);
            manaLensCooldown_ = 1.0F;
        }
        if (source == DamageSource::PlayerBasicAttack)
        {
            if (relics_.has(relics::WarriorAxeFragmentId) && warriorAxeCooldown_ <= 0.0F)
            {
                spells_.reduceLongestRegularCooldown(0.25F);
                warriorAxeCooldown_ = 1.0F;
            }
            if (relics_.has(relics::KraftBracerId) && enemy.kraftCooldown <= 0.0F
                && (enemy.markedRemaining > 0.0F || enemy.frozenRemaining > 0.0F
                    || enemy.goldenBindRemaining > 0.0F || enemy.skillSealRemaining > 0.0F))
            {
                static_cast<void>(enemy.damageResolver.resolve(enemy.health,
                    {DamageSource::Event, sequence * 32U + 29U, 8}));
                player_.reduceDashCooldown(0.25F);
                enemy.kraftCooldown = 0.6F;
            }
        }
        if (relics_.has(relics::PurpleGrapeHairpinId) && enemy.relicComboCooldown <= 0.0F
            && (slot || source == DamageSource::PlayerBasicAttack))
        {
            if (enemy.relicComboWindow <= 0.0F) enemy.relicComboHits = 0U;
            enemy.relicComboWindow = 2.5F;
            ++enemy.relicComboHits;
            if (enemy.relicComboHits >= 3U)
            {
                static_cast<void>(enemy.damageResolver.resolve(enemy.health,
                    {DamageSource::Event, sequence * 32U + 31U, 18}));
                if (slot) spells_.reduceRegularCooldown(*slot, 0.8F);
                enemy.relicComboHits = 0U;
                enemy.relicComboCooldown = 3.0F;
            }
        }
    };
    const auto damageEnemies = [this, &onActiveHit, &regularSlotForSource](const DamageSource source, const std::uint64_t sequence,
        const int damage, const float multiplier, const Aabb& area)
    {
        std::size_t hitIndex = 0U;
        for (auto& enemy : enemies_)
            if (enemy.health.isAlive() && intersects(area, enemy.controller.bounds()))
            {
                float targetMultiplier = multiplier
                    * (enemy.exposedRemaining > 0.0F ? 1.25F : 1.0F)
                    * (enemy.markedRemaining > 0.0F ? 1.12F : 1.0F);
                if (regularSlotForSource(source) && relics_.has(relics::ElderSealFragmentId))
                    targetMultiplier *= hitIndex == 1U ? 1.12F : (hitIndex >= 2U ? 1.24F : 1.0F);
                const auto result = enemy.damageResolver.resolve(enemy.health,
                    {source, sequence, damage, targetMultiplier});
                if (result.appliedDamage > 0) onActiveHit(enemy, source, sequence);
                ++hitIndex;
            }
    };
    const auto onInterrupt = [this](EnemyRuntime& enemy)
    {
        if (relics_.has(relics::FlammeNotesId) && !tacticalInterruptUsed_)
        {
            enemy.markedRemaining += 3.0F;
            player_.refreshDash();
            tacticalInterruptUsed_ = true;
        }
    };
    const auto applyCast = [&](const spells::SpellCastResult& cast, const DamageSource source,
        const spells::SpellDefinition* definition)
    {
        if (!cast.cast || !definition) return;
        const auto visualDuration = [](const spells::SpellEffect effect) {
            switch (effect)
            {
            case spells::SpellEffect::FlowerField: return 4.0F;
            case spells::SpellEffect::GoddessBlessing: return 5.0F;
            case spells::SpellEffect::FlameBurst: return 3.0F;
            case spells::SpellEffect::Phantom: return 4.0F;
            case spells::SpellEffect::SourGrape: return 5.0F;
            case spells::SpellEffect::HotTea: return 1.0F;
            case spells::SpellEffect::ManaTrace: return 4.0F;
            case spells::SpellEffect::FloatSlam: return 1.0F;
            case spells::SpellEffect::StoneGolem: return 5.0F;
            case spells::SpellEffect::Flight: return 2.5F;
            case spells::SpellEffect::LightningStaff: return 4.0F;
            case spells::SpellEffect::DefensiveBarrier: return 5.0F;
            case spells::SpellEffect::GravityWell: return 2.5F;
            case spells::SpellEffect::GoddessSpears: return 0.6F;
            case spells::SpellEffect::DestructionLightning: return 0.8F;
            case spells::SpellEffect::HellfireStorm: return 3.0F;
            case spells::SpellEffect::JudgmentBeam: return 2.0F;
            case spells::SpellEffect::EarthPillars: return 3.0F;
            case spells::SpellEffect::MirrorArray: return 8.0F;
            default: return 0.35F;
            }
        };
        float duration = visualDuration(cast.effect);
        const bool extendPersistent = relics_.has(relics::SteelPetalBookmarkId)
            && (cast.effect == spells::SpellEffect::FlowerField
                || cast.effect == spells::SpellEffect::Phantom
                || cast.effect == spells::SpellEffect::StoneGolem
                || cast.effect == spells::SpellEffect::GravityWell);
        if (extendPersistent) duration *= 1.25F;
        activeSpellEffects_.push_back({cast.spellId, cast.effectBounds, duration, duration});
        float multiplier = relics_.outgoingDamageMultiplier()
            * (blessingRemaining_ > 0.0F ? 1.15F : 1.0F)
            * (goddessTabletDamageRemaining_ > 0.0F ? 1.10F : 1.0F);
        const bool regularCast = source != DamageSource::PlayerUltimateSpell;
        if (regularCast && relics_.has(relics::SeriePageId)) multiplier *= 0.92F;
        const bool attackSpell = definition->damage > 0
            || cast.effect == spells::SpellEffect::BloodMagic
            || cast.effect == spells::SpellEffect::MultiZoltraak
            || cast.effect == spells::SpellEffect::ManaStrike
            || cast.effect == spells::SpellEffect::SpatialShatter
            || cast.effect == spells::SpellEffect::HomingVolley
            || cast.effect == spells::SpellEffect::WindPressure
            || cast.effect == spells::SpellEffect::GravityWell;
        if (cast.effect == spells::SpellEffect::BloodMagic
            && relics_.has(relics::DemonCoinId)) multiplier *= 1.25F;
        if (regularCast && attackSpell && relics_.has(relics::ManaSuppressionEarringId)
            && !firstDamageSpellUsed_)
        {
            multiplier *= 1.30F;
            firstDamageSpellUsed_ = true;
        }
        if (regularCast && attackSpell && relics_.has(relics::OldSpellBookmarkId))
        {
            const auto views = spells_.view();
            float shortest = std::numeric_limits<float>::max();
            std::optional<std::uint32_t> shortestId;
            for (const auto& view : views)
                if (view.id && view.cooldownDuration < shortest)
                {
                    shortest = view.cooldownDuration;
                    shortestId = view.id;
                }
            if (shortestId == definition->id) multiplier *= 1.12F;
        }
        if (flightBoostAvailable_ && attackSpell)
        {
            multiplier *= 1.15F;
            flightBoostAvailable_ = false;
        }
        if (regularCast) lastRegularCastMultiplier_ = multiplier;
        if (cast.effect == spells::SpellEffect::FlowerField)
        {
            flowerFieldRemaining_ = extendPersistent ? 5.0F : 4.0F;
            flowerFieldCenterX_ = playerCenter;
            flowerHealingAccumulator_ = 0.0F;
        }
        else if (cast.effect == spells::SpellEffect::GoddessBlessing)
        {
            blessingRemaining_ = 5.0F;
            goddessTabletTriggered_ = false;
        }
        else if (cast.effect == spells::SpellEffect::BloodMagic)
        {
            const int cost = std::max(1, (playerHealth_.current() * 12 + 99) / 100);
            static_cast<void>(playerDamageResolver_.resolve(playerHealth_,
                {DamageSource::Event, ++selfDamageSequence_, cost}));
            actualHpLost_ = true;
            damageEnemies(source, cast.sequence, (playerHealth_.current() * 55) / 100,
                multiplier, cast.effectBounds);
        }
        else if (cast.effect == spells::SpellEffect::FrostLance)
        {
            damageEnemies(source, cast.sequence, cast.damage, multiplier, cast.effectBounds);
            for (auto& enemy : enemies_)
                if (enemy.health.isAlive() && intersects(cast.effectBounds, enemy.controller.bounds()))
                {
                    const bool alreadySlowed = enemy.frostSlowRemaining > 0.0F || enemy.slowed;
                    enemy.frostSlowRemaining = 2.0F;
                    if (alreadySlowed) enemy.frozenRemaining = 0.65F * relicControlMultiplier(enemy);
                }
        }
        else if (cast.effect == spells::SpellEffect::FlameBurst)
        {
            damageEnemies(source, cast.sequence, cast.damage, multiplier, cast.effectBounds);
            for (auto& enemy : enemies_)
                if (enemy.health.isAlive() && intersects(cast.effectBounds, enemy.controller.bounds()))
                {
                    const bool thermalShock = enemy.frostSlowRemaining > 0.0F
                        || enemy.frozenRemaining > 0.0F;
                    if (thermalShock)
                        static_cast<void>(enemy.damageResolver.resolve(enemy.health,
                            {source, cast.sequence * 16U + 14U, 10, multiplier}));
                    enemy.frostSlowRemaining = 0.0F;
                    enemy.frozenRemaining = 0.0F;
                    enemy.burnRemaining = 3.0F;
                    enemy.burnTickAccumulator = 0.0F;
                    enemy.burnSequenceBase = cast.sequence * 16U;
                    enemy.burnTick = 0U;
                    enemy.burnSource = source;
                    enemy.burnMultiplier = multiplier;
                }
        }
        else if (cast.effect == spells::SpellEffect::MagicThread)
        {
            damageEnemies(source, cast.sequence, cast.damage, multiplier, cast.effectBounds);
            for (auto& enemy : enemies_)
                if (enemy.health.isAlive() && intersects(cast.effectBounds, enemy.controller.bounds()))
                {
                    const float direction = playerCenter < enemy.controller.position().x ? -1.0F : 1.0F;
                    enemy.controller.translateHorizontal(direction * 110.0F, request_.worldBounds);
                    const bool boss = enemy.archetype == EnemyArchetype::Aura
                        || enemy.archetype == EnemyArchetype::RedMirrorDragon
                        || enemy.archetype == EnemyArchetype::Boss;
                    enemy.controlRemaining = (boss ? 0.3F : 0.9F) * relicControlMultiplier(enemy);
                }
        }
        else if (cast.effect == spells::SpellEffect::StoneShot)
        {
            damageEnemies(source, cast.sequence, cast.damage, multiplier, cast.effectBounds);
            for (auto& enemy : enemies_)
                if (enemy.health.isAlive() && intersects(cast.effectBounds, enemy.controller.bounds()))
                {
                    const float before = enemy.controller.position().x;
                    enemy.controller.translateHorizontal(player_.facingDirection() * 80.0F,
                        request_.worldBounds);
                    if (std::abs(enemy.controller.position().x - before) < 79.0F)
                        static_cast<void>(enemy.damageResolver.resolve(enemy.health,
                            {source, cast.sequence * 16U + 15U, 14, multiplier}));
                    if (enemy.goldenBindRemaining > 0.0F)
                    {
                        enemy.goldenBindRemaining = 0.0F;
                        static_cast<void>(enemy.damageResolver.resolve(enemy.health,
                            {source, cast.sequence * 16U + 13U, 16, multiplier}));
                    }
                }
            if (flowerFieldRemaining_ > 0.0F)
            {
                flowerFieldRemaining_ = 0.0F;
                burningFlowerRemaining_ = relics_.has(relics::SteelPetalBookmarkId) ? 2.5F : 2.0F;
                burningFlowerAccumulator_ = 0.0F;
                burningFlowerSequenceBase_ = cast.sequence * 32U;
                burningFlowerTick_ = 0U;
                burningFlowerSource_ = source;
                burningFlowerMultiplier_ = multiplier;
                const Aabb field {flowerFieldCenterX_ - 300.0F,
                    request_.worldBounds.groundTop - 300.0F, 600.0F, 300.0F};
                activeSpellEffects_.push_back({1006U, field, 2.0F, 2.0F});
            }
        }
        else if (cast.effect == spells::SpellEffect::Phantom)
        {
            phantomRemaining_ = extendPersistent ? 5.0F : 4.0F;
            const float left = player_.facingDirection() > 0.0F
                ? player_.position().x + 60.0F : player_.position().x - 60.0F;
            phantomBounds_ = {left, player_.position().y,
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
            static_cast<void>(healPlayer(10));
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
                found->controlRemaining = (boss ? 0.7F : 2.5F) * relicControlMultiplier(*found);
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
            if (found != enemies_.end()) found->markedRemaining = 4.0F;
        }
        else if (cast.effect == spells::SpellEffect::MultiZoltraak)
        {
            std::size_t hitIndex = 0U;
            for (auto& enemy : enemies_)
                if (enemy.health.isAlive() && intersects(cast.effectBounds, enemy.controller.bounds()))
                {
                    const float multiTargetMultiplier = relics_.has(relics::ElderSealFragmentId)
                        ? (hitIndex == 1U ? 1.12F : (hitIndex >= 2U ? 1.24F : 1.0F)) : 1.0F;
                    for (std::uint64_t beam = 1U; beam <= 3U && enemy.health.isAlive(); ++beam)
                    {
                        const int damage = beam == 3U && enemy.markedRemaining > 0.0F ? 20 : 10;
                        const auto result = enemy.damageResolver.resolve(enemy.health,
                            {source, cast.sequence * 8U + beam, damage,
                                multiplier * multiTargetMultiplier
                                    * (enemy.markedRemaining > 0.0F ? 1.12F : 1.0F)});
                        if (result.appliedDamage > 0)
                            onActiveHit(enemy, source, cast.sequence * 8U + beam);
                    }
                    ++hitIndex;
                }
        }
        else if (cast.effect == spells::SpellEffect::Dispel)
        {
            auto found = std::find_if(enemies_.begin(), enemies_.end(), [&](const auto& enemy) {
                const bool casting = enemy.controller.action() == ai::EnemyAction::Windup
                    || enemy.controller.action() == ai::EnemyAction::Active
                    || enemy.breathWindup > 0.0F || enemy.breathRemaining > 0.0F;
                return enemy.health.isAlive() && casting
                    && intersects(cast.effectBounds, enemy.controller.bounds());
            });
            if (found != enemies_.end())
            {
                found->controller.interrupt();
                found->breathWindup = 0.0F;
                found->breathRemaining = 0.0F;
                found->markedRemaining = std::max(found->markedRemaining, 4.0F);
                onInterrupt(*found);
                spells_.reduceLongestRegularCooldown(1.5F);
            }
        }
        else if (cast.effect == spells::SpellEffect::ManaStrike)
        {
            for (auto& enemy : enemies_)
                if (enemy.health.isAlive() && intersects(cast.effectBounds, enemy.controller.bounds()))
                {
                    const int damage = cast.damage + (postDashComboRemaining_ > 0.0F ? 10 : 0);
                    static_cast<void>(enemy.damageResolver.resolve(enemy.health,
                        {source, cast.sequence, damage, multiplier}));
                    const float before = enemy.controller.position().x;
                    enemy.controller.translateHorizontal(player_.facingDirection() * 120.0F,
                        request_.worldBounds);
                    if (std::abs(enemy.controller.position().x - before) < 119.0F)
                        static_cast<void>(enemy.damageResolver.resolve(enemy.health,
                            {source, cast.sequence * 16U + 15U, 12, multiplier}));
                }
            postDashComboRemaining_ = 0.0F;
        }
        else if (cast.effect == spells::SpellEffect::GoldenBinding)
        {
            damageEnemies(source, cast.sequence, cast.damage, multiplier, cast.effectBounds);
            for (auto& enemy : enemies_)
                if (enemy.health.isAlive() && intersects(cast.effectBounds, enemy.controller.bounds()))
                {
                    const bool boss = enemy.archetype == EnemyArchetype::Aura
                        || enemy.archetype == EnemyArchetype::RedMirrorDragon
                        || enemy.archetype == EnemyArchetype::Boss;
                    enemy.goldenBindRemaining = (boss ? 0.4F : 1.4F)
                        * relicControlMultiplier(enemy);
                    enemy.controlRemaining = std::max(enemy.controlRemaining,
                        enemy.goldenBindRemaining);
                }
        }
        else if (cast.effect == spells::SpellEffect::FloatSlam)
        {
            bool frozen = false;
            for (auto& enemy : enemies_)
                if (enemy.health.isAlive() && intersects(cast.effectBounds, enemy.controller.bounds()))
                {
                    frozen = frozen || enemy.frozenRemaining > 0.0F;
                    const bool boss = enemy.archetype == EnemyArchetype::Aura
                        || enemy.archetype == EnemyArchetype::RedMirrorDragon
                        || enemy.archetype == EnemyArchetype::Boss;
                    enemy.controlRemaining = std::max(enemy.controlRemaining,
                        (boss ? 0.35F : 1.0F) * relicControlMultiplier(enemy));
                }
            pendingSpellImpacts_.push_back({cast.spellId, cast.effectBounds, 1.0F,
                frozen ? 36 : 24, cast.sequence, source, multiplier});
        }
        else if (cast.effect == spells::SpellEffect::StoneGolem)
        {
            golemRemaining_ = extendPersistent ? 6.25F : 5.0F;
            const float left = player_.facingDirection() > 0.0F
                ? player_.position().x + 60.0F : player_.position().x - 60.0F;
            golemBounds_ = {left, request_.worldBounds.groundTop - 64.0F, 48.0F, 64.0F};
            golemSource_ = source;
            golemMultiplier_ = multiplier;
            golemSequence_ = cast.sequence;
            activeSpellEffects_.back().bounds = golemBounds_;
        }
        else if (cast.effect == spells::SpellEffect::Flight)
        {
            player_.grantFlight(2.5F);
            flightBoostAvailable_ = true;
        }
        else if (cast.effect == spells::SpellEffect::SpatialShatter)
        {
            bool interrupted = false;
            for (auto& enemy : enemies_)
                if (enemy.health.isAlive()
                    && enemy.controller.action() == ai::EnemyAction::Active
                    && intersects(cast.effectBounds, enemy.controller.attackBounds()))
                {
                    enemy.controller.interrupt();
                    onInterrupt(enemy);
                    interrupted = true;
                }
            damageEnemies(source, cast.sequence, interrupted ? 38 : 26, multiplier, cast.effectBounds);
            if (interrupted) spellInvulnerableRemaining_ = 0.35F;
        }
        else if (cast.effect == spells::SpellEffect::Seal)
        {
            damageEnemies(source, cast.sequence, cast.damage, multiplier, cast.effectBounds);
            for (auto& enemy : enemies_)
                if (enemy.health.isAlive() && intersects(cast.effectBounds, enemy.controller.bounds()))
                {
                    const bool boss = enemy.archetype == EnemyArchetype::Aura
                        || enemy.archetype == EnemyArchetype::RedMirrorDragon
                        || enemy.archetype == EnemyArchetype::Boss;
                    enemy.controller.interrupt();
                    onInterrupt(enemy);
                    enemy.skillSealRemaining = (boss ? 1.5F : 4.0F)
                        + (enemy.markedRemaining > 0.0F ? 1.0F : 0.0F);
                }
        }
        else if (cast.effect == spells::SpellEffect::LightningStaff)
        {
            lightningStaffRemaining_ = 4.0F;
            lightningStaffCharges_ = 3U;
        }
        else if (cast.effect == spells::SpellEffect::HomingVolley)
        {
            auto found = std::find_if(enemies_.begin(), enemies_.end(), [&](const auto& enemy) {
                return enemy.health.isAlive() && intersects(cast.effectBounds, enemy.controller.bounds());
            });
            if (found != enemies_.end())
                for (std::uint64_t bolt = 1U; bolt <= 3U && found->health.isAlive(); ++bolt)
                {
                    const auto result = found->damageResolver.resolve(found->health,
                        {source, cast.sequence * 8U + bolt,
                            7 + (bolt == 3U && found->controller.config().flying ? 6 : 0),
                            multiplier * (found->markedRemaining > 0.0F ? 1.12F : 1.0F)});
                    if (result.appliedDamage > 0)
                        onActiveHit(*found, source, cast.sequence * 8U + bolt);
                }
        }
        else if (cast.effect == spells::SpellEffect::DefensiveBarrier)
        {
            barrierShield_ = 24;
            barrierRemaining_ = 5.0F;
        }
        else if (cast.effect == spells::SpellEffect::WindPressure)
        {
            std::size_t hitIndex = 0U;
            for (auto& enemy : enemies_)
            {
                if (!enemy.health.isAlive() || !intersects(cast.effectBounds, enemy.controller.bounds())) continue;
                const float multiTargetMultiplier = relics_.has(relics::ElderSealFragmentId)
                    ? (hitIndex == 1U ? 1.12F : (hitIndex >= 2U ? 1.24F : 1.0F)) : 1.0F;
                const auto result = enemy.damageResolver.resolve(enemy.health,
                    {source, cast.sequence, 14, multiplier * multiTargetMultiplier
                        * (enemy.markedRemaining > 0.0F ? 1.12F : 1.0F)});
                if (result.appliedDamage > 0) onActiveHit(enemy, source, cast.sequence);
                if (enemy.controller.action() == ai::EnemyAction::Windup
                    || enemy.controller.action() == ai::EnemyAction::Active)
                {
                    enemy.controller.interrupt();
                    onInterrupt(enemy);
                }
                enemy.controller.translateHorizontal(player_.facingDirection() * 130.0F,
                    request_.worldBounds);
                const auto moved = enemy.controller.bounds();
                const bool hitWall = moved.left <= request_.worldBounds.left + 0.01F
                    || moved.left + moved.width >= request_.worldBounds.right - 0.01F;
                if (hitWall && enemy.health.isAlive())
                    static_cast<void>(enemy.damageResolver.resolve(enemy.health,
                        {source, cast.sequence * 8U + 7U, 10, multiplier}));
                ++hitIndex;
            }
        }
        else if (cast.effect == spells::SpellEffect::GravityWell)
        {
            gravityWellRemaining_ = extendPersistent ? 3.125F : 2.5F;
            gravityWellTickAccumulator_ = 0.0F;
            gravityWellBounds_ = cast.effectBounds;
            gravityWellSource_ = source;
            gravityWellMultiplier_ = multiplier;
            gravityWellSequenceBase_ = cast.sequence * 8U;
            gravityWellTick_ = 0U;
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
            if (fullHit) static_cast<void>(healPlayer(12));
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
                    if (enemy.goldenBindRemaining > 0.0F)
                    {
                        enemy.goldenBindRemaining = 0.0F;
                        static_cast<void>(enemy.damageResolver.resolve(enemy.health,
                            {source, cast.sequence * 16U + 13U, 16, multiplier}));
                    }
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
                damageEnemies(source, cast.sequence,
                    std::max(60, static_cast<int>(std::lround(copiedDamage * 2.0F))),
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
            damageEnemies(source, cast.sequence * 8U, 45, multiplier, cast.effectBounds);
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
                                25, multiplier}));
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
        const bool firstBoostAvailable = !firstDamageSpellUsed_;
        applyCast(cast, SpellDamageSources[slot], id ? spells::findDefinition(*id) : nullptr);
        int mirrorBaseDamage = cast.damage;
        if (cast.effect == spells::SpellEffect::BloodMagic && cast.cast)
            mirrorBaseDamage = playerHealth_.current() / 2;
        else if (cast.effect == spells::SpellEffect::MultiZoltraak) mirrorBaseDamage = 30;
        else if (cast.effect == spells::SpellEffect::HomingVolley) mirrorBaseDamage = 21;
        else if (cast.effect == spells::SpellEffect::SpatialShatter) mirrorBaseDamage = 26;
        else if (cast.effect == spells::SpellEffect::GravityWell) mirrorBaseDamage = 15;
        if (cast.cast && relics_.has(relics::HolyStaffId) && !holyStaffTriggered_[slot])
        {
            holyStaffTriggered_[slot] = true;
            for (std::size_t other = 0U; other < holyStaffTriggered_.size(); ++other)
                if (other != slot) spells_.reduceRegularCooldown(other, 0.75F);
        }
        if (cast.cast && id && relics_.has(relics::OldSpellBookmarkId))
        {
            const auto views = spells_.view();
            float longest = 0.0F;
            std::optional<std::uint32_t> longestId;
            for (const auto& view : views)
                if (view.id && view.cooldownDuration > longest)
                {
                    longest = view.cooldownDuration;
                    longestId = view.id;
                }
            const auto* definition = spells::findDefinition(*id);
            if (definition && longestId == definition->id)
                spells_.reduceRegularCooldown(slot, definition->cooldownSeconds * 0.10F);
        }
        if (cast.cast && firstBoostAvailable && firstDamageSpellUsed_
            && relics_.has(relics::ManaSuppressionEarringId) && id)
        {
            const auto* definition = spells::findDefinition(*id);
            if (definition) spells_.addRegularCooldown(slot, definition->cooldownSeconds * 0.25F);
        }
        if (cast.cast && mirrorBaseDamage > 0 && relics_.has(relics::MirrorLotusRingId)
            && !mirrorRelicUsed_)
        {
            mirrorRelicUsed_ = true;
            const int echoDamage = std::max(1, static_cast<int>(std::lround(mirrorBaseDamage * 0.40F)));
            pendingSpellImpacts_.push_back({cast.spellId, cast.effectBounds, 0.35F, echoDamage,
                cast.sequence * 32U + 23U, SpellDamageSources[slot],
                lastRegularCastMultiplier_});
        }
        if (cast.cast && mirrorCopies_ > 0U && mirrorBaseDamage > 0)
        {
            const float mirrorMultiplier = relics_.outgoingDamageMultiplier()
                * (blessingRemaining_ > 0.0F ? 1.2F : 1.0F);
            for (std::uint32_t copy = 0U; copy < mirrorCopies_; ++copy)
                damageEnemies(SpellDamageSources[slot], cast.sequence * 16U + copy + 8U,
                    static_cast<int>(std::lround(mirrorBaseDamage * 0.45F)),
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
        const auto cast = spells_.tryCastUltimate(player_.position(), player_.facingDirection(),
            firstLivingEnemyBounds());
        applyCast(cast, DamageSource::PlayerUltimateSpell,
            id ? spells::findDefinition(*id) : nullptr);
    }
    if (attack_.isActive())
    {
        damageEnemies(DamageSource::PlayerBasicAttack, attack_.sequence(), AttackDamage,
            relics_.outgoingDamageMultiplier() * (blessingRemaining_ > 0.0F ? 1.15F : 1.0F)
                * (goddessTabletDamageRemaining_ > 0.0F ? 1.10F : 1.0F), attackBounds());
        if (lightningStaffCharges_ > 0U && handledLightningAttackSequence_ != attack_.sequence())
        {
            handledLightningAttackSequence_ = attack_.sequence();
            damageEnemies(DamageSource::PlayerBasicAttack, attack_.sequence() * 16U + 7U, 7,
                relics_.outgoingDamageMultiplier()
                    * (goddessTabletDamageRemaining_ > 0.0F ? 1.10F : 1.0F), attackBounds());
            --lightningStaffCharges_;
            if (lightningStaffCharges_ == 0U)
            {
                const Aabb chainArea {playerCenter - 180.0F,
                    player_.position().y - 80.0F, 360.0F, 224.0F};
                damageEnemies(DamageSource::PlayerBasicAttack,
                    attack_.sequence() * 16U + 12U, 12,
                    relics_.outgoingDamageMultiplier()
                        * (goddessTabletDamageRemaining_ > 0.0F ? 1.10F : 1.0F), chainArea);
                lightningStaffRemaining_ = 0.0F;
            }
        }
    }

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
            if (guardRemaining_ > 0.0F) handleGuard();
            int damage = 20;
            if (config.skill == ai::EnemySkill::Thread) damage = 10;
            if (config.skill == ai::EnemySkill::Domination) damage = 0;
            if (config.skill == ai::EnemySkill::DragonClaw) damage = 25;
            if (config.skill == ai::EnemySkill::LeafBlade
                || config.skill == ai::EnemySkill::WolfClaw
                || config.skill == ai::EnemySkill::KillingMagic
                || config.skill == ai::EnemySkill::SideKick) damage = 15;
            if (config.skill == ai::EnemySkill::Blood)
                damage = (enemy.health.maximum() - enemy.health.current()) / 2;
            if (request_.enemies.empty() && enemy.archetype != EnemyArchetype::Aura
                && enemy.archetype != EnemyArchetype::RedMirrorDragon)
                damage = request_.enemyAttackDamage;
            const std::uint64_t skillSequence = (static_cast<std::uint64_t>(enemyIndex + 1U) << 32U)
                | enemy.controller.attackSequence();
            const auto result = resolvePlayerDamage(
                {DamageSource::EnemyAttack, skillSequence, damage,
                    1.0F, relics_.incomingDamageMultiplier()
                        * (cleanseProtectionRemaining_ > 0.0F ? 0.8F : 1.0F), 0,
                    player_.isDashing() || guardRemaining_ > 0.0F
                        || spellInvulnerableRemaining_ > 0.0F
                        || (player_.flightRemaining() > 0.0F
                            && config.skill == ai::EnemySkill::LeapingCleave)});
            if (result.appliedDamage > 0) beamRemaining_ = 0.0F;
            const bool dominationHit = config.skill == ai::EnemySkill::Domination
                && !player_.isDashing() && guardRemaining_ <= 0.0F
                && spellInvulnerableRemaining_ <= 0.0F;
            if ((result.appliedDamage > 0 || dominationHit) && blessingRemaining_ <= 0.0F)
            {
                if (config.skill == ai::EnemySkill::Thread)
                    player_.applyLaunch(540.0F, playerControlDuration(0.28F));
                else if (config.skill == ai::EnemySkill::Domination)
                    player_.applyHitReaction(0.0F, playerControlDuration(1.5F));
                else if (config.skill == ai::EnemySkill::Thrust)
                    player_.applyHitReaction(0.0F, playerControlDuration(1.0F));
                else player_.applyHitReaction(enemy.controller.facingDirection() * KnockbackSpeed,
                    playerControlDuration(HitStunSeconds));
            }
            else if ((result.appliedDamage > 0 || dominationHit) && blessingRemaining_ > 0.0F)
                onBlessingPreventedControl();
        }
        if (config.hasContactDamage && enemy.contactCooldown <= 0.0F
            && intersects(playerBounds(), enemy.controller.bounds()))
        {
            if (guardRemaining_ > 0.0F) handleGuard();
            const std::uint64_t contactSequence = (static_cast<std::uint64_t>(enemyIndex + 1U) << 32U)
                | ++enemy.contactSequence;
            const bool legacyDamage = request_.enemies.empty()
                && enemy.archetype != EnemyArchetype::Aura
                && enemy.archetype != EnemyArchetype::RedMirrorDragon;
            const int contactDamage = legacyDamage ? request_.enemyContactDamage : 15;
            const auto contactResult = resolvePlayerDamage(
                {DamageSource::EnemyContact, contactSequence, contactDamage,
                    1.0F, relics_.incomingDamageMultiplier()
                        * (cleanseProtectionRemaining_ > 0.0F ? 0.8F : 1.0F),
                    relics_.collisionFlatReduction(),
                    player_.isDashing() || guardRemaining_ > 0.0F
                        || spellInvulnerableRemaining_ > 0.0F});
            if (contactResult.appliedDamage > 0) beamRemaining_ = 0.0F;
            if (contactResult.appliedDamage > 0 && relics_.retaliatesOnCollision())
            {
                const auto player = playerBounds();
                const Aabb retaliation {player.left - 54.0F, player.top - 54.0F,
                    player.width + 108.0F, player.height + 108.0F};
                for (auto& target : enemies_)
                    if (target.health.isAlive() && intersects(retaliation, target.controller.bounds()))
                        static_cast<void>(target.damageResolver.resolve(target.health,
                            {DamageSource::Event, contactSequence, 30}));
            }
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
        relics_.vulnerableRemaining(), flowerFieldRemaining_, player_.flightRemaining(),
        barrierShield_ + persistentShield_};
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
        if (enemy.archetype == EnemyArchetype::Richter && enemy.specialWindup > 0.0F)
            skillBounds = enemy.specialTargetBounds;
        if (enemy.markedRemaining > 0.0F || tacticalNotesRemaining_ > 0.0F)
        {
            const auto area = enemy.controller.attackBounds();
            if (area.width > 0.0F && area.height > 0.0F) skillBounds = area;
        }
        const bool windingUp = enemy.controller.action() == ai::EnemyAction::Windup
            || enemy.breathWindup > 0.0F || enemy.specialWindup > 0.0F;
        const bool active = enemy.controller.action() == ai::EnemyAction::Active
            || enemy.breathRemaining > 0.0F;
        views.push_back({enemy.archetype, enemy.controller.position(), bounds.width, bounds.height,
            enemy.health.current(), enemy.health.maximum(), enemy.health.isAlive(),
            windingUp, active, enemy.slowed, skillBounds, enemy.controller.facingDirection(),
            enemy.markedRemaining > 0.0F});
    }
    return views;
}

std::vector<SpellEffectView> CombatSession::spellEffects() const
{
    std::vector<SpellEffectView> views;
    views.reserve(activeSpellEffects_.size() + activePillars_.size() + activeTornadoes_.size());
    for (const auto& effect : activeSpellEffects_)
        views.push_back({effect.spellId, effect.bounds, effect.remaining, effect.duration});
    for (const auto& pillar : activePillars_)
        views.push_back({9001U, pillar.bounds, pillar.remaining, 4.0F});
    for (const auto& tornado : activeTornadoes_)
        views.push_back({tornado.evolutionRemaining > 0.0F ? 9002U : 9003U,
            tornado.bounds, tornado.remaining, 7.0F});
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
    if (enemy.markedRemaining > 0.0F || tacticalNotesRemaining_ > 0.0F)
    {
        const auto area = enemy.controller.attackBounds();
        if (area.width > 0.0F && area.height > 0.0F) skillBounds = area;
    }
    return {enemy.archetype, enemy.controller.position(), bounds.width, bounds.height,
        enemy.health.current(), enemy.health.maximum(), enemy.health.isAlive(),
        enemy.controller.action() == ai::EnemyAction::Windup || enemy.breathWindup > 0.0F,
        enemy.controller.action() == ai::EnemyAction::Active || enemy.breathRemaining > 0.0F,
        enemy.slowed, skillBounds, enemy.controller.facingDirection(),
        enemy.markedRemaining > 0.0F};
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
    const int flawlessGold = outcome == CombatOutcome::Victory && !actualHpLost_
        && relics_.has(relics::OldCopperCoinId) ? 10 : 0;
    result_ = {outcome, request_.encounterId,
        outcome == CombatOutcome::Victory ? static_cast<int>(enemies_.size()) : 0,
        outcome == CombatOutcome::Victory ? request_.goldReward + flawlessGold : 0,
        playerHealth_.current()};
}
}
