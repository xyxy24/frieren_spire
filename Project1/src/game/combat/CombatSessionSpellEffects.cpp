#include "game/combat/CombatSession.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace arcane::game
{
namespace
{
constexpr float FrostLanceSlowSeconds = 2.0F;
constexpr float FrostLanceChillSeconds = 5.5F;
constexpr float FrostLanceFreezeSeconds = 0.65F;

constexpr bool isBossArchetype(const EnemyArchetype archetype) noexcept
{
    switch (archetype)
    {
    case EnemyArchetype::Aura:
    case EnemyArchetype::Revolte:
    case EnemyArchetype::RedMirrorDragon:
    case EnemyArchetype::WaterMirrorDemon:
    case EnemyArchetype::StarkCopy:
    case EnemyArchetype::FernCopy:
    case EnemyArchetype::FrierenCopy:
    case EnemyArchetype::Boss: return true;
    default: return false;
    }
}

constexpr bool isDemonArchetype(const EnemyArchetype archetype) noexcept
{
    switch (archetype)
    {
    case EnemyArchetype::BirdDemon:
    case EnemyArchetype::Lugner:
    case EnemyArchetype::Linie:
    case EnemyArchetype::Draht:
    case EnemyArchetype::Qual:
    case EnemyArchetype::Heimon:
    case EnemyArchetype::DemonWarrior:
    case EnemyArchetype::LargeBirdDemon:
    case EnemyArchetype::ThreeHeadedDemon:
    case EnemyArchetype::SwordDemon:
    case EnemyArchetype::Aura:
    case EnemyArchetype::Revolte:
    case EnemyArchetype::WaterMirrorDemon: return true;
    default: return false;
    }
}

constexpr bool isPlayerMagic(const DamageSource source) noexcept
{
    return source == DamageSource::PlayerSpell0 || source == DamageSource::PlayerSpell1
        || source == DamageSource::PlayerSpell2 || source == DamageSource::PlayerUltimateSpell;
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
    case SpellEffect::FlameBurst: return fullClipDuration(8U, 16.0F);
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

constexpr bool visualDurationScalesWithMastery(const spells::SpellEffect effect) noexcept
{
    using spells::SpellEffect;
    switch (effect)
    {
    case SpellEffect::FlowerField:
    case SpellEffect::GoddessBlessing:
    case SpellEffect::Phantom:
    case SpellEffect::ManaTrace:
    case SpellEffect::StoneGolem:
    case SpellEffect::Flight:
    case SpellEffect::LightningStaff:
    case SpellEffect::DefensiveBarrier:
    case SpellEffect::GravityWell: return true;
    default: return false;
    }
}

constexpr bool visualFollowsPlayer(const spells::SpellEffect effect) noexcept
{
    using spells::SpellEffect;
    return effect == SpellEffect::GoddessBlessing || effect == SpellEffect::Flight
        || effect == SpellEffect::LightningStaff || effect == SpellEffect::DefensiveBarrier;
}

constexpr bool visualUsesSelectedGround(const spells::SpellEffect effect) noexcept
{
    using spells::SpellEffect;
    return effect == SpellEffect::FloatSlam || effect == SpellEffect::GravityWell
        || effect == SpellEffect::GoddessSpears
        || effect == SpellEffect::DestructionLightning
        || effect == SpellEffect::EarthPillars;
}
}

std::optional<std::size_t> CombatSession::regularSlotForSource(
    const DamageSource source) noexcept
{
    if (source == DamageSource::PlayerSpell0) return 0U;
    if (source == DamageSource::PlayerSpell1) return 1U;
    if (source == DamageSource::PlayerSpell2) return 2U;
    return std::nullopt;
}

void CombatSession::onActiveHit(EnemyRuntime& enemy, const DamageSource source,
    const std::uint64_t sequence)
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
}

void CombatSession::damageEnemies(const DamageSource source, const std::uint64_t sequence,
    const int damage, const float multiplier, const Aabb& area)
{
    std::size_t hitIndex = 0U;
    for (auto& enemy : enemies_)
        if (enemy.health.isAlive() && intersects(area, enemy.controller.bounds()))
        {
            if (isPlayerMagic(source) && enemy.concealmentProgress >= 1.0F) continue;
            float targetMultiplier = multiplier;
            if (regularSlotForSource(source) && relics_.has(relics::ElderSealFragmentId))
                targetMultiplier *= hitIndex == 1U ? 1.12F : (hitIndex >= 2U ? 1.24F : 1.0F);
            const auto result = resolveEnemyDamage(enemy,
                {source, sequence, damage, targetMultiplier});
            if (result.appliedDamage > 0) onActiveHit(enemy, source, sequence);
            ++hitIndex;
        }
}

void CombatSession::onInterrupt(EnemyRuntime& enemy)
{
    if (relics_.has(relics::FlammeNotesId) && !tacticalInterruptUsed_)
    {
        enemy.markedRemaining += 3.0F;
        player_.refreshDash();
        tacticalInterruptUsed_ = true;
    }
}

void CombatSession::applyCast(const spells::SpellCastResult& cast, const DamageSource source,
    const spells::SpellDefinition* definition)
{
    if (!cast.cast || !definition) return;
    ++playerCastSequence_;
    const float playerCenter = player_.position().x + PlayerController::Width * 0.5F;
    const bool regularCast = source != DamageSource::PlayerUltimateSpell;
    const float mastery = regularCast ? cast.masteryPowerMultiplier : 1.0F;
    float duration = spellVisualDuration(cast.effect);
    const bool extendPersistent = relics_.has(relics::SteelPetalBookmarkId)
        && (cast.effect == spells::SpellEffect::FlowerField
            || cast.effect == spells::SpellEffect::Phantom
            || cast.effect == spells::SpellEffect::StoneGolem
            || cast.effect == spells::SpellEffect::GravityWell);
    if (visualDurationScalesWithMastery(cast.effect)) duration *= mastery;
    if (extendPersistent) duration *= 1.25F;
    activeSpellEffects_.push_back({cast.spellId, cast.effectBounds, duration, duration,
        player_.facingDirection(), visualFollowsPlayer(cast.effect)
            ? ActiveSpellEffect::Anchor::Player : ActiveSpellEffect::Anchor::World});
    const Aabb selectedTarget = firstLivingEnemyBounds();
    if (visualUsesSelectedGround(cast.effect) && selectedTarget.width > 0.0F)
        activeSpellEffects_.back().bounds.top = selectedTarget.top + selectedTarget.height
            - activeSpellEffects_.back().bounds.height;
    float multiplier = relics_.outgoingDamageMultiplier()
        * (blessingRemaining_ > 0.0F ? 1.15F : 1.0F)
        * (goddessTabletDamageRemaining_ > 0.0F ? 1.10F : 1.0F);
    if (regularCast)
        multiplier *= request_.regularSpellDamageMultiplier * cast.masteryPowerMultiplier;
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
        multiplier *= flightBoostMultiplier_;
        flightBoostAvailable_ = false;
    }
    if (regularCast) lastRegularCastMultiplier_ = multiplier;
    if (cast.effect == spells::SpellEffect::DirectDamage)
    {
        damageEnemies(source, cast.sequence, cast.damage, multiplier, cast.effectBounds);
        if (cast.rank >= 3U)
            for (auto& enemy : enemies_)
                if (enemy.health.isAlive() && enemy.markedRemaining > 0.0F
                    && intersects(cast.effectBounds, enemy.controller.bounds()))
                    static_cast<void>(resolveEnemyDamage(enemy,
                        {source, cast.sequence * 32U + 28U, 8, multiplier}));
    }
    else if (cast.effect == spells::SpellEffect::FlowerField)
    {
        flowerFieldRemaining_ = (extendPersistent ? 5.0F : 4.0F) * mastery;
        flowerFieldCenterX_ = playerCenter;
        flowerHealingAccumulator_ = 0.0F;
        flowerHealingRate_ = 3.0F * mastery;
        flowerFieldMastered_ = cast.rank >= 3U;
        flowerFieldDamageMultiplier_ = multiplier;
    }
    else if (cast.effect == spells::SpellEffect::GoddessBlessing)
    {
        blessingRemaining_ = 5.0F * mastery;
        goddessTabletTriggered_ = false;
        if (cast.rank >= 3U)
        {
            const int cap = playerHealth_.maximum() * 3 / 10;
            persistentShield_ += std::min(10,
                std::max(0, cap - persistentShield_ - barrierShield_));
        }
    }
    else if (cast.effect == spells::SpellEffect::BloodMagic)
    {
        const int costPercent = cast.rank >= 3U ? 8 : 12;
        const int cost = std::max(1,
            (playerHealth_.current() * costPercent + 99) / 100);
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
                const bool shatter = cast.rank >= 3U && enemy.frozenRemaining > 0.0F;
                enemy.frostSlowRemaining = FrostLanceSlowSeconds * mastery;
                enemy.frostChillRemaining = FrostLanceChillSeconds * mastery;
                if (canFreeze) enemy.frozenRemaining = FrostLanceFreezeSeconds
                    * mastery * relicControlMultiplier(enemy);
                if (shatter)
                {
                    const auto center = enemy.controller.bounds();
                    const Aabb burst {center.left - 60.0F, center.top - 48.0F,
                        center.width + 120.0F, center.height + 96.0F};
                    damageEnemies(source, cast.sequence * 32U + 31U, 18,
                        multiplier, burst);
                }
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
                {
                    if (cast.rank >= 3U)
                    {
                        const auto center = enemy.controller.bounds();
                        const Aabb burst {center.left - 72.0F, center.top - 56.0F,
                            center.width + 144.0F, center.height + 112.0F};
                        damageEnemies(source, cast.sequence * 32U + 30U, 20,
                            multiplier, burst);
                    }
                    else
                        static_cast<void>(resolveEnemyDamage(enemy,
                            {source, cast.sequence * 16U + 14U, 10, multiplier}));
                }
                enemy.frostSlowRemaining = 0.0F;
                enemy.frostChillRemaining = 0.0F;
                enemy.frozenRemaining = 0.0F;
                enemy.burnRemaining = 3.0F * mastery;
                enemy.burnTickAccumulator = 0.0F;
                enemy.burnSequenceBase = cast.sequence * 16U;
                enemy.burnTick = 0U;
                enemy.burnSource = source;
                enemy.burnMultiplier = multiplier;
            }
        if (flowerFieldRemaining_ > 0.0F)
        {
            flowerFieldRemaining_ = 0.0F;
            std::erase_if(activeSpellEffects_, [](const auto& effect) {
                return effect.spellId == 1001U;
            });
            burningFlowerRemaining_ = relics_.has(relics::SteelPetalBookmarkId) ? 2.5F : 2.0F;
            burningFlowerAccumulator_ = 0.0F;
            burningFlowerSequenceBase_ = cast.sequence * 32U;
            burningFlowerTick_ = 0U;
            burningFlowerSource_ = source;
            burningFlowerMultiplier_ = multiplier;
            const Aabb field {flowerFieldCenterX_ - 300.0F,
                request_.worldBounds.groundTop - 300.0F, 600.0F, 300.0F};
            if (flowerFieldMastered_)
                damageEnemies(source, cast.sequence * 32U + 26U, 16,
                    flowerFieldDamageMultiplier_, field);
            flowerFieldMastered_ = false;
            flowerFieldDamageMultiplier_ = 1.0F;
            activeSpellEffects_.push_back({BurningFlowerVisualId, field,
                burningFlowerRemaining_, burningFlowerRemaining_});
        }
    }
    else if (cast.effect == spells::SpellEffect::MagicThread)
    {
        damageEnemies(source, cast.sequence, cast.damage, multiplier, cast.effectBounds);
        for (auto& enemy : enemies_)
            if (enemy.health.isAlive() && intersects(cast.effectBounds, enemy.controller.bounds()))
            {
                const float direction = playerCenter < enemy.controller.position().x ? -1.0F : 1.0F;
                enemy.controller.translateHorizontal(direction * 110.0F
                    * (cast.rank >= 3U ? 1.25F : 1.0F), movementBoundsFor(enemy));
                enemy.controlRemaining = (isBossArchetype(enemy.archetype) ? 0.3F : 0.9F)
                    * mastery
                    * (cast.rank >= 3U ? 1.25F : 1.0F) * relicControlMultiplier(enemy);
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
                    movementBoundsFor(enemy));
                if (std::abs(enemy.controller.position().x - before) < 79.0F)
                static_cast<void>(resolveEnemyDamage(enemy,
                        {source, cast.sequence * 16U + 15U,
                            cast.rank >= 3U ? 24 : 14, multiplier}));
                if (enemy.goldenBindRemaining > 0.0F)
                {
                    const bool refundBinding = enemy.goldenBindMastered;
                    enemy.goldenBindRemaining = 0.0F;
                    enemy.goldenBindMastered = false;
                static_cast<void>(resolveEnemyDamage(enemy,
                        {source, cast.sequence * 16U + 13U, 16, multiplier}));
                    if (refundBinding)
                    {
                        const auto views = spells_.view();
                        for (std::size_t index = 0U; index < views.size(); ++index)
                            if (views[index].id == 1020U)
                                spells_.reduceRegularCooldown(index,
                                    views[index].cooldownDuration * 0.30F);
                    }
                }
            }
    }
    else if (cast.effect == spells::SpellEffect::Phantom)
    {
        phantomRemaining_ = (extendPersistent ? 5.0F : 4.0F) * mastery;
        const float left = player_.facingDirection() > 0.0F
            ? player_.position().x + 60.0F : player_.position().x - 60.0F;
        phantomBounds_ = {left, player_.position().y,
            PlayerController::Width, PlayerController::Height};
        activeSpellEffects_.back().bounds = phantomBounds_;
        phantomExpiryBurst_ = cast.rank >= 3U;
        phantomDamageMultiplier_ = multiplier;
    }
    else if (cast.effect == spells::SpellEffect::ManaTrace)
    {
        const float playerCenterY = player_.position().y + PlayerController::Height * 0.5F;
        const auto distanceToPlayer = [playerCenter, playerCenterY](const auto& enemy) {
            if (!enemy.health.isAlive()
                || enemy.archetype == EnemyArchetype::WaterMirrorDemon)
                return std::numeric_limits<float>::max();
            const auto bounds = enemy.controller.bounds();
            const float dx = bounds.left + bounds.width * 0.5F - playerCenter;
            const float dy = bounds.top + bounds.height * 0.5F - playerCenterY;
            return dx * dx + dy * dy;
        };
        auto found = std::min_element(enemies_.begin(), enemies_.end(), [&](const auto& left,
            const auto& right) { return distanceToPlayer(left) < distanceToPlayer(right); });
        if (found != enemies_.end()
            && !intersects(cast.effectBounds, found->controller.bounds())) found = enemies_.end();
        if (found != enemies_.end())
        {
            found->markedRemaining = 4.0F * mastery;
            activeSpellEffects_.back().anchor = ActiveSpellEffect::Anchor::Enemy;
            activeSpellEffects_.back().enemyIndex = static_cast<std::size_t>(
                std::distance(enemies_.begin(), found));
            activeSpellEffects_.back().bounds = found->controller.bounds();
            if (cast.rank >= 3U)
            {
                auto second = std::min_element(enemies_.begin(), enemies_.end(), [&](const auto& left,
                    const auto& right) {
                    const float leftDistance = &left == &*found ? std::numeric_limits<float>::max()
                        : distanceToPlayer(left);
                    const float rightDistance = &right == &*found ? std::numeric_limits<float>::max()
                        : distanceToPlayer(right);
                    return leftDistance < rightDistance;
                });
                if (second != enemies_.end() && second->health.isAlive()
                    && intersects(cast.effectBounds, second->controller.bounds()))
                    second->markedRemaining = 4.0F * mastery;
            }
        }
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
                            multiplier * multiTargetMultiplier});
                    if (result.appliedDamage > 0)
                        onActiveHit(enemy, source, cast.sequence * 8U + beam);
                }
                if (cast.rank >= 3U && enemy.health.isAlive())
                {
                    const auto result = resolveEnemyDamage(enemy,
                        {source, cast.sequence * 8U + 4U, 10,
                            multiplier * multiTargetMultiplier});
                    if (result.appliedDamage > 0)
                        onActiveHit(enemy, source, cast.sequence * 8U + 4U);
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
            spells_.reduceLongestRegularCooldown(cast.rank >= 3U ? 2.5F : 1.5F);
        }
    }
    else if (cast.effect == spells::SpellEffect::ManaStrike)
    {
        for (auto& enemy : enemies_)
            if (enemy.health.isAlive() && intersects(cast.effectBounds, enemy.controller.bounds()))
            {
                const bool postDashHit = postDashComboRemaining_ > 0.0F;
                const int damage = cast.damage + (postDashHit ? 10 : 0);
                static_cast<void>(resolveEnemyDamage(enemy,
                    {source, cast.sequence, damage, multiplier}));
                if (postDashHit && cast.rank >= 3U) player_.reduceDashCooldown(999.0F);
                const float before = enemy.controller.position().x;
                enemy.controller.translateHorizontal(player_.facingDirection() * 120.0F,
                    movementBoundsFor(enemy));
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
                enemy.goldenBindRemaining = (isBossArchetype(enemy.archetype) ? 0.4F : 1.4F)
                    * mastery * relicControlMultiplier(enemy);
                enemy.goldenBindMastered = cast.rank >= 3U;
                enemy.controlRemaining = std::max(enemy.controlRemaining,
                    enemy.goldenBindRemaining);
            }
    }
    else if (cast.effect == spells::SpellEffect::FloatSlam)
    {
        PendingSpellImpact slam {cast.spellId, cast.effectBounds, 1.0F,
            24, cast.sequence, source, multiplier};
        slam.frozenBonusDamage = 12;
        for (std::size_t enemyIndex = 0U; enemyIndex < enemies_.size(); ++enemyIndex)
        {
            auto& enemy = enemies_[enemyIndex];
            if (enemy.health.isAlive() && intersects(cast.effectBounds, enemy.controller.bounds()))
            {
                if (enemy.frozenRemaining > 0.0F)
                    slam.frozenEnemyIndices.push_back(enemyIndex);
                enemy.controlRemaining = std::max(enemy.controlRemaining,
                    (isBossArchetype(enemy.archetype) ? 0.35F : 1.0F)
                        * relicControlMultiplier(enemy));
            }
        }
        pendingSpellImpacts_.push_back(std::move(slam));
        if (cast.rank >= 3U)
            pendingSpellImpacts_.push_back({cast.spellId, cast.effectBounds, 1.05F,
                18, cast.sequence * 32U + 27U, source, multiplier});
    }
    else if (cast.effect == spells::SpellEffect::StoneGolem)
    {
        golemRemaining_ = (extendPersistent ? 6.25F : 5.0F) * mastery;
        const float left = player_.facingDirection() > 0.0F
            ? player_.position().x + 60.0F : player_.position().x - 60.0F;
        golemBounds_ = {left, request_.worldBounds.groundTop - 64.0F, 48.0F, 64.0F};
        golemSource_ = source;
        golemMultiplier_ = multiplier;
        golemSequence_ = cast.sequence;
        golemHitsRemaining_ = cast.rank >= 3U ? 2U : 1U;
        activeSpellEffects_.back().bounds = golemBounds_;
    }
    else if (cast.effect == spells::SpellEffect::Flight)
    {
        player_.grantFlight(2.5F * mastery);
        flightBoostAvailable_ = true;
        flightBoostMultiplier_ = cast.rank >= 3U ? 1.30F : 1.15F;
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
        if (interrupted)
        {
            spellInvulnerableRemaining_ = 0.35F * mastery;
            if (cast.rank >= 3U) spells_.reduceLongestRegularCooldown(1.5F);
        }
    }
    else if (cast.effect == spells::SpellEffect::Seal)
    {
        damageEnemies(source, cast.sequence, cast.damage, multiplier, cast.effectBounds);
        for (auto& enemy : enemies_)
            if (enemy.health.isAlive() && intersects(cast.effectBounds, enemy.controller.bounds()))
            {
                enemy.controller.interrupt();
                onInterrupt(enemy);
                enemy.skillSealRemaining = (isBossArchetype(enemy.archetype) ? 1.5F : 4.0F)
                    + (enemy.markedRemaining > 0.0F
                        ? (cast.rank >= 3U ? 2.0F : 1.0F) : 0.0F);
                enemy.skillSealRemaining *= mastery;
            }
    }
    else if (cast.effect == spells::SpellEffect::LightningStaff)
    {
        lightningStaffRemaining_ = 4.0F * mastery;
        lightningStaffCharges_ = cast.rank >= 3U ? 4U : 3U;
        lightningStaffDamageMultiplier_ = multiplier;
        lightningStaffBurstDamage_ = cast.rank >= 3U ? 20 : 12;
    }
    else if (cast.effect == spells::SpellEffect::HomingVolley)
    {
        const float playerCenterY = player_.position().y + PlayerController::Height * 0.5F;
        const auto distanceToPlayer = [playerCenter, playerCenterY, &cast](const auto& enemy) {
            if (!enemy.health.isAlive()
                || enemy.archetype == EnemyArchetype::WaterMirrorDemon
                || !intersects(cast.effectBounds, enemy.controller.bounds()))
                return std::numeric_limits<float>::max();
            const auto bounds = enemy.controller.bounds();
            const float dx = bounds.left + bounds.width * 0.5F - playerCenter;
            const float dy = bounds.top + bounds.height * 0.5F - playerCenterY;
            return dx * dx + dy * dy;
        };
        auto found = std::min_element(enemies_.begin(), enemies_.end(), [&](const auto& left,
            const auto& right) {
            return distanceToPlayer(left) < distanceToPlayer(right);
        });
        if (found != enemies_.end()
            && distanceToPlayer(*found) == std::numeric_limits<float>::max())
            found = enemies_.end();
        if (found != enemies_.end())
            for (std::uint64_t bolt = 1U;
                bolt <= (cast.rank >= 3U ? 4U : 3U) && found->health.isAlive(); ++bolt)
            {
                const auto result = resolveEnemyDamage(*found,
                    {source, cast.sequence * 8U + bolt,
                        (bolt == 4U ? 10 : 7)
                            + (bolt == 3U && found->controller.config().flying ? 6 : 0),
                        multiplier});
                if (result.appliedDamage > 0)
                    onActiveHit(*found, source, cast.sequence * 8U + bolt);
            }
    }
    else if (cast.effect == spells::SpellEffect::DefensiveBarrier)
    {
        barrierShield_ = static_cast<int>(std::lround(24.0F * mastery));
        barrierRemaining_ = 5.0F * mastery;
        barrierBreakDamage_ = cast.rank >= 3U ? 20 : 12;
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
                {source, cast.sequence, 14, multiplier * multiTargetMultiplier});
            if (result.appliedDamage > 0) onActiveHit(enemy, source, cast.sequence);
            if (enemy.controller.action() == ai::EnemyAction::Windup
                || enemy.controller.action() == ai::EnemyAction::Active)
            {
                enemy.controller.interrupt();
                onInterrupt(enemy);
            }
            enemy.controller.translateHorizontal(player_.facingDirection() * 130.0F,
                movementBoundsFor(enemy));
            const auto moved = enemy.controller.bounds();
            const auto movementBounds = movementBoundsFor(enemy);
            const bool hitWall = moved.left <= movementBounds.left + 0.01F
                || moved.left + moved.width >= movementBounds.right - 0.01F;
            if (hitWall && enemy.health.isAlive())
                static_cast<void>(resolveEnemyDamage(enemy,
                    {source, cast.sequence * 8U + 7U,
                        cast.rank >= 3U ? 18 : 10, multiplier}));
            ++hitIndex;
        }
    }
    else if (cast.effect == spells::SpellEffect::GravityWell)
    {
        gravityWellRemaining_ = (extendPersistent ? 3.125F : 2.5F) * mastery;
        gravityWellTickAccumulator_ = 0.0F;
        gravityWellBounds_ = cast.effectBounds;
        gravityWellSource_ = source;
        gravityWellMultiplier_ = multiplier;
        gravityWellPullSpeed_ = cast.rank >= 3U ? 121.5F : 90.0F;
        gravityWellTickDamage_ = cast.rank >= 3U ? 8 : 6;
        gravityWellSequenceBase_ = cast.sequence * 8U;
        gravityWellTick_ = 0U;
    }
    else if (cast.effect == spells::SpellEffect::BossZoltraak)
    {
        for (auto& enemy : enemies_)
        {
            if (!enemy.health.isAlive() || !intersects(cast.effectBounds, enemy.controller.bounds()))
                continue;
                static_cast<void>(resolveEnemyDamage(enemy,
                {source, cast.sequence, cast.damage,
                    multiplier * (isDemonArchetype(enemy.archetype) ? 1.3F : 1.0F)}));
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
                    const bool refundBinding = enemy.goldenBindMastered;
                    enemy.goldenBindRemaining = 0.0F;
                    enemy.goldenBindMastered = false;
                static_cast<void>(resolveEnemyDamage(enemy,
                        {source, cast.sequence * 16U + 13U, 16, multiplier}));
                    if (refundBinding)
                    {
                        const auto views = spells_.view();
                        for (std::size_t index = 0U; index < views.size(); ++index)
                            if (views[index].id == 1020U)
                                spells_.reduceRegularCooldown(index,
                                    views[index].cooldownDuration * 0.30F);
                    }
                }
            }
    }
    else if (cast.effect == spells::SpellEffect::Mimic)
    {
        const float playerCenterY = player_.position().y + PlayerController::Height * 0.5F;
        const auto distanceToPlayer = [playerCenter, playerCenterY](const auto& enemy) {
            if (!enemy.health.isAlive()
                || enemy.archetype == EnemyArchetype::WaterMirrorDemon)
                return std::numeric_limits<float>::max();
            const auto bounds = enemy.controller.bounds();
            const float dx = bounds.left + bounds.width * 0.5F - playerCenter;
            const float dy = bounds.top + bounds.height * 0.5F - playerCenterY;
            return dx * dx + dy * dy;
        };
        const auto found = std::min_element(enemies_.begin(), enemies_.end(), [&](const auto& left,
            const auto& right) { return distanceToPlayer(left) < distanceToPlayer(right); });
        const float copyRangeSquared = definition->range * definition->range;
        if (found != enemies_.end() && distanceToPlayer(*found) <= copyRangeSquared)
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
            const Aabb copiedTemplate = found->controller.attackBounds();
            const float copiedDirection = player_.facingDirection();
            const Aabb copiedArea {copiedDirection > 0.0F
                    ? player_.position().x + PlayerController::Width
                    : player_.position().x - copiedTemplate.width,
                playerCenterY - copiedTemplate.height * 0.5F,
                copiedTemplate.width, copiedTemplate.height};
            activeSpellEffects_.back().bounds = copiedArea;
            activeSpellEffects_.back().facingDirection = copiedDirection;
            damageEnemies(source, cast.sequence,
                std::max(60, static_cast<int>(std::lround(copiedDamage * 2.0F))),
                multiplier, copiedArea);
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
        if (selectedTarget.width > 0.0F)
            activeSpellEffects_.back().bounds.top = selectedTarget.top + selectedTarget.height
                - activeSpellEffects_.back().bounds.height;
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
        const float impactGround = selectedTarget.width > 0.0F
            ? selectedTarget.top + selectedTarget.height
            : cast.effectBounds.top + cast.effectBounds.height;
        damageEnemies(source, cast.sequence * 8U, 45, multiplier, cast.effectBounds);
        for (std::uint64_t pillarIndex = 0U; pillarIndex < 3U; ++pillarIndex)
        {
            const Aabb pillar {center - 88.0F + static_cast<float>(pillarIndex) * 64.0F,
                impactGround - 160.0F, 48.0F, 160.0F};
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

    if (cast.effect == spells::SpellEffect::DirectDamage
        || cast.effect == spells::SpellEffect::MultiZoltraak
        || cast.effect == spells::SpellEffect::BossZoltraak)
    {
        const float muzzleDuration = fullClipDuration(6U, 15.0F);
        activeSpellEffects_.push_back({ZoltraakMuzzleVisualId, playerBounds(),
            muzzleDuration, muzzleDuration, player_.facingDirection(),
            ActiveSpellEffect::Anchor::Player});
    }
}
}
