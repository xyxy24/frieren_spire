#pragma once

#include "game/combat/CombatContracts.hpp"
#include "common/ui/UiStates.hpp"

#include <span>
#include <vector>

namespace arcane::presentation::viewmodel
{
using DamageNumberViewModel = common::ui::DamageNumberState;
using ImpactBurstViewModel = common::ui::ImpactBurstState;

struct CombatFeedbackTuning
{
    float playerFlashSeconds {0.16F};
    float enemyFlashSeconds {0.12F};
    float damageNumberSeconds {0.72F};
    float impactBurstSeconds {0.18F};
    float enemyVisualKickSeconds {0.10F};
    float lightHitStopSeconds {0.06F};
    float mediumHitStopSeconds {0.09F};
    float heavyHitStopSeconds {0.13F};
    float bossHitStopMultiplier {0.45F};
    float bossHitStopMaximumSeconds {0.055F};
    float playerHitStopSeconds {0.075F};
};

using CombatFeedbackSnapshot = common::ui::CombatFeedbackState;

class CombatFeedbackViewModel
{
public:
    explicit CombatFeedbackViewModel(CombatFeedbackTuning tuning = {}) noexcept;

    void reset() noexcept;
    void update(const game::PlayerStateView& player,
        std::span<const game::EnemyStateView> enemies, float deltaSeconds);

    [[nodiscard]] const CombatFeedbackSnapshot& snapshot() const noexcept;
    [[nodiscard]] float combatDeltaSeconds(float realDeltaSeconds) const noexcept;

private:
    void beginShake(float strength, float duration) noexcept;
    void beginHitStop(float duration) noexcept;
    void addDamageNumber(game::Vec2 position, int amount, bool playerTarget);
    void addImpactBurst(game::Vec2 position, bool playerTarget, bool lethal);

    CombatFeedbackTuning tuning_;
    bool initialized_ {};
    int previousPlayerHealth_ {};
    std::vector<int> previousEnemyHealth_;
    std::vector<game::Vec2> previousEnemyPositions_;
    std::vector<bool> previousEnemyAlive_;
    float playerFlashRemaining_ {};
    std::vector<float> enemyFlashRemaining_;
    std::vector<float> enemyVisualKickRemaining_;
    std::vector<float> enemyVisualKickDirection_;
    float shakeRemaining_ {};
    float shakeDuration_ {};
    float shakeStrength_ {};
    float shakeClock_ {};
    float hitStopRemaining_ {};
    CombatFeedbackSnapshot snapshot_;
};
}
