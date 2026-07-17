#pragma once

#include "common/geometry/Geometry.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace arcane::common::ui
{
using ContentId = std::uint32_t;
using Seed = std::uint64_t;

inline constexpr float PlayerWidth = 42.0F;
inline constexpr float PlayerHeight = 64.0F;

enum class ApplicationScreen : std::uint8_t { Start, Playing, Pause, Result };
enum class PauseMenuItem : std::uint8_t { ReplayCurrentFloor, SaveAndExit };
enum class ContentKind : std::uint8_t { Unknown, Spell, Relic };
enum class SpellRangeKind : std::uint8_t { Self, ForwardBox, SelfArea, TargetArea, Summon };
enum class LoadoutPage : std::uint8_t { Spells, Relics };
enum class SpellSection : std::uint8_t { Regular, Boss };
enum class FloorKind : std::uint8_t { Combat, Event, Merchant, Boss };
enum class GameplayPhase : std::uint8_t {
    FloorLoading, InEncounter, LootPending, Reward, Breakthrough,
    FloorComplete, Defeat, Victory
};
enum class EventFloorState : std::uint8_t { Untriggered, Choosing, Result };
enum class EventKind : std::uint8_t { AldenBall, HalfCenturyMeteorShower, SwordVillage, SouthernHero };
enum class ArenaTheme : std::uint8_t { AuraOccupation, NorthernFrontier, MageExam };
enum class EnemyArchetype : std::uint8_t {
    ChestMimic, HeadlessKnight, BirdDemon, Lugner, Linie, Draht,
    ChaosFlower, FrostWolf, Qual, Laufen, Richter, Denken,
    Heimon, DemonWarrior, LargeBirdDemon, Gargoyle, ThreeHeadedDemon, SwordDemon,
    Aura, Revolte, RedMirrorDragon, WaterMirrorDemon, StarkCopy, FernCopy, FrierenCopy, Boss
};

inline constexpr std::uint32_t BurningFlowerVisualId = 9006U;
inline constexpr std::uint32_t PhantomBreakVisualId = 9007U;
inline constexpr std::uint32_t StoneGolemResolveVisualId = 9008U;
inline constexpr std::uint32_t LightningStaffDischargeVisualId = 9009U;
inline constexpr std::uint32_t DefensiveBarrierBreakVisualId = 9010U;
inline constexpr std::uint32_t MirrorArrayBreakVisualId = 9011U;
inline constexpr std::uint32_t ZoltraakMuzzleVisualId = 9012U;
inline constexpr std::uint32_t FrierenCopyBeamVisualId = 9400U;
inline constexpr std::uint32_t FrierenCopyLightningVisualId = 9401U;
inline constexpr std::uint32_t FrierenCopyGroundFireVisualId = 9402U;
inline constexpr std::uint32_t CopyBeamTelegraphVisualId = 9403U;
inline constexpr std::uint32_t FrierenCopyGroundFireTelegraphVisualId = 9404U;
inline constexpr std::uint32_t RevolteCrossSlashFirstTelegraphVisualId = 9500U;
inline constexpr std::uint32_t RevolteCrossSlashSecondTelegraphVisualId = 9501U;
inline constexpr std::uint32_t RevolteCrossSlashVisualId = 9502U;
inline constexpr std::uint32_t RevolteCrossSlashImpactVisualId = 9503U;
inline constexpr std::uint32_t RevolteFlyingBladeTelegraphVisualId = 9504U;
inline constexpr std::uint32_t RevolteFlyingBladeVisualId = 9505U;

struct SpellSlotState
{
    std::optional<ContentId> id;
    float cooldownRemaining {};
    float cooldownDuration {};
};

struct PlayerCombatState
{
    common::Vec2 position;
    common::Vec2 velocity;
    int currentHealth {};
    int maximumHealth {};
    bool grounded {};
    float facingDirection {1.0F};
    bool attackActive {};
    float attackCooldownRemaining {};
    std::uint64_t attackSequence {};
    std::uint64_t castSequence {};
    std::array<SpellSlotState, 3> spellSlots;
    SpellSlotState ultimateSpellSlot;
    float dashRemaining {};
    float dashCooldownRemaining {};
    bool shadowDashing {};
    float shadowDashChargeRemaining {};
    bool stunned {};
    float stunRemaining {};
    float hitInvulnerabilityRemaining {};
    std::uint64_t hurtSequence {};
    float blessingRemaining {};
    float vulnerableRemaining {};
    float flowerFieldRemaining {};
    float flightRemaining {};
    int shield {};
    float sleepRemaining {};
};

struct PlayerVisualState
{
    common::Vec2 position;
    common::Vec2 velocity;
    int currentHealth {1};
    bool grounded {};
    float facingDirection {1.0F};
    std::uint64_t attackSequence {};
    std::uint64_t castSequence {};
    std::uint64_t hurtSequence {};
    float dashRemaining {};
    float shadowDashChargeRemaining {};
    bool shadowDashing {};
    bool stunned {};
};

struct EnemyState
{
    EnemyArchetype archetype {EnemyArchetype::HeadlessKnight};
    common::Vec2 position;
    float width {48.0F};
    float height {64.0F};
    int currentHealth {};
    int maximumHealth {};
    bool alive {};
    bool windingUp {};
    bool attackActive {};
    bool slowed {};
    std::optional<common::RectF> skillEffectBounds;
    float facingDirection {-1.0F};
    bool marked {};
    float skillEffectProgress {};
    float concealmentProgress {};
    bool specialWindingUp {};
    bool specialAttackActive {};
    int skillVariant {-1};
    bool returningToAir {};
    bool activated {};
};

struct SpellEffectState
{
    ContentId spellId {};
    common::RectF bounds;
    float remaining {};
    float duration {};
    float facingDirection {1.0F};
    float rotationDegrees {};
};

struct CombatDialogueState
{
    std::string_view speaker;
    std::string_view text;
    std::string_view portrait;
};

struct BossIntroState
{
    std::string_view name;
    float remaining {};
    float duration {};
};

struct ArenaState
{
    ContentId id {};
    ArenaTheme theme {ArenaTheme::AuraOccupation};
    bool safeRoom {};
    std::vector<common::RectF> oneWayPlatforms;
};

struct StaircaseState
{
    common::RectF bounds;
    bool unlocked {};
    ArenaTheme theme {ArenaTheme::AuraOccupation};
};

struct SpellDetailState
{
    float cooldownSeconds {};
    SpellRangeKind rangeKind {SpellRangeKind::Self};
    float range {};
    float height {};
};

struct ContentSummaryState
{
    ContentId id {};
    ContentKind kind {ContentKind::Unknown};
    std::string_view name {"UNKNOWN CONTENT"};
    bool bossSpell {};
    bool selected {};
    std::uint8_t rank {};
};

struct ContentDetailState
{
    struct SynergyHint
    {
        ContentId ownedSpellId {};
        std::string_view ownedSpellName;
        std::string_view description;
    };

    ContentSummaryState summary;
    std::string_view description {"NO DESCRIPTION AVAILABLE"};
    std::optional<SpellDetailState> spell;
    std::string masteryDescription;
    std::vector<SynergyHint> synergies;
    bool upgradeReward {};
};

struct EquippedSlotsState
{
    std::array<std::optional<ContentSummaryState>, 3> regular;
    std::optional<ContentSummaryState> ultimate;
    bool ultimateUnlocked {};
};

struct RewardState
{
    std::array<ContentDetailState, 3> cards;
    bool showRerollHint {};
};

struct BreakthroughCardState
{
    std::string_view name;
    std::string_view description;
    std::uint8_t currentRank {};
};

struct BreakthroughState
{
    std::array<BreakthroughCardState, 3> cards;
};

struct MerchantItemState
{
    ContentSummaryState content;
    int price {};
};

struct MerchantState
{
    std::vector<MerchantItemState> items;
    std::optional<ContentDetailState> selectedDetail;
};

struct LoadoutState
{
    LoadoutPage page {LoadoutPage::Spells};
    SpellSection spellSection {SpellSection::Regular};
    std::vector<ContentSummaryState> regularSpells;
    std::vector<ContentSummaryState> bossSpells;
    std::vector<ContentSummaryState> relics;
    std::optional<ContentDetailState> selectedDetail;
    EquippedSlotsState equipped;
};

struct SpellAcquisitionState
{
    ContentDetailState content;
    float elapsedSeconds {};
    float referenceElapsedSeconds {};
    float registrationProgress {};
    float circulationProgress {};
    float burstProgress {};
    float revealProgress {};
    bool canSkip {};
    bool canDismiss {};
    bool bossSpell {};
};

struct DamageNumberState
{
    common::Vec2 position;
    int amount {};
    float remaining {};
    float lifetime {};
    bool playerTarget {};
};

struct ImpactBurstState
{
    common::Vec2 position;
    float remaining {};
    float lifetime {};
    bool playerTarget {};
    bool lethal {};
};

struct CombatFeedbackState
{
    common::Vec2 cameraOffset;
    float playerFlashRatio {};
    std::vector<float> enemyFlashRatios;
    std::vector<float> enemyImpactOffsets;
    std::vector<DamageNumberState> damageNumbers;
    std::vector<ImpactBurstState> impactBursts;
    float hitStopRemaining {};
};

struct EventChoiceState
{
    ContentId id {};
    int goldDelta {};
};

struct EventPanelState
{
    bool open {};
    EventKind kind {EventKind::AldenBall};
    EventFloorState state {EventFloorState::Untriggered};
    std::optional<ContentId> resultChoice;
    std::array<EventChoiceState, 3> choices;
};

struct CombatSceneState
{
    ArenaState arena;
    StaircaseState staircase;
    PlayerCombatState player;
    std::vector<EnemyState> enemies;
    std::vector<SpellEffectState> spellEffects;
    common::RectF attackBounds;
    std::optional<CombatDialogueState> dialogue;
    std::optional<BossIntroState> bossIntro;
    std::optional<common::RectF> lootDrop;
};

struct SpecialFloorState
{
    ArenaState arena;
    StaircaseState staircase;
    std::optional<PlayerVisualState> player;
    common::RectF npcBounds;
    FloorKind floorKind {FloorKind::Event};
    EventPanelState event;
};

struct RunHeaderState
{
    Seed runSeed {};
    std::uint32_t floorIndex {};
    std::uint32_t act {1U};
    std::uint32_t bossesDefeated {};
    GameplayPhase phase {GameplayPhase::FloorLoading};
    FloorKind floorKind {FloorKind::Combat};
    int currentHp {};
    int maximumHp {};
    int gold {};
    bool specialPanelOpen {};
};

struct ApplicationState
{
    ApplicationScreen screen {ApplicationScreen::Start};
    PauseMenuItem pauseMenuItem {PauseMenuItem::ReplayCurrentFloor};
    bool canContinue {};
    bool victory {};
    std::string windowTitle {"Arcane Spire"};
    std::optional<RunHeaderState> run;
    std::optional<CombatSceneState> combat;
    std::optional<SpecialFloorState> specialFloor;
    std::optional<RewardState> reward;
    std::optional<BreakthroughState> breakthrough;
    std::optional<MerchantState> merchant;
    std::optional<LoadoutState> loadout;
    std::optional<SpellAcquisitionState> spellAcquisition;
    std::optional<EquippedSlotsState> equippedSlots;
};
}
