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
constexpr bool isPlayerMagic(const DamageSource source) noexcept
{
    return source == DamageSource::PlayerSpell0 || source == DamageSource::PlayerSpell1
        || source == DamageSource::PlayerSpell2 || source == DamageSource::PlayerUltimateSpell;
}

constexpr bool isPlayerDamage(const DamageSource source) noexcept
{
    return source == DamageSource::PlayerBasicAttack || isPlayerMagic(source);
}

constexpr float fullClipDuration(const std::uint32_t frameCount, const float framesPerSecond) noexcept
{
    return static_cast<float>(frameCount) / framesPerSecond;
}

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
        return EnemyConfig {240.0F, 42.0F, 42.0F, 0.5F, 0.6F, 0.0F, 0.0F,
            42.0F, 58.0F, 2.0F, true, false, EnemySkill::Slash};
    case EnemyArchetype::BirdDemon:
        return EnemyConfig {180.0F, 480.0F, 480.0F, 0.5F, 480.0F / 320.0F, 0.0F, 0.0F,
            42.0F, 32.0F, 6.0F, false, true, EnemySkill::Dive};
    case EnemyArchetype::Lugner:
        return EnemyConfig {160.0F, 88.0F, 88.0F, 0.5F, 0.6F, 0.0F, 0.0F,
            42.0F, 72.0F, 5.0F, false, false, EnemySkill::Blood};
    case EnemyArchetype::Linie:
        return EnemyConfig {180.0F, 84.0F, 84.0F, 0.5F, 1.9F, 0.0F, 144.0F,
            42.0F, 64.0F, 4.0F, true, false, EnemySkill::LeapingCleave};
    case EnemyArchetype::Draht:
        return EnemyConfig {160.0F, 96.0F, 96.0F, 0.5F, 0.6F, 0.0F, 0.0F,
            42.0F, 64.0F, 8.0F, false, false, EnemySkill::Thread};
    case EnemyArchetype::ChaosFlower:
        return EnemyConfig {0.0F, 84.0F, 84.0F, 0.5F, 0.6F, 0.0F, 0.0F,
            64.0F, 84.0F, 4.0F, true, false, EnemySkill::LeafBlade};
    case EnemyArchetype::FrostWolf:
        return EnemyConfig {240.0F, 64.0F, 64.0F, 0.5F, 0.6F, 0.0F, 0.0F,
            64.0F, 42.0F, 4.0F, true, false, EnemySkill::WolfClaw};
    case EnemyArchetype::Qual:
        return EnemyConfig {120.0F, 96.0F, 96.0F, 0.5F, 0.6F, 0.0F, 0.0F,
            64.0F, 96.0F, 3.0F, true, false, EnemySkill::KillingMagic};
    case EnemyArchetype::Laufen:
        return EnemyConfig {180.0F, 64.0F, 64.0F, 0.5F, 0.6F, 0.0F, 0.0F,
            42.0F, 64.0F, 3.0F, false, false, EnemySkill::SideKick};
    case EnemyArchetype::Richter:
        return EnemyConfig {120.0F, 84.0F, 84.0F, 0.5F, 0.6F, 0.0F, 0.0F,
            42.0F, 72.0F, 6.0F, false, false, EnemySkill::EarthMagic};
    case EnemyArchetype::Denken:
        return EnemyConfig {100.0F, 120.0F, 120.0F, 0.5F, 0.6F, 0.0F, 0.0F,
            42.0F, 64.0F, 9.0F, false, false, EnemySkill::TornadoMagic};
    case EnemyArchetype::Heimon:
        return EnemyConfig {160.0F, 96.0F, 96.0F, 0.5F, 0.6F, 0.0F, 0.0F,
            42.0F, 72.0F, 5.0F, false, false, EnemySkill::FogAttack};
    case EnemyArchetype::DemonWarrior:
        return EnemyConfig {180.0F, 124.0F, 42.0F, 0.5F, 0.6F, 0.0F, 0.0F,
            42.0F, 64.0F, 5.0F, true, false, EnemySkill::WhirlwindSlash};
    case EnemyArchetype::LargeBirdDemon:
        return EnemyConfig {160.0F, 144.0F, 144.0F, 0.5F, 4.0F, 0.0F, 0.0F,
            54.0F, 42.0F, 6.0F, true, true, EnemySkill::Swoop};
    case EnemyArchetype::Gargoyle:
        return EnemyConfig {100.0F, 240.0F, 240.0F, 0.5F, 0.6F, 0.0F, 0.0F,
            42.0F, 42.0F, 4.0F, false, false, EnemySkill::BossAttack};
    case EnemyArchetype::ThreeHeadedDemon:
        return EnemyConfig {120.0F, 64.0F, 64.0F, 0.5F, 0.6F, 0.0F, 0.0F,
            84.0F, 64.0F, 4.0F, true, false, EnemySkill::BossAttack};
    case EnemyArchetype::SwordDemon:
        return EnemyConfig {160.0F, 180.0F, 54.0F, 0.5F, 0.6F, 0.0F, 0.0F,
            42.0F, 72.0F, 3.0F, false, false, EnemySkill::BossAttack};
    case EnemyArchetype::Revolte:
        return EnemyConfig {180.0F, 64.0F, 64.0F, 0.5F, 0.6F, 0.0F, 0.0F,
            64.0F, 96.0F, 7.0F, true, false, EnemySkill::BossAttack};
    case EnemyArchetype::Aura:
        return EnemyConfig {120.0F, 420.0F, 420.0F, 0.8F, 0.2F, 0.0F, 0.0F,
            42.0F, 64.0F, 7.0F, false, false, EnemySkill::Domination};
    case EnemyArchetype::RedMirrorDragon:
        return EnemyConfig {96.0F, 72.0F, 72.0F, 0.5F, 0.6F, 0.0F, 0.0F,
            128.0F, 84.0F, 7.0F, true, false, EnemySkill::DragonClaw};
    case EnemyArchetype::WaterMirrorDemon:
        return EnemyConfig {0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F,
            84.0F, 84.0F, 999.0F, false, true, EnemySkill::BossAttack};
    case EnemyArchetype::StarkCopy:
        return EnemyConfig {200.0F, 54.0F, 54.0F, 0.4F, 0.6F, 0.0F, 0.0F,
            54.0F, 72.0F, 4.0F, false, false, EnemySkill::BossAttack};
    case EnemyArchetype::FernCopy:
        return EnemyConfig {120.0F, 256.0F, 256.0F, 0.4F, 0.6F, 0.0F, 0.0F,
            54.0F, 72.0F, 6.0F, false, false, EnemySkill::BossAttack};
    case EnemyArchetype::FrierenCopy:
        return EnemyConfig {100.0F, 256.0F, 256.0F, 0.3F, 0.6F, 0.0F, 0.0F,
            54.0F, 72.0F, 3.0F, false, true, EnemySkill::BossAttack};
    case EnemyArchetype::Boss:
        return EnemyConfig {125.0F, 72.0F, 90.0F, 0.55F, 0.16F, 0.0F, 20.0F,
            48.0F, 64.0F, 1.3F, true, false, EnemySkill::BossAttack};
    }
    return {};
}

