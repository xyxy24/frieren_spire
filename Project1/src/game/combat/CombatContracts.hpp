#pragma once

#include "game/math/Vec2.hpp"
#include "game/combat/Aabb.hpp"
#include "game/player/PlayerController.hpp"
#include "common/ui/UiStates.hpp"

#include <cstdint>
#include <array>
#include <optional>
#include <string_view>
#include <vector>

#include "game/spells/SpellSystem.hpp"

namespace arcane::game
{
inline constexpr auto BurningFlowerVisualId = common::ui::BurningFlowerVisualId;
inline constexpr auto PhantomBreakVisualId = common::ui::PhantomBreakVisualId;
inline constexpr auto StoneGolemResolveVisualId = common::ui::StoneGolemResolveVisualId;
inline constexpr auto LightningStaffDischargeVisualId = common::ui::LightningStaffDischargeVisualId;
inline constexpr auto DefensiveBarrierBreakVisualId = common::ui::DefensiveBarrierBreakVisualId;
inline constexpr auto MirrorArrayBreakVisualId = common::ui::MirrorArrayBreakVisualId;
inline constexpr auto ZoltraakMuzzleVisualId = common::ui::ZoltraakMuzzleVisualId;
inline constexpr auto FrierenCopyBeamVisualId = common::ui::FrierenCopyBeamVisualId;
inline constexpr auto FrierenCopyLightningVisualId = common::ui::FrierenCopyLightningVisualId;
inline constexpr auto FrierenCopyGroundFireVisualId = common::ui::FrierenCopyGroundFireVisualId;
inline constexpr auto CopyBeamTelegraphVisualId = common::ui::CopyBeamTelegraphVisualId;
inline constexpr auto FrierenCopyGroundFireTelegraphVisualId = common::ui::FrierenCopyGroundFireTelegraphVisualId;
inline constexpr auto RevolteCrossSlashFirstTelegraphVisualId = common::ui::RevolteCrossSlashFirstTelegraphVisualId;
inline constexpr auto RevolteCrossSlashSecondTelegraphVisualId = common::ui::RevolteCrossSlashSecondTelegraphVisualId;
inline constexpr auto RevolteCrossSlashVisualId = common::ui::RevolteCrossSlashVisualId;
inline constexpr auto RevolteCrossSlashImpactVisualId = common::ui::RevolteCrossSlashImpactVisualId;
inline constexpr auto RevolteFlyingBladeTelegraphVisualId = common::ui::RevolteFlyingBladeTelegraphVisualId;
inline constexpr auto RevolteFlyingBladeVisualId = common::ui::RevolteFlyingBladeVisualId;

enum class CombatOutcome
{
    Victory,
    Defeat
};
using EnemyArchetype = common::ui::EnemyArchetype;

struct EnemySpawn
{
    EnemyArchetype archetype {EnemyArchetype::HeadlessKnight};
    Vec2 position {800.0F, 576.0F};
    std::optional<WorldBounds> movementBounds;
    bool flyingPlacement {};
};

struct CombatRequest
{
    std::uint32_t encounterId {1};
    std::uint64_t seed {0};
    Vec2 playerSpawn {160.0F, 576.0F};
    Vec2 enemySpawn {800.0F, 576.0F};
    WorldBounds worldBounds {0.0F, 1280.0F, 640.0F};
    std::vector<Aabb> oneWayPlatforms;
    int playerMaximumHealth {100};
    int playerCurrentHealth {100};
    int enemyMaximumHealth {100};
    int enemyContactDamage {10};
    int enemyAttackDamage {10};
    float enemyControlSeconds {0.28F};
    EnemyArchetype enemyArchetype {EnemyArchetype::HeadlessKnight};
    std::vector<EnemySpawn> enemies;
    int goldReward {10};
    std::array<std::optional<std::uint32_t>, 3> equippedSpellIds;
    std::array<std::uint8_t, 3> equippedSpellRanks {1U, 1U, 1U};
    std::optional<std::uint32_t> equippedUltimateSpellId;
    float regularSpellDamageMultiplier {1.0F};
    float regularSpellCooldownMultiplier {1.0F};
    float playerDamageMultiplier {1.0F};
    int startingShield {};
    std::vector<std::uint32_t> relicIds;
};

struct CombatResult
{
    CombatOutcome outcome {CombatOutcome::Defeat};
    std::uint32_t encounterId {0};
    int defeatedEnemies {0};
    int goldAwarded {0};
    int playerHealthRemaining {0};
};

using PlayerStateView = common::ui::PlayerCombatState;
using EnemyStateView = common::ui::EnemyState;
using SpellEffectView = common::ui::SpellEffectState;
using CombatDialogueLineView = common::ui::CombatDialogueState;
using BossIntroView = common::ui::BossIntroState;
}
