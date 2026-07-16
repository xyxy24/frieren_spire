#pragma once

#include "game/combat/Aabb.hpp"
#include "game/combat/AttackState.hpp"
#include "game/combat/CombatContracts.hpp"
#include "game/combat/Health.hpp"
#include "game/combat/DamageResolver.hpp"
#include "game/contracts/PlayerIntent.hpp"
#include "game/player/PlayerController.hpp"
#include "game/spells/SpellSystem.hpp"
#include "game/ai/EnemyController.hpp"
#include "game/relics/RelicSystem.hpp"

#include <cstdint>
#include <array>
#include <optional>
#include <vector>

namespace arcane::game
{
class CombatSession
{
public:
    static constexpr float EnemyWidth = ai::EnemyController::Width;
    static constexpr float EnemyHeight = ai::EnemyController::Height;
    static constexpr float PlayerHitInvulnerabilitySeconds = 0.60F;

    explicit CombatSession(CombatRequest request);

    void update(const PlayerIntent& intent, float deltaSeconds);

    [[nodiscard]] PlayerStateView playerState() const noexcept;
    [[nodiscard]] EnemyStateView enemyState() const noexcept;
    [[nodiscard]] std::vector<EnemyStateView> enemyStates() const;
    [[nodiscard]] std::vector<SpellEffectView> spellEffects() const;
    void populateEnemyStates(std::vector<EnemyStateView>& views) const;
    void populateSpellEffects(std::vector<SpellEffectView>& views) const;
    [[nodiscard]] Aabb attackBounds() const noexcept;
    [[nodiscard]] const std::optional<CombatResult>& result() const noexcept;
    [[nodiscard]] std::optional<CombatDialogueLineView> dialogueLine() const noexcept;
    [[nodiscard]] std::optional<BossIntroView> bossIntro() const noexcept;
    [[nodiscard]] bool equipSpell(std::size_t slot, std::optional<std::uint32_t> id,
        std::uint8_t rank = 1U) noexcept;
    [[nodiscard]] bool equipUltimateSpell(std::optional<std::uint32_t> id) noexcept;
    void settlePlayerForReward() noexcept;

private:
    static constexpr float AttackRange = 58.0F;
    static constexpr float AttackHeight = 36.0F;
    static constexpr float AttackVerticalOffset = 14.0F;
    static constexpr int AttackDamage = 15;
    static constexpr float HitStunSeconds = 0.28F;
    static constexpr float KnockbackSpeed = 360.0F;
    static constexpr float RevolteCrossSlashHalfThickness = 26.0F;
    static constexpr float RevolteCrossSlashHalfLength = 220.0F;
    static constexpr float RevolteCrossSlashFirstWindupSeconds = 0.58F;
    static constexpr float RevolteCrossSlashSecondWindupSeconds = 0.62F;
    static constexpr float RevolteCrossSlashRecoverySeconds = 0.45F;
    static constexpr float RevolteFlyingBladeWindupSeconds = 0.55F;
    static constexpr float RevolteFlyingBladeSpeed = 520.0F;