CombatSession::CombatSession(CombatRequest request)
    : request_(std::move(request)), player_(request_.playerSpawn),
      playerHealth_(request_.playerMaximumHealth, request_.playerCurrentHealth),
      spells_(request_.equippedSpellIds, request_.equippedUltimateSpellId,
          request_.equippedSpellRanks, request_.regularSpellCooldownMultiplier),
      relics_(request_.relicIds)
{
    persistentShield_ = std::min(request_.startingShield, playerHealth_.maximum() * 3 / 10);
    if (relics_.has(relics::FlammeNotesId)) tacticalNotesRemaining_ = 8.0F;
    if (relics_.has(relics::SeriePageId)) spells_.setUltimateCooldown(15.3F);
    auto spawns = request_.enemies;
    if (spawns.empty()) spawns.push_back({request_.enemyArchetype, request_.enemySpawn});
    enemies_.reserve(spawns.size() + 3U);
    for (const auto& spawn : spawns)
    {
        const auto config = enemyConfigFor(spawn.archetype);
        Vec2 position = spawn.position;
        if (spawn.movementBounds)
            position.y = spawn.movementBounds->groundTop - config.height;
        else if (!request_.enemies.empty() || spawn.archetype == EnemyArchetype::Aura
            || spawn.archetype == EnemyArchetype::Revolte
            || spawn.archetype == EnemyArchetype::RedMirrorDragon
            || spawn.archetype == EnemyArchetype::WaterMirrorDemon)
            position.y = request_.worldBounds.groundTop - config.height;
        if (config.flying)
        {
            const float defaultHoverHeight = spawn.archetype == EnemyArchetype::LargeBirdDemon
                ? 144.0F
                : (spawn.archetype == EnemyArchetype::FrierenCopy
                        ? FrierenCopyHoverHeight : 132.0F);
            position.y = spawn.flyingPlacement
                ? spawn.position.y - config.height * 0.5F
                : request_.worldBounds.groundTop - defaultHoverHeight - config.height;
        }
        if (spawn.archetype == EnemyArchetype::WaterMirrorDemon)
        {
            position.x = (request_.worldBounds.left + request_.worldBounds.right
                - config.width) * 0.5F;
            position.y = request_.worldBounds.groundTop - 144.0F - config.height;
        }
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
            case EnemyArchetype::Heimon: return 125;
            case EnemyArchetype::DemonWarrior: return 150;
            case EnemyArchetype::LargeBirdDemon: return 100;
            case EnemyArchetype::Gargoyle: return 125;
            case EnemyArchetype::ThreeHeadedDemon: return 180;
            case EnemyArchetype::SwordDemon: return 100;
            case EnemyArchetype::Aura: return 225;
            case EnemyArchetype::Revolte: return 300;
            case EnemyArchetype::RedMirrorDragon: return 300;
            case EnemyArchetype::WaterMirrorDemon: return 200;
            case EnemyArchetype::StarkCopy: return 300;
            case EnemyArchetype::FernCopy: return 180;
            case EnemyArchetype::FrierenCopy: return 350;
            case EnemyArchetype::Boss: return request_.enemyMaximumHealth;
            }
            return 1;
        }();
        enemies_.push_back({spawn.archetype, ai::EnemyController(position, config), Health(health, health),
            {}, 0.0F, 0U, 0U, false, spawn.archetype == EnemyArchetype::Aura ? 12.0F : 0.0F, 0U});
        enemies_.back().movementBounds = spawn.movementBounds;
        if (spawn.archetype == EnemyArchetype::RedMirrorDragon)
            enemies_.back().breathCooldown = 6.0F;
        if (spawn.archetype == EnemyArchetype::ChaosFlower)
            enemies_.back().specialCooldown = 3.5F;
        if (spawn.archetype == EnemyArchetype::Aura)
            enemies_.back().secondaryCooldown = 5.0F;
        if (spawn.archetype == EnemyArchetype::FrostWolf)
            enemies_.back().specialCooldown = 3.0F;
        if (spawn.archetype == EnemyArchetype::Laufen)
            enemies_.back().specialCooldown = 2.5F;
        if (spawn.archetype == EnemyArchetype::Richter)
            enemies_.back().specialCooldown = 3.0F;
        if (spawn.archetype == EnemyArchetype::Denken)
            enemies_.back().specialCooldown = 4.5F;
        if (spawn.archetype == EnemyArchetype::Heimon)
            enemies_.back().specialCooldown = 1.5F;
        if (spawn.archetype == EnemyArchetype::Revolte)
            enemies_.back().revolteCooldowns = {3.0F, 3.0F, 3.75F, 4.0F, 2.5F, 7.0F};
        if (spawn.archetype == EnemyArchetype::Gargoyle)
            enemies_.back().specialCooldown = 2.0F;
        if (spawn.archetype == EnemyArchetype::ThreeHeadedDemon)
        {
            enemies_.back().specialCooldown = 3.5F;
            enemies_.back().secondaryCooldown = 2.0F;
        }
        if (spawn.archetype == EnemyArchetype::SwordDemon)
        {
            enemies_.back().specialCooldown = 2.0F;
            enemies_.back().secondaryCooldown = 1.5F;
        }
        if (spawn.archetype == EnemyArchetype::StarkCopy)
            enemies_.back().revolteCooldowns = {4.0F, 2.0F, 0.0F, 0.0F, 0.0F};
        if (spawn.archetype == EnemyArchetype::FernCopy)
            enemies_.back().specialCooldown = 3.0F;
        if (spawn.archetype == EnemyArchetype::FrierenCopy)
            enemies_.back().revolteCooldowns = {4.0F, 4.0F, 1.5F, 0.0F, 0.0F};
    }
    const auto waterMirror = std::find_if(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
        return enemy.archetype == EnemyArchetype::WaterMirrorDemon;
    });
    if (waterMirror != enemies_.end())
    {
        const auto spawnCopy = [&](const EnemyArchetype archetype, const float x, const int hp) {
            const auto config = enemyConfigFor(archetype);
            const Vec2 position {x, request_.worldBounds.groundTop - config.height};
            enemies_.push_back({archetype, ai::EnemyController(position, config), Health(hp, hp)});
        };
        const bool preview = waterMirror->health.maximum() != 200;
        spawnCopy(EnemyArchetype::StarkCopy, 720.0F,
            preview ? waterMirror->health.maximum() : 300);
        spawnCopy(EnemyArchetype::FernCopy, 980.0F,
            preview ? waterMirror->health.maximum() : 180);
        enemies_[enemies_.size() - 2U].revolteCooldowns = {4.0F, 2.0F, 0.0F, 0.0F, 0.0F};
        enemies_.back().specialCooldown = 3.0F;
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
    if (std::any_of(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
        return enemy.archetype == EnemyArchetype::Aura
            || enemy.archetype == EnemyArchetype::Revolte
            || enemy.archetype == EnemyArchetype::WaterMirrorDemon;
    })) bossIntroRemaining_ = 2.4F;
}

DamageResult CombatSession::resolvePlayerDamage(DamageRequest request) noexcept
{
    request.blocked = request.blocked || playerHitInvulnerabilityRemaining_ > 0.0F;
    const int beforeBarrier = barrierShield_;
    const int beforePersistent = persistentShield_;
    const int beforeShield = beforeBarrier + beforePersistent;
    if (beforeShield > 0) request.flatReduction += beforeShield;
    const int incoming = std::max(0, static_cast<int>(std::lround(
        static_cast<float>(request.baseDamage) * request.sourceMultiplier * request.targetMultiplier))
        - (request.flatReduction - beforeShield));
    const int absorbed = request.blocked ? 0 : std::min(beforeShield, incoming);
    const auto result = playerDamageResolver_.resolve(playerHealth_, request);
    if (result.appliedDamage > 0)
    {
        actualHpLost_ = true;
        playerHitInvulnerabilityRemaining_ = PlayerHitInvulnerabilitySeconds;
        ++playerHurtSequence_;
    }
    const int barrierAbsorbed = std::min(beforeBarrier, absorbed);
    barrierShield_ -= barrierAbsorbed;
    persistentShield_ -= absorbed - barrierAbsorbed;
    if (beforeBarrier > 0 && barrierShield_ <= 0)
    {
        barrierShield_ = 0;
        barrierRemaining_ = 0.0F;
        const auto player = playerBounds();
        std::erase_if(activeSpellEffects_, [](const auto& effect) {
            return effect.spellId == 1028U;
        });
        activeSpellEffects_.push_back({DefensiveBarrierBreakVisualId, player,
            fullClipDuration(2U, 12.0F), fullClipDuration(2U, 12.0F)});
        const Aabb burst {player.left - 80.0F, player.top - 48.0F,
            player.width + 160.0F, player.height + 96.0F};
        for (auto& enemy : enemies_)
            if (enemy.health.isAlive() && intersects(burst, enemy.controller.bounds()))
                    static_cast<void>(resolveEnemyDamage(enemy,
                    {DamageSource::Event, ++environmentalSequence_, barrierBreakDamage_}));
    }
    return result;
}

