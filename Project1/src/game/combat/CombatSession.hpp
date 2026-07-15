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
    [[nodiscard]] Aabb attackBounds() const noexcept;
    [[nodiscard]] const std::optional<CombatResult>& result() const noexcept;
    [[nodiscard]] std::optional<CombatDialogueLineView> dialogueLine() const noexcept;
    [[nodiscard]] std::optional<BossIntroView> bossIntro() const noexcept;
    [[nodiscard]] bool equipSpell(std::size_t slot, std::optional<std::uint32_t> id) noexcept;
    [[nodiscard]] bool equipUltimateSpell(std::optional<std::uint32_t> id) noexcept;
    void settlePlayerForReward() noexcept;

private:
    static constexpr float AttackRange = 58.0F;
    static constexpr float AttackHeight = 36.0F;
    static constexpr float AttackVerticalOffset = 14.0F;
    static constexpr int AttackDamage = 15;
    static constexpr float HitStunSeconds = 0.28F;
    static constexpr float KnockbackSpeed = 360.0F;

    struct EnemyRuntime;
    [[nodiscard]] Aabb playerBounds() const noexcept;
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
        std::array<float, 5> revolteCooldowns {};
        int revolteSkill {-1};
        bool revolteSecondPhase {};
        bool revolteTransitionPending {};
        bool revolteCounterDashPending {};
        float secondaryCooldown {};
        int manualSkill {-1};
        bool activated {};
        Vec2 specialTarget;
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
    };
    struct ActiveEnemyBeam
    {
        Vec2 start;
        Vec2 end;
        float remaining {0.6F};
        std::uint64_t sequence {};
    };
    [[nodiscard]] static ai::EnemyConfig enemyConfigFor(EnemyArchetype archetype);
    [[nodiscard]] Aabb firstLivingEnemyBounds() const noexcept;
    void finish(CombatOutcome outcome) noexcept;
    enum class DialogueScript : std::uint8_t {
        None, AuraPreBattle, AuraFirstDomination, AuraDefeat,
        RevoltePreBattle, RevolteSecondPhase, RevolteDefeat
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
    float burningFlowerRemaining_ {};
    float burningFlowerAccumulator_ {};
    std::uint64_t burningFlowerSequenceBase_ {};
    std::uint32_t burningFlowerTick_ {};
    DamageSource burningFlowerSource_ {DamageSource::PlayerSpell0};
    float burningFlowerMultiplier_ {1.0F};
    float phantomRemaining_ {};
    Aabb phantomBounds_;
    float postDashComboRemaining_ {};
    float spellInvulnerableRemaining_ {};
    float sleepRemaining_ {};
    bool flightBoostAvailable_ {};
    float lightningStaffRemaining_ {};
    std::uint32_t lightningStaffCharges_ {};
    std::uint64_t handledLightningAttackSequence_ {};
    float golemRemaining_ {};
    Aabb golemBounds_;
    DamageSource golemSource_ {DamageSource::PlayerSpell0};
    float golemMultiplier_ {1.0F};
    std::uint64_t golemSequence_ {};
    int barrierShield_ {};
    int persistentShield_ {};
    float barrierRemaining_ {};
    float gravityWellRemaining_ {};
    float gravityWellTickAccumulator_ {};
    Aabb gravityWellBounds_;
    DamageSource gravityWellSource_ {DamageSource::PlayerSpell0};
    float gravityWellMultiplier_ {1.0F};
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
    std::uint64_t environmentalSequence_ {};
    std::optional<CombatResult> result_;
    DialogueScript dialogueScript_ {DialogueScript::None};
    std::uint8_t dialogueLineIndex_ {};
    bool auraFirstDominationDialogueShown_ {};
    bool auraGuaranteedDominationAvailable_ {true};
    bool auraDefeatDialogueShown_ {};
    bool revolteDefeatDialogueShown_ {};
    std::optional<CombatOutcome> outcomeAfterDialogue_;
    float bossIntroRemaining_ {};
};
}