    struct SlashSegment { Vec2 start; Vec2 end; };
    struct EnemyRuntime;
    [[nodiscard]] Aabb playerBounds() const noexcept;
    [[nodiscard]] std::array<SlashSegment, 2> revolteCrossSlashSegments(
        Vec2 center) const noexcept;
    [[nodiscard]] DamageResult resolvePlayerDamage(DamageRequest request) noexcept;
    [[nodiscard]] DamageResult resolveEnemyDamage(EnemyRuntime& enemy,
        DamageRequest request) noexcept;
    int healPlayer(int amount) noexcept;
    float relicControlMultiplier(EnemyRuntime& enemy) noexcept;
    float playerControlDuration(float seconds) noexcept;
    void onBlessingPreventedControl() noexcept;
    struct EnemyRuntime
    {
        EnemyArchetype archetype;
        ai::EnemyController controller;
        Health health;
        DamageResolver damageResolver;
        float contactCooldown {};
        std::uint64_t contactSequence {};
        std::uint64_t handledSkillSequence {};
        bool slowed {};
        float summonCooldown {};
        std::uint32_t summonCount {};
        float breathCooldown {};
        float breathWindup {};
        float breathRemaining {};
        float breathTickAccumulator {};
        std::uint64_t breathSequence {};
        float frostSlowRemaining {};
        float frostChillRemaining {};
        float controlRemaining {};
        float burnRemaining {};
        float burnTickAccumulator {};
        std::uint64_t burnSequenceBase {};
        std::uint32_t burnTick {};
        DamageSource burnSource {DamageSource::PlayerSpell0};
        float burnMultiplier {1.0F};
        float exposedRemaining {};
        float markedRemaining {};
        float frozenRemaining {};
        float goldenBindRemaining {};
        bool goldenBindMastered {};
        float skillSealRemaining {};
        bool controlRelicTriggered {};
        float relicComboWindow {};
        float relicComboCooldown {};
        std::uint32_t relicComboHits {};
        float kraftCooldown {};
        float specialCooldown {};
        float specialWindup {};
        float specialActive {};
        float specialElapsed {};
        float specialDirection {-1.0F};
        Aabb specialTargetBounds;
        float concealmentProgress {};
        bool fogCreated {};
        std::array<float, 7> revolteCooldowns {};
        int revolteSkill {-1};
        bool revolteSecondPhase {};
        bool revolteTransitionPending {};
        bool revolteCounterDashPending {};
        std::uint8_t revolteCrossSlashRound {};
        std::array<Vec2, 3> revolteFlyingBladeStarts;
        std::array<Vec2, 3> revolteFlyingBladeTargets;
        float secondaryCooldown {};
        int manualSkill {-1};
        bool activated {};
        Vec2 specialTarget;
        std::optional<WorldBounds> movementBounds;
    };
    struct ActiveSpellEffect
    {
        std::uint32_t spellId {};
        Aabb bounds;
        float remaining {};
        float duration {};
    };
    struct PendingSpellImpact
    {
        std::uint32_t spellId {};
        Aabb bounds;
        float delayRemaining {};
        int damage {};
        std::uint64_t sequence {};
        DamageSource source {DamageSource::PlayerUltimateSpell};
        float multiplier {1.0F};
    };
    struct ActivePillar { Aabb bounds; float remaining {}; };
    struct ActiveTornado
    {
        Aabb bounds;
        float remaining {7.0F};
        float evolutionRemaining {3.0F};
        float tickAccumulator {};
        std::uint64_t sequence {};
        bool launched {};
    };
    struct ActiveEnemyProjectile
    {
        Aabb bounds;
        float direction {};
        float remainingDistance {};
        float totalDistance {};
        std::uint64_t sequence {};
        int damage {20};
        float speed {200.0F};
        bool tracksPlayer {};
        float homingTurnRateRadians {};
        Vec2 velocity;
        std::uint32_t visualId {};
        bool usesVelocity {};
    };
    struct ActiveEnemyBeam
    {
        Vec2 start;
        Vec2 end;
        float remaining {0.6F};
        std::uint64_t sequence {};
        std::uint32_t visualId {9200U};
        float visualThickness {18.0F};
        float duration {0.6F};
    };
    struct PendingEnemyLightning
    {
        Aabb bounds;
        float delayRemaining {0.4F};
        std::uint64_t sequence {};
        float telegraphDuration {0.4F};
        int damage {20};
        std::uint32_t telegraphVisualId {FrierenCopyLightningVisualId};
        std::uint32_t impactVisualId {FrierenCopyLightningVisualId};
        float impactDuration {0.6F};
    };
    struct ActiveEnemyLightningStorm
    {
        std::uint32_t strikesRemaining {5U};
        float nextStrikeRemaining {};
    };
    struct ActiveEnemyGroundFire
    {
        Aabb bounds;
        float remaining {5.0F};
        float tickAccumulator {};
        std::uint64_t sequence {};
    };
    [[nodiscard]] static ai::EnemyConfig enemyConfigFor(EnemyArchetype archetype);
    static constexpr float AuraGuillotineWindupSeconds = 0.9F;
    static constexpr float AuraGuillotineActiveSeconds = 0.18F;
    static constexpr float AuraGuillotineCooldownSeconds = 9.0F;
    static constexpr float AuraGuillotineWidth = 192.0F;
    static constexpr float AuraGuillotineHeight = 420.0F;
    static constexpr int AuraGuillotineDamage = 22;
    [[nodiscard]] WorldBounds movementBoundsFor(const EnemyRuntime& enemy) const noexcept;
    [[nodiscard]] bool updateEnemySkills(float deltaSeconds, float playerCenter);
    [[nodiscard]] Aabb firstLivingEnemyBounds() const noexcept;
    void finish(CombatOutcome outcome) noexcept;
    enum class DialogueScript : std::uint8_t {
        None, AuraPreBattle, AuraFirstDomination, AuraDefeat,
        RevoltePreBattle, RevolteSecondPhase, RevolteDefeat,
        WaterMirrorPreBattle, WaterMirrorSecondPhase, WaterMirrorDefeat
    };
    void beginDialogue(DialogueScript script) noexcept;
    void advanceDialogue() noexcept;