DamageResult CombatSession::resolveEnemyDamage(EnemyRuntime& enemy,
    DamageRequest request) noexcept
{
    if (isPlayerDamage(request.source))
    {
        request.sourceMultiplier *= request_.playerDamageMultiplier;
        if (enemy.exposedRemaining > 0.0F) request.targetMultiplier *= 1.25F;
        if (enemy.markedRemaining > 0.0F) request.targetMultiplier *= 1.12F;
    }
    if (sleepRemaining_ > 0.0F && isPlayerDamage(request.source))
        request.sourceMultiplier *= 0.7F;
    if (enemy.concealmentProgress >= 1.0F) request.blocked = true;
    if (enemy.archetype == EnemyArchetype::Revolte && enemy.revolteTransitionPending)
        request.blocked = true;
    if (enemy.archetype == EnemyArchetype::WaterMirrorDemon)
        request.blocked = true;
    const bool parryingMagic = enemy.archetype == EnemyArchetype::Revolte
        && enemy.revolteSkill == 3 && enemy.specialActive > 0.0F
        && isPlayerMagic(request.source);
    if (parryingMagic) request.targetMultiplier *= 0.5F;

    if (enemy.archetype == EnemyArchetype::Revolte && !enemy.revolteSecondPhase
        && !enemy.revolteTransitionPending && !request.blocked && request.baseDamage > 0)
    {
        const int predicted = std::max(0, static_cast<int>(std::lround(
            static_cast<float>(request.baseDamage) * request.sourceMultiplier
                * request.targetMultiplier)) - std::max(0, request.flatReduction));
        if (predicted > enemy.health.current() - 5)
        {
            DamageRequest registered = request;
            registered.blocked = true;
            auto result = enemy.damageResolver.resolve(enemy.health, registered);
            result.blocked = false;
            result.resolvedDamage = std::max(0, enemy.health.current() - 5);
            result.appliedDamage = enemy.health.damage(result.resolvedDamage);
            result.healthAfter = enemy.health.current();
            enemy.revolteTransitionPending = true;
            enemy.specialActive = 0.0F;
            enemy.revolteSkill = -1;
            beginDialogue(DialogueScript::RevolteSecondPhase);
            return result;
        }
    }

    const auto result = enemy.damageResolver.resolve(enemy.health, request);
    if (enemy.archetype == EnemyArchetype::StarkCopy && result.appliedDamage > 0)
    {
        enemy.revolteCooldowns[0] = std::max(0.0F, enemy.revolteCooldowns[0] - 1.0F);
        enemy.revolteCooldowns[1] = std::max(0.0F, enemy.revolteCooldowns[1] - 1.0F);
    }
    if (parryingMagic && result.appliedDamage > 0)
    {
        enemy.revolteCounterDashPending = true;
        enemy.specialActive = 0.0F;
        enemy.revolteSkill = -1;
    }
    if (enemy.archetype == EnemyArchetype::SwordDemon && enemy.health.isAlive()
        && result.appliedDamage > 0 && isPlayerMagic(request.source))
    {
        enemy.manualSkill = 0;
        enemy.specialWindup = 0.0F;
        enemy.specialActive = 144.0F / 320.0F;
        enemy.specialDirection = enemy.controller.facingDirection();
        enemy.specialCooldown = 4.0F;
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
    if (bossIntroRemaining_ > 0.0F)
    {
        bossIntroRemaining_ = std::max(0.0F, bossIntroRemaining_ - deltaSeconds);
        if (bossIntroRemaining_ <= 0.0F)
        {
            const bool revolte = std::any_of(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
                return enemy.archetype == EnemyArchetype::Revolte;
            });
            const bool waterMirror = std::any_of(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
                return enemy.archetype == EnemyArchetype::WaterMirrorDemon;
            });
            beginDialogue(waterMirror ? DialogueScript::WaterMirrorPreBattle
                : (revolte ? DialogueScript::RevoltePreBattle
                           : DialogueScript::AuraPreBattle));
        }
        return;
    }
    if (dialogueScript_ != DialogueScript::None)
    {
        if (intent.menuConfirmPressed) advanceDialogue();
        return;
    }
    if (result_)
    {
        if (result_->outcome == CombatOutcome::Victory)
        {
            attack_.update(deltaSeconds);
            player_.update(intent, deltaSeconds, request_.worldBounds,
                request_.oneWayPlatforms);
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
    playerHitInvulnerabilityRemaining_ = std::max(
        0.0F, playerHitInvulnerabilityRemaining_ - deltaSeconds);
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
                    static_cast<void>(resolveEnemyDamage(enemy,
                        {burningFlowerSource_, burningFlowerSequenceBase_ + burningFlowerTick_,
                            8, burningFlowerMultiplier_}));
        }
    }
    const float previousPhantomRemaining = phantomRemaining_;
    phantomRemaining_ = std::max(0.0F, phantomRemaining_ - deltaSeconds);
    if (previousPhantomRemaining > 0.0F && phantomRemaining_ <= 0.0F && phantomExpiryBurst_)
    {
        const Aabb burst {phantomBounds_.left - 54.0F, phantomBounds_.top - 48.0F,
            phantomBounds_.width + 108.0F, phantomBounds_.height + 96.0F};
        for (auto& enemy : enemies_)
            if (enemy.health.isAlive() && intersects(burst, enemy.controller.bounds()))
                static_cast<void>(resolveEnemyDamage(enemy,
                    {DamageSource::Event, ++environmentalSequence_, 18,
                        phantomDamageMultiplier_}));
        phantomExpiryBurst_ = false;
    }
    postDashComboRemaining_ = std::max(0.0F, postDashComboRemaining_ - deltaSeconds);
    spellInvulnerableRemaining_ = std::max(0.0F, spellInvulnerableRemaining_ - deltaSeconds);
    sleepRemaining_ = std::max(0.0F, sleepRemaining_ - deltaSeconds);
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
                * gravityWellPullSpeed_ * activeDelta, movementBoundsFor(enemy));
        }
        while (gravityWellTickAccumulator_ >= 1.0F)
        {
            gravityWellTickAccumulator_ -= 1.0F;
            ++gravityWellTick_;
            for (auto& enemy : enemies_)
                if (enemy.health.isAlive() && intersects(gravityWellBounds_, enemy.controller.bounds()))
                    static_cast<void>(resolveEnemyDamage(enemy,
                        {gravityWellSource_, gravityWellSequenceBase_ + gravityWellTick_,
                            gravityWellTickDamage_, gravityWellMultiplier_}));
        }
    }
    const float previousGolemRemaining = golemRemaining_;
    golemRemaining_ = std::max(0.0F, golemRemaining_ - deltaSeconds);
    if (previousGolemRemaining > 0.0F && golemRemaining_ <= 0.0F)
    {
        std::erase_if(activeSpellEffects_, [](const auto& effect) {
            return effect.spellId == 1022U;
        });
        activeSpellEffects_.push_back({StoneGolemResolveVisualId, golemBounds_,
            fullClipDuration(4U, 10.0F), fullClipDuration(4U, 10.0F)});
        const auto found = std::find_if(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
            return enemy.health.isAlive();
        });
        if (found != enemies_.end())
            static_cast<void>(resolveEnemyDamage(*found,
                {golemSource_, golemSequence_ * 16U + 1U, 20, golemMultiplier_}));
    }
    for (auto& effect : activeSpellEffects_)
        effect.remaining = std::max(0.0F, effect.remaining - deltaSeconds);
    std::erase_if(activeSpellEffects_, [](const auto& effect) { return effect.remaining <= 0.0F; });
    for (auto& impact : pendingSpellImpacts_)
    {
        impact.delayRemaining -= deltaSeconds;
        if (impact.delayRemaining > 0.0F) continue;
        for (std::size_t enemyIndex = 0U; enemyIndex < enemies_.size(); ++enemyIndex)
        {
            auto& enemy = enemies_[enemyIndex];
            if (enemy.health.isAlive() && enemy.concealmentProgress < 1.0F
                && intersects(impact.bounds, enemy.controller.bounds()))
            {
                const bool frozenAtCast = std::find(impact.frozenEnemyIndices.begin(),
                    impact.frozenEnemyIndices.end(), enemyIndex)
                    != impact.frozenEnemyIndices.end();
                static_cast<void>(resolveEnemyDamage(enemy,
                    {impact.source, impact.sequence,
                        impact.damage + (frozenAtCast ? impact.frozenBonusDamage : 0),
                        impact.multiplier}));
            }
        }
    }
    std::erase_if(pendingSpellImpacts_, [](const auto& impact) {
        return impact.delayRemaining <= 0.0F;
    });
    const bool wasDashing = player_.isDashing();
    const Vec2 beforePlayerUpdate = player_.position();
    PlayerIntent playerIntent = intent;
    if (sleepRemaining_ > 0.0F) playerIntent.moveAxis *= 0.8F;
    if (beamRemaining_ > 0.0F)
    {
        playerIntent.moveAxis *= 0.3F;
        playerIntent.jumpPressed = false;
        playerIntent.dashPressed = false;
    }
    player_.update(playerIntent, deltaSeconds, request_.worldBounds,
        request_.oneWayPlatforms);
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
        const bool evolvesNow = tornado.evolutionRemaining > 0.0F
            && tornado.evolutionRemaining - activeDelta <= 0.0F;
        tornado.evolutionRemaining -= activeDelta;
        if (evolvesNow)
        {
            const auto caster = std::find_if(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
                return enemy.archetype == EnemyArchetype::Denken && enemy.health.isAlive();
            });
            if (caster != enemies_.end()) caster->specialActive = 0.6F;
        }
        const float tornadoCenter = tornado.bounds.left + tornado.bounds.width * 0.5F;
        const float currentPlayerCenter = player_.position().x + PlayerController::Width * 0.5F;
        tornado.bounds.left += std::clamp(currentPlayerCenter - tornadoCenter,
            -120.0F * activeDelta, 120.0F * activeDelta);
        const float movedCenter = tornado.bounds.left + tornado.bounds.width * 0.5F;
        player_.translateHorizontal((movedCenter > currentPlayerCenter ? 1.0F : -1.0F)
            * 80.0F * activeDelta, request_.worldBounds);
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
                        player_.isShadowDashing() || spellInvulnerableRemaining_ > 0.0F}));
            }
        }
        else tornado.tickAccumulator = 0.0F;
    }
    std::erase_if(activeTornadoes_, [](const auto& tornado) { return tornado.remaining <= 0.0F; });
    for (auto& projectile : activeEnemyProjectiles_)
    {
        float homingDelta = deltaSeconds;
        if (projectile.homingDelayRemaining > 0.0F)
        {
            const float consumed = std::min(homingDelta, projectile.homingDelayRemaining);
            projectile.homingDelayRemaining -= consumed;
            homingDelta -= consumed;
        }
        if (projectile.tracksPlayer && homingDelta > 0.0F)
        {
            const auto target = playerBounds();
            const float targetCenterX = target.left + target.width * 0.5F;
            const float targetCenterY = target.top + target.height * 0.5F;
            const float projectileCenterX = projectile.bounds.left
                + projectile.bounds.width * 0.5F;
            const float projectileCenterY = projectile.bounds.top
                + projectile.bounds.height * 0.5F;
            const float desiredAngle = std::atan2(
                targetCenterY - projectileCenterY, targetCenterX - projectileCenterX);
            float currentAngle = std::atan2(projectile.velocity.y, projectile.velocity.x);
            float angleDifference = desiredAngle - currentAngle;
            constexpr float Pi = 3.14159265358979323846F;
            while (angleDifference > Pi) angleDifference -= Pi * 2.0F;
            while (angleDifference < -Pi) angleDifference += Pi * 2.0F;
            const float maximumTurn = projectile.homingTurnRateRadians * homingDelta;
            currentAngle += std::clamp(angleDifference, -maximumTurn, maximumTurn);
            projectile.velocity = {std::cos(currentAngle) * projectile.speed,
                std::sin(currentAngle) * projectile.speed};
            if (std::abs(projectile.velocity.x) > 0.01F)
                projectile.direction = projectile.velocity.x > 0.0F ? 1.0F : -1.0F;
        }
        const float travel = std::min(projectile.remainingDistance, projectile.speed * deltaSeconds);
        if (projectile.tracksPlayer || projectile.usesVelocity)
        {
            const float ratio = projectile.speed > 0.0F ? travel / projectile.speed : 0.0F;
            projectile.bounds.left += projectile.velocity.x * ratio;
            projectile.bounds.top += projectile.velocity.y * ratio;
            if (projectile.tracksPlayer)
                projectile.bounds.top = std::clamp(projectile.bounds.top, 0.0F,
                    request_.worldBounds.groundTop - projectile.bounds.height);
        }
        else projectile.bounds.left += projectile.direction * travel;
        projectile.remainingDistance -= travel;
        if (intersects(projectile.bounds, playerBounds()))
        {
            const auto result = resolvePlayerDamage({DamageSource::EnemyAttack,
                projectile.sequence, projectile.damage, 1.0F,
                relics_.incomingDamageMultiplier(), 0,
                player_.isShadowDashing() || spellInvulnerableRemaining_ > 0.0F});
            if (result.appliedDamage > 0)
                player_.applyHitReaction(projectile.direction * KnockbackSpeed,
                    playerControlDuration(HitStunSeconds));
            projectile.remainingDistance = 0.0F;
        }
    }
    std::erase_if(activeEnemyProjectiles_, [](const auto& projectile) {
        return projectile.remainingDistance <= 0.0F;
    });
    for (auto& beam : activeEnemyBeams_)
    {
        const float previousElapsed = std::max(0.0F, beam.duration - beam.remaining);
        beam.remaining = std::max(0.0F, beam.remaining - deltaSeconds);
        if (!beam.propagatesFromCaster || beam.damageResolved) continue;

        const float elapsed = std::max(0.0F, beam.duration - beam.remaining);
        const float pathDx = beam.end.x - beam.start.x;
        const float pathDy = beam.end.y - beam.start.y;
        const float pathLength = std::max(0.001F, std::sqrt(pathDx * pathDx + pathDy * pathDy));
        const float inversePathLength = 1.0F / pathLength;
        const float previousTailDistance = std::clamp(
            previousElapsed * beam.travelSpeed - beam.maximumVisualLength,
            0.0F, pathLength);
        const float currentHeadDistance = std::clamp(
            elapsed * beam.travelSpeed, 0.0F, pathLength);
        const Vec2 sweptStart {beam.start.x + pathDx * inversePathLength * previousTailDistance,
            beam.start.y + pathDy * inversePathLength * previousTailDistance};
        const Vec2 sweptEnd {beam.start.x + pathDx * inversePathLength * currentHeadDistance,
            beam.start.y + pathDy * inversePathLength * currentHeadDistance};
        if (currentHeadDistance <= previousTailDistance
            || !segmentIntersectsAabb(sweptStart, sweptEnd, playerBounds(),
                beam.visualThickness * 0.5F)) continue;

        const float direction = beam.end.x >= beam.start.x ? 1.0F : -1.0F;
        const auto result = resolvePlayerDamage({DamageSource::EnemyAttack,
            beam.sequence, beam.damage, 1.0F, relics_.incomingDamageMultiplier(), 0,
            player_.isShadowDashing() || spellInvulnerableRemaining_ > 0.0F});
        if (result.appliedDamage > 0)
            player_.applyHitReaction(direction * KnockbackSpeed,
                playerControlDuration(HitStunSeconds));
        beam.damageResolved = true;
    }
    std::erase_if(activeEnemyBeams_, [](const auto& beam) { return beam.remaining <= 0.0F; });
    for (auto& storm : activeEnemyLightningStorms_)
    {
        storm.nextStrikeRemaining -= deltaSeconds;
        while (storm.strikesRemaining > 0U && storm.nextStrikeRemaining <= 0.0F)
        {
            const auto target = playerBounds();
            pendingEnemyLightning_.push_back({{target.left + target.width * 0.5F - 42.0F,
                request_.worldBounds.groundTop - FrierenCopyLightningHeight,
                84.0F, FrierenCopyLightningHeight},
                0.4F + deltaSeconds, ++environmentalSequence_});
            --storm.strikesRemaining;
            storm.nextStrikeRemaining += 0.8F;
        }
    }
    std::erase_if(activeEnemyLightningStorms_, [](const auto& storm) {
        return storm.strikesRemaining == 0U;
    });
    for (auto& lightning : pendingEnemyLightning_)
    {
        lightning.delayRemaining -= deltaSeconds;
        if (lightning.delayRemaining <= 0.0F && lightning.delayRemaining + deltaSeconds > 0.0F)
        {
            activeSpellEffects_.push_back(
                {lightning.impactVisualId, lightning.bounds,
                    lightning.impactDuration, lightning.impactDuration});
            if (intersects(lightning.bounds, playerBounds()))
                static_cast<void>(resolvePlayerDamage({DamageSource::EnemyAttack,
                    lightning.sequence, lightning.damage, 1.0F,
                    relics_.incomingDamageMultiplier(), 0,
                    player_.isShadowDashing() || spellInvulnerableRemaining_ > 0.0F}));
        }
    }
    std::erase_if(pendingEnemyLightning_, [](const auto& lightning) {
        return lightning.delayRemaining <= 0.0F;
    });
    for (auto& fire : activeEnemyGroundFire_)
    {
        const float activeDelta = std::min(deltaSeconds, fire.remaining);
        fire.remaining -= activeDelta;
        fire.tickAccumulator += activeDelta;
        while (fire.tickAccumulator >= 0.5F)
        {
            fire.tickAccumulator -= 0.5F;
            if (intersects(fire.bounds, playerBounds()))
                static_cast<void>(resolvePlayerDamage({DamageSource::EnemyAttack,
                    ++fire.sequence, 5, 1.0F, relics_.incomingDamageMultiplier(), 0,
                    player_.isShadowDashing() || spellInvulnerableRemaining_ > 0.0F}));
        }
    }
    std::erase_if(activeEnemyGroundFire_, [](const auto& fire) { return fire.remaining <= 0.0F; });
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
        const bool shadowDash = player_.isShadowDashing();
        bool crossedEnemy = false;
        for (auto& enemy : enemies_)
            if (enemy.health.isAlive() && intersects(dashPath, enemy.controller.bounds()))
            {
                if (shadowDash) enemy.markedRemaining = std::max(enemy.markedRemaining, 2.0F);
                if (shadowDash && relics_.has(relics::BrokenDashFormulaId))
                {
                    static_cast<void>(resolveEnemyDamage(enemy,
                        {DamageSource::Event, ++environmentalSequence_, 20}));
                    crossedEnemy = true;
                }
            }
        if (crossedEnemy) spells_.reduceLongestRegularCooldown(0.5F);
    }
    const float playerCenter = player_.position().x + PlayerController::Width * 0.5F;
    const bool canAct = !player_.isStunned() && !player_.isDashing()
        && beamRemaining_ <= 0.0F;

    if (!updateEnemySkills(deltaSeconds, playerCenter)) return;
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
                    * 24.0F * activeDelta, movementBoundsFor(enemy));
            }
        while (hellfireTickAccumulator_ >= 0.5F)
        {
            hellfireTickAccumulator_ -= 0.5F;
            ++hellfireTick_;
            const int tickDamage = hellfireTick_ <= 4U ? 15 : 14;
            for (auto& enemy : enemies_)
                if (enemy.health.isAlive() && intersects(hellfireBounds_, enemy.controller.bounds()))
                    static_cast<void>(resolveEnemyDamage(enemy,
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
                    static_cast<void>(resolveEnemyDamage(enemy,
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
        flowerHealingAccumulator_ += flowerHealingRate_ * deltaSeconds;
        const int healing = static_cast<int>(flowerHealingAccumulator_);
        if (healing > 0)
        {
            static_cast<void>(healPlayer(healing));
            flowerHealingAccumulator_ -= static_cast<float>(healing);
        }
    }
    if (intent.attackPressed && canAct) attack_.tryStart();

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
            Aabb mirrorBounds = playerBounds();
            const auto mirrorEffect = std::find_if(activeSpellEffects_.begin(),
                activeSpellEffects_.end(), [](const auto& effect) {
                    return effect.spellId == 2012U;
                });
            if (mirrorEffect != activeSpellEffects_.end()) mirrorBounds = mirrorEffect->bounds;
            std::erase_if(activeSpellEffects_, [](const auto& effect) {
                return effect.spellId == 2012U;
            });
            activeSpellEffects_.push_back({MirrorArrayBreakVisualId, mirrorBounds,
                fullClipDuration(3U, 12.0F), fullClipDuration(3U, 12.0F)});
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
                lightningStaffDamageMultiplier_, attackBounds());
            --lightningStaffCharges_;
            if (lightningStaffCharges_ == 0U)
            {
                const Aabb chainArea {playerCenter - 180.0F,
                    player_.position().y - 80.0F, 360.0F, 224.0F};
                damageEnemies(DamageSource::PlayerBasicAttack,
                    attack_.sequence() * 16U + 12U, lightningStaffBurstDamage_,
                    lightningStaffDamageMultiplier_, chainArea);
                lightningStaffRemaining_ = 0.0F;
                std::erase_if(activeSpellEffects_, [](const auto& effect) {
                    return effect.spellId == 1026U;
                });
                activeSpellEffects_.push_back({LightningStaffDischargeVisualId, chainArea,
                    fullClipDuration(2U, 12.0F), fullClipDuration(2U, 12.0F)});
            }
        }
    }

    if (!updateBossPhaseRules()) return;
    for (std::size_t enemyIndex = 0U; enemyIndex < enemies_.size(); ++enemyIndex)
    {
        auto& enemy = enemies_[enemyIndex];
        if (!enemy.health.isAlive())
        {
            enemy.controller.markDead();
            continue;
        }
        const auto& config = enemy.controller.config();
        const bool skillJustActivated = enemy.controller.action() == ai::EnemyAction::Active
            && enemy.handledSkillSequence != enemy.controller.attackSequence();
        if (skillJustActivated)
        {
            enemy.handledSkillSequence = enemy.controller.attackSequence();
            if (config.skill == ai::EnemySkill::Blood)
                    static_cast<void>(resolveEnemyDamage(enemy,
                    {DamageSource::Event, enemy.handledSkillSequence, 10}));
            if (config.skill == ai::EnemySkill::WhirlwindSlash)
            {
                const auto caster = enemy.controller.bounds();
                const float direction = enemy.controller.facingDirection();
                const float left = direction > 0.0F
                    ? caster.left + caster.width + 26.0F : caster.left - 42.0F - 26.0F;
                const std::uint64_t sequence = (1ULL << 62U)
                    | (static_cast<std::uint64_t>(enemyIndex + 1U) << 32U)
                    | enemy.handledSkillSequence;
                activeEnemyProjectiles_.push_back({{left,
                    request_.worldBounds.groundTop - 42.0F, 32.0F, 42.0F},
                    direction, 144.0F, 144.0F, sequence, 20});
            }
        }
        const bool firstDomination = skillJustActivated
            && config.skill == ai::EnemySkill::Domination
            && auraFirstDominationAvailable_;
        if (firstDomination) auraFirstDominationAvailable_ = false;
        if (enemy.controller.action() == ai::EnemyAction::Active
            && config.skill != ai::EnemySkill::Swoop
            && (config.skill != ai::EnemySkill::Domination || skillJustActivated)
            && ((config.skill != ai::EnemySkill::Slash
                    && config.skill != ai::EnemySkill::WhirlwindSlash
                    && config.skill != ai::EnemySkill::WolfClaw
                    && config.skill != ai::EnemySkill::LeafBlade
                    && config.skill != ai::EnemySkill::SideKick) || skillJustActivated)
            && intersects(playerBounds(), enemy.controller.attackBounds()))
        {
            int damage = 20;
            if (config.skill == ai::EnemySkill::Thread) damage = 10;
            if (config.skill == ai::EnemySkill::Domination) damage = 0;
            if (config.skill == ai::EnemySkill::DragonClaw) damage = 25;
            if (config.skill == ai::EnemySkill::LeafBlade
                || config.skill == ai::EnemySkill::WolfClaw
                || config.skill == ai::EnemySkill::KillingMagic
                || config.skill == ai::EnemySkill::SideKick) damage = 15;
            if (config.skill == ai::EnemySkill::Blood)
                damage = (enemy.health.maximum() - enemy.health.current()) * 2 / 5;
            if (request_.enemies.empty() && enemy.archetype != EnemyArchetype::Aura
                && enemy.archetype != EnemyArchetype::RedMirrorDragon)
                damage = request_.enemyAttackDamage;
            const std::uint64_t skillSequence = (static_cast<std::uint64_t>(enemyIndex + 1U) << 32U)
                | enemy.controller.attackSequence();
            const auto result = resolvePlayerDamage(
                {DamageSource::EnemyAttack, skillSequence, damage,
                    1.0F, relics_.incomingDamageMultiplier(), 0,
                    player_.isShadowDashing() || spellInvulnerableRemaining_ > 0.0F
                        || (player_.flightRemaining() > 0.0F
                            && config.skill == ai::EnemySkill::LeapingCleave)});
            if (result.appliedDamage > 0) beamRemaining_ = 0.0F;
            const bool dominationHit = config.skill == ai::EnemySkill::Domination
                && !player_.isShadowDashing() && spellInvulnerableRemaining_ <= 0.0F;
            if ((result.appliedDamage > 0 || dominationHit)
                && (blessingRemaining_ <= 0.0F || firstDomination))
            {
                if (config.skill == ai::EnemySkill::Thread)
                    player_.applyLaunch(540.0F, playerControlDuration(0.28F));
                else if (config.skill == ai::EnemySkill::Domination)
                {
                    player_.applyHitReaction(0.0F,
                        firstDomination ? 1.0F : playerControlDuration(1.0F));
                }
                else if (config.skill == ai::EnemySkill::Thrust)
                    player_.applyHitReaction(0.0F, playerControlDuration(1.0F));
                else player_.applyHitReaction(enemy.controller.facingDirection() * KnockbackSpeed,
                    playerControlDuration(HitStunSeconds));
            }
            else if ((result.appliedDamage > 0 || dominationHit) && blessingRemaining_ > 0.0F)
                onBlessingPreventedControl();
        }
        if (config.hasContactDamage && enemy.concealmentProgress < 1.0F
            && enemy.contactCooldown <= 0.0F
            && intersects(playerBounds(), enemy.controller.bounds()))
        {
            const std::uint64_t contactSequence = (static_cast<std::uint64_t>(enemyIndex + 1U) << 32U)
                | ++enemy.contactSequence;
            const bool legacyDamage = request_.enemies.empty()
                && enemy.archetype != EnemyArchetype::Aura
                && enemy.archetype != EnemyArchetype::RedMirrorDragon;
            const int contactDamage = legacyDamage ? request_.enemyContactDamage : 15;
            const auto contactResult = resolvePlayerDamage(
                {DamageSource::EnemyContact, contactSequence, contactDamage,
                    1.0F, relics_.incomingDamageMultiplier(),
                    relics_.collisionFlatReduction(),
                    player_.isShadowDashing() || spellInvulnerableRemaining_ > 0.0F});
            if (contactResult.appliedDamage > 0) beamRemaining_ = 0.0F;
            if (contactResult.appliedDamage > 0 && relics_.retaliatesOnCollision())
            {
                const auto player = playerBounds();
                const Aabb retaliation {player.left - 54.0F, player.top - 54.0F,
                    player.width + 108.0F, player.height + 108.0F};
                for (auto& target : enemies_)
                    if (target.health.isAlive() && intersects(retaliation, target.controller.bounds()))
                        static_cast<void>(resolveEnemyDamage(target,
                            {DamageSource::Event, contactSequence, 30}));
            }
            enemy.contactCooldown = 1.0F;
        }
    }
    resolveCombatEnd();
}

