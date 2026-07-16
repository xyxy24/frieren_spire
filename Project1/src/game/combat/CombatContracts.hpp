#pragma once

#include "game/math/Vec2.hpp"
#include "game/combat/Aabb.hpp"
#include "game/player/PlayerController.hpp"

#include <cstdint>
#include <array>
#include <optional>
#include <string_view>
#include <vector>

#include "game/spells/SpellSystem.hpp"

namespace arcane::game
{
inline constexpr std::uint32_t BurningFlowerVisualId = 9006U;
inline constexpr std::uint32_t PhantomBreakVisualId = 9007U;
inline constexpr std::uint32_t StoneGolemResolveVisualId = 9008U;
inline constexpr std::uint32_t LightningStaffDischargeVisualId = 9009U;
inline constexpr std::uint32_t DefensiveBarrierBreakVisualId = 9010U;
inline constexpr std::uint32_t MirrorArrayBreakVisualId = 9011U;
inline constexpr std::uint32_t FrierenCopyBeamVisualId = 9400U;
inline constexpr std::uint32_t FrierenCopyLightningVisualId = 9401U;
inline constexpr std::uint32_t FrierenCopyGroundFireVisualId = 9402U;

enum class CombatOutcome
{
    Victory,
    Defeat
};
enum class EnemyArchetype : std::uint8_t {
    ChestMimic, HeadlessKnight, BirdDemon, Lugner, Linie, Draht,
    ChaosFlower, FrostWolf, Qual, Laufen, Richter, Denken,
    Heimon, DemonWarrior, LargeBirdDemon, Gargoyle, ThreeHeadedDemon, SwordDemon,
    Aura, Revolte, RedMirrorDragon, WaterMirrorDemon, StarkCopy, FernCopy, FrierenCopy, Boss
};

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

struct PlayerStateView
{
    Vec2 position;
    Vec2 velocity;
    int currentHealth {0};
    int maximumHealth {0};
    bool grounded {false};
    float facingDirection {1.0F};
    bool attackActive {false};
    float attackCooldownRemaining {0.0F};
    std::uint64_t attackSequence {};
    std::uint64_t castSequence {};
    std::array<spells::SpellSlotView, 3> spellSlots;
    spells::SpellSlotView ultimateSpellSlot;
    float dashRemaining {0.0F};
    float dashCooldownRemaining {0.0F};
    bool shadowDashing {false};
    float shadowDashChargeRemaining {0.0F};
    bool stunned {false};
    float stunRemaining {0.0F};
    float hitInvulnerabilityRemaining {0.0F};
    std::uint64_t hurtSequence {};
    float blessingRemaining {0.0F};
    float vulnerableRemaining {0.0F};
    float flowerFieldRemaining {0.0F};
    float flightRemaining {0.0F};
    int shield {0};
    float sleepRemaining {0.0F};
};

struct EnemyStateView
{
    EnemyArchetype archetype {EnemyArchetype::HeadlessKnight};
    Vec2 position;
    float width {48.0F};
    float height {64.0F};
    int currentHealth {0};
    int maximumHealth {0};
    bool alive {false};
    bool windingUp {false};
    bool attackActive {false};
    bool slowed {false};
    std::optional<Aabb> skillEffectBounds;
    float facingDirection {-1.0F};
    bool marked {false};
    float skillEffectProgress {};
    float concealmentProgress {};
    bool specialWindingUp {false};
    bool specialAttackActive {false};
    int skillVariant {-1};
    bool returningToAir {false};
    bool activated {false};
};

struct SpellEffectView
{
    std::uint32_t spellId {};
    Aabb bounds;
    float remaining {};
    float duration {};
    float facingDirection {1.0F};
    float rotationDegrees {0.0F};
};

struct CombatDialogueLineView
{
    std::string_view speaker;
    std::string_view text;
    std::string_view portrait;
};

struct BossIntroView
{
    std::string_view name;
    float remaining {};
    float duration {};
};
}
