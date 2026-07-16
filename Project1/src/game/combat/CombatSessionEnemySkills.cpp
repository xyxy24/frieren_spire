#include "game/combat/CombatSession.hpp"

#include <algorithm>
#include <cmath>

namespace arcane::game
{
namespace
{
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
}

bool CombatSession::updateEnemySkills(const float deltaSeconds, const float playerCenter)
{
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
                enemy.controller.setPosition(position, movementBoundsFor(enemy));
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
                        enemy.specialDirection * 320.0F * activeDelta, movementBoundsFor(enemy));
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
                    enemy.controller.setPosition(position, movementBoundsFor(enemy));
                }
                enemy.specialActive -= activeDelta;
                if (enemy.specialActive <= 0.0F)
                {
                    if (enemy.manualSkill == 0)
                    {
                        const auto landed = enemy.controller.bounds();
                        const Aabb impact {landed.left + landed.width * 0.5F - 144.0F,
                            request_.worldBounds.groundTop - 144.0F, 288.0F, 144.0F};
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
                        ^ (static_cast<std::uint64_t>(enemy.summonCount)
                            * 0x9e3779b97f4a7c15ULL);
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
                        enemy.specialDirection * 220.0F * activeDelta, movementBoundsFor(enemy));
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
                enemy.specialDirection = enemy.controller.facingDirection();
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
                enemy.controller.setPosition(position, movementBoundsFor(enemy));
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
        if (enemy.archetype == EnemyArchetype::Aura)
        {
            enemy.secondaryCooldown = std::max(0.0F,
                enemy.secondaryCooldown - deltaSeconds);
            if (enemy.specialWindup > 0.0F)
            {
                enemy.specialElapsed += deltaSeconds;
                enemy.specialWindup = std::max(0.0F, enemy.specialWindup - deltaSeconds);
                if (enemy.specialWindup <= 0.0F)
                {
                    enemy.specialActive = AuraGuillotineActiveSeconds;
                    enemy.specialElapsed = 0.0F;
                    const auto result = intersects(playerBounds(), enemy.specialTargetBounds)
                        ? resolvePlayerDamage({DamageSource::EnemyAttack,
                            (static_cast<std::uint64_t>(enemyIndex + 1U) << 32U)
                                | ++environmentalSequence_, AuraGuillotineDamage,
                            1.0F, relics_.incomingDamageMultiplier(), 0,
                            player_.isShadowDashing() || spellInvulnerableRemaining_ > 0.0F})
                        : DamageResult {};
                    if (result.appliedDamage > 0) beamRemaining_ = 0.0F;
                }
                continue;
            }
            if (enemy.specialActive > 0.0F)
            {
                enemy.specialElapsed += deltaSeconds;
                enemy.specialActive = std::max(0.0F, enemy.specialActive - deltaSeconds);
                if (enemy.specialActive <= 0.0F)
                {
                    enemy.secondaryCooldown = AuraGuillotineCooldownSeconds;
                    enemy.manualSkill = -1;
                }
                continue;
            }
            if (enemy.secondaryCooldown <= 0.0F && auraFirstDominationDialogueShown_
                && enemy.controller.action() == ai::EnemyAction::Chase
                && enemy.skillSealRemaining <= 0.0F && enemy.controlRemaining <= 0.0F
                && enemy.frozenRemaining <= 0.0F)
            {
                const auto player = playerBounds();
                const float targetCenter = player.left + player.width * 0.5F;
                const float left = std::clamp(targetCenter - AuraGuillotineWidth * 0.5F,
                    request_.worldBounds.left,
                    request_.worldBounds.right - AuraGuillotineWidth);
                enemy.specialTargetBounds = {left,
                    request_.worldBounds.groundTop - AuraGuillotineHeight,
                    AuraGuillotineWidth, AuraGuillotineHeight};
                enemy.specialWindup = AuraGuillotineWindupSeconds;
                enemy.specialElapsed = 0.0F;
                enemy.manualSkill = 0;
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
            enemy.controller.update(target, deltaSeconds, movementBoundsFor(enemy), speedMultiplier,
                enemy.skillSealRemaining <= 0.0F && !manualSkill);
            if (enemy.archetype == EnemyArchetype::Aura
                && enemy.controller.action() == ai::EnemyAction::Windup
                && !auraFirstDominationDialogueShown_)
            {
                auraFirstDominationDialogueShown_ = true;
                beginDialogue(DialogueScript::AuraFirstDomination);
                return false;
            }
        }
        if (phantomRemaining_ > 0.0F && enemy.controller.action() == ai::EnemyAction::Active
            && intersects(phantomBounds_, enemy.controller.attackBounds()))
        {
            phantomRemaining_ = 0.0F;
            enemy.markedRemaining = std::max(enemy.markedRemaining, 3.0F);
            spells_.reduceLongestRegularCooldown(1.0F);
            std::erase_if(activeSpellEffects_, [](const auto& effect) { return effect.spellId == 1011U; });
            activeSpellEffects_.push_back({PhantomBreakVisualId, phantomBounds_,
                4.0F / 12.0F, 4.0F / 12.0F});
        }
        if (golemRemaining_ > 0.0F && enemy.controller.action() == ai::EnemyAction::Active
            && intersects(golemBounds_, enemy.controller.attackBounds()))
        {
            if (golemHitsRemaining_ > 1U)
            {
                --golemHitsRemaining_;
                enemy.controller.interrupt();
                continue;
            }
            golemRemaining_ = 0.0F;
            golemHitsRemaining_ = 0U;
            std::erase_if(activeSpellEffects_, [](const auto& effect) {
                return effect.spellId == 1022U;
            });
            const Aabb blast {golemBounds_.left - 49.0F, golemBounds_.top - 35.0F, 140.0F, 140.0F};
            for (auto& target : enemies_)
                if (target.health.isAlive() && intersects(blast, target.controller.bounds()))
                    static_cast<void>(resolveEnemyDamage(target,
                        {golemSource_, golemSequence_ * 16U + 2U, 24, golemMultiplier_}));
            activeSpellEffects_.push_back({StoneGolemResolveVisualId, blast,
                4.0F / 10.0F, 4.0F / 10.0F});
        }
    }
    return true;
}
}