PlayerStateView CombatSession::playerState() const noexcept
{
    return {player_.position(), player_.velocity(), playerHealth_.current(), playerHealth_.maximum(),
        player_.isGrounded(), player_.facingDirection(), attack_.isActive(),
        attack_.cooldownRemaining(), attack_.sequence(), playerCastSequence_, spells_.view(),
        spells_.ultimateView(), player_.dashRemaining(), player_.dashCooldownRemaining(),
        player_.isShadowDashing(), player_.shadowDashChargeRemaining(),
        player_.isStunned(), player_.stunRemaining(), playerHitInvulnerabilityRemaining_,
        playerHurtSequence_, blessingRemaining_,
        relics_.vulnerableRemaining(), flowerFieldRemaining_, player_.flightRemaining(),
        barrierShield_ + persistentShield_, sleepRemaining_};
}

void CombatSession::settlePlayerForReward() noexcept
{
    player_.settleOnGround(request_.worldBounds);
}

void CombatSession::populateEnemyStates(std::vector<EnemyStateView>& views) const
{
    views.clear();
    views.reserve(enemies_.size());
    for (const auto& enemy : enemies_)
    {
        const auto bounds = enemy.controller.bounds();
        std::optional<Aabb> skillBounds;
        const auto skill = enemy.controller.config().skill;
        if ((enemy.controller.action() == ai::EnemyAction::Windup
                || enemy.controller.action() == ai::EnemyAction::Active)
            && skill != ai::EnemySkill::Thrust && skill != ai::EnemySkill::Dive
            && skill != ai::EnemySkill::Slash && skill != ai::EnemySkill::Thread
            && skill != ai::EnemySkill::WolfClaw
            && skill != ai::EnemySkill::LeafBlade)
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
        if (enemy.archetype == EnemyArchetype::Revolte
            && (enemy.specialWindup > 0.0F || enemy.specialActive > 0.0F)
            && enemy.revolteSkill >= 0 && enemy.revolteSkill <= 2)
        {
            const float range = enemy.revolteSkill == 2 ? 64.0F : 42.0F;
            skillBounds = Aabb {enemy.specialDirection > 0.0F ? bounds.left + bounds.width
                    : bounds.left - range,
                bounds.top, range, bounds.height};
        }
        if (enemy.archetype == EnemyArchetype::StarkCopy && enemy.manualSkill == 1
            && (enemy.specialWindup > 0.0F || enemy.specialActive > 0.0F))
            skillBounds = Aabb {bounds.left - 54.0F, bounds.top,
                bounds.width + 108.0F, bounds.height};
        if ((enemy.markedRemaining > 0.0F || tacticalNotesRemaining_ > 0.0F)
            && skill != ai::EnemySkill::WolfClaw && skill != ai::EnemySkill::LeafBlade)
        {
            const auto area = enemy.controller.attackBounds();
            if (area.width > 0.0F && area.height > 0.0F) skillBounds = area;
        }
        if (enemy.archetype == EnemyArchetype::Aura && enemy.manualSkill == 0
            && (enemy.specialWindup > 0.0F || enemy.specialActive > 0.0F))
            skillBounds = enemy.specialTargetBounds;
        float skillEffectProgress = enemy.controller.activeProgress();
        if (enemy.archetype == EnemyArchetype::Aura && enemy.manualSkill == 0)
            skillEffectProgress = enemy.specialWindup > 0.0F
                ? std::clamp(enemy.specialElapsed / AuraGuillotineWindupSeconds, 0.0F, 1.0F)
                : std::clamp(enemy.specialElapsed / AuraGuillotineActiveSeconds, 0.0F, 1.0F);
        const bool windingUp = enemy.controller.action() == ai::EnemyAction::Windup
            || enemy.breathWindup > 0.0F || enemy.specialWindup > 0.0F;
        const bool active = enemy.controller.action() == ai::EnemyAction::Active
            || enemy.breathRemaining > 0.0F || enemy.specialActive > 0.0F;
        const float visualFacing = enemy.archetype == EnemyArchetype::Revolte
                && enemy.revolteSkill >= 0
            ? enemy.specialDirection : enemy.controller.facingDirection();
        views.push_back({enemy.archetype, enemy.controller.position(), bounds.width, bounds.height,
            enemy.health.current(), enemy.health.maximum(), enemy.health.isAlive(),
            windingUp, active, enemy.slowed, skillBounds, visualFacing,
            enemy.markedRemaining > 0.0F, skillEffectProgress,
            enemy.concealmentProgress, enemy.specialWindup > 0.0F,
            enemy.specialActive > 0.0F,
            enemy.archetype == EnemyArchetype::Revolte ? enemy.revolteSkill : enemy.manualSkill,
            enemy.controller.isSwoopAscending(), enemy.activated});
    }
}

