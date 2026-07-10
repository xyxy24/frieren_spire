#pragma once

#include "game/math/Vec2.hpp"
#include "game/player/PlayerController.hpp"

#include <cstdint>
#include <array>
#include <optional>

#include "game/spells/SpellSystem.hpp"

namespace arcane::game
{
enum class CombatOutcome
{
    Victory,
    Defeat
};

struct CombatRequest
{
    std::uint32_t encounterId {1};
    std::uint64_t seed {0};
    Vec2 playerSpawn {160.0F, 576.0F};
    Vec2 enemySpawn {800.0F, 576.0F};
    WorldBounds worldBounds {0.0F, 1280.0F, 640.0F};
    int playerMaximumHealth {100};
    int playerCurrentHealth {100};
    int enemyMaximumHealth {100};
    int enemyContactDamage {10};
    int goldReward {10};
    std::array<std::optional<std::uint32_t>, 3> equippedSpellIds;
};

struct CombatResult
{
    CombatOutcome outcome {CombatOutcome::Defeat};
    std::uint32_t encounterId {0};
    int defeatedEnemies {0};
    int goldAwarded {0};
    int playerHealthRemaining {0};
};

struct PlayerStateView
{
    Vec2 position;
    int currentHealth {0};
    int maximumHealth {0};
    bool grounded {false};
    float facingDirection {1.0F};
    bool attackActive {false};
    float attackCooldownRemaining {0.0F};
    std::array<spells::SpellSlotView, 3> spellSlots;
    bool stunned {false};
    float stunRemaining {0.0F};
};

struct EnemyStateView
{
    Vec2 position;
    int currentHealth {0};
    int maximumHealth {0};
    bool alive {false};
    bool windingUp {false};
    bool attackActive {false};
};
}