    CombatRequest request_;
    PlayerController player_;
    Health playerHealth_;
    std::vector<EnemyRuntime> enemies_;
    AttackState attack_;
    std::uint64_t playerCastSequence_ {};
    spells::SpellSystem spells_;
    relics::RelicRuntime relics_;
    DamageResolver playerDamageResolver_;
    float playerHitInvulnerabilityRemaining_ {};
    std::uint64_t playerHurtSequence_ {};
    float blessingRemaining_ {};
    float flowerFieldRemaining_ {};
    float flowerFieldCenterX_ {};
    float flowerHealingAccumulator_ {};
    float flowerHealingRate_ {3.0F};
    bool flowerFieldMastered_ {};
    float flowerFieldDamageMultiplier_ {1.0F};
    float burningFlowerRemaining_ {};
    float burningFlowerAccumulator_ {};
    std::uint64_t burningFlowerSequenceBase_ {};
    std::uint32_t burningFlowerTick_ {};
    DamageSource burningFlowerSource_ {DamageSource::PlayerSpell0};
    float burningFlowerMultiplier_ {1.0F};
    float phantomRemaining_ {};
    Aabb phantomBounds_;
    bool phantomExpiryBurst_ {};
    float phantomDamageMultiplier_ {1.0F};
    float postDashComboRemaining_ {};
    float spellInvulnerableRemaining_ {};
    float sleepRemaining_ {};
    bool flightBoostAvailable_ {};
    float flightBoostMultiplier_ {1.15F};
    float lightningStaffRemaining_ {};
    std::uint32_t lightningStaffCharges_ {};
    float lightningStaffDamageMultiplier_ {1.0F};
    int lightningStaffBurstDamage_ {12};
    std::uint64_t handledLightningAttackSequence_ {};
    float golemRemaining_ {};
    Aabb golemBounds_;
    DamageSource golemSource_ {DamageSource::PlayerSpell0};
    float golemMultiplier_ {1.0F};
    std::uint64_t golemSequence_ {};
    std::uint32_t golemHitsRemaining_ {1U};
    int barrierShield_ {};
    int barrierBreakDamage_ {12};
    int persistentShield_ {};
    float barrierRemaining_ {};
    float gravityWellRemaining_ {};
    float gravityWellTickAccumulator_ {};
    Aabb gravityWellBounds_;
    DamageSource gravityWellSource_ {DamageSource::PlayerSpell0};
    float gravityWellMultiplier_ {1.0F};
    float gravityWellPullSpeed_ {90.0F};
    int gravityWellTickDamage_ {6};
    std::uint64_t gravityWellSequenceBase_ {};
    std::uint32_t gravityWellTick_ {};
    float hellfireRemaining_ {};
    float hellfireTickAccumulator_ {};
    Aabb hellfireBounds_;
    std::uint64_t hellfireSequenceBase_ {};
    std::uint32_t hellfireTick_ {};
    float hellfireMultiplier_ {1.0F};
    float beamRemaining_ {};
    float beamTickAccumulator_ {};
    Aabb beamBounds_;
    std::uint64_t beamSequenceBase_ {};
    std::uint32_t beamTick_ {};
    float beamMultiplier_ {1.0F};
    std::uint32_t mirrorCopies_ {};
    std::uint64_t selfDamageSequence_ {};
    bool actualHpLost_ {};
    int flowerCrownConverted_ {};
    float tacticalNotesRemaining_ {};
    bool tacticalInterruptUsed_ {};
    bool globalControlRelicUsed_ {};
    std::array<bool, 3> holyStaffTriggered_ {};
    bool mirrorRelicUsed_ {};
    float lastRegularCastMultiplier_ {1.0F};
    bool firstDamageSpellUsed_ {};
    bool firstIncomingControlUsed_ {};
    float manaLensCooldown_ {};
    float warriorAxeCooldown_ {};
    bool goddessTabletTriggered_ {};
    float goddessTabletDamageRemaining_ {};
    std::vector<ActiveSpellEffect> activeSpellEffects_;
    std::vector<PendingSpellImpact> pendingSpellImpacts_;
    std::vector<ActivePillar> activePillars_;
    std::vector<ActiveTornado> activeTornadoes_;
    std::vector<ActiveEnemyProjectile> activeEnemyProjectiles_;
    std::vector<ActiveEnemyBeam> activeEnemyBeams_;
    std::vector<PendingEnemyLightning> pendingEnemyLightning_;
    std::vector<ActiveEnemyLightningStorm> activeEnemyLightningStorms_;
    std::vector<ActiveEnemyGroundFire> activeEnemyGroundFire_;
    std::uint64_t environmentalSequence_ {};
    std::optional<CombatResult> result_;
    DialogueScript dialogueScript_ {DialogueScript::None};
    std::uint8_t dialogueLineIndex_ {};
    bool auraFirstDominationDialogueShown_ {};
    bool auraFirstDominationAvailable_ {true};
    bool auraDefeatDialogueShown_ {};
    bool revolteDefeatDialogueShown_ {};
    bool waterMirrorSecondPhase_ {};
    bool waterMirrorDefeatDialogueShown_ {};
    std::optional<CombatOutcome> outcomeAfterDialogue_;
    float bossIntroRemaining_ {};
};
}