std::vector<EnemyStateView> CombatSession::enemyStates() const
{
    std::vector<EnemyStateView> views;
    populateEnemyStates(views);
    return views;
}

void CombatSession::populateSpellEffects(std::vector<SpellEffectView>& views) const
{
    views.clear();
    views.reserve(activeSpellEffects_.size() + activePillars_.size() + activeTornadoes_.size()
        + activeEnemyProjectiles_.size() + activeEnemyBeams_.size()
        + pendingEnemyLightning_.size() + activeEnemyGroundFire_.size() + enemies_.size());
    for (const auto& effect : activeSpellEffects_)
    {
        Aabb bounds = effect.bounds;
        if (effect.anchor == ActiveSpellEffect::Anchor::Player)
            bounds = playerBounds();
        else if (effect.anchor == ActiveSpellEffect::Anchor::Enemy
            && effect.enemyIndex < enemies_.size())
            bounds = enemies_[effect.enemyIndex].controller.bounds();
        views.push_back({effect.spellId, bounds, effect.remaining, effect.duration,
            effect.facingDirection});
    }
    for (const auto& pillar : activePillars_)
        views.push_back({9001U, pillar.bounds, pillar.remaining, 4.0F});
    for (const auto& tornado : activeTornadoes_)
        views.push_back({tornado.evolutionRemaining > 0.0F ? 9002U : 9003U,
            tornado.bounds, tornado.remaining, 7.0F});
    for (const auto& projectile : activeEnemyProjectiles_)
    {
        const float rotation = projectile.visualId == RevolteFlyingBladeVisualId
            ? std::atan2(projectile.velocity.y, projectile.velocity.x)
                * 180.0F / 3.14159265358979323846F
            : (projectile.tracksPlayer
                ? std::atan2(projectile.velocity.y, std::abs(projectile.velocity.x))
                    * 180.0F / 3.14159265358979323846F
                : 0.0F);
        const std::uint32_t visualId = projectile.visualId != 0U ? projectile.visualId
            : (projectile.bounds.height >= 96.0F ? 9302U
                : (projectile.bounds.width >= 54.0F ? 9102U : 9101U));
        views.push_back({visualId,
            projectile.bounds, projectile.remainingDistance / projectile.speed,
            projectile.totalDistance / projectile.speed, projectile.direction, rotation});
    }
    for (const auto& beam : activeEnemyBeams_)
    {
        Vec2 visibleStart = beam.start;
        Vec2 visibleEnd = beam.end;
        if (beam.propagatesFromCaster)
        {
            const float elapsed = std::max(0.0F, beam.duration - beam.remaining);
            const float pathDx = beam.end.x - beam.start.x;
            const float pathDy = beam.end.y - beam.start.y;
            const float pathLength = std::max(
                0.001F, std::sqrt(pathDx * pathDx + pathDy * pathDy));
            const float inversePathLength = 1.0F / pathLength;
            const float travelDistance = elapsed * beam.travelSpeed;
            const float headDistance = std::clamp(travelDistance, 0.0F, pathLength);
            const float tailDistance = std::clamp(
                travelDistance - beam.maximumVisualLength, 0.0F, pathLength);
            visibleStart = {beam.start.x + pathDx * inversePathLength * tailDistance,
                beam.start.y + pathDy * inversePathLength * tailDistance};
            visibleEnd = {beam.start.x + pathDx * inversePathLength * headDistance,
                beam.start.y + pathDy * inversePathLength * headDistance};
        }
        const float dx = visibleEnd.x - visibleStart.x;
        const float dy = visibleEnd.y - visibleStart.y;
        const float length = std::sqrt(dx * dx + dy * dy);
        const float originalDx = beam.end.x - beam.start.x;
        const float originalDy = beam.end.y - beam.start.y;
        const float angle = std::atan2(originalDy, originalDx)
            * 180.0F / 3.14159265358979323846F;
        views.push_back({beam.visualId, {visibleStart.x,
                visibleStart.y - beam.visualThickness * 0.5F, length, beam.visualThickness},
            beam.remaining, beam.duration, dx >= 0.0F ? 1.0F : -1.0F, angle});
    }
    for (const auto& enemy : enemies_)
    {
        if (enemy.archetype != EnemyArchetype::Revolte || !enemy.health.isAlive()
            || enemy.revolteSkill != 5 || enemy.specialWindup <= 0.0F) continue;
        const auto segments = revolteCrossSlashSegments(enemy.specialTarget);
        const bool secondRound = enemy.revolteCrossSlashRound > 0U;
        const float duration = secondRound ? RevolteCrossSlashSecondWindupSeconds
            : RevolteCrossSlashFirstWindupSeconds;
        for (const auto& segment : segments)
        {
            const float dx = segment.end.x - segment.start.x;
            const float dy = segment.end.y - segment.start.y;
            const float length = std::sqrt(dx * dx + dy * dy);
            const float angle = std::atan2(dy, dx) * 180.0F / 3.14159265358979323846F;
            views.push_back({secondRound ? RevolteCrossSlashSecondTelegraphVisualId
                                         : RevolteCrossSlashFirstTelegraphVisualId,
                {segment.start.x, segment.start.y - RevolteCrossSlashHalfThickness,
                    length, RevolteCrossSlashHalfThickness * 2.0F},
                enemy.specialWindup, duration, 1.0F, angle});
        }
    }
    for (const auto& enemy : enemies_)
    {
        if (enemy.archetype != EnemyArchetype::Revolte || !enemy.health.isAlive()
            || enemy.revolteSkill != 6 || enemy.specialWindup <= 0.0F) continue;
        for (std::size_t index = 0U; index < enemy.revolteFlyingBladeStarts.size(); ++index)
        {
            const Vec2 start = enemy.revolteFlyingBladeStarts[index];
            const Vec2 target = enemy.revolteFlyingBladeTargets[index];
            const float dx = target.x - start.x;
            const float dy = target.y - start.y;
            const float length = std::max(0.001F, std::sqrt(dx * dx + dy * dy));
            views.push_back({RevolteFlyingBladeTelegraphVisualId,
                {start.x, start.y - 2.0F, length, 4.0F}, enemy.specialWindup,
                RevolteFlyingBladeWindupSeconds, dx >= 0.0F ? 1.0F : -1.0F,
                std::atan2(dy, dx) * 180.0F / 3.14159265358979323846F});
        }
    }
    for (const auto& enemy : enemies_)
    {
        const bool fernBeam = enemy.archetype == EnemyArchetype::FernCopy
            && enemy.manualSkill == 2;
        const bool frierenBeam = enemy.archetype == EnemyArchetype::FrierenCopy
            && enemy.manualSkill == 2;
        if (!enemy.health.isAlive() || enemy.specialWindup <= 0.0F
            || (!fernBeam && !frierenBeam)) continue;
        const auto caster = enemy.controller.bounds();
        const Vec2 start {caster.left + caster.width * 0.5F,
            caster.top + caster.height * 0.4F};
        const float dx = enemy.specialTarget.x - start.x;
        const float dy = enemy.specialTarget.y - start.y;
        constexpr float length = CopyBeamTravelDistance;
        const float duration = fernBeam
            ? (enemy.fernVolleyShots == 0U ? FernCopyFirstBeamWindupSeconds
                                           : FernCopyFollowupBeamWindupSeconds)
            : FrierenCopyBeamWindupSeconds;
        views.push_back({CopyBeamTelegraphVisualId,
            {start.x, start.y - 2.0F, length, 4.0F}, enemy.specialWindup, duration,
            dx >= 0.0F ? 1.0F : -1.0F,
            std::atan2(dy, dx) * 180.0F / 3.14159265358979323846F});
    }
    for (const auto& enemy : enemies_)
    {
        if (enemy.archetype != EnemyArchetype::FrierenCopy || !enemy.health.isAlive()
            || enemy.manualSkill != 1 || enemy.specialWindup <= 0.0F) continue;
        views.push_back({FrierenCopyGroundFireTelegraphVisualId,
            {request_.worldBounds.left, request_.worldBounds.groundTop - 24.0F,
                request_.worldBounds.right - request_.worldBounds.left, 24.0F},
            enemy.specialWindup, FrierenCopyGroundFireWindupSeconds});
    }
    for (const auto& lightning : pendingEnemyLightning_)
        views.push_back({lightning.telegraphVisualId, lightning.bounds,
            lightning.delayRemaining, lightning.telegraphDuration});
    for (const auto& fire : activeEnemyGroundFire_)
        views.push_back({FrierenCopyGroundFireVisualId, fire.bounds, fire.remaining, 5.0F});
    for (const auto& enemy : enemies_)
        if (enemy.archetype == EnemyArchetype::Heimon && enemy.health.isAlive()
            && enemy.fogCreated)
        {
            views.push_back({9100U, enemy.specialTargetBounds, 1.0F, 1.0F});
        }
}

