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
constexpr float FrostLanceSlowSeconds = 2.0F;
constexpr float FrostLanceChillSeconds = 5.5F;
constexpr float FrostLanceFreezeSeconds = 0.65F;

constexpr bool isPlayerMagic(const DamageSource source) noexcept
{
    return source == DamageSource::PlayerSpell0 || source == DamageSource::PlayerSpell1
        || source == DamageSource::PlayerSpell2 || source == DamageSource::PlayerUltimateSpell;
}

constexpr bool isPlayerDamage(const DamageSource source) noexcept
{
    return source == DamageSource::PlayerBasicAttack || isPlayerMagic(source);
}

bool segmentIntersectsAabb(const Vec2 start, const Vec2 end, const Aabb& bounds,
    const float halfThickness) noexcept
{
    const float left = bounds.left - halfThickness;
    const float right = bounds.left + bounds.width + halfThickness;
    const float top = bounds.top - halfThickness;
    const float bottom = bounds.top + bounds.height + halfThickness;
    const float dx = end.x - start.x;
    const float dy = end.y - start.y;
    float minimum = 0.0F;
    float maximum = 1.0F;
    const auto clip = [&](const float origin, const float delta,
                          const float low, const float high) {
        if (std::abs(delta) < 0.0001F) return origin >= low && origin <= high;
        const float first = (low - origin) / delta;
        const float second = (high - origin) / delta;
        minimum = std::max(minimum, std::min(first, second));
        maximum = std::min(maximum, std::max(first, second));
        return minimum <= maximum;
    };
    return clip(start.x, dx, left, right) && clip(start.y, dy, top, bottom);
}

constexpr float fullClipDuration(const std::uint32_t frameCount, const float framesPerSecond) noexcept
{
    return static_cast<float>(frameCount) / framesPerSecond;
}

constexpr float spellVisualDuration(const spells::SpellEffect effect) noexcept
{
    using spells::SpellEffect;
    switch (effect)
    {
    case SpellEffect::DirectDamage: return fullClipDuration(8U, 16.0F);
    case SpellEffect::FlowerField: return 4.0F;
    case SpellEffect::GoddessBlessing: return 5.0F;
    case SpellEffect::BloodMagic: return fullClipDuration(8U, 16.0F);
    case SpellEffect::FrostLance: return fullClipDuration(6U, 18.0F);
    case SpellEffect::FlameBurst: return 3.0F;
    case SpellEffect::MagicThread: return fullClipDuration(8U, 16.0F);
    case SpellEffect::StoneShot: return fullClipDuration(6U, 16.0F);
    case SpellEffect::InnateDash: return PlayerController::DashDurationSeconds;
    case SpellEffect::Phantom: return 4.0F;
    case SpellEffect::ManaTrace: return 4.0F;
    case SpellEffect::BossZoltraak: return fullClipDuration(8U, 16.0F);
    case SpellEffect::GoddessSpears: return fullClipDuration(10U, 14.0F);
    case SpellEffect::SeveringSlash: return fullClipDuration(10U, 16.0F);
    case SpellEffect::Mimic: return fullClipDuration(10U, 14.0F);
    case SpellEffect::DestructionLightning: return 0.8F;
    case SpellEffect::HellfireStorm: return 3.0F;
    case SpellEffect::JudgmentBeam: return 2.0F;
    case SpellEffect::EarthPillars: return 3.0F;
    case SpellEffect::MirrorArray: return 8.0F;
    case SpellEffect::MultiZoltraak: return fullClipDuration(8U, 16.0F);
    case SpellEffect::Dispel: return fullClipDuration(8U, 16.0F);
    case SpellEffect::ManaStrike: return fullClipDuration(8U, 16.0F);
    case SpellEffect::GoldenBinding: return fullClipDuration(8U, 14.0F);
    case SpellEffect::FloatSlam: return 1.0F;
    case SpellEffect::StoneGolem: return 5.0F;
    case SpellEffect::Flight: return 2.5F;
    case SpellEffect::SpatialShatter: return fullClipDuration(10U, 16.0F);
    case SpellEffect::Seal: return fullClipDuration(8U, 14.0F);
    case SpellEffect::LightningStaff: return 4.0F;
    case SpellEffect::HomingVolley: return fullClipDuration(6U, 20.0F);
    case SpellEffect::DefensiveBarrier: return 5.0F;
    case SpellEffect::WindPressure: return fullClipDuration(8U, 16.0F);
    case SpellEffect::GravityWell: return 2.5F;
    }
    return 0.0F;
}

