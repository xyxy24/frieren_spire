#include "presentation/viewmodel/CombatFeedbackViewModel.hpp"

#include "game/player/PlayerController.hpp"

#include <algorithm>
#include <cmath>

namespace arcane::presentation::viewmodel
{
CombatFeedbackViewModel::CombatFeedbackViewModel(const CombatFeedbackTuning tuning) noexcept
    : tuning_(tuning)
{
}

void CombatFeedbackViewModel::reset() noexcept
{
    initialized_ = false;
    previousPlayerHealth_ = 0;
    previousEnemyHealth_.clear();
    previousEnemyPositions_.clear();
    previousEnemyAlive_.clear();
    playerFlashRemaining_ = 0.0F;
    enemyFlashRemaining_.clear();
    enemyVisualKickRemaining_.clear();
    enemyVisualKickDirection_.clear();
    shakeRemaining_ = 0.0F;
    shakeDuration_ = 0.0F;
    shakeStrength_ = 0.0F;
    shakeClock_ = 0.0F;
    hitStopRemaining_ = 0.0F;
    snapshot_ = {};
}

void CombatFeedbackViewModel::update(const game::PlayerStateView& player,
    const std::span<const game::EnemyStateView> enemies, const float deltaSeconds)
{
    const float elapsed = std::max(0.0F, deltaSeconds);
    if (!initialized_)
    {
        initialized_ = true;
        previousPlayerHealth_ = player.currentHealth;
        previousEnemyHealth_.reserve(enemies.size());
        previousEnemyPositions_.reserve(enemies.size());
        previousEnemyAlive_.reserve(enemies.size());
        for (const auto& enemy : enemies)
        {
            previousEnemyHealth_.push_back(enemy.currentHealth);
            previousEnemyPositions_.push_back(enemy.position);
            previousEnemyAlive_.push_back(enemy.alive);
        }
        enemyFlashRemaining_.assign(enemies.size(), 0.0F);
        enemyVisualKickRemaining_.assign(enemies.size(), 0.0F);
        enemyVisualKickDirection_.assign(enemies.size(), 0.0F);
    }

    playerFlashRemaining_ = std::max(0.0F, playerFlashRemaining_ - elapsed);
    for (float& remaining : enemyFlashRemaining_)
        remaining = std::max(0.0F, remaining - elapsed);
    for (float& remaining : enemyVisualKickRemaining_)
        remaining = std::max(0.0F, remaining - elapsed);
    shakeRemaining_ = std::max(0.0F, shakeRemaining_ - elapsed);
    hitStopRemaining_ = std::max(0.0F, hitStopRemaining_ - elapsed);
    shakeClock_ += elapsed;
    for (auto& number : snapshot_.damageNumbers)
    {
        number.remaining = std::max(0.0F, number.remaining - elapsed);
        number.position.y -= 42.0F * elapsed;
    }
    std::erase_if(snapshot_.damageNumbers,
        [](const auto& number) { return number.remaining <= 0.0F; });
    for (auto& burst : snapshot_.impactBursts)
        burst.remaining = std::max(0.0F, burst.remaining - elapsed);
    std::erase_if(snapshot_.impactBursts,
        [](const auto& burst) { return burst.remaining <= 0.0F; });

    const int playerDamage = std::max(0, previousPlayerHealth_ - player.currentHealth);
    if (playerDamage > 0)
    {
        playerFlashRemaining_ = tuning_.playerFlashSeconds;
        beginHitStop(tuning_.playerHitStopSeconds);
        beginShake(std::clamp(5.0F + static_cast<float>(playerDamage) * 0.18F, 6.0F, 11.0F), 0.22F);
        addDamageNumber({player.position.x + game::PlayerController::Width * 0.5F,
            player.position.y - 8.0F}, playerDamage, true);
        addImpactBurst({player.position.x + game::PlayerController::Width * 0.5F,
            player.position.y + game::PlayerController::Height * 0.45F}, true,
            player.currentHealth <= 0);
    }
    previousPlayerHealth_ = player.currentHealth;

    if (previousEnemyHealth_.size() < enemies.size())
    {
        const std::size_t oldSize = previousEnemyHealth_.size();
        previousEnemyHealth_.resize(enemies.size());
        previousEnemyPositions_.resize(enemies.size());
        previousEnemyAlive_.resize(enemies.size());
        enemyFlashRemaining_.resize(enemies.size(), 0.0F);
        enemyVisualKickRemaining_.resize(enemies.size(), 0.0F);
        enemyVisualKickDirection_.resize(enemies.size(), 0.0F);
        for (std::size_t index = oldSize; index < enemies.size(); ++index)
        {
            previousEnemyHealth_[index] = enemies[index].currentHealth;
            previousEnemyPositions_[index] = enemies[index].position;
            previousEnemyAlive_[index] = enemies[index].alive;
        }
    }
    for (std::size_t index = 0U; index < enemies.size(); ++index)
    {
        const auto& enemy = enemies[index];
        const int damage = std::max(0, previousEnemyHealth_[index] - enemy.currentHealth);
        if (damage > 0)
        {
            enemyFlashRemaining_[index] = tuning_.enemyFlashSeconds;
            enemyVisualKickRemaining_[index] = tuning_.enemyVisualKickSeconds;
            const float playerCenter = player.position.x + game::PlayerController::Width * 0.5F;
            const float enemyCenter = enemy.position.x + enemy.width * 0.5F;
            enemyVisualKickDirection_[index] = enemyCenter >= playerCenter ? 1.0F : -1.0F;
            beginHitStop(damage <= 18 ? tuning_.lightHitStopSeconds
                : damage <= 35 ? tuning_.mediumHitStopSeconds : tuning_.heavyHitStopSeconds);
            beginShake(std::clamp(2.0F + static_cast<float>(damage) * 0.08F, 2.5F, 6.0F), 0.12F);
            const game::Vec2 position = enemy.alive ? enemy.position : previousEnemyPositions_[index];
            addDamageNumber({position.x + enemy.width * 0.5F, position.y - 8.0F}, damage, false);
            addImpactBurst({position.x + enemy.width * 0.5F,
                position.y + enemy.height * 0.45F}, false,
                previousEnemyAlive_[index] && !enemy.alive);
        }
        previousEnemyHealth_[index] = enemy.currentHealth;
        previousEnemyPositions_[index] = enemy.position;
        previousEnemyAlive_[index] = enemy.alive;
    }

    const float shakeRatio = shakeDuration_ > 0.0F
        ? std::clamp(shakeRemaining_ / shakeDuration_, 0.0F, 1.0F) : 0.0F;
    snapshot_.cameraOffset = {
        std::sin(shakeClock_ * 91.0F) * shakeStrength_ * shakeRatio,
        std::cos(shakeClock_ * 73.0F) * shakeStrength_ * 0.55F * shakeRatio};
    snapshot_.playerFlashRatio = tuning_.playerFlashSeconds > 0.0F
        ? std::clamp(playerFlashRemaining_ / tuning_.playerFlashSeconds, 0.0F, 1.0F) : 0.0F;
    snapshot_.enemyFlashRatios.resize(enemies.size());
    snapshot_.enemyImpactOffsets.resize(enemies.size());
    for (std::size_t index = 0U; index < enemies.size(); ++index)
    {
        snapshot_.enemyFlashRatios[index] = tuning_.enemyFlashSeconds > 0.0F
            ? std::clamp(enemyFlashRemaining_[index] / tuning_.enemyFlashSeconds, 0.0F, 1.0F)
            : 0.0F;
        const float kickRatio = tuning_.enemyVisualKickSeconds > 0.0F
            ? std::clamp(enemyVisualKickRemaining_[index] / tuning_.enemyVisualKickSeconds,
                0.0F, 1.0F) : 0.0F;
        snapshot_.enemyImpactOffsets[index] = enemyVisualKickDirection_[index] * 7.0F * kickRatio;
    }
    snapshot_.hitStopRemaining = hitStopRemaining_;
}

const CombatFeedbackSnapshot& CombatFeedbackViewModel::snapshot() const noexcept
{
    return snapshot_;
}

float CombatFeedbackViewModel::combatDeltaSeconds(const float realDeltaSeconds) const noexcept
{
    return hitStopRemaining_ > 0.0F ? 0.0F : std::max(0.0F, realDeltaSeconds);
}

void CombatFeedbackViewModel::beginShake(const float strength, const float duration) noexcept
{
    if (strength < shakeStrength_ && shakeRemaining_ > duration * 0.5F) return;
    shakeStrength_ = strength;
    shakeDuration_ = duration;
    shakeRemaining_ = duration;
}

void CombatFeedbackViewModel::beginHitStop(const float duration) noexcept
{
    hitStopRemaining_ = std::max(hitStopRemaining_, std::max(0.0F, duration));
}

void CombatFeedbackViewModel::addDamageNumber(
    const game::Vec2 position, const int amount, const bool playerTarget)
{
    snapshot_.damageNumbers.push_back(
        {position, amount, tuning_.damageNumberSeconds, tuning_.damageNumberSeconds, playerTarget});
}

void CombatFeedbackViewModel::addImpactBurst(
    const game::Vec2 position, const bool playerTarget, const bool lethal)
{
    const float lifetime = lethal ? tuning_.impactBurstSeconds * 1.6F : tuning_.impactBurstSeconds;
    snapshot_.impactBursts.push_back({position, lifetime, lifetime, playerTarget, lethal});
}
}