std::vector<SpellEffectView> CombatSession::spellEffects() const
{
    std::vector<SpellEffectView> views;
    populateSpellEffects(views);
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
        && skill != ai::EnemySkill::Slash && skill != ai::EnemySkill::Thread
        && skill != ai::EnemySkill::WolfClaw
        && skill != ai::EnemySkill::LeafBlade)
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
    if ((enemy.markedRemaining > 0.0F || tacticalNotesRemaining_ > 0.0F)
        && skill != ai::EnemySkill::WolfClaw && skill != ai::EnemySkill::LeafBlade)
    {
        const auto area = enemy.controller.attackBounds();
        if (area.width > 0.0F && area.height > 0.0F) skillBounds = area;
    }
    if (enemy.archetype == EnemyArchetype::Aura && enemy.manualSkill == 0
        && (enemy.specialWindup > 0.0F || enemy.specialActive > 0.0F))
        skillBounds = enemy.specialTargetBounds;
    float skillEffectProgress = enemy.controller.activeProgress();
    if (enemy.archetype == EnemyArchetype::Aura && enemy.manualSkill == 0)
        skillEffectProgress = enemy.specialWindup > 0.0F
            ? std::clamp(enemy.specialElapsed / AuraGuillotineWindupSeconds, 0.0F, 1.0F)
            : std::clamp(enemy.specialElapsed / AuraGuillotineActiveSeconds, 0.0F, 1.0F);
    const float visualFacing = enemy.archetype == EnemyArchetype::Revolte
            && enemy.revolteSkill >= 0
        ? enemy.specialDirection : enemy.controller.facingDirection();
    return {enemy.archetype, enemy.controller.position(), bounds.width, bounds.height,
        enemy.health.current(), enemy.health.maximum(), enemy.health.isAlive(),
        enemy.controller.action() == ai::EnemyAction::Windup || enemy.breathWindup > 0.0F
            || enemy.specialWindup > 0.0F,
        enemy.controller.action() == ai::EnemyAction::Active || enemy.breathRemaining > 0.0F
            || enemy.specialActive > 0.0F,
        enemy.slowed, skillBounds, visualFacing,
        enemy.markedRemaining > 0.0F, skillEffectProgress,
        enemy.concealmentProgress, enemy.specialWindup > 0.0F,
        enemy.specialActive > 0.0F,
        enemy.archetype == EnemyArchetype::Revolte ? enemy.revolteSkill : enemy.manualSkill,
        enemy.controller.isSwoopAscending(), enemy.activated};
}