constexpr std::array AuraPreBattleDialogue {
    CombatDialogueLineView {"AURA", "LUGNER'S PRESENCE IS GONE. IT SEEMS HE WAS KILLED.", "aura-initial"},
    CombatDialogueLineView {"FRIEREN", "NOT ONLY LUGNER. I DEFEATED ALL OF YOUR EXECUTIONERS.", "frieren-pre"},
    CombatDialogueLineView {"AURA", "IF I DEFEAT YOU HERE, THE RESULT WILL STILL BE ENOUGH.", "aura-idle"}
};
constexpr std::array AuraFirstDominationDialogue {
    CombatDialogueLineView {"AURA", "DOMINATION MAGIC!", "aura-windup"},
    CombatDialogueLineView {"AURA", "YOUR MANA HAS BARELY GROWN SINCE EIGHTY YEARS AGO.", "aura-windup"},
    CombatDialogueLineView {"FRIEREN", "...", "frieren"}
};
constexpr std::array AuraDefeatDialogue {
    CombatDialogueLineView {"FRIEREN", "KILL YOURSELF, AURA.", "frieren"},
    CombatDialogueLineView {"AURA", "IMPOSSIBLE... HOW COULD I...", "aura-die"}
};
constexpr std::array RevoltePreBattleDialogue {
    CombatDialogueLineView {"REVOLTE", "AN ELF MAGE. WHO ARE YOU?", "revolte-1"},
    CombatDialogueLineView {"FRIEREN", "FOUR BLADES... EVEN DEFENSIVE MAGIC COULD HARDLY TAKE THEM HEAD-ON.", "frieren"},
    CombatDialogueLineView {"REVOLTE", "I ASKED WHO YOU ARE.", "revolte-1"},
    CombatDialogueLineView {"FRIEREN", "DO YOU GIVE YOUR NAME EVERY TIME YOU EXTERMINATE VERMIN?", "frieren"}
};
constexpr std::array RevolteSecondPhaseDialogue {
    CombatDialogueLineView {"REVOLTE", "INTERESTING. IT SEEMS I MUST FIGHT YOU SERIOUSLY.", "revolte-2"},
    CombatDialogueLineView {"FRIEREN", "...", "frieren"}
};
constexpr std::array RevolteDefeatDialogue {
    CombatDialogueLineView {"REVOLTE", "YOU WIN, ELF.", "revolte-3"}
};
constexpr std::array WaterMirrorPreBattleDialogue {
    CombatDialogueLineView {"FRIEREN", "SO THIS IS THE TOP. THAT WAS EASIER THAN I EXPECTED.", "frieren"},
    CombatDialogueLineView {"WATER MIRROR DEMON", "...", "water-mirror"},
    CombatDialogueLineView {"FRIEREN", "COPIES THAT REPRODUCE EVEN INDIVIDUAL ABILITIES... INTERESTING.", "frieren"},
    CombatDialogueLineView {"WATER MIRROR DEMON", "...", "water-mirror"}
};
constexpr std::array WaterMirrorSecondPhaseDialogue {
    CombatDialogueLineView {"FRIEREN", "IS THAT... A COPY OF ME?", "frieren"},
    CombatDialogueLineView {"WATER MIRROR DEMON", "...", "water-mirror"},
    CombatDialogueLineView {"FRIEREN", "THEN LET US SEE WHAT IT CAN DO.", "frieren"}
};
constexpr std::array WaterMirrorDefeatDialogue {
    CombatDialogueLineView {"WATER MIRROR DEMON", "...", "water-mirror"},
    CombatDialogueLineView {"FRIEREN", "WHEW. THAT WAS NOT EASY.", "frieren"}
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
        return EnemyConfig {160.0F, 64.0F, 64.0F, 0.5F, 0.6F, 0.0F, 0.0F,
            64.0F, 96.0F, 7.0F, true, false, EnemySkill::BossAttack};
    case EnemyArchetype::Aura:
        return EnemyConfig {120.0F, 420.0F, 420.0F, 0.8F, 0.2F, 0.0F, 0.0F,
            42.0F, 64.0F, 7.0F, false, false, EnemySkill::Domination};
    case EnemyArchetype::RedMirrorDragon:
        return EnemyConfig {96.0F, 72.0F, 72.0F, 0.5F, 0.6F, 0.0F, 0.0F,
            128.0F, 84.0F, 7.0F, true, false, EnemySkill::DragonClaw};
    case EnemyArchetype::WaterMirrorDemon:
        return EnemyConfig {0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F,
            54.0F, 72.0F, 999.0F, false, false, EnemySkill::BossAttack};
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
      spells_(request_.equippedSpellIds, request_.equippedUltimateSpellId), relics_(request_.relicIds)
{
    if (relics_.has(relics::FlammeNotesId)) tacticalNotesRemaining_ = 8.0F;
    if (relics_.has(relics::SeriePageId)) spells_.setUltimateCooldown(15.3F);
    auto spawns = request_.enemies;
    if (spawns.empty()) spawns.push_back({request_.enemyArchetype, request_.enemySpawn});
    enemies_.reserve(spawns.size() + 3U);
    for (const auto& spawn : spawns)
    {
        const auto config = enemyConfigFor(spawn.archetype);
        Vec2 position = spawn.position;
        if (!request_.enemies.empty() || spawn.archetype == EnemyArchetype::Aura
            || spawn.archetype == EnemyArchetype::Revolte
            || spawn.archetype == EnemyArchetype::RedMirrorDragon
            || spawn.archetype == EnemyArchetype::WaterMirrorDemon)
            position.y = request_.worldBounds.groundTop - config.height;
        if (config.flying) position.y = request_.worldBounds.groundTop
            - (spawn.archetype == EnemyArchetype::LargeBirdDemon ? 144.0F : 132.0F) - config.height;
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
        if (spawn.archetype == EnemyArchetype::RedMirrorDragon)
            enemies_.back().breathCooldown = 6.0F;
        if (spawn.archetype == EnemyArchetype::ChaosFlower)
            enemies_.back().specialCooldown = 3.5F;
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
            enemies_.back().revolteCooldowns = {3.5F, 3.5F, 4.5F, 4.5F, 3.0F};
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
        const Aabb burst {player.left - 80.0F, player.top - 48.0F,
            player.width + 160.0F, player.height + 96.0F};
        for (auto& enemy : enemies_)
            if (enemy.health.isAlive() && intersects(burst, enemy.controller.bounds()))
                    static_cast<void>(resolveEnemyDamage(enemy,
                    {DamageSource::Event, ++environmentalSequence_, 12}));
    }
    return result;
}

DamageResult CombatSession::resolveEnemyDamage(EnemyRuntime& enemy,
    DamageRequest request) noexcept
{
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
    phantomRemaining_ = std::max(0.0F, phantomRemaining_ - deltaSeconds);
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
                * 90.0F * activeDelta, request_.worldBounds);
        }
        while (gravityWellTickAccumulator_ >= 1.0F)
        {
            gravityWellTickAccumulator_ -= 1.0F;
            ++gravityWellTick_;
            for (auto& enemy : enemies_)
                if (enemy.health.isAlive() && intersects(gravityWellBounds_, enemy.controller.bounds()))
                    static_cast<void>(resolveEnemyDamage(enemy,
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
        for (auto& enemy : enemies_)
            if (enemy.health.isAlive() && enemy.concealmentProgress < 1.0F
                && intersects(impact.bounds, enemy.controller.bounds()))
                static_cast<void>(resolveEnemyDamage(enemy,
                    {impact.source, impact.sequence, impact.damage, impact.multiplier
                        * (enemy.exposedRemaining > 0.0F ? 1.25F : 1.0F)
                        * (enemy.markedRemaining > 0.0F ? 1.12F : 1.0F)}));
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
        const float travel = std::min(projectile.remainingDistance, projectile.speed * deltaSeconds);
        projectile.bounds.left += projectile.direction * travel;
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
        beam.remaining = std::max(0.0F, beam.remaining - deltaSeconds);
    std::erase_if(activeEnemyBeams_, [](const auto& beam) { return beam.remaining <= 0.0F; });
    for (auto& storm : activeEnemyLightningStorms_)
    {
        storm.nextStrikeRemaining -= deltaSeconds;
        while (storm.strikesRemaining > 0U && storm.nextStrikeRemaining <= 0.0F)
        {
            const auto target = playerBounds();
            pendingEnemyLightning_.push_back({{target.left + target.width * 0.5F - 42.0F,
                request_.worldBounds.groundTop - 180.0F, 84.0F, 180.0F},
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
            activeSpellEffects_.push_back({9300U, lightning.bounds, 0.6F, 0.6F});
            if (intersects(lightning.bounds, playerBounds()))
                static_cast<void>(resolvePlayerDamage({DamageSource::EnemyAttack,
                    lightning.sequence, 20, 1.0F, relics_.incomingDamageMultiplier(), 0,
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

    std::vector<Aabb> fogAreas;
    for (const auto& caster : enemies_)
        if (caster.archetype == EnemyArchetype::Heimon && caster.health.isAlive()
            && caster.fogCreated)
        {
            fogAreas.push_back(caster.specialTargetBounds);
        }

    for (std::size_t enemyIndex = 0U; enemyIndex < enemies_.size(); ++enemyIndex)
    {
        auto& enemy = enemies_[enemyIndex];
        if (!enemy.health.isAlive()) continue;
        enemy.frostSlowRemaining = std::max(0.0F, enemy.frostSlowRemaining - deltaSeconds);
        enemy.frostChillRemaining = std::max(0.0F,
            enemy.frostChillRemaining - deltaSeconds);
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
        if (enemy.burnRemaining > 0.0F)
        {
            const float activeDelta = std::min(deltaSeconds, enemy.burnRemaining);
            enemy.burnRemaining -= activeDelta;
            enemy.burnTickAccumulator += activeDelta;
            while (enemy.burnTickAccumulator >= 1.0F && enemy.health.isAlive())
            {
                enemy.burnTickAccumulator -= 1.0F;
                ++enemy.burnTick;
                    static_cast<void>(resolveEnemyDamage(enemy,
                    {enemy.burnSource, enemy.burnSequenceBase + enemy.burnTick, 4,
                        enemy.burnMultiplier}));
            }
        }
        if (!enemy.health.isAlive()) continue;
        enemy.contactCooldown = std::max(0.0F, enemy.contactCooldown - deltaSeconds);
        const auto bounds = enemy.controller.bounds();
        const bool insideFog = std::any_of(fogAreas.begin(), fogAreas.end(), [&](const Aabb& fog) {
            return intersects(fog, bounds);
        });
        const bool revealingForSkill = enemy.controller.action() == ai::EnemyAction::Windup
            || enemy.controller.action() == ai::EnemyAction::Active || enemy.specialWindup > 0.0F
            || enemy.specialActive > 0.0F;
        if (insideFog && !revealingForSkill)
            enemy.concealmentProgress = std::min(1.0F,
                enemy.concealmentProgress + deltaSeconds / 0.8F);
        else enemy.concealmentProgress = 0.0F;
        const bool flowerSlowed = flowerFieldRemaining_ > 0.0F
            && std::abs(bounds.left + bounds.width * 0.5F - flowerFieldCenterX_) <= 300.0F;
        enemy.slowed = flowerSlowed || enemy.frostSlowRemaining > 0.0F;
        if (enemy.archetype == EnemyArchetype::Heimon && enemy.specialActive > 0.0F)
        {
            enemy.specialActive = std::max(0.0F, enemy.specialActive - deltaSeconds);
            continue;
        }
        if (enemy.archetype == EnemyArchetype::Heimon && !enemy.fogCreated)
        {
            enemy.specialCooldown = std::max(0.0F, enemy.specialCooldown - deltaSeconds);
            if (enemy.specialWindup > 0.0F)
            {
                enemy.specialWindup = std::max(0.0F, enemy.specialWindup - deltaSeconds);
                if (enemy.specialWindup <= 0.0F)
                {
                    const auto caster = enemy.controller.bounds();
                    enemy.specialTargetBounds = {caster.left + caster.width * 0.5F - 320.0F,
                        request_.worldBounds.groundTop - 96.0F, 640.0F, 96.0F};
                    enemy.fogCreated = true;
                    enemy.specialActive = 0.6F;
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
        if (enemy.archetype == EnemyArchetype::Gargoyle)
        {
            const auto caster = enemy.controller.bounds();
            const float casterCenter = caster.left + caster.width * 0.5F;
            if (!enemy.activated)
            {
                if (std::abs(playerCenter - casterCenter) <= 320.0F) enemy.activated = true;
                if (!enemy.activated) continue;
            }
            const float airborneY = request_.worldBounds.groundTop - 144.0F - caster.height;
            if (caster.top > airborneY)
            {
                auto position = enemy.controller.position();
                position.y = std::max(airborneY, position.y - 80.0F * deltaSeconds);
                enemy.controller.setPosition(position, request_.worldBounds);
                continue;
            }
            enemy.specialCooldown = std::max(0.0F, enemy.specialCooldown - deltaSeconds);
            if (enemy.specialActive > 0.0F)
            {
                enemy.specialActive = std::max(0.0F, enemy.specialActive - deltaSeconds);
                continue;
            }
            if (enemy.specialWindup > 0.0F)
            {
                enemy.specialWindup = std::max(0.0F, enemy.specialWindup - deltaSeconds);
                if (enemy.specialWindup <= 0.0F)
                {
                    const auto current = enemy.controller.bounds();
                    const Vec2 start {current.left + current.width * 0.5F,
                        current.top + current.height * 0.5F};
                    const float dx = enemy.specialTarget.x - start.x;
                    const float dy = enemy.specialTarget.y - start.y;
                    const float distance = std::max(0.001F, std::sqrt(dx * dx + dy * dy));
                    const Vec2 end {start.x + dx / distance * 240.0F,
                        std::min(request_.worldBounds.groundTop - 29.0F,
                            start.y + dy / distance * 240.0F)};
                    const std::uint64_t sequence = ++environmentalSequence_;
                    activeEnemyBeams_.push_back({start, end, 0.6F, sequence});
                    if (segmentIntersectsAabb(start, end, playerBounds(), 9.0F))
                        static_cast<void>(resolvePlayerDamage({DamageSource::EnemyAttack,
                            sequence, 20, 1.0F, relics_.incomingDamageMultiplier(), 0,
                            player_.isShadowDashing() || spellInvulnerableRemaining_ > 0.0F}));
                    enemy.specialActive = 0.6F;
                    enemy.specialCooldown = 4.0F;
                }
                continue;
            }
            const Vec2 playerTarget {playerBounds().left + playerBounds().width * 0.5F,
                playerBounds().top + playerBounds().height * 0.5F};
            const float dx = playerTarget.x - casterCenter;
            const float dy = playerTarget.y - (caster.top + caster.height * 0.5F);
            if (enemy.specialCooldown <= 0.0F && std::sqrt(dx * dx + dy * dy) <= 261.0F)
            {
                enemy.specialTarget = playerTarget;
                enemy.specialDirection = dx >= 0.0F ? 1.0F : -1.0F;
                enemy.specialWindup = 0.5F;
                continue;
            }
        }
        if (enemy.archetype == EnemyArchetype::ThreeHeadedDemon)
        {
            enemy.specialCooldown = std::max(0.0F, enemy.specialCooldown - deltaSeconds);
            enemy.secondaryCooldown = std::max(0.0F, enemy.secondaryCooldown - deltaSeconds);
            if (enemy.specialActive > 0.0F)
            {
                enemy.specialActive = std::max(0.0F, enemy.specialActive - deltaSeconds);
                if (enemy.specialActive <= 0.0F) enemy.manualSkill = -1;
                continue;
            }
            if (enemy.specialWindup > 0.0F)
            {
                enemy.specialWindup = std::max(0.0F, enemy.specialWindup - deltaSeconds);
                if (enemy.specialWindup <= 0.0F)
                {
                    if (enemy.manualSkill == 0)
                    {
                        const int targetHealth = enemy.health.current() < 60 ? 60 : 120;
                        static_cast<void>(enemy.health.heal(targetHealth - enemy.health.current()));
                        enemy.specialCooldown = 7.0F;
                    }
                    else
                    {
                        const auto area = enemy.controller.attackBounds();
                        if (intersects(playerBounds(), area))
                            static_cast<void>(resolvePlayerDamage({DamageSource::EnemyAttack,
                                ++environmentalSequence_, 20, 1.0F,
                                relics_.incomingDamageMultiplier()}));
                        enemy.secondaryCooldown = 4.0F;
                    }
                    enemy.specialActive = 0.6F;
                }
                continue;
            }
            if (enemy.specialCooldown <= 0.0F && enemy.health.current() < 120)
            {
                enemy.manualSkill = 0;
                enemy.specialWindup = 0.5F;
                continue;
            }
            const float distance = std::abs(playerCenter
                - (bounds.left + bounds.width * 0.5F));
            if (enemy.secondaryCooldown <= 0.0F
                && distance <= bounds.width * 0.5F + 64.0F + PlayerController::Width * 0.5F)
            {
                enemy.manualSkill = 1;
                enemy.specialDirection = enemy.controller.facingDirection();
                enemy.specialWindup = 0.5F;
                continue;
            }
        }
        if (enemy.archetype == EnemyArchetype::SwordDemon)
        {
            enemy.specialCooldown = std::max(0.0F, enemy.specialCooldown - deltaSeconds);
            enemy.secondaryCooldown = std::max(0.0F, enemy.secondaryCooldown - deltaSeconds);
            if (enemy.specialActive > 0.0F)
            {
                const float activeDelta = std::min(deltaSeconds, enemy.specialActive);
                if (enemy.manualSkill == 0)
                    enemy.controller.translateHorizontal(
                        enemy.specialDirection * 320.0F * activeDelta, request_.worldBounds);
                enemy.specialActive -= activeDelta;
                if (enemy.specialActive <= 0.0F) enemy.manualSkill = -1;
                continue;
            }
            if (enemy.specialWindup > 0.0F)
            {
                enemy.specialWindup = std::max(0.0F, enemy.specialWindup - deltaSeconds);
                if (enemy.specialWindup <= 0.0F)
                {
                    const auto area = enemy.controller.attackBounds();
                    if (intersects(playerBounds(), area))
                        static_cast<void>(resolvePlayerDamage({DamageSource::EnemyAttack,
                            ++environmentalSequence_, 20, 1.0F,
                            relics_.incomingDamageMultiplier()}));
                    enemy.specialActive = 0.6F;
                    enemy.secondaryCooldown = 3.0F;
                }
                continue;
            }
            const float distance = std::abs(playerCenter
                - (bounds.left + bounds.width * 0.5F));
            if (enemy.specialCooldown <= 0.0F && distance <= 180.0F)
            {
                enemy.manualSkill = 0;
                enemy.specialDirection = enemy.controller.facingDirection();
                enemy.specialActive = 144.0F / 320.0F;
                enemy.specialCooldown = 4.0F;
                continue;
            }
            if (enemy.secondaryCooldown <= 0.0F
                && distance <= bounds.width * 0.5F + 54.0F + PlayerController::Width * 0.5F)
            {
                enemy.manualSkill = 1;
                enemy.specialDirection = enemy.controller.facingDirection();
                enemy.specialWindup = 0.5F;
                continue;
            }
        }
        if (enemy.archetype == EnemyArchetype::StarkCopy)
        {
            for (std::size_t skill = 0U; skill < 2U; ++skill)
                enemy.revolteCooldowns[skill] = std::max(
                    0.0F, enemy.revolteCooldowns[skill] - deltaSeconds);
            if (enemy.specialActive > 0.0F)
            {
                const float activeDelta = std::min(deltaSeconds, enemy.specialActive);
                if (enemy.manualSkill == 0)
                {
                    enemy.specialElapsed += activeDelta;
                    auto position = enemy.controller.position();
                    position.x += enemy.specialDirection * 180.0F * activeDelta;
                    constexpr float gravity = 1600.0F;
                    const float height = std::max(0.0F, 840.0F * enemy.specialElapsed
                        - 0.5F * gravity * enemy.specialElapsed * enemy.specialElapsed);
                    position.y = request_.worldBounds.groundTop - bounds.height - height;
                    enemy.controller.setPosition(position, request_.worldBounds);
                }
                enemy.specialActive -= activeDelta;
                if (enemy.specialActive <= 0.0F)
                {
                    if (enemy.manualSkill == 0)
                    {
                        const auto landed = enemy.controller.bounds();
                        const Aabb impact {landed.left + landed.width * 0.5F - 144.0F,
                            request_.worldBounds.groundTop - 144.0F, 288.0F, 144.0F};
                        activeSpellEffects_.push_back({9303U,
                            {impact.left, request_.worldBounds.groundTop - 72.0F,
                                impact.width, 72.0F},
                            0.6F, 0.6F});
                        if (intersects(impact, playerBounds()))
                            static_cast<void>(resolvePlayerDamage({DamageSource::EnemyAttack,
                                ++environmentalSequence_, 20, 1.0F,
                                relics_.incomingDamageMultiplier()}));
                        const float left = enemy.specialDirection > 0.0F
                            ? landed.left + landed.width : landed.left - 32.0F;
                        activeEnemyProjectiles_.push_back({{left,
                            request_.worldBounds.groundTop - 96.0F, 32.0F, 96.0F},
                            enemy.specialDirection, 300.0F, 300.0F,
                            ++environmentalSequence_, 20, 220.0F});
                        enemy.manualSkill = 2;
                        enemy.specialActive = 0.6F;
                    }
                    else enemy.manualSkill = -1;
                }
                continue;
            }
            if (enemy.specialWindup > 0.0F)
            {
                enemy.specialWindup = std::max(0.0F, enemy.specialWindup - deltaSeconds);
                if (enemy.specialWindup <= 0.0F)
                {
                    if (enemy.manualSkill == 0)
                    {
                        enemy.specialElapsed = 0.0F;
                        enemy.specialActive = 1.05F;
                        enemy.revolteCooldowns[0] = 8.0F;
                    }
                    else
                    {
                        const auto caster = enemy.controller.bounds();
                        const Aabb slash {caster.left - 54.0F, caster.top,
                            caster.width + 108.0F, caster.height};
                        if (intersects(slash, playerBounds()))
                            static_cast<void>(resolvePlayerDamage({DamageSource::EnemyAttack,
                                ++environmentalSequence_, 20, 1.0F,
                                relics_.incomingDamageMultiplier()}));
                        enemy.specialActive = 0.6F;
                        enemy.revolteCooldowns[1] = 4.0F;
                    }
                }
                continue;
            }
            const float distance = std::abs(playerCenter
                - (bounds.left + bounds.width * 0.5F));
            if (enemy.revolteCooldowns[0] <= 0.0F)
            {
                enemy.manualSkill = 0;
                enemy.specialDirection = enemy.controller.facingDirection();
                enemy.specialWindup = 0.8F;
                continue;
            }
            if (enemy.revolteCooldowns[1] <= 0.0F
                && distance <= bounds.width * 0.5F + 54.0F + PlayerController::Width * 0.5F)
            {
                enemy.manualSkill = 1;
                enemy.specialDirection = enemy.controller.facingDirection();
                enemy.specialWindup = 0.4F;
                continue;
            }
        }
        if (enemy.archetype == EnemyArchetype::FernCopy)
        {
            enemy.specialCooldown = std::max(0.0F, enemy.specialCooldown - deltaSeconds);
            if (enemy.specialActive > 0.0F)
            {
                enemy.specialActive = std::max(0.0F, enemy.specialActive - deltaSeconds);
                continue;
            }
            if (enemy.specialWindup > 0.0F)
            {
                enemy.specialWindup = std::max(0.0F, enemy.specialWindup - deltaSeconds);
                if (enemy.specialWindup <= 0.0F)
                {
                    const auto caster = enemy.controller.bounds();
                    const Vec2 start {caster.left + caster.width * 0.5F,
                        caster.top + caster.height * 0.4F};
                    const auto target = playerBounds();
                    const float dx = target.left + target.width * 0.5F - start.x;
                    const float dy = target.top + target.height * 0.5F - start.y;
                    const float length = std::max(0.001F, std::sqrt(dx * dx + dy * dy));
                    const Vec2 end {start.x + dx / length * 256.0F,
                        start.y + dy / length * 256.0F};
                    const auto sequence = ++environmentalSequence_;
                    activeEnemyBeams_.push_back({start, end, 0.6F, sequence});
                    if (segmentIntersectsAabb(start, end, playerBounds(), 9.0F))
                        static_cast<void>(resolvePlayerDamage({DamageSource::EnemyAttack,
                            sequence, 15, 1.0F, relics_.incomingDamageMultiplier()}));
                    ++enemy.summonCount;
                    const std::uint64_t roll = request_.seed
                        ^ (static_cast<std::uint64_t>(enemy.summonCount) * 0x9e3779b97f4a7c15ULL);
                    if ((roll & 3ULL) != 0ULL) enemy.specialWindup = 0.4F;
                    else enemy.specialCooldown = 6.0F;
                    enemy.specialActive = 0.6F;
                }
                continue;
            }
            if (enemy.specialCooldown <= 0.0F)
            {
                enemy.specialWindup = 0.4F;
                continue;
            }
        }
        if (enemy.archetype == EnemyArchetype::FrierenCopy)
        {
            for (std::size_t skill = 0U; skill < 3U; ++skill)
                enemy.revolteCooldowns[skill] = std::max(
                    0.0F, enemy.revolteCooldowns[skill] - deltaSeconds);
            if (enemy.specialActive > 0.0F)
            {
                enemy.specialActive = std::max(0.0F, enemy.specialActive - deltaSeconds);
                continue;
            }
            if (enemy.specialWindup > 0.0F)
            {
                enemy.specialWindup = std::max(0.0F, enemy.specialWindup - deltaSeconds);
                if (enemy.specialWindup <= 0.0F)
                {
                    const auto target = playerBounds();
                    if (enemy.manualSkill == 0)
                    {
                        activeEnemyLightningStorms_.push_back({5U, 0.0F});
                        enemy.revolteCooldowns[0] = 8.0F;
                    }
                    else if (enemy.manualSkill == 1)
                    {
                        activeEnemyGroundFire_.push_back({{request_.worldBounds.left,
                            request_.worldBounds.groundTop - 24.0F,
                            request_.worldBounds.right - request_.worldBounds.left, 24.0F},
                            5.0F, 0.0F, (++environmentalSequence_) << 16U});
                        enemy.revolteCooldowns[1] = 8.0F;
                    }
                    else
                    {
                        const auto caster = enemy.controller.bounds();
                        const Vec2 start {caster.left + caster.width * 0.5F,
                            caster.top + caster.height * 0.4F};
                        const float dx = target.left + target.width * 0.5F - start.x;
                        const float dy = target.top + target.height * 0.5F - start.y;
                        const float length = std::max(0.001F, std::sqrt(dx * dx + dy * dy));
                        const Vec2 end {start.x + dx / length * 256.0F,
                            start.y + dy / length * 256.0F};
                        const auto sequence = ++environmentalSequence_;
                        activeEnemyBeams_.push_back({start, end, 0.6F, sequence});
                        if (segmentIntersectsAabb(start, end, playerBounds(), 9.0F))
                            static_cast<void>(resolvePlayerDamage({DamageSource::EnemyAttack,
                                sequence, 15, 1.0F, relics_.incomingDamageMultiplier()}));
                        enemy.revolteCooldowns[2] = 3.0F;
                    }
                    enemy.specialActive = enemy.manualSkill == 0 ? 0.0F : 0.6F;
                    if (enemy.manualSkill == 0) enemy.manualSkill = -1;
                }
                continue;
            }
            for (int skill = 0; skill < 3; ++skill)
                if (enemy.revolteCooldowns[static_cast<std::size_t>(skill)] <= 0.0F)
                {
                    enemy.manualSkill = skill;
                    enemy.specialWindup = skill == 2 ? 0.3F : 0.5F;
                    break;
                }
            if (enemy.specialWindup > 0.0F) continue;
        }
        if (enemy.archetype == EnemyArchetype::Revolte)
        {
            for (auto& cooldown : enemy.revolteCooldowns)
                cooldown = std::max(0.0F, cooldown - deltaSeconds);
            if (enemy.specialWindup > 0.0F)
            {
                enemy.specialWindup = std::max(0.0F, enemy.specialWindup - deltaSeconds);
                if (enemy.specialWindup <= 0.0F)
                {
                    const auto caster = enemy.controller.bounds();
                    const float direction = enemy.specialDirection;
                    const auto frontArea = [&](const float range) {
                        return Aabb {direction > 0.0F ? caster.left + caster.width
                                : caster.left - range,
                            caster.top, range, caster.height};
                    };
                    const auto hitPlayer = [&](const float range, const int damage) {
                        if (!intersects(playerBounds(), frontArea(range))) return;
                        const auto result = resolvePlayerDamage({DamageSource::EnemyAttack,
                            ++environmentalSequence_, damage, 1.0F,
                            relics_.incomingDamageMultiplier(), 0,
                            player_.isShadowDashing() || spellInvulnerableRemaining_ > 0.0F});
                        if (result.appliedDamage > 0)
                            player_.applyHitReaction(direction * KnockbackSpeed,
                                playerControlDuration(HitStunSeconds));
                    };
                    if (enemy.revolteSkill == 0 || enemy.revolteSkill == 1)
                    {
                        hitPlayer(42.0F, 20);
                        const float left = direction > 0.0F
                            ? caster.left + caster.width + 26.0F
                            : caster.left - 42.0F - 26.0F;
                        const float top = request_.worldBounds.groundTop
                            - (enemy.revolteSkill == 0 ? 84.0F : 42.0F);
                        activeEnemyProjectiles_.push_back({{left, top, 32.0F, 42.0F},
                            direction, 144.0F, 144.0F, ++environmentalSequence_, 20});
                        enemy.specialActive = 0.6F;
                    }
                    else if (enemy.revolteSkill == 2)
                    {
                        hitPlayer(64.0F, 25);
                        const float left = direction > 0.0F
                            ? caster.left + caster.width + 37.0F
                            : caster.left - 64.0F - 37.0F;
                        activeEnemyProjectiles_.push_back({{left,
                            request_.worldBounds.groundTop - 72.0F, 54.0F, 72.0F},
                            direction, 168.0F, 168.0F, ++environmentalSequence_, 25});
                        enemy.specialActive = 0.6F;
                    }
                    else if (enemy.revolteSkill == 3) enemy.specialActive = 1.2F;
                    else if (enemy.revolteSkill == 4)
                        enemy.specialActive = 144.0F / 220.0F;
                }
                continue;
            }
            if (enemy.specialActive > 0.0F)
            {
                const float activeDelta = std::min(deltaSeconds, enemy.specialActive);
                if (enemy.revolteSkill == 4)
                    enemy.controller.translateHorizontal(
                        enemy.specialDirection * 220.0F * activeDelta, request_.worldBounds);
                enemy.specialActive -= activeDelta;
                if (enemy.specialActive <= 0.0F)
                {
                    if (enemy.revolteSkill == 4)
                        enemy.revolteCooldowns = {0.0F, 0.0F, 0.0F, 0.0F, 6.0F};
                    enemy.revolteSkill = -1;
                }
                continue;
            }
            if (enemy.revolteCounterDashPending)
            {
                enemy.revolteCounterDashPending = false;
                enemy.revolteSkill = 4;
                enemy.specialDirection = enemy.controller.facingDirection();
                enemy.specialWindup = 0.5F;
                continue;
            }
            for (int skill = 0; skill < 5; ++skill)
            {
                if (skill == 4 && !enemy.revolteSecondPhase) continue;
                if (enemy.revolteCooldowns[static_cast<std::size_t>(skill)] > 0.0F) continue;
                const float distance = std::abs(playerCenter
                    - (bounds.left + bounds.width * 0.5F));
                if (skill < 2 && distance > bounds.width * 0.5F + 124.0F) continue;
                if (skill == 2 && distance > bounds.width * 0.5F + 464.0F / 3.0F) continue;
                enemy.revolteSkill = skill;
                enemy.specialDirection = playerCenter
                        >= bounds.left + bounds.width * 0.5F ? 1.0F : -1.0F;
                if (skill == 3) enemy.specialActive = 1.2F;
                else enemy.specialWindup = skill < 2 ? 0.3F : 0.5F;
                enemy.revolteCooldowns[static_cast<std::size_t>(skill)] =
                    skill < 2 ? 7.0F : (skill == 4 ? 6.0F : 9.0F);
                break;
            }
            if (enemy.revolteSkill >= 0) continue;
        }
        if (enemy.archetype == EnemyArchetype::ChaosFlower)
        {
            enemy.specialCooldown = std::max(0.0F, enemy.specialCooldown - deltaSeconds);
            if (enemy.specialCooldown <= 0.0F
                && enemy.controller.action() == ai::EnemyAction::Chase)
            {
                const auto flower = enemy.controller.bounds();
                const Aabb curse {flower.left + flower.width * 0.5F - 300.0F,
                    flower.top + flower.height * 0.5F - 300.0F, 600.0F, 600.0F};
                if (intersects(playerBounds(), curse) && blessingRemaining_ <= 0.0F
                    && !player_.isDashing() && spellInvulnerableRemaining_ <= 0.0F)
                    sleepRemaining_ = 5.0F;
                else if (intersects(playerBounds(), curse) && blessingRemaining_ > 0.0F)
                    onBlessingPreventedControl();
                enemy.specialCooldown = 7.0F;
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
                    enemy.specialActive = 0.45F;
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
                position.x += enemy.specialDirection * 260.0F * activeDelta;
                constexpr float FrostWolfPounceGravity = 1600.0F;
                const float verticalDisplacement = std::max(0.0F,
                    360.0F * enemy.specialElapsed
                        - 0.5F * FrostWolfPounceGravity * enemy.specialElapsed
                            * enemy.specialElapsed);
                position.y = request_.worldBounds.groundTop - bounds.height
                    - verticalDisplacement;
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
                        enemy.specialCooldown = 6.0F;
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
            if (enemy.specialActive > 0.0F)
            {
                enemy.specialActive = std::max(0.0F, enemy.specialActive - deltaSeconds);
                continue;
            }
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
                        static_cast<void>(resolvePlayerDamage(
                            {DamageSource::EnemyAttack, sequence, 15, 1.0F,
                                relics_.incomingDamageMultiplier(),
                                enemy.breathRemaining < 1.0F
                                    && relics_.has(relics::NorthernCloakId) ? 4 : 0,
                                player_.isShadowDashing() || spellInvulnerableRemaining_ > 0.0F}));
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
                : (enemy.frostSlowRemaining > 0.0F ? 0.65F
                    : (enemy.archetype == EnemyArchetype::StarkCopy
                        && enemy.health.current() < 150 ? 1.2F : 1.0F));
            const bool manualSkill = enemy.archetype == EnemyArchetype::Richter
                || enemy.archetype == EnemyArchetype::Denken
                || enemy.archetype == EnemyArchetype::Revolte
                || enemy.archetype == EnemyArchetype::Gargoyle
                || enemy.archetype == EnemyArchetype::ThreeHeadedDemon
                || enemy.archetype == EnemyArchetype::SwordDemon
                || enemy.archetype == EnemyArchetype::WaterMirrorDemon
                || enemy.archetype == EnemyArchetype::StarkCopy
                || enemy.archetype == EnemyArchetype::FernCopy
                || enemy.archetype == EnemyArchetype::FrierenCopy;
            enemy.controller.update(target, deltaSeconds, request_.worldBounds, speedMultiplier,
                enemy.skillSealRemaining <= 0.0F && !manualSkill);
            if (enemy.archetype == EnemyArchetype::Aura
                && enemy.controller.action() == ai::EnemyAction::Windup
                && !auraFirstDominationDialogueShown_)
            {
                auraFirstDominationDialogueShown_ = true;
                beginDialogue(DialogueScript::AuraFirstDomination);
                return;
            }
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
                    static_cast<void>(resolveEnemyDamage(target,
                        {golemSource_, golemSequence_ * 16U + 2U, 24, golemMultiplier_}));
            activeSpellEffects_.push_back({1022U, blast, 1.0F, 1.0F});
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
                    static_cast<void>(resolveEnemyDamage(enemy,
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
                    static_cast<void>(resolveEnemyDamage(enemy,
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
                if (isPlayerMagic(source) && enemy.concealmentProgress >= 1.0F) continue;
                float targetMultiplier = multiplier
                    * (enemy.exposedRemaining > 0.0F ? 1.25F : 1.0F)
                    * (enemy.markedRemaining > 0.0F ? 1.12F : 1.0F);
                if (regularSlotForSource(source) && relics_.has(relics::ElderSealFragmentId))
                    targetMultiplier *= hitIndex == 1U ? 1.12F : (hitIndex >= 2U ? 1.24F : 1.0F);
                const auto result = resolveEnemyDamage(enemy,
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
        ++playerCastSequence_;
        float duration = spellVisualDuration(cast.effect);
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
                    const bool canFreeze = enemy.frostChillRemaining > 0.0F || enemy.slowed;
                    enemy.frostSlowRemaining = FrostLanceSlowSeconds;
                    enemy.frostChillRemaining = FrostLanceChillSeconds;
                    if (canFreeze) enemy.frozenRemaining = FrostLanceFreezeSeconds
                        * relicControlMultiplier(enemy);
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
                    static_cast<void>(resolveEnemyDamage(enemy,
                            {source, cast.sequence * 16U + 14U, 10, multiplier}));
                    enemy.frostSlowRemaining = 0.0F;
                    enemy.frostChillRemaining = 0.0F;
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
                    static_cast<void>(resolveEnemyDamage(enemy,
                            {source, cast.sequence * 16U + 15U, 14, multiplier}));
                    if (enemy.goldenBindRemaining > 0.0F)
                    {
                        enemy.goldenBindRemaining = 0.0F;
                    static_cast<void>(resolveEnemyDamage(enemy,
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
                        const auto result = resolveEnemyDamage(enemy,
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
                    static_cast<void>(resolveEnemyDamage(enemy,
                        {source, cast.sequence, damage, multiplier}));
                    const float before = enemy.controller.position().x;
                    enemy.controller.translateHorizontal(player_.facingDirection() * 120.0F,
                        request_.worldBounds);
                    if (std::abs(enemy.controller.position().x - before) < 119.0F)
                    static_cast<void>(resolveEnemyDamage(enemy,
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
                    const auto result = resolveEnemyDamage(*found,
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
                const auto result = resolveEnemyDamage(enemy,
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
                    static_cast<void>(resolveEnemyDamage(enemy,
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
                    static_cast<void>(resolveEnemyDamage(enemy,
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
                    static_cast<void>(resolveEnemyDamage(enemy,
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
                    static_cast<void>(resolveEnemyDamage(enemy,
                        {source, cast.sequence, cast.damage,
                            multiplier * (executeRange ? 1.3F : 1.0F)}));
                    if (enemy.goldenBindRemaining > 0.0F)
                    {
                        enemy.goldenBindRemaining = 0.0F;
                    static_cast<void>(resolveEnemyDamage(enemy,
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
                    static_cast<void>(resolveEnemyDamage(enemy,
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
            activeSpellEffects_.push_back({2012U, cast.effectBounds, 0.75F, 0.75F});
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

    const auto defeatedAura = std::find_if(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
        return enemy.archetype == EnemyArchetype::Aura && !enemy.health.isAlive();
    });
    if (defeatedAura != enemies_.end())
    {
        for (auto& enemy : enemies_)
        {
            if (&enemy == &*defeatedAura || !enemy.health.isAlive()) continue;
            static_cast<void>(enemy.health.damage(enemy.health.current()));
            enemy.controller.markDead();
        }
    }

    auto waterMirror = std::find_if(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
        return enemy.archetype == EnemyArchetype::WaterMirrorDemon;
    });
    if (waterMirror != enemies_.end() && waterMirror->health.isAlive())
    {
        const bool starkAlive = std::any_of(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
            return enemy.archetype == EnemyArchetype::StarkCopy && enemy.health.isAlive();
        });
        const bool fernAlive = std::any_of(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
            return enemy.archetype == EnemyArchetype::FernCopy && enemy.health.isAlive();
        });
        const bool frierenPresent = std::any_of(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
            return enemy.archetype == EnemyArchetype::FrierenCopy;
        });
        if (!waterMirrorSecondPhase_ && !starkAlive && !fernAlive)
        {
            waterMirrorSecondPhase_ = true;
            static_cast<void>(waterMirror->health.damage(waterMirror->health.current() / 2));
            const auto config = enemyConfigFor(EnemyArchetype::FrierenCopy);
            Vec2 position {820.0F,
                request_.worldBounds.groundTop - 48.0F - config.height};
            const int copyHealth = waterMirror->health.maximum() == 200
                ? 350 : waterMirror->health.maximum();
            enemies_.push_back({EnemyArchetype::FrierenCopy,
                ai::EnemyController(position, config), Health(copyHealth, copyHealth)});
            enemies_.back().revolteCooldowns = {4.0F, 4.0F, 1.5F, 0.0F, 0.0F};
            beginDialogue(DialogueScript::WaterMirrorSecondPhase);
            return;
        }
        const bool frierenAlive = std::any_of(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
            return enemy.archetype == EnemyArchetype::FrierenCopy && enemy.health.isAlive();
        });
        if (waterMirrorSecondPhase_ && frierenPresent && !frierenAlive)
        {
            static_cast<void>(waterMirror->health.damage(waterMirror->health.current()));
            waterMirror->controller.markDead();
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
    if (std::none_of(enemies_.begin(), enemies_.end(), [](const auto& enemy) { return enemy.health.isAlive(); }))
    {
        const bool auraEncounter = std::any_of(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
            return enemy.archetype == EnemyArchetype::Aura;
        });
        const bool revolteEncounter = std::any_of(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
            return enemy.archetype == EnemyArchetype::Revolte;
        });
        const bool waterMirrorEncounter = std::any_of(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
            return enemy.archetype == EnemyArchetype::WaterMirrorDemon;
        });
        if (auraEncounter && !auraDefeatDialogueShown_)
        {
            auraDefeatDialogueShown_ = true;
            outcomeAfterDialogue_ = CombatOutcome::Victory;
            beginDialogue(DialogueScript::AuraDefeat);
        }
        else if (revolteEncounter && !revolteDefeatDialogueShown_)
        {
            revolteDefeatDialogueShown_ = true;
            outcomeAfterDialogue_ = CombatOutcome::Victory;
            beginDialogue(DialogueScript::RevolteDefeat);
        }
        else if (waterMirrorEncounter && !waterMirrorDefeatDialogueShown_)
        {
            waterMirrorDefeatDialogueShown_ = true;
            outcomeAfterDialogue_ = CombatOutcome::Victory;
            beginDialogue(DialogueScript::WaterMirrorDefeat);
        }
        else finish(CombatOutcome::Victory);
    }
    else if (!playerHealth_.isAlive()) finish(CombatOutcome::Defeat);
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
        const bool windingUp = enemy.controller.action() == ai::EnemyAction::Windup
            || enemy.breathWindup > 0.0F || enemy.specialWindup > 0.0F;
        const bool active = enemy.controller.action() == ai::EnemyAction::Active
            || enemy.breathRemaining > 0.0F || enemy.specialActive > 0.0F;
        views.push_back({enemy.archetype, enemy.controller.position(), bounds.width, bounds.height,
            enemy.health.current(), enemy.health.maximum(), enemy.health.isAlive(),
            windingUp, active, enemy.slowed, skillBounds, enemy.controller.facingDirection(),
            enemy.markedRemaining > 0.0F, enemy.controller.activeProgress(),
            enemy.concealmentProgress, enemy.specialWindup > 0.0F,
            enemy.specialActive > 0.0F,
            enemy.archetype == EnemyArchetype::Revolte ? enemy.revolteSkill : enemy.manualSkill,
            enemy.controller.isSwoopAscending(), enemy.activated});
    }
    return views;
}

std::vector<SpellEffectView> CombatSession::spellEffects() const
{
    std::vector<SpellEffectView> views;
    views.reserve(activeSpellEffects_.size() + activePillars_.size() + activeTornadoes_.size()
        + activeEnemyProjectiles_.size() + activeEnemyBeams_.size()
        + pendingEnemyLightning_.size() + activeEnemyGroundFire_.size() + enemies_.size());
    for (const auto& effect : activeSpellEffects_)
        views.push_back({effect.spellId, effect.bounds, effect.remaining, effect.duration});
    for (const auto& pillar : activePillars_)
        views.push_back({9001U, pillar.bounds, pillar.remaining, 4.0F});
    for (const auto& tornado : activeTornadoes_)
        views.push_back({tornado.evolutionRemaining > 0.0F ? 9002U : 9003U,
            tornado.bounds, tornado.remaining, 7.0F});
    for (const auto& projectile : activeEnemyProjectiles_)
        views.push_back({projectile.bounds.height >= 96.0F ? 9302U
                : (projectile.bounds.width >= 54.0F ? 9102U : 9101U),
            projectile.bounds, projectile.remainingDistance / projectile.speed,
            projectile.totalDistance / projectile.speed, projectile.direction});
    for (const auto& beam : activeEnemyBeams_)
    {
        const float dx = beam.end.x - beam.start.x;
        const float dy = beam.end.y - beam.start.y;
        const float length = std::sqrt(dx * dx + dy * dy);
        const float angle = std::atan2(dy, dx) * 180.0F / 3.14159265358979323846F;
        views.push_back({9200U, {beam.start.x, beam.start.y - 9.0F, length, 18.0F},
            beam.remaining, 0.6F, dx >= 0.0F ? 1.0F : -1.0F, angle});
    }
    for (const auto& lightning : pendingEnemyLightning_)
        views.push_back({9300U, lightning.bounds, lightning.delayRemaining, 0.4F});
    for (const auto& fire : activeEnemyGroundFire_)
        views.push_back({9301U, fire.bounds, fire.remaining, 5.0F});
    for (const auto& enemy : enemies_)
        if (enemy.archetype == EnemyArchetype::Heimon && enemy.health.isAlive()
            && enemy.fogCreated)
        {
            views.push_back({9100U, enemy.specialTargetBounds, 1.0F, 1.0F});
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
    return {enemy.archetype, enemy.controller.position(), bounds.width, bounds.height,
        enemy.health.current(), enemy.health.maximum(), enemy.health.isAlive(),
        enemy.controller.action() == ai::EnemyAction::Windup || enemy.breathWindup > 0.0F
            || enemy.specialWindup > 0.0F,
        enemy.controller.action() == ai::EnemyAction::Active || enemy.breathRemaining > 0.0F
            || enemy.specialActive > 0.0F,
        enemy.slowed, skillBounds, enemy.controller.facingDirection(),
        enemy.markedRemaining > 0.0F, enemy.controller.activeProgress(),
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
std::optional<CombatDialogueLineView> CombatSession::dialogueLine() const noexcept
{
    switch (dialogueScript_)
    {
    case DialogueScript::AuraPreBattle: return AuraPreBattleDialogue[dialogueLineIndex_];
    case DialogueScript::AuraFirstDomination: return AuraFirstDominationDialogue[dialogueLineIndex_];
    case DialogueScript::AuraDefeat: return AuraDefeatDialogue[dialogueLineIndex_];
    case DialogueScript::RevoltePreBattle: return RevoltePreBattleDialogue[dialogueLineIndex_];
    case DialogueScript::RevolteSecondPhase: return RevolteSecondPhaseDialogue[dialogueLineIndex_];
    case DialogueScript::RevolteDefeat: return RevolteDefeatDialogue[dialogueLineIndex_];
    case DialogueScript::WaterMirrorPreBattle: return WaterMirrorPreBattleDialogue[dialogueLineIndex_];
    case DialogueScript::WaterMirrorSecondPhase: return WaterMirrorSecondPhaseDialogue[dialogueLineIndex_];
    case DialogueScript::WaterMirrorDefeat: return WaterMirrorDefeatDialogue[dialogueLineIndex_];
    case DialogueScript::None: return std::nullopt;
    }
    return std::nullopt;
}
std::optional<BossIntroView> CombatSession::bossIntro() const noexcept
{
    if (bossIntroRemaining_ <= 0.0F) return std::nullopt;
    const bool revolte = std::any_of(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
        return enemy.archetype == EnemyArchetype::Revolte;
    });
    const bool waterMirror = std::any_of(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
        return enemy.archetype == EnemyArchetype::WaterMirrorDemon;
    });
    return BossIntroView {waterMirror ? "WATER MIRROR DEMON"
            : (revolte ? "REVOLTE" : "GUILLOTINE AURA"),
        bossIntroRemaining_, 2.4F};
}
void CombatSession::beginDialogue(const DialogueScript script) noexcept
{
    dialogueScript_ = script;
    dialogueLineIndex_ = 0U;
}
void CombatSession::advanceDialogue() noexcept
{
    std::size_t lineCount = 0U;
    switch (dialogueScript_)
    {
    case DialogueScript::AuraPreBattle: lineCount = AuraPreBattleDialogue.size(); break;
    case DialogueScript::AuraFirstDomination: lineCount = AuraFirstDominationDialogue.size(); break;
    case DialogueScript::AuraDefeat: lineCount = AuraDefeatDialogue.size(); break;
    case DialogueScript::RevoltePreBattle: lineCount = RevoltePreBattleDialogue.size(); break;
    case DialogueScript::RevolteSecondPhase: lineCount = RevolteSecondPhaseDialogue.size(); break;
    case DialogueScript::RevolteDefeat: lineCount = RevolteDefeatDialogue.size(); break;
    case DialogueScript::WaterMirrorPreBattle: lineCount = WaterMirrorPreBattleDialogue.size(); break;
    case DialogueScript::WaterMirrorSecondPhase: lineCount = WaterMirrorSecondPhaseDialogue.size(); break;
    case DialogueScript::WaterMirrorDefeat: lineCount = WaterMirrorDefeatDialogue.size(); break;
    case DialogueScript::None: return;
    }
    ++dialogueLineIndex_;
    if (dialogueLineIndex_ < lineCount) return;
    const DialogueScript completedScript = dialogueScript_;
    dialogueScript_ = DialogueScript::None;
    dialogueLineIndex_ = 0U;
    if (completedScript == DialogueScript::RevolteSecondPhase)
    {
        const auto revolte = std::find_if(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
            return enemy.archetype == EnemyArchetype::Revolte && enemy.health.isAlive();
        });
        if (revolte != enemies_.end())
        {
            static_cast<void>(revolte->health.heal(
                revolte->health.maximum() / 2 - revolte->health.current()));
            revolte->revolteSecondPhase = true;
            revolte->revolteTransitionPending = false;
            revolte->revolteCooldowns = {7.0F, 7.0F, 9.0F, 9.0F, 3.0F};
        }
    }
    if (outcomeAfterDialogue_)
    {
        const CombatOutcome outcome = *outcomeAfterDialogue_;
        outcomeAfterDialogue_.reset();
        finish(outcome);
    }
}
bool CombatSession::equipSpell(const std::size_t slot, const std::optional<std::uint32_t> id) noexcept
{ return spells_.equip(slot, id); }
bool CombatSession::equipUltimateSpell(const std::optional<std::uint32_t> id) noexcept
{ return spells_.equipUltimate(id); }
Aabb CombatSession::playerBounds() const noexcept
{ const auto p = player_.position(); return {p.x, p.y, PlayerController::Width, PlayerController::Height}; }
Aabb CombatSession::firstLivingEnemyBounds() const noexcept
{
    const auto found = std::find_if(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
        return enemy.health.isAlive()
            && enemy.archetype != EnemyArchetype::WaterMirrorDemon;
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