Aabb CombatSession::attackBounds() const noexcept
{
    const auto position = player_.position();
    const float left = player_.facingDirection() > 0.0F
        ? position.x + PlayerController::Width : position.x - AttackRange;
    return {left, position.y + AttackVerticalOffset, AttackRange, AttackHeight};
}

const std::optional<CombatResult>& CombatSession::result() const noexcept { return result_; }
bool CombatSession::equipSpell(const std::size_t slot, const std::optional<std::uint32_t> id,
    const std::uint8_t rank) noexcept
{ return spells_.equip(slot, id, rank); }
bool CombatSession::equipUltimateSpell(const std::optional<std::uint32_t> id) noexcept
{ return spells_.equipUltimate(id); }
WorldBounds CombatSession::movementBoundsFor(const EnemyRuntime& enemy) const noexcept
{
    return enemy.movementBounds.value_or(request_.worldBounds);
}
void CombatSession::clearCombatTransientEffects() noexcept
{
    activeSpellEffects_.clear();
    pendingSpellImpacts_.clear();
    activePillars_.clear();
    activeTornadoes_.clear();
    activeEnemyProjectiles_.clear();
    activeEnemyBeams_.clear();
    pendingEnemyLightning_.clear();
    activeEnemyLightningStorms_.clear();
    activeEnemyGroundFire_.clear();
    for (auto& enemy : enemies_)
    {
        if (enemy.health.isAlive()) continue;
        enemy.specialWindup = 0.0F;
        enemy.specialActive = 0.0F;
        enemy.manualSkill = -1;
        enemy.revolteSkill = -1;
        enemy.breathWindup = 0.0F;
        enemy.breathRemaining = 0.0F;
    }
}
Aabb CombatSession::playerBounds() const noexcept
{ const auto p = player_.position(); return {p.x, p.y, PlayerController::Width, PlayerController::Height}; }
Aabb CombatSession::firstLivingEnemyBounds() const noexcept
{
    const auto player = playerBounds();
    const float playerCenterX = player.left + player.width * 0.5F;
    const float playerCenterY = player.top + player.height * 0.5F;
    const auto distanceToPlayer = [playerCenterX, playerCenterY](const auto& enemy) {
        if (!enemy.health.isAlive()
            || enemy.archetype == EnemyArchetype::WaterMirrorDemon)
            return std::numeric_limits<float>::max();
        const auto bounds = enemy.controller.bounds();
        const float dx = bounds.left + bounds.width * 0.5F - playerCenterX;
        const float dy = bounds.top + bounds.height * 0.5F - playerCenterY;
        return dx * dx + dy * dy;
    };
    const auto found = std::min_element(enemies_.begin(), enemies_.end(), [&](const auto& left,
        const auto& right) {
        return distanceToPlayer(left) < distanceToPlayer(right);
    });
    return found == enemies_.end()
        || distanceToPlayer(*found) == std::numeric_limits<float>::max()
        ? Aabb {} : found->controller.bounds();
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
