#include "game/combat/CombatSession.hpp"
#include "game/combat/Health.hpp"
#include "game/spells/SpellSystem.hpp"
#include "game/ai/EnemyController.hpp"
#include "game/combat/DamageResolver.hpp"

#include <iostream>
#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <cmath>
#include <string_view>

namespace
{
bool expect(const bool condition, const std::string_view message)
{
    if (!condition)
    {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }

    return true;
}

arcane::game::CombatRequest adjacentEnemyRequest()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {160.0F, 576.0F};
    request.enemySpawn = {210.0F, 576.0F};
    return request;
}

bool healthClampsDamageAndHealing()
{
    arcane::game::Health health(100, 75);

    const int healed = health.heal(50);
    const int damaged = health.damage(140);

    return expect(healed == 25, "healing should report only the amount actually restored")
        && expect(damaged == 100, "damage should report only the amount actually removed")
        && expect(health.current() == 0, "damage should clamp health to zero")
        && expect(!health.isAlive(), "zero health should be dead")
        && expect(health.heal(10) == 0, "combat healing should not revive a dead entity");
}

bool oneAttackHitsOnlyOnce()
{
    arcane::game::CombatSession combat(adjacentEnemyRequest());
    arcane::game::PlayerIntent attack;
    attack.attackPressed = true;
    combat.update(attack, 0.01F);

    const int healthAfterFirstHit = combat.enemyState().currentHealth;
    const arcane::game::PlayerIntent idle;
    for (int frame = 0; frame < 5; ++frame)
    {
        combat.update(idle, 0.01F);
    }

    return expect(healthAfterFirstHit == 85, "a basic attack should deal the configured 15 damage")
        && expect(combat.enemyState().currentHealth == 85, "an active attack must hit an enemy only once");
}

bool playerViewSequencesOnlyAdvanceForSuccessfulActions()
{
    auto request = adjacentEnemyRequest();
    request.equippedSpellIds[0] = 1004U;
    request.enemyContactDamage = 0;
    request.enemyAttackDamage = 0;
    arcane::game::CombatSession combat(request);

    arcane::game::PlayerIntent actions;
    actions.attackPressed = true;
    actions.spellPressed[0] = true;
    combat.update(actions, 0.01F);
    const auto afterSuccess = combat.playerState();
    if (!expect(afterSuccess.attackSequence == 1U,
        "player view must expose a newly accepted basic attack")) return false;
    if (!expect(afterSuccess.castSequence == 1U,
        "player view must expose a newly accepted spell cast")) return false;
    if (!expect(std::abs(afterSuccess.velocity.x) < 0.01F,
        "player view must expose authoritative movement velocity")) return false;

    combat.update(actions, 0.01F);
    const auto afterCooldownRejection = combat.playerState();
    return expect(afterCooldownRejection.attackSequence == afterSuccess.attackSequence,
            "rejected basic attacks must not retrigger presentation")
        && expect(afterCooldownRejection.castSequence == afterSuccess.castSequence,
            "rejected spell casts must not retrigger presentation");
}

bool repeatedAttacksProduceVictoryResult()
{
    arcane::game::CombatRequest request = adjacentEnemyRequest();
    request.encounterId = 42;
    request.goldReward = 17;
    request.enemyContactDamage = 0;
    request.enemyAttackDamage = 0;
    arcane::game::CombatSession combat(request);

    for (int attackNumber = 0; attackNumber < 7; ++attackNumber)
    {
        arcane::game::PlayerIntent attack;
        attack.attackPressed = true;
        combat.update(attack, 0.01F);

        if (attackNumber < 6)
        {
            combat.update(arcane::game::PlayerIntent {}, 0.36F);
        }
    }

    return expect(combat.result().has_value(), "defeating the enemy should complete combat")
        && expect(combat.result()->outcome == arcane::game::CombatOutcome::Victory, "enemy defeat should be victory")
        && expect(combat.result()->encounterId == 42, "result should preserve the encounter identifier")
        && expect(combat.result()->defeatedEnemies == 1, "victory should report one defeated enemy")
        && expect(combat.result()->goldAwarded == 17, "victory should report the configured gold reward")
        && expect(combat.result()->playerHealthRemaining == 100, "victory should preserve remaining player health");
}

bool enemyAttackHasWindupAndHitsOnce()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {160.0F, 576.0F};
    request.enemySpawn = {180.0F, 576.0F};
    request.enemyContactDamage = 0;
    arcane::game::CombatSession combat(request);

    combat.update(arcane::game::PlayerIntent {}, 2.99F);
    if (!expect(!combat.enemyState().skillEffectBounds.has_value(),
        "headless knight slash must not expose a visible range rectangle")) return false;
    combat.update(arcane::game::PlayerIntent {}, 0.30F);
    if (!expect(combat.playerState().currentHealth == 100,
        "enemy windup must not deal early damage")) return false;
    combat.update(arcane::game::PlayerIntent {}, 0.01F);
    combat.update(arcane::game::PlayerIntent {}, 0.50F);
    combat.update(arcane::game::PlayerIntent {}, 0.01F);
    const auto afterHit = combat.playerState();
    combat.update(arcane::game::PlayerIntent {}, 0.05F);

    return expect(afterHit.currentHealth == 90, "active enemy attack should deal configured damage")
        && expect(afterHit.stunned, "enemy hit should apply player hit stun")
        && expect(combat.playerState().currentHealth == 90, "one enemy attack must hit only once");
}

bool zeroHealthProducesDefeatResult()
{
    arcane::game::CombatRequest request;
    request.encounterId = 9;
    request.playerSpawn = {160.0F, 576.0F};
    request.enemySpawn = {180.0F, 576.0F};
    request.playerCurrentHealth = 10;
    request.enemyContactDamage = 10;
    arcane::game::CombatSession combat(request);

    combat.update(arcane::game::PlayerIntent {}, 0.01F);
    combat.update(arcane::game::PlayerIntent {}, 0.46F);

    return expect(combat.result().has_value(), "zero player health should complete combat")
        && expect(combat.result()->outcome == arcane::game::CombatOutcome::Defeat, "player death should be defeat")
        && expect(combat.result()->encounterId == 9, "defeat should preserve the encounter identifier")
        && expect(combat.result()->defeatedEnemies == 0, "defeat should not report an enemy kill")
        && expect(combat.result()->goldAwarded == 0, "defeat should not award gold")
        && expect(combat.result()->playerHealthRemaining == 0, "defeat should report zero remaining health");
}

bool bloodMagicUsesCurrentHealth()
{
    auto request = adjacentEnemyRequest();
    request.enemySpawn = {300.0F, 576.0F};
    request.enemyMaximumHealth = 100;
    request.equippedSpellIds[0] = 1003U;
    request.enemyContactDamage = 0;
    arcane::game::CombatSession combat(request);
    arcane::game::PlayerIntent cast;
    cast.spellPressed[0] = true;
    combat.update(cast, 0.01F);
    return expect(combat.playerState().currentHealth == 88, "blood magic must cost 12% current HP")
        && expect(combat.enemyState().currentHealth == 52,
            "blood magic damage must equal 55% of HP remaining after its cost");
}

bool flowerFieldHealsAndMoonFlowerAutoCasts()
{
    auto request = adjacentEnemyRequest();
    request.playerCurrentHealth = 50;
    request.enemySpawn = {450.0F, 576.0F};
    request.relicIds = {arcane::game::relics::MoonGrassFlowerId};
    request.enemyContactDamage = 0;
    arcane::game::CombatSession combat(request);
    if (!expect(combat.playerState().flowerFieldRemaining == 4.0F,
        "moon grass flower must cast the field at combat start")) return false;
    combat.update(arcane::game::PlayerIntent {}, 1.0F);
    return expect(combat.playerState().currentHealth == 53, "flower field must heal 3 HP each second")
        && expect(combat.enemyState().slowed, "enemy inside flower field must be slowed");
}

bool goddessBlessingBoostsDamageAndPreventsControl()
{
    auto request = adjacentEnemyRequest();
    request.enemySpawn = {210.0F, 576.0F};
    request.equippedSpellIds[0] = 1002U;
    request.equippedSpellIds[1] = 1004U;
    request.enemyContactDamage = 15;
    arcane::game::CombatSession combat(request);
    arcane::game::PlayerIntent bless;
    bless.spellPressed[0] = true;
    combat.update(bless, 0.01F);
    arcane::game::PlayerIntent damage;
    damage.spellPressed[1] = true;
    combat.update(damage, 0.01F);
    return expect(combat.playerState().blessingRemaining > 4.0F, "blessing must last five seconds")
        && expect(!combat.playerState().stunned, "blessing must prevent control effects")
        && expect(combat.enemyState().currentHealth == 77, "blessing must increase outgoing damage by 15%");
}

bool darkDragonHornAppliesRiskAndReward()
{
    auto request = adjacentEnemyRequest();
    request.enemySpawn = {195.0F, 576.0F};
    request.relicIds = {arcane::game::relics::DarkDragonHornId};
    request.enemyContactDamage = 15;
    request.enemyAttackDamage = 0;
    arcane::game::CombatSession combat(request);
    arcane::game::PlayerIntent attack;
    attack.attackPressed = true;
    combat.update(attack, 0.01F);
    return expect(combat.enemyState().currentHealth == 82, "dragon horn must increase outgoing damage by 20%")
        && expect(combat.playerState().currentHealth == 82, "opening vulnerability must increase incoming damage by 20%")
        && expect(combat.playerState().vulnerableRemaining > 14.0F, "opening vulnerability must start at 15 seconds");
}

bool damageResolverReportsAppliedAndLethalDamage()
{
    arcane::game::Health target(100, 30);
    arcane::game::DamageResolver resolver;
    const auto result = resolver.resolve(target,
        {arcane::game::DamageSource::PlayerBasicAttack, 1U, 50});
    return expect(result.resolvedDamage == 50, "resolver must report resolved damage")
        && expect(result.appliedDamage == 30, "applied damage must match actual HP loss")
        && expect(result.healthBefore == 30 && result.healthAfter == 0, "resolver must snapshot HP change")
        && expect(result.killed, "lethal damage must be reported");
}

bool damageResolverDeduplicatesPerSource()
{
    arcane::game::Health target(100, 100);
    arcane::game::DamageResolver resolver;
    const auto first = resolver.resolve(target,
        {arcane::game::DamageSource::PlayerBasicAttack, 7U, 20});
    const auto duplicate = resolver.resolve(target,
        {arcane::game::DamageSource::PlayerBasicAttack, 7U, 20});
    const auto independent = resolver.resolve(target,
        {arcane::game::DamageSource::PlayerSpell0, 7U, 30});
    return expect(first.appliedDamage == 20, "first sequence must apply")
        && expect(duplicate.duplicate && duplicate.appliedDamage == 0, "same source sequence must not apply twice")
        && expect(independent.appliedDamage == 30, "same sequence from another source must remain independent")
        && expect(target.current() == 50, "only unique damage requests must change HP");
}

bool damageResolverCentralizesModifiersAndBlocking()
{
    arcane::game::Health target(100, 100);
    arcane::game::DamageResolver resolver;
    arcane::game::DamageRequest modified {arcane::game::DamageSource::PlayerSpell1, 1U, 10};
    modified.sourceMultiplier = 1.5F;
    modified.targetMultiplier = 0.5F;
    modified.flatReduction = 2;
    const auto result = resolver.resolve(target, modified);
    arcane::game::DamageRequest blocked {arcane::game::DamageSource::EnemyAttack, 1U, 99};
    blocked.blocked = true;
    const auto blockedResult = resolver.resolve(target, blocked);
    return expect(result.resolvedDamage == 6 && result.appliedDamage == 6,
            "modifiers, reduction and rounding must be centralized")
        && expect(blockedResult.blocked && blockedResult.appliedDamage == 0,
            "blocked damage must not change health")
        && expect(target.current() == 94, "blocked request must preserve target HP");
}

bool enemyChaseIsDeltaTimeStableAndStopsWhenDead()
{
    const arcane::game::Aabb player {160.0F, 576.0F, 42.0F, 64.0F};
    const arcane::game::WorldBounds bounds {0.0F, 1280.0F, 640.0F};
    arcane::game::ai::EnemyController oneStep({800.0F, 576.0F});
    arcane::game::ai::EnemyController manySteps({800.0F, 576.0F});
    oneStep.update(player, 1.1F, bounds);
    for (int frame = 0; frame < 11; ++frame) manySteps.update(player, 0.1F, bounds);
    oneStep.update(player, 1.0F, bounds);
    for (int frame = 0; frame < 10; ++frame) manySteps.update(player, 0.1F, bounds);
    const float oneStepX = oneStep.position().x;
    const float manyStepsX = manySteps.position().x;
    manySteps.markDead();
    manySteps.update(player, 1.0F, bounds);
    return expect(oneStepX < 800.0F, "enemy must chase the player")
        && expect(std::abs(oneStepX - manyStepsX) <= 0.001F, "enemy chase must use delta time")
        && expect(manySteps.position().x == manyStepsX, "dead enemy must stop updating");
}

bool hitStunSuppressesInputAndAppliesKnockback()
{
    arcane::game::PlayerController player({300.0F, 576.0F});
    const arcane::game::WorldBounds bounds {0.0F, 1280.0F, 640.0F};
    player.applyHitReaction(-360.0F, 0.28F);
    arcane::game::PlayerIntent moveAgainstKnockback;
    moveAgainstKnockback.moveAxis = 1.0F;
    moveAgainstKnockback.jumpPressed = true;
    player.update(moveAgainstKnockback, 0.10F, bounds);
    const float duringStunX = player.position().x;
    if (!expect(duringStunX < 300.0F && player.isStunned(),
        "stunned player must be knocked back instead of following input")) return false;
    player.update(arcane::game::PlayerIntent {}, 0.20F, bounds);
    player.update(moveAgainstKnockback, 0.10F, bounds);
    return expect(player.position().x > duringStunX, "player input must resume after hit stun");
}

bool spellSlotsHaveIndependentCooldowns()
{
    std::array<std::optional<std::uint32_t>, 3> equipped {1004U, 1005U, std::nullopt};
    arcane::game::spells::SpellSystem spells(equipped);
    const arcane::game::Aabb target {260.0F, 576.0F, 48.0F, 64.0F};
    const auto first = spells.tryCast(0U, {160.0F, 576.0F}, 1.0F, target);
    const auto blocked = spells.tryCast(0U, {160.0F, 576.0F}, 1.0F, target);
    const auto otherSlot = spells.tryCast(1U, {160.0F, 576.0F}, 1.0F, target);
    const auto emptySlot = spells.tryCast(2U, {160.0F, 576.0F}, 1.0F, target);
    spells.update(1.80F);
    const auto readyAgain = spells.tryCast(0U, {160.0F, 576.0F}, 1.0F, target);
    return expect(first.cast && first.hit && first.damage == 20, "equipped spell must cast and hit")
        && expect(!blocked.cast, "spell must be blocked during its own cooldown")
        && expect(otherSlot.cast, "one slot cooldown must not block another slot")
        && expect(!emptySlot.cast, "empty spell slot must not cast")
        && expect(readyAgain.cast, "spell must become available after its cooldown");
}

bool spellCooldownIsDeltaTimeStable()
{
    std::array<std::optional<std::uint32_t>, 3> equipped {1004U, std::nullopt, std::nullopt};
    arcane::game::spells::SpellSystem oneStep(equipped);
    arcane::game::spells::SpellSystem manySteps(equipped);
    const arcane::game::Aabb target {260.0F, 576.0F, 48.0F, 64.0F};
    static_cast<void>(oneStep.tryCast(0U, {160.0F, 576.0F}, 1.0F, target));
    static_cast<void>(manySteps.tryCast(0U, {160.0F, 576.0F}, 1.0F, target));
    oneStep.update(1.80F);
    for (int frame = 0; frame < 18; ++frame) manySteps.update(0.10F);
    return expect(oneStep.tryCast(0U, {160.0F, 576.0F}, 1.0F, target).cast,
            "single-step cooldown must finish")
        && expect(manySteps.tryCast(0U, {160.0F, 576.0F}, 1.0F, target).cast,
            "partitioned delta time must finish the same cooldown");
}

bool regularSpellCatalogHasDefinitionsAndAuthoritativeRanges()
{
    using arcane::game::spells::SpellTier;
    constexpr std::array collectible {
        1001U, 1002U, 1003U, 1004U, 1005U, 1006U, 1008U, 1009U, 1011U, 1016U,
        1017U, 1018U, 1019U, 1020U, 1021U, 1022U, 1023U, 1024U, 1025U, 1026U
    };
    for (const std::uint32_t id : collectible)
    {
        const auto* definition = arcane::game::spells::findDefinition(id);
        if (!expect(definition != nullptr, "every spell from 1001 through 1016 must be defined"))
            return false;
        if (!expect(definition->tier == SpellTier::Regular,
                "every collectible spell must use the regular tier"))
            return false;
    }
    if (!expect(arcane::game::spells::findDefinition(1007U) == nullptr
            && arcane::game::spells::findDefinition(1012U) == nullptr
            && arcane::game::spells::findDefinition(1013U) == nullptr
            && arcane::game::spells::findDefinition(1014U) == nullptr
            && arcane::game::spells::findDefinition(1015U) == nullptr,
        "retired spells 1007 and 1012 through 1015 must not remain in the catalog")) return false;

    std::array<std::optional<std::uint32_t>, 3> equipped {1001U, 1008U, 1021U};
    arcane::game::spells::SpellSystem spells(equipped);
    const arcane::game::Aabb target {260.0F, 576.0F, 48.0F, 64.0F};
    const auto field = spells.tryCast(0U, {160.0F, 576.0F}, 1.0F, target);
    const auto thread = spells.tryCast(1U, {160.0F, 576.0F}, 1.0F, target);
    const auto slam = spells.tryCast(2U, {160.0F, 576.0F}, 1.0F, target);
    return expect(field.effectBounds.width == 600.0F && field.effectBounds.height == 300.0F,
            "self-area spell must expose its full horizontal and vertical diameter")
        && expect(thread.effectBounds.width == 280.0F && thread.effectBounds.height == 12.0F,
            "line spell must expose its authoritative line range")
        && expect(slam.effectBounds.width == 120.0F && slam.effectBounds.height == 120.0F,
            "target-area spell must expose its authoritative area")
        && expect(!spells.equip(0U, 1007U) && !spells.equip(0U, 1010U),
            "retired guard and innate dash must not occupy learned spell slots");
}

bool innateActionsExposeSpellEffectPlaceholders()
{
    auto request = adjacentEnemyRequest();
    request.enemyContactDamage = 0;
    request.enemyAttackDamage = 0;
    arcane::game::CombatSession dashCombat(request);
    arcane::game::PlayerIntent dash;
    dash.dashPressed = true;
    dashCombat.update(dash, 0.01F);
    const auto dashEffects = dashCombat.spellEffects();
    return expect(dashEffects.size() == 1U && dashEffects.front().spellId == 1010U
            && dashEffects.front().bounds.width == arcane::game::PlayerController::DashDistance,
        "K dash must expose its 150px effect placeholder");
}

bool collectibleSpellsEmitEffectPlaceholders()
{
    constexpr std::array ids {
        1001U, 1002U, 1003U, 1004U, 1005U, 1006U, 1008U, 1009U, 1011U, 1016U,
        1017U, 1018U, 1019U, 1020U, 1021U, 1022U, 1023U, 1024U, 1025U, 1026U,
        1027U, 1028U, 1029U, 1030U
    };
    for (const std::uint32_t id : ids)
    {
        auto request = adjacentEnemyRequest();
        request.enemyMaximumHealth = 1000;
        request.enemyContactDamage = 0;
        request.enemyAttackDamage = 0;
        request.equippedSpellIds[0] = id;
        arcane::game::CombatSession combat(request);
        arcane::game::PlayerIntent cast;
        cast.spellPressed[0] = true;
        combat.update(cast, 0.01F);
        const auto effects = combat.spellEffects();
        if (!expect(effects.size() == 1U && effects.front().spellId == id
                && effects.front().bounds.width > 0.0F && effects.front().bounds.height > 0.0F,
            "every collectible spell must emit a non-empty effect placeholder")) return false;
    }
    return true;
}

bool postHitInvulnerabilityBlocksOverlappingDamage()
{
    auto request = adjacentEnemyRequest();
    request.enemyAttackDamage = 0;
    request.enemyContactDamage = 15;
    request.enemies = {
        {arcane::game::EnemyArchetype::HeadlessKnight, {195.0F, 576.0F}},
        {arcane::game::EnemyArchetype::HeadlessKnight, {195.0F, 576.0F}}
    };
    arcane::game::CombatSession combat(request);
    combat.update(arcane::game::PlayerIntent {}, 0.01F);
    const auto afterHit = combat.playerState();
    if (!expect(afterHit.currentHealth == 85,
            "overlapping enemies must only damage the player once during the post-hit window")
        || !expect(afterHit.hurtSequence == 1U,
            "only applied HP damage may advance the authoritative hurt sequence")
        || !expect(afterHit.hitInvulnerabilityRemaining > 0.58F,
            "applied HP damage must start the configured post-hit invulnerability")) return false;

    combat.update(arcane::game::PlayerIntent {}, 0.30F);
    if (!expect(combat.playerState().currentHealth == 85,
            "incoming damage inside the post-hit window must remain blocked")) return false;
    combat.update(arcane::game::PlayerIntent {}, 0.31F);
    return expect(combat.playerState().hitInvulnerabilityRemaining <= 0.001F,
        "post-hit invulnerability must expire using combat delta time");
}

bool everySpellEffectKeepsItsCompleteVisualTimeline()
{
    struct Expectation
    {
        std::uint32_t id;
        float minimumDuration;
        bool boss;
    };
    constexpr std::array expectations {
        Expectation {1001U, 4.0F, false}, Expectation {1002U, 5.0F, false},
        Expectation {1003U, 8.0F / 16.0F, false}, Expectation {1004U, 8.0F / 16.0F, false},
        Expectation {1005U, 6.0F / 18.0F, false}, Expectation {1006U, 8.0F / 16.0F, false},
        Expectation {1008U, 8.0F / 16.0F, false}, Expectation {1009U, 6.0F / 16.0F, false},
        Expectation {1011U, 4.0F, false}, Expectation {1016U, 4.0F, false},
        Expectation {1017U, 8.0F / 16.0F, false}, Expectation {1018U, 8.0F / 16.0F, false},
        Expectation {1019U, 8.0F / 16.0F, false}, Expectation {1020U, 8.0F / 14.0F, false},
        Expectation {1021U, 1.0F, false}, Expectation {1022U, 5.0F, false},
        Expectation {1023U, 2.5F, false}, Expectation {1024U, 10.0F / 16.0F, false},
        Expectation {1025U, 8.0F / 14.0F, false}, Expectation {1026U, 4.0F, false},
        Expectation {1027U, 6.0F / 20.0F, false}, Expectation {1028U, 5.0F, false},
        Expectation {1029U, 8.0F / 16.0F, false}, Expectation {1030U, 2.5F, false},
        Expectation {2001U, 8.0F / 16.0F, true}, Expectation {2002U, 10.0F / 14.0F, true},
        Expectation {2003U, 10.0F / 16.0F, true}, Expectation {2006U, 10.0F / 14.0F, true},
        Expectation {2007U, 0.8F, true}, Expectation {2009U, 3.0F, true},
        Expectation {2010U, 2.0F, true}, Expectation {2011U, 3.0F, true},
        Expectation {2012U, 8.0F, true}
    };

    for (const auto expectation : expectations)
    {
        auto request = adjacentEnemyRequest();
        request.enemyMaximumHealth = 100000;
        request.enemyContactDamage = 0;
        request.enemyAttackDamage = 0;
        if (expectation.boss) request.equippedUltimateSpellId = expectation.id;
        else request.equippedSpellIds[0] = expectation.id;
        arcane::game::CombatSession combat(request);
        arcane::game::PlayerIntent cast;
        if (expectation.boss) cast.ultimateSpellPressed = true;
        else cast.spellPressed[0] = true;
        combat.update(cast, 0.01F);

        const auto effects = combat.spellEffects();
        const auto found = std::find_if(effects.begin(), effects.end(), [&](const auto& effect) {
            return effect.spellId == expectation.id;
        });
        if (!expect(found != effects.end(),
                "every implemented spell must create a visual timeline")
            || !expect(found->duration + 0.001F >= expectation.minimumDuration,
                "a spell visual timeline must be long enough to reach its final frame")) return false;
    }
    return true;
}

bool secondarySpellEffectsKeepTheirCompleteVisualTimelines()
{
    auto golemRequest = adjacentEnemyRequest();
    golemRequest.enemyMaximumHealth = 100000;
    golemRequest.enemyContactDamage = 0;
    golemRequest.enemyAttackDamage = 0;
    golemRequest.equippedSpellIds[0] = 1022U;
    arcane::game::CombatSession golem(golemRequest);
    arcane::game::PlayerIntent summon;
    summon.spellPressed[0] = true;
    golem.update(summon, 0.01F);
    bool foundGolemResolution = false;
    for (int frame = 0; frame < 600 && !foundGolemResolution; ++frame)
    {
        golem.update({}, 0.01F);
        const auto effects = golem.spellEffects();
        foundGolemResolution = std::any_of(effects.begin(), effects.end(), [](const auto& effect) {
            return effect.spellId == arcane::game::StoneGolemResolveVisualId
                && std::abs(effect.duration - 0.4F) < 0.001F;
        });
    }
    if (!expect(foundGolemResolution,
            "Stone Golem resolution must skip its construction and idle frames")) return false;

    auto mirrorRequest = adjacentEnemyRequest();
    mirrorRequest.enemyMaximumHealth = 100000;
    mirrorRequest.enemyContactDamage = 0;
    mirrorRequest.enemyAttackDamage = 0;
    mirrorRequest.equippedSpellIds[0] = 1004U;
    mirrorRequest.equippedUltimateSpellId = 2012U;
    arcane::game::CombatSession mirror(mirrorRequest);
    arcane::game::PlayerIntent summonMirrors;
    summonMirrors.ultimateSpellPressed = true;
    mirror.update(summonMirrors, 0.01F);
    arcane::game::PlayerIntent triggerMirrors;
    triggerMirrors.spellPressed[0] = true;
    mirror.update(triggerMirrors, 0.01F);
    const auto mirrorEffects = mirror.spellEffects();
    return expect(std::any_of(mirrorEffects.begin(), mirrorEffects.end(), [](const auto& effect) {
            return effect.spellId == arcane::game::MirrorArrayBreakVisualId
                && std::abs(effect.duration - 0.25F) < 0.001F;
        }), "Mirror Array response must start directly from its shatter frames");
}

bool statefulSpellVisualsUseTheirMatchingTransitionPhases()
{
    auto phantomRequest = adjacentEnemyRequest();
    phantomRequest.enemyContactDamage = 0;
    phantomRequest.equippedSpellIds[0] = 1011U;
    arcane::game::CombatSession phantom(phantomRequest);
    arcane::game::PlayerIntent summonPhantom;
    summonPhantom.spellPressed[0] = true;
    phantom.update(summonPhantom, 0.01F);
    bool phantomBroke = false;
    for (int step = 0; step < 600 && !phantomBroke; ++step)
    {
        phantom.update({}, 0.01F);
        const auto effects = phantom.spellEffects();
        phantomBroke = std::any_of(effects.begin(), effects.end(), [](const auto& effect) {
            return effect.spellId == arcane::game::PhantomBreakVisualId;
        });
    }
    if (!expect(phantomBroke,
            "a struck Phantom must switch from idle frames to its break-only timeline"))
        return false;

    auto barrierRequest = adjacentEnemyRequest();
    barrierRequest.enemyContactDamage = 0;
    barrierRequest.enemyAttackDamage = 40;
    barrierRequest.equippedSpellIds[0] = 1028U;
    arcane::game::CombatSession barrier(barrierRequest);
    arcane::game::PlayerIntent raiseBarrier;
    raiseBarrier.spellPressed[0] = true;
    barrier.update(raiseBarrier, 0.01F);
    bool barrierBroke = false;
    for (int step = 0; step < 600 && !barrierBroke; ++step)
    {
        barrier.update({}, 0.01F);
        const auto effects = barrier.spellEffects();
        barrierBroke = std::any_of(effects.begin(), effects.end(), [](const auto& effect) {
            return effect.spellId == arcane::game::DefensiveBarrierBreakVisualId;
        });
        if (barrierBroke && !expect(std::none_of(effects.begin(), effects.end(), [](const auto& effect) {
                return effect.spellId == 1028U;
            }), "a broken barrier must stop rendering its intact loop")) return false;
    }
    return expect(barrierBroke,
        "a depleted Defensive Barrier must start directly from its shatter frames");
}

bool flameBurstUsesOneShotVisualAndSeparateBurningField()
{
    auto request = adjacentEnemyRequest();
    request.enemyMaximumHealth = 100000;
    request.enemyContactDamage = 0;
    request.enemyAttackDamage = 0;
    request.equippedSpellIds[0] = 1001U;
    request.equippedSpellIds[1] = 1006U;
    arcane::game::CombatSession combat(request);

    arcane::game::PlayerIntent field;
    field.spellPressed[0] = true;
    combat.update(field, 0.01F);
    arcane::game::PlayerIntent ignite;
    ignite.spellPressed[1] = true;
    combat.update(ignite, 0.01F);

    auto effects = combat.spellEffects();
    const auto burst = std::find_if(effects.begin(), effects.end(), [](const auto& effect) {
        return effect.spellId == 1006U;
    });
    const auto burningField = std::find_if(effects.begin(), effects.end(), [](const auto& effect) {
        return effect.spellId == arcane::game::BurningFlowerVisualId;
    });
    if (!expect(burst != effects.end() && std::abs(burst->duration - 0.5F) < 0.001F,
            "Flame Burst must play its eight-frame clip once instead of looping for the burn DOT")
        || !expect(burningField != effects.end()
                && std::abs(burningField->duration - 2.0F) < 0.001F,
            "ignited Flower Field must use a distinct persistent fire visual")
        || !expect(std::none_of(effects.begin(), effects.end(), [](const auto& effect) {
                return effect.spellId == 1001U;
            }), "igniting Flower Field must remove the replaced flower visual"))
        return false;

    combat.update({}, 0.55F);
    effects = combat.spellEffects();
    return expect(std::none_of(effects.begin(), effects.end(), [](const auto& effect) {
            return effect.spellId == 1006U;
        }), "one-shot Flame Burst visual must disappear after its final frame")
        && expect(std::any_of(effects.begin(), effects.end(), [](const auto& effect) {
            return effect.spellId == arcane::game::BurningFlowerVisualId;
        }), "the separate burning field must remain visible for its gameplay duration");
}

bool newCombatSpellsApplyTheirCoreInteractions()
{
    auto multiRequest = adjacentEnemyRequest();
    multiRequest.enemyContactDamage = 0;
    multiRequest.enemyAttackDamage = 0;
    multiRequest.equippedSpellIds[0] = 1016U;
    multiRequest.equippedSpellIds[1] = 1017U;
    arcane::game::CombatSession multi(multiRequest);
    arcane::game::PlayerIntent trace;
    trace.spellPressed[0] = true;
    multi.update(trace, 0.01F);
    arcane::game::PlayerIntent volley;
    volley.spellPressed[1] = true;
    multi.update(volley, 0.01F);
    if (!expect(multi.enemyState().currentHealth == 56,
            "analyzed Multi Zoltraak must deal two eleven-damage hits and a twenty-two-damage finisher"))
        return false;

    auto comboRequest = adjacentEnemyRequest();
    comboRequest.enemyContactDamage = 0;
    comboRequest.enemyAttackDamage = 0;
    comboRequest.equippedSpellIds[0] = 1020U;
    comboRequest.equippedSpellIds[1] = 1009U;
    arcane::game::CombatSession combo(comboRequest);
    arcane::game::PlayerIntent bind;
    bind.spellPressed[0] = true;
    combo.update(bind, 0.01F);
    arcane::game::PlayerIntent stone;
    stone.spellPressed[1] = true;
    combo.update(stone, 0.01F);
    if (!expect(combo.enemyState().currentHealth == 42,
            "Stone Shot must consume golden binding for sixteen extra damage")) return false;

    auto flightRequest = adjacentEnemyRequest();
    flightRequest.enemyContactDamage = 0;
    flightRequest.enemyAttackDamage = 0;
    flightRequest.equippedSpellIds[0] = 1023U;
    flightRequest.equippedSpellIds[1] = 1004U;
    arcane::game::CombatSession flight(flightRequest);
    arcane::game::PlayerIntent takeOff;
    takeOff.spellPressed[0] = true;
    flight.update(takeOff, 0.01F);
    arcane::game::PlayerIntent boostedBolt;
    boostedBolt.spellPressed[1] = true;
    flight.update(boostedBolt, 0.01F);
    return expect(flight.playerState().flightRemaining > 2.0F,
            "Flight Magic must expose its active flight duration")
        && expect(flight.enemyState().currentHealth == 77,
            "the first attack spell during flight must gain fifteen percent damage");
}

bool lightningStaffEnhancesExactlyThreeBasicAttacks()
{
    auto request = adjacentEnemyRequest();
    request.enemyMaximumHealth = 100;
    request.enemyContactDamage = 0;
    request.enemyAttackDamage = 0;
    request.equippedSpellIds[0] = 1026U;
    arcane::game::CombatSession combat(request);
    arcane::game::PlayerIntent enchant;
    enchant.spellPressed[0] = true;
    combat.update(enchant, 0.01F);
    for (int attackIndex = 0; attackIndex < 3; ++attackIndex)
    {
        arcane::game::PlayerIntent attack;
        attack.attackPressed = true;
        combat.update(attack, 0.01F);
        if (attackIndex < 2) combat.update({}, 0.35F);
    }
    const auto effects = combat.spellEffects();
    return expect(combat.enemyState().currentHealth == 22,
            "Lightning Staff must add three seven-damage hits and one twelve-damage burst")
        && expect(std::none_of(effects.begin(), effects.end(), [](const auto& effect) {
                return effect.spellId == 1026U;
            }), "spent Lightning Staff must stop its persistent charge animation")
        && expect(std::any_of(effects.begin(), effects.end(), [](const auto& effect) {
                return effect.spellId == arcane::game::LightningStaffDischargeVisualId;
            }), "the third Lightning Staff hit must start directly from its discharge frames");
}

bool frostLanceCanFreezeAfterItsOwnBaseCooldown()
{
    auto request = adjacentEnemyRequest();
    request.enemyArchetype = arcane::game::EnemyArchetype::ChestMimic;
    request.enemySpawn = {360.0F, 576.0F};
    request.enemyMaximumHealth = 200;
    request.enemyContactDamage = 0;
    request.enemyAttackDamage = 0;
    request.equippedSpellIds[0] = 1005U;
    request.equippedSpellIds[1] = 1021U;
    arcane::game::CombatSession combat(request);

    arcane::game::PlayerIntent frost;
    frost.spellPressed[0] = true;
    combat.update(frost, 0.01F);
    const int healthAfterFirstLance = combat.enemyState().currentHealth;
    combat.update({}, 0.01F);
    if (!expect(healthAfterFirstLance == 176,
            "the first Frost Lance should deal twenty-four damage")
        || !expect(combat.enemyState().slowed,
            "the first Frost Lance should apply its two-second movement slow"))
        return false;

    bool observedExpiredSlow = false;
    for (int step = 0; step < 600
        && combat.playerState().spellSlots[0].cooldownRemaining > 0.0F; ++step)
    {
        combat.update({}, 0.01F);
        observedExpiredSlow = observedExpiredSlow || !combat.enemyState().slowed;
    }
    if (!expect(observedExpiredSlow,
            "Frost Lance movement slow should still expire before the base cooldown")
        || !expect(combat.playerState().spellSlots[0].cooldownRemaining <= 0.0F,
            "Frost Lance should become available after its base cooldown"))
        return false;

    combat.update(frost, 0.01F);
    if (!expect(combat.enemyState().currentHealth == 152,
        "the second Frost Lance should hit while the hidden Chill window remains"))
        return false;

    arcane::game::PlayerIntent slam;
    slam.spellPressed[1] = true;
    combat.update(slam, 0.01F);
    combat.update({}, 1.01F);
    return expect(combat.enemyState().currentHealth == 116,
        "the second Frost Lance must freeze and enable Float and Slam's twelve bonus damage");
}

bool combatSessionAppliesEquippedSpellDamage()
{
    auto request = adjacentEnemyRequest();
    request.enemySpawn = {300.0F, 576.0F};
    request.enemyMaximumHealth = 25;
    request.equippedSpellIds[0] = 1006U;
    arcane::game::CombatSession combat(request);
    arcane::game::PlayerIntent cast;
    cast.spellPressed[0] = true;
    combat.update(cast, 0.01F);
    return expect(combat.result().has_value()
            && combat.result()->outcome == arcane::game::CombatOutcome::Victory,
        "spell damage must use combat health and produce victory");
}

bool ultimateSlotUsesSharedCooldownAndRejectsRegularSpells()
{
    arcane::game::spells::SpellSystem spells({}, 2001U);
    const arcane::game::Aabb target {260.0F, 576.0F, 48.0F, 64.0F};
    const auto first = spells.tryCastUltimate({160.0F, 576.0F}, 1.0F, target);
    if (!expect(first.cast && first.hit && first.damage == 70,
            "equipped boss spell must cast from the ultimate slot")
        || !expect(spells.ultimateView().cooldownRemaining
                == arcane::game::spells::SpellSystem::UltimateCooldownSeconds,
            "ultimate cast must start the shared eighteen-second cooldown")
        || !expect(spells.equipUltimate(2002U),
            "another boss spell must be equipable during cooldown")
        || !expect(!spells.tryCastUltimate({160.0F, 576.0F}, 1.0F, target).cast,
            "switching boss spells must not refresh the shared cooldown")
        || !expect(!spells.equipUltimate(1004U),
            "regular spells must be rejected by the ultimate slot")
        || !expect(!spells.equip(0U, 2001U),
            "boss spells must be rejected by regular slots")) return false;

    spells.update(18.0F);
    return expect(spells.tryCastUltimate({160.0F, 576.0F}, 1.0F, target).cast,
        "ultimate slot must become available after the shared cooldown");
}

bool spellDamageTargetsEveryEnemyInsideTheAuthoritativeArea()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {160.0F, 576.0F};
    request.enemies = {
        {arcane::game::EnemyArchetype::HeadlessKnight, {20.0F, 576.0F}},
        {arcane::game::EnemyArchetype::HeadlessKnight, {260.0F, 576.0F}}
    };
    request.equippedSpellIds[0] = 1004U;
    arcane::game::CombatSession combat(request);
    arcane::game::PlayerIntent cast;
    cast.spellPressed[0] = true;
    combat.update(cast, 0.01F);
    const auto enemies = combat.enemyStates();
    return expect(enemies.size() == 2U && enemies[0].currentHealth == 75,
            "an out-of-range first enemy must remain undamaged")
        && expect(enemies[1].currentHealth == 55,
            "an in-range later enemy must receive spell damage even when the first enemy is missed");
}

bool bossRewardDescriptionsExposeImplementedEffects()
{
    const auto* zoltraak = arcane::game::spells::findDefinition(2001U);
    const auto* spears = arcane::game::spells::findDefinition(2002U);
    const auto* severing = arcane::game::spells::findDefinition(2003U);
    return expect(zoltraak && std::string_view {zoltraak->description}.find("70") != std::string_view::npos,
            "Zoltraak reward text must state its implemented damage")
        && expect(spears && std::string_view {spears->description}.find("28") != std::string_view::npos,
            "Goddess Spears reward text must state its per-hit damage")
        && expect(severing && std::string_view {severing->description}.find("78") != std::string_view::npos,
            "Severing Magic reward text must state its implemented damage");
}

bool selectedBossSpellCatalogIsDefinedAndOmitsRejectedChoices()
{
    constexpr std::array ids {2001U, 2002U, 2003U, 2006U, 2007U, 2009U, 2010U, 2011U, 2012U};
    const arcane::game::Aabb target {260.0F, 576.0F, 48.0F, 64.0F};
    for (const std::uint32_t id : ids)
    {
        const auto* definition = arcane::game::spells::findDefinition(id);
        if (!expect(definition && arcane::game::spells::isBossSpell(id),
                "every selected Boss reward must have a Boss-tier definition")) return false;
        arcane::game::spells::SpellSystem spells({}, id);
        const auto cast = spells.tryCastUltimate({160.0F, 576.0F}, 1.0F, target);
        if (!expect(cast.cast && cast.effectBounds.width > 0.0F && cast.effectBounds.height > 0.0F,
                "every selected Boss spell must expose a non-empty effect range")) return false;
    }
    return expect(arcane::game::spells::findDefinition(2004U) == nullptr
            && arcane::game::spells::findDefinition(2005U) == nullptr
            && arcane::game::spells::findDefinition(2008U) == nullptr,
        "Boss spells 2004, 2005 and 2008 must stay out of the implemented catalog");
}

bool goddessSpearsDealsThreeHitsAndHealsOnFullHit()
{
    auto request = adjacentEnemyRequest();
    request.playerCurrentHealth = 50;
    request.enemyMaximumHealth = 200;
    request.enemyContactDamage = 0;
    request.enemyAttackDamage = 0;
    request.equippedUltimateSpellId = 2002U;
    arcane::game::CombatSession combat(request);
    arcane::game::PlayerIntent cast;
    cast.ultimateSpellPressed = true;
    combat.update(cast, 0.01F);
    return expect(combat.enemyState().currentHealth == 116,
            "three goddess spears must deal three times twenty-eight damage")
        && expect(combat.playerState().currentHealth == 62,
            "landing all three spears must heal twelve HP");
}

bool delayedAndSustainedBossSpellsResolveOverTime()
{
    auto lightningRequest = adjacentEnemyRequest();
    lightningRequest.enemyArchetype = arcane::game::EnemyArchetype::ChestMimic;
    lightningRequest.enemyMaximumHealth = 200;
    lightningRequest.enemyContactDamage = 0;
    lightningRequest.enemyAttackDamage = 0;
    lightningRequest.equippedUltimateSpellId = 2007U;
    arcane::game::CombatSession lightning(lightningRequest);
    arcane::game::PlayerIntent cast;
    cast.ultimateSpellPressed = true;
    lightning.update(cast, 0.01F);
    if (!expect(lightning.enemyState().currentHealth == 200,
            "destruction lightning must show its warning before damage")) return false;
    lightning.update({}, 0.81F);
    if (!expect(lightning.enemyState().currentHealth == 110,
            "destruction lightning must strike after its delay")) return false;

    auto fireRequest = adjacentEnemyRequest();
    fireRequest.enemyMaximumHealth = 200;
    fireRequest.enemyContactDamage = 0;
    fireRequest.enemyAttackDamage = 0;
    fireRequest.equippedUltimateSpellId = 2009U;
    arcane::game::CombatSession fire(fireRequest);
    fire.update(cast, 0.01F);
    fire.update({}, 3.0F);
    if (!expect(fire.enemyState().currentHealth == 112,
            "hellfire storm must deal eighty-eight damage over six ticks")) return false;

    auto beamRequest = adjacentEnemyRequest();
    beamRequest.enemyMaximumHealth = 200;
    beamRequest.enemyContactDamage = 0;
    beamRequest.enemyAttackDamage = 0;
    beamRequest.equippedUltimateSpellId = 2010U;
    arcane::game::CombatSession beam(beamRequest);
    beam.update(cast, 0.01F);
    beam.update({}, 2.0F);
    return expect(beam.enemyState().currentHealth == 80,
        "judgment beam must deal eight ticks of fifteen damage when uninterrupted");
}

bool mirrorArrayRepeatsTheNextRegularDamageSpell()
{
    auto request = adjacentEnemyRequest();
    request.enemyContactDamage = 0;
    request.enemyAttackDamage = 0;
    request.equippedSpellIds[0] = 1004U;
    request.equippedUltimateSpellId = 2012U;
    arcane::game::CombatSession combat(request);
    arcane::game::PlayerIntent mirror;
    mirror.ultimateSpellPressed = true;
    combat.update(mirror, 0.01F);
    arcane::game::PlayerIntent regular;
    regular.spellPressed[0] = true;
    combat.update(regular, 0.01F);
    return expect(combat.enemyState().currentHealth == 62,
        "two mirrors must add two rounded thirty-five-percent copies to the next damage spell");
}

bool directBossSpellsApplyTheirDistinctRules()
{
    arcane::game::CombatRequest demonRequest;
    demonRequest.enemies = {{arcane::game::EnemyArchetype::Lugner, {210.0F, 576.0F}}};
    demonRequest.equippedUltimateSpellId = 2001U;
    arcane::game::CombatSession zoltraak(demonRequest);
    arcane::game::PlayerIntent cast;
    cast.ultimateSpellPressed = true;
    zoltraak.update(cast, 0.01F);
    if (!expect(zoltraak.enemyState().currentHealth == 9,
            "demon-killing Zoltraak must apply its thirty-percent demon bonus")) return false;

    auto mimicRequest = adjacentEnemyRequest();
    mimicRequest.enemyMaximumHealth = 100;
    mimicRequest.enemyContactDamage = 0;
    mimicRequest.enemyAttackDamage = 0;
    mimicRequest.equippedUltimateSpellId = 2006U;
    arcane::game::CombatSession mimic(mimicRequest);
    mimic.update(cast, 0.01F);
    if (!expect(mimic.enemyState().currentHealth == 40,
            "mimic magic must guarantee sixty copied damage")) return false;

    auto earthRequest = adjacentEnemyRequest();
    earthRequest.enemyMaximumHealth = 200;
    earthRequest.enemyContactDamage = 0;
    earthRequest.enemyAttackDamage = 0;
    earthRequest.equippedUltimateSpellId = 2011U;
    arcane::game::CombatSession earth(earthRequest);
    earth.update(cast, 0.01F);
    return expect(earth.enemyState().currentHealth == 130,
        "earth dominion must combine the central eruption and one pillar for seventy damage");
}

bool severingMagicGainsExecuteDamageBelowTwentyPercent()
{
    auto request = adjacentEnemyRequest();
    request.enemyMaximumHealth = 500;
    request.enemyContactDamage = 0;
    request.enemyAttackDamage = 0;
    request.equippedSpellIds[0] = 1004U;
    request.equippedSpellIds[1] = 1020U;
    request.equippedUltimateSpellId = 2003U;
    arcane::game::CombatSession combat(request);
    for (int castIndex = 0; castIndex < 20; ++castIndex)
    {
        arcane::game::PlayerIntent bolt;
        bolt.spellPressed[0] = true;
        combat.update(bolt, 0.01F);
        if (castIndex < 19) combat.update({}, 1.8F);
    }
    arcane::game::PlayerIntent binding;
    binding.spellPressed[1] = true;
    combat.update(binding, 0.01F);
    if (!expect(combat.enemyState().currentHealth == 94,
            "test setup must place the target below twenty percent HP")) return false;
    arcane::game::PlayerIntent sever;
    sever.ultimateSpellPressed = true;
    combat.update(sever, 0.01F);
    return expect(combat.result().has_value()
            && combat.result()->outcome == arcane::game::CombatOutcome::Victory,
        "severing magic must gain enough execute damage to defeat a twenty-percent target");
}

bool combatSessionCastsUltimateWithDedicatedInput()
{
    auto request = adjacentEnemyRequest();
    request.enemySpawn = {300.0F, 576.0F};
    request.enemyMaximumHealth = 100;
    request.enemyContactDamage = 0;
    request.enemyAttackDamage = 0;
    request.equippedUltimateSpellId = 2001U;
    arcane::game::CombatSession combat(request);
    arcane::game::PlayerIntent cast;
    cast.ultimateSpellPressed = true;
    combat.update(cast, 0.01F);
    const auto afterFirst = combat.playerState();
    combat.update(cast, 0.01F);
    return expect(combat.enemyState().currentHealth == 30,
            "ultimate input must apply boss spell damage exactly once during cooldown")
        && expect(afterFirst.ultimateSpellSlot.cooldownRemaining == 18.0F,
            "combat view must expose the authoritative ultimate cooldown");
}

bool brokenHighSpeedFormulaDamagesCrossedEnemiesAndReducesCooldown()
{
    auto request = adjacentEnemyRequest();
    request.enemySpawn = {250.0F, 576.0F};
    request.enemyAttackDamage = 0;
    request.enemyContactDamage = 0;
    request.equippedSpellIds[0] = 1004U;
    request.relicIds = {arcane::game::relics::BrokenDashFormulaId};
    arcane::game::CombatSession combat(request);
    arcane::game::PlayerIntent cast;
    cast.spellPressed[0] = true;
    combat.update(cast, 0.01F);
    arcane::game::PlayerIntent dash;
    dash.dashPressed = true;
    combat.update(dash, 0.01F);
    const auto state = combat.playerState();
    return expect(combat.enemyState().currentHealth == 60,
            "broken high-speed formula must deal 20 relic damage to an enemy crossed by Dash")
        && expect(std::abs(state.spellSlots[0].cooldownRemaining - 1.29F) < 0.001F,
            "a successful relic dash must reduce the longest regular cooldown by 0.5 seconds");
}

bool multiEnemyEncounterExposesConfiguredContentAndStartsOnCooldown()
{
    arcane::game::CombatRequest request;
    request.enemies = {
        {arcane::game::EnemyArchetype::ChestMimic, {500.0F, 0.0F}},
        {arcane::game::EnemyArchetype::HeadlessKnight, {700.0F, 0.0F}},
        {arcane::game::EnemyArchetype::BirdDemon, {900.0F, 0.0F}}
    };
    arcane::game::CombatSession combat(request);
    const auto initial = combat.enemyStates();
    combat.update({}, 0.1F);
    const auto afterUpdate = combat.enemyStates();
    return expect(initial.size() == 3U, "normal encounter must own three enemy instances")
        && expect(initial[0].currentHealth == 75 && initial[0].width == 42.0F && initial[0].height == 42.0F,
            "chest mimic content values must be authoritative")
        && expect(initial[1].currentHealth == 75 && initial[1].width == 42.0F && initial[1].height == 58.0F,
            "headless knight content values must be authoritative")
        && expect(initial[2].currentHealth == 45 && initial[2].position.y == 476.0F,
            "bird demon must start 132 pixels above ground using its own height")
        && expect(std::none_of(afterUpdate.begin(), afterUpdate.end(), [](const auto& enemy) {
            return enemy.windingUp || enemy.attackActive;
        }), "all enemy skills must begin on cooldown");
}

bool chestMimicTriggersFromTheFrontAfterHalfCooldown()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {160.0F, 576.0F};
    request.enemies = {{arcane::game::EnemyArchetype::ChestMimic, {223.0F, 0.0F}}};
    arcane::game::CombatSession combat(request);
    combat.update({}, 2.49F);
    if (!expect(!combat.enemyState().windingUp,
        "chest mimic must begin with half of its five-second cooldown")) return false;
    combat.update({}, 0.02F);
    return expect(combat.enemyState().windingUp,
        "player entering the front half-range must trigger chest mimic windup");
}

bool enemyChasePreservesConfiguredBodyGap()
{
    const arcane::game::Aabb player {160.0F, 576.0F, 42.0F, 64.0F};
    const arcane::game::WorldBounds world {0.0F, 1280.0F, 640.0F};
    arcane::game::ai::EnemyConfig contactConfig;
    contactConfig.moveSpeed = 1000.0F;
    contactConfig.attackRange = 1.0F;
    contactConfig.width = 42.0F;
    contactConfig.hasContactDamage = true;
    contactConfig.cooldownSeconds = 100.0F;
    arcane::game::ai::EnemyController contact({600.0F, 576.0F}, contactConfig);
    contact.update(player, 1.0F, world);

    auto rangedConfig = contactConfig;
    rangedConfig.hasContactDamage = false;
    arcane::game::ai::EnemyController ranged({600.0F, 576.0F}, rangedConfig);
    ranged.update(player, 1.0F, world);
    const float playerRight = player.left + player.width;
    return expect(std::abs(contact.position().x - playerRight - 20.0F) < 0.01F,
            "contact enemies must preserve a twenty-pixel body gap")
        && expect(std::abs(ranged.position().x - playerRight - 42.0F) < 0.01F,
            "non-contact enemies must preserve a forty-two-pixel body gap");
}

bool frostWolfClawUsesAnInstantHitWindow()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {160.0F, 576.0F};
    request.enemies = {{arcane::game::EnemyArchetype::FrostWolf, {245.0F, 0.0F}}};
    arcane::game::CombatSession combat(request);

    combat.update({}, 2.01F);
    if (!expect(!combat.enemyState().skillEffectBounds.has_value(),
        "Frost Wolf claw must not expose a visible range rectangle")) return false;
    arcane::game::PlayerIntent moveAway;
    moveAway.moveAxis = -1.0F;
    combat.update(moveAway, 0.20F);
    combat.update({}, 0.30F);
    const int healthWhenClawActivates = combat.playerState().currentHealth;

    arcane::game::PlayerIntent moveIntoClaw;
    moveIntoClaw.moveAxis = 1.0F;
    combat.update(moveIntoClaw, 0.30F);

    return expect(healthWhenClawActivates == 100,
            "Frost Wolf claw must not hit a player outside its range when windup ends")
        && expect(combat.playerState().currentHealth == healthWhenClawActivates,
            "Frost Wolf claw must not hit a player who enters during the visual active time");
}

bool chestMimicBiteStunsWithoutKnockback()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {160.0F, 576.0F};
    request.enemies = {{arcane::game::EnemyArchetype::ChestMimic, {223.0F, 0.0F}}};
    arcane::game::CombatSession combat(request);
    combat.update({}, 2.51F);
    combat.update({}, 0.50F);
    combat.update({}, 0.05F);
    const auto bitten = combat.playerState();
    combat.update({}, 0.10F);
    return expect(bitten.stunned && bitten.stunRemaining > 0.9F,
            "chest mimic bite must hold the player for one second")
        && expect(std::abs(combat.playerState().position.x - bitten.position.x) < 0.01F,
            "chest mimic bite must not knock the player away");
}

bool linieUsesGroundedMediumImpactArea()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {160.0F, 576.0F};
    request.enemies = {{arcane::game::EnemyArchetype::Linie, {223.0F, 0.0F}}};
    arcane::game::CombatSession combat(request);
    if (!expect(combat.enemyState().currentHealth == 120, "Linie must have one hundred twenty HP")) return false;
    combat.update({}, 2.51F);
    combat.update({}, 0.50F);
    combat.update({}, 0.91F);
    const auto area = combat.enemyState().skillEffectBounds;
    if (!(expect(area.has_value(), "Linie landing must expose its impact rectangle")
        && expect(area->top == 616.0F && area->height == 24.0F,
            "Linie impact rectangle must sit on the ground and be twenty-four pixels high")
        && expect(area->width == 210.0F,
            "Linie impact must extend eighty-four pixels on both sides"))) return false;
    combat.update({}, 0.80F);
    return expect(combat.enemyState().attackActive
            && combat.enemyState().skillEffectBounds.has_value(),
        "Linie landing pose and effect must remain active for one full second");
}

void advanceDialogue(arcane::game::CombatSession& combat)
{
    while (combat.dialogueLine())
    {
        arcane::game::PlayerIntent confirm;
        confirm.menuConfirmPressed = true;
        combat.update(confirm, 0.01F);
    }
}

bool displacementSkillTriggersWhenPlayerEdgeEntersExtendedRange()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {160.0F, 576.0F};
    request.enemies = {{arcane::game::EnemyArchetype::Linie, {430.0F, 0.0F}}};
    arcane::game::CombatSession combat(request);
    combat.update({}, 2.01F);
    return expect(combat.enemyState().windingUp,
        "a displacement skill must wind up as soon as the player's edge enters its extended range");
}

bool auraOpensWithTwoHeadlessKnights()
{
    arcane::game::CombatRequest request;
    request.enemyArchetype = arcane::game::EnemyArchetype::Aura;
    request.enemyMaximumHealth = 225;
    arcane::game::CombatSession combat(request);
    const auto enemies = combat.enemyStates();
    return expect(enemies.size() == 3U, "Aura must open combat by summoning two knights")
        && expect(enemies[0].archetype == arcane::game::EnemyArchetype::Aura
                && enemies[0].currentHealth == 225
                && enemies[0].width == 42.0F && enemies[0].height == 64.0F,
            "first boss must be Guillotine Aura with 225 HP and a 42 by 64 body")
        && expect(enemies[1].archetype == arcane::game::EnemyArchetype::HeadlessKnight
                && enemies[2].archetype == arcane::game::EnemyArchetype::HeadlessKnight,
            "Aura opening army must contain two headless knights");
}

bool auraDominationUsesTelegraphedFrontRange()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {160.0F, 576.0F};
    request.enemyArchetype = arcane::game::EnemyArchetype::Aura;
    request.enemySpawn = {900.0F, 576.0F};
    request.enemyMaximumHealth = 225;
    request.equippedSpellIds[0] = 1002U;
    arcane::game::CombatSession combat(request);
    if (!expect(combat.bossIntro().has_value() && !combat.dialogueLine(),
        "Aura combat must reveal the boss name before dialogue")) return false;
    combat.update({}, 2.40F);
    if (!expect(combat.dialogueLine().has_value(),
        "Aura combat must begin with pre-battle dialogue")) return false;
    advanceDialogue(combat);
    arcane::game::PlayerIntent blessing;
    blessing.spellPressed[0] = true;
    combat.update(blessing, 0.01F);
    combat.update({}, 3.51F);
    combat.update({}, 0.01F);
    if (!expect(combat.dialogueLine().has_value(),
        "Aura's first domination must pause combat for dialogue")) return false;
    const auto winding = combat.enemyState();
    if (!expect(winding.windingUp && winding.skillEffectBounds.has_value(),
        "Aura domination must expose its telegraphed range during windup")) return false;
    const auto area = *winding.skillEffectBounds;
    if (!expect(area.width == 420.0F && area.height == 180.0F,
            "Aura domination must use the configured 420 by 180 area")
        || !expect(area.left + area.width <= winding.position.x,
            "left-facing domination must remain entirely in front of Aura")) return false;
    const float cooldownDuringDialogue = winding.skillEffectProgress;
    combat.update({}, 100.0F);
    if (!expect(combat.enemyState().skillEffectProgress == cooldownDuringDialogue,
        "combat simulation must remain frozen while dialogue is active")) return false;
    advanceDialogue(combat);
    combat.update({}, 0.80F);
    combat.update({}, 0.01F);
    if (!expect(combat.playerState().stunRemaining > 0.9F
            && combat.playerState().stunRemaining <= 1.0F,
        "first domination must control a player who remains inside the telegraph")) return false;

    arcane::game::CombatRequest dodgeRequest = request;
    dodgeRequest.equippedSpellIds[0] = std::nullopt;
    arcane::game::CombatSession dodge(dodgeRequest);
    dodge.update({}, 2.40F);
    advanceDialogue(dodge);
    dodge.update({}, 3.51F);
    dodge.update({}, 0.01F);
    if (!expect(dodge.dialogueLine().has_value(),
        "dodge scenario must reach the first domination dialogue")) return false;
    advanceDialogue(dodge);
    arcane::game::PlayerIntent escape;
    escape.moveAxis = -1.0F;
    dodge.update(escape, 0.65F);
    dodge.update({}, 0.16F);
    if (!expect(dodge.playerState().stunRemaining <= 0.0F,
        "leaving the telegraphed rectangle during windup must avoid domination")) return false;

    arcane::game::CombatSession shadowDash(dodgeRequest);
    shadowDash.update({}, 2.40F);
    advanceDialogue(shadowDash);
    shadowDash.update({}, 3.51F);
    shadowDash.update({}, 0.01F);
    advanceDialogue(shadowDash);
    shadowDash.update({}, 0.69F);
    arcane::game::PlayerIntent dash;
    dash.dashPressed = true;
    shadowDash.update(dash, 0.01F);
    shadowDash.update({}, 0.11F);
    shadowDash.update({}, 0.20F);
    return expect(shadowDash.playerState().stunRemaining <= 0.0F,
        "a charged Shade Dash must avoid even the first domination impact");
}

bool auraSoulGuillotineLocksPositionAndCanBeSidestepped()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {160.0F, 576.0F};
    request.enemyArchetype = arcane::game::EnemyArchetype::Aura;
    request.enemySpawn = {900.0F, 576.0F};
    request.enemyMaximumHealth = 225;
    request.enemyAttackDamage = 0;
    request.enemyContactDamage = 0;

    const auto reachGuillotine = [](arcane::game::CombatSession& combat) {
        combat.update({}, 2.40F);
        advanceDialogue(combat);
        combat.update({}, 3.51F);
        combat.update({}, 0.01F);
        if (!combat.dialogueLine()) return false;
        advanceDialogue(combat);
        arcane::game::PlayerIntent escapeDomination;
        escapeDomination.moveAxis = -1.0F;
        combat.update(escapeDomination, 0.65F);
        combat.update({}, 0.16F);
        for (int step = 0; step < 500; ++step)
        {
            const auto aura = combat.enemyState();
            if (aura.specialWindingUp && aura.skillVariant == 0
                && aura.skillEffectBounds) return true;
            combat.update({}, 0.01F);
        }
        return false;
    };

    arcane::game::CombatSession hit(request);
    if (!expect(reachGuillotine(hit),
            "Aura must unlock Soul Guillotine after her first Domination")) return false;
    const auto telegraph = hit.enemyState();
    const auto area = *telegraph.skillEffectBounds;
    if (!expect(area.width == 192.0F && area.height == 420.0F,
            "Soul Guillotine must expose its narrow authoritative execution column")
        || !expect(arcane::game::intersects(area, arcane::game::Aabb {hit.playerState().position.x,
                hit.playerState().position.y, 42.0F, 64.0F}),
            "Soul Guillotine must lock the player's position when windup begins")) return false;
    hit.update({}, 0.91F);
    if (!expect(hit.playerState().currentHealth == 78,
            "remaining inside Soul Guillotine must deal twenty-two damage")) return false;

    arcane::game::CombatSession dodge(request);
    if (!expect(reachGuillotine(dodge),
            "the dodge scenario must reach Soul Guillotine windup")) return false;
    arcane::game::PlayerIntent sidestep;
    sidestep.moveAxis = 1.0F;
    dodge.update(sidestep, 0.80F);
    dodge.update({}, 0.11F);
    return expect(dodge.playerState().currentHealth == 100,
        "moving outside the locked column during windup must avoid Soul Guillotine");
}

bool defeatingAuraClearsHerArmyAndStartsDefeatDialogue()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {160.0F, 576.0F};
    request.enemySpawn = {210.0F, 576.0F};
    request.enemyArchetype = arcane::game::EnemyArchetype::Aura;
    request.enemyMaximumHealth = 1;
    arcane::game::CombatSession combat(request);
    combat.update({}, 2.40F);
    advanceDialogue(combat);
    arcane::game::PlayerIntent attack;
    attack.attackPressed = true;
    combat.update(attack, 0.01F);
    const auto enemies = combat.enemyStates();
    const bool allDefeated = std::all_of(enemies.begin(), enemies.end(), [](const auto& enemy) {
        return !enemy.alive;
    });
    return expect(allDefeated, "Aura's death must immediately eliminate every summoned enemy")
        && expect(combat.dialogueLine().has_value()
                && combat.dialogueLine()->speaker == "FRIEREN",
            "Aura's death must pause on the post-battle dialogue before victory resolves")
        && expect(!combat.result().has_value(),
            "Aura victory must not resolve until the post-battle dialogue ends");
}

bool redMirrorDragonBreathRespectsPostHitInvulnerability()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {160.0F, 576.0F};
    request.enemySpawn = {300.0F, 556.0F};
    request.enemyArchetype = arcane::game::EnemyArchetype::RedMirrorDragon;
    request.enemyMaximumHealth = 300;
    arcane::game::CombatSession combat(request);
    const auto dragon = combat.enemyState();
    if (!expect(dragon.currentHealth == 300 && dragon.width == 128.0F && dragon.height == 84.0F,
        "red mirror dragon content values must be authoritative")) return false;
    combat.update({}, 6.01F);
    combat.update({}, 1.01F);
    combat.update({}, 1.0F);
    combat.update({}, 1.0F);
    return expect(combat.playerState().currentHealth == 70,
        "half-second breath ticks inside the 0.6-second post-hit window must be blocked");
}

bool enemyDirectionLocksWhenWindupBegins()
{
    arcane::game::ai::EnemyConfig config;
    config.attackDistance = 500.0F;
    config.attackRange = 500.0F;
    config.cooldownSeconds = 0.0F;
    config.windupSeconds = 0.5F;
    config.activeSeconds = 1.0F;
    config.skill = arcane::game::ai::EnemySkill::Dive;
    arcane::game::ai::EnemyController enemy({500.0F, 476.0F}, config);
    const arcane::game::WorldBounds world {0.0F, 1280.0F, 640.0F};
    enemy.update({300.0F, 576.0F, 42.0F, 64.0F}, 0.01F, world);
    if (!expect(enemy.action() == arcane::game::ai::EnemyAction::Windup
            && enemy.facingDirection() < 0.0F,
        "enemy must choose its direction when windup begins")) return false;
    enemy.update({700.0F, 576.0F, 42.0F, 64.0F}, 0.50F, world);
    return expect(enemy.action() == arcane::game::ai::EnemyAction::Active
            && enemy.facingDirection() < 0.0F,
        "enemy must not turn around after the player crosses during windup");
}

bool secondActEnemiesExposeConfiguredContent()
{
    arcane::game::CombatRequest request;
    request.enemies = {
        {arcane::game::EnemyArchetype::ChaosFlower, {500.0F, 0.0F}},
        {arcane::game::EnemyArchetype::FrostWolf, {700.0F, 0.0F}},
        {arcane::game::EnemyArchetype::Qual, {900.0F, 0.0F}}
    };
    arcane::game::CombatSession combat(request);
    const auto enemies = combat.enemyStates();
    return expect(enemies.size() == 3U, "second-act opening encounter must contain three enemies")
        && expect(enemies[0].currentHealth == 125 && enemies[0].width == 64.0F
                && enemies[0].height == 84.0F,
            "chaos flower content values must be authoritative")
        && expect(enemies[1].currentHealth == 120 && enemies[1].width == 64.0F
                && enemies[1].height == 42.0F,
            "frost wolf content values must be authoritative")
        && expect(enemies[2].currentHealth == 150 && enemies[2].width == 64.0F
                && enemies[2].height == 96.0F,
            "Qual content values must be authoritative");
}

bool qualKillingMagicUsesRestrictedEffectHeight()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {160.0F, 576.0F};
    request.enemies = {{arcane::game::EnemyArchetype::Qual, {260.0F, 0.0F}}};
    arcane::game::CombatSession combat(request);

    combat.update({}, 1.51F);
    combat.update({}, 0.50F);
    const auto area = combat.enemyState().skillEffectBounds;
    return expect(area.has_value(), "Qual killing magic must expose its active effect area")
        && expect(std::abs(area->width - 96.0F) < 0.01F,
            "Qual killing magic must preserve its configured horizontal range")
        && expect(std::abs(area->height - 34.0F) < 0.01F,
            "Qual killing magic collision height must match its narrow visual effect")
        && expect(std::abs(area->top - 565.0F) < 0.01F,
            "Qual killing magic must sit ten pixels above body center");
}

bool lugnerBloodMagicUsesRaisedSpriteHeightArea()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {160.0F, 576.0F};
    request.enemies = {{arcane::game::EnemyArchetype::Lugner, {230.0F, 0.0F}}};
    arcane::game::CombatSession combat(request);
    combat.update({}, 3.01F);
    const auto enemy = combat.enemyState();
    return expect(enemy.skillEffectBounds.has_value(),
            "Lugner blood magic must expose its windup area")
        && expect(enemy.skillEffectBounds->height == 54.0F,
            "blood magic height must match the effect image")
        && expect(enemy.skillEffectBounds->width == 88.0F,
            "blood magic must use its updated eighty-eight pixel range")
        && expect(enemy.skillEffectBounds->top == enemy.position.y - 18.0F,
            "blood magic must be raised above Lugner's body origin");
}

bool chaosFlowerSleepReducesOutgoingDamageWithoutWindup()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {160.0F, 576.0F};
    request.enemies = {{arcane::game::EnemyArchetype::ChaosFlower, {223.0F, 0.0F}}};
    arcane::game::CombatSession combat(request);
    combat.update({}, 3.51F);
    const auto cursed = combat.playerState();
    if (!expect(cursed.sleepRemaining > 4.99F && !cursed.stunned,
            "sleep curse must apply immediately for five seconds without stunning")
        || !expect(!combat.enemyState().specialWindingUp,
            "sleep curse must not enter a windup state")) return false;

    arcane::game::PlayerIntent moveLeft;
    moveLeft.moveAxis = -1.0F;
    combat.update(moveLeft, 0.10F);
    if (!expect(std::abs(combat.playerState().position.x - 139.2F) < 0.01F,
        "sleep must reduce normal horizontal movement speed by twenty percent")) return false;
    arcane::game::PlayerIntent moveRight;
    moveRight.moveAxis = 1.0F;
    combat.update(moveRight, 0.10F);

    arcane::game::PlayerIntent attack;
    attack.attackPressed = true;
    combat.update(attack, 0.01F);
    return expect(combat.enemyState().currentHealth == 115,
            "sleep must reduce the player's fifteen damage attack by thirty percent")
        && expect(!combat.enemyState().skillEffectBounds.has_value(),
            "Chaos Flower skills must not expose a visible range rectangle");
}

bool lateSecondActEnemiesExposeConfiguredContent()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {300.0F, 576.0F};
    request.enemies = {
        {arcane::game::EnemyArchetype::Laufen, {700.0F, 0.0F}},
        {arcane::game::EnemyArchetype::Richter, {900.0F, 0.0F}},
        {arcane::game::EnemyArchetype::Denken, {1100.0F, 0.0F}}
    };
    arcane::game::CombatSession combat(request);
    const auto enemies = combat.enemyStates();
    return expect(enemies.size() == 3U, "late second-act encounter must contain three enemies")
        && expect(enemies[0].currentHealth == 160 && enemies[0].width == 42.0F
                && enemies[0].height == 64.0F,
            "Laufen content values must be authoritative")
        && expect(enemies[1].currentHealth == 150 && enemies[1].width == 42.0F
                && enemies[1].height == 72.0F,
            "Richter content values must be authoritative")
        && expect(enemies[2].currentHealth == 135 && enemies[2].width == 42.0F
                && enemies[2].height == 64.0F,
            "Denken content values must be authoritative");
}

bool laufenTeleportsBehindAndImmediatelySideKicks()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {300.0F, 576.0F};
    request.enemies = {{arcane::game::EnemyArchetype::Laufen, {800.0F, 0.0F}}};
    arcane::game::CombatSession combat(request);
    combat.update({}, 2.51F);
    combat.update({}, 0.50F);
    const auto enemy = combat.enemyState();
    if (!expect(std::abs(enemy.position.x - 234.0F) < 0.01F,
            "Laufen must prefer the position twenty-four pixels behind the player")
        || !expect(combat.playerState().currentHealth == 85,
            "speed magic must force an immediate side kick")) return false;
    combat.update({}, 0.30F);
    return expect(combat.playerState().currentHealth == 85,
        "Laufen side kick must only deal damage once when its windup ends");
}

bool swordRelicsModifyCollisionDamage()
{
    arcane::game::CombatRequest reductionRequest;
    reductionRequest.playerSpawn = {160.0F, 576.0F};
    reductionRequest.enemies = {{arcane::game::EnemyArchetype::ChestMimic, {160.0F, 0.0F}}};
    reductionRequest.relicIds = {arcane::game::relics::HeroSwordId};
    arcane::game::CombatSession reduction(reductionRequest);
    reduction.update({}, 0.01F);
    if (!expect(reduction.playerState().currentHealth == 95,
        "Hero Sword must reduce fifteen collision damage to five")) return false;

    auto retaliationRequest = reductionRequest;
    retaliationRequest.relicIds = {arcane::game::relics::TrueHeroSwordId};
    arcane::game::CombatSession retaliation(retaliationRequest);
    retaliation.update({}, 0.01F);
    return expect(retaliation.enemyState().currentHealth == 45,
        "True Hero Sword must retaliate for thirty nearby damage");
}

bool expandedSpellAndRelicCatalogIsComplete()
{
    for (std::uint32_t id = 1027U; id <= 1030U; ++id)
        if (!expect(arcane::game::spells::findDefinition(id) != nullptr,
            "new spell IDs 1027 through 1030 must have definitions")) return false;
    for (std::uint32_t id = 4001U; id <= 4024U; ++id)
        if (!expect(arcane::game::relics::findDefinition(id) != nullptr,
            "relic IDs 4001 through 4024 must have definitions")) return false;
    return true;
}

bool newSpellsApplyTargetingShieldWindAndGravity()
{
    auto request = adjacentEnemyRequest();
    request.equippedSpellIds = {1027U, 1028U, 1030U};
    request.enemyAttackDamage = 0;
    request.enemyContactDamage = 0;
    arcane::game::CombatSession combat(request);

    arcane::game::PlayerIntent homing;
    homing.spellPressed[0] = true;
    combat.update(homing, 0.01F);
    if (!expect(combat.enemyState().currentHealth == 79,
        "Homing Volley must deal three seven-damage hits to a ground enemy")) return false;

    arcane::game::PlayerIntent barrier;
    barrier.spellPressed[1] = true;
    combat.update(barrier, 0.01F);
    if (!expect(combat.playerState().shield == 24,
        "Defensive Barrier must grant twenty-four shield")) return false;

    arcane::game::PlayerIntent gravity;
    gravity.spellPressed[2] = true;
    combat.update(gravity, 0.01F);
    combat.update({}, 1.01F);
    if (!expect(combat.enemyState().currentHealth == 73,
        "Gravity Well must deal its first six-damage tick")) return false;

    auto windRequest = adjacentEnemyRequest();
    windRequest.equippedSpellIds[0] = 1029U;
    windRequest.enemyAttackDamage = 0;
    windRequest.enemyContactDamage = 0;
    arcane::game::CombatSession wind(windRequest);
    arcane::game::PlayerIntent castWind;
    castWind.spellPressed[0] = true;
    wind.update(castWind, 0.01F);
    return expect(wind.enemyState().currentHealth == 86,
            "Wind Pressure must deal fourteen damage")
        && expect(wind.enemyState().position.x > 210.0F,
            "Wind Pressure must knock the target away");
}

bool combatRelicsModifyCooldownBloodUltimateAndGold()
{
    auto cooldownRequest = adjacentEnemyRequest();
    cooldownRequest.equippedSpellIds = {1004U, 1005U, std::nullopt};
    cooldownRequest.relicIds = {arcane::game::relics::HolyStaffId};
    cooldownRequest.enemyAttackDamage = 0;
    cooldownRequest.enemyContactDamage = 0;
    arcane::game::CombatSession cooldown(cooldownRequest);
    arcane::game::PlayerIntent frost;
    frost.spellPressed[1] = true;
    cooldown.update(frost, 0.01F);
    arcane::game::PlayerIntent zoltraak;
    zoltraak.spellPressed[0] = true;
    cooldown.update(zoltraak, 0.01F);
    if (!expect(cooldown.playerState().spellSlots[1].cooldownRemaining < 3.8F,
        "Holy Staff must reduce the other regular slot on its first cast")) return false;

    auto bloodRequest = adjacentEnemyRequest();
    bloodRequest.equippedSpellIds[0] = 1003U;
    bloodRequest.relicIds = {arcane::game::relics::DemonCoinId};
    bloodRequest.enemyAttackDamage = 0;
    bloodRequest.enemyContactDamage = 0;
    arcane::game::CombatSession blood(bloodRequest);
    arcane::game::PlayerIntent castBlood;
    castBlood.spellPressed[0] = true;
    blood.update(castBlood, 0.01F);
    if (!expect(blood.enemyState().currentHealth == 40,
        "Demon Coin must increase Blood Magic final damage by twenty-five percent")) return false;

    auto ultimateRequest = adjacentEnemyRequest();
    ultimateRequest.equippedUltimateSpellId = 2001U;
    ultimateRequest.relicIds = {arcane::game::relics::SeriePageId};
    ultimateRequest.enemyMaximumHealth = 200;
    ultimateRequest.enemyAttackDamage = 0;
    ultimateRequest.enemyContactDamage = 0;
    arcane::game::CombatSession ultimate(ultimateRequest);
    arcane::game::PlayerIntent castUltimate;
    castUltimate.ultimateSpellPressed = true;
    ultimate.update(castUltimate, 0.01F);
    if (!expect(std::abs(ultimate.playerState().ultimateSpellSlot.cooldownRemaining - 15.3F) < 0.01F,
        "Serie Page must reduce the authoritative ultimate cooldown to 15.3 seconds")) return false;

    auto goldRequest = adjacentEnemyRequest();
    goldRequest.enemyMaximumHealth = 15;
    goldRequest.enemyAttackDamage = 0;
    goldRequest.enemyContactDamage = 0;
    goldRequest.goldReward = 10;
    goldRequest.relicIds = {arcane::game::relics::OldCopperCoinId};
    arcane::game::CombatSession gold(goldRequest);
    arcane::game::PlayerIntent attack;
    attack.attackPressed = true;
    gold.update(attack, 0.01F);
    return expect(gold.result() && gold.result()->goldAwarded == 20,
        "Old Copper Coin must add ten gold to a flawless combat victory");
}

bool revolteLocksAtFiveAndHealsForSecondPhase()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {160.0F, 576.0F};
    request.enemySpawn = {210.0F, 544.0F};
    request.enemyArchetype = arcane::game::EnemyArchetype::Revolte;
    request.enemyMaximumHealth = 30;
    request.enemyContactDamage = 0;
    arcane::game::CombatSession combat(request);
    if (!expect(combat.bossIntro().has_value() && combat.bossIntro()->name == "REVOLTE",
            "the second boss must expose Revolte's boss introduction")) return false;
    combat.update({}, 2.4F);
    advanceDialogue(combat);

    arcane::game::PlayerIntent attack;
    attack.attackPressed = true;
    combat.update(attack, 0.01F);
    combat.update({}, 0.5F);
    combat.update(attack, 0.01F);
    if (!expect(combat.enemyState().currentHealth == 5,
            "Revolte's first lethal threshold must lock at five HP")
        || !expect(combat.dialogueLine().has_value()
                && combat.dialogueLine()->speaker == "REVOLTE",
            "reaching the threshold must pause combat for second-phase dialogue")) return false;
    advanceDialogue(combat);
    if (!expect(combat.enemyState().currentHealth == 30,
            "second-phase dialogue must restore Revolte to full maximum HP")) return false;
    const float beforeDash = combat.enemyState().position.x;
    combat.update({}, 3.0F);
    arcane::game::PlayerIntent crossBehind;
    crossBehind.moveAxis = 1.0F;
    combat.update(crossBehind, 0.5F);
    const auto releaseState = combat.enemyState();
    const float playerCenterAtRelease = combat.playerState().position.x
        + arcane::game::PlayerController::Width * 0.5F;
    const float bossCenterAtRelease = releaseState.position.x + releaseState.width * 0.5F;
    const float releaseDirection = playerCenterAtRelease >= bossCenterAtRelease ? 1.0F : -1.0F;
    if (!expect(releaseState.facingDirection == releaseDirection,
            "Revolte must retarget and face the player's current side when dash windup ends"))
        return false;
    combat.update({}, 0.7F);
    return expect((combat.enemyState().position.x - beforeDash) * releaseDirection >= 195.9F,
        "second-phase dash must travel 196 pixels toward the player's release-time position");
}

bool denkenExposesCremationPoseWhenTornadoEvolves()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {300.0F, 576.0F};
    request.enemies = {{arcane::game::EnemyArchetype::Denken, {700.0F, 0.0F}}};
    arcane::game::CombatSession combat(request);
    combat.update({}, 4.51F);
    combat.update({}, 0.50F);
    combat.update({}, 2.90F);
    combat.update({}, 0.11F);
    const auto effects = combat.spellEffects();
    return expect(combat.enemyState().specialAttackActive,
            "Denken must expose the cremation-magic pose when a tornado evolves")
        && expect(std::any_of(effects.begin(), effects.end(), [](const auto& effect) {
                return effect.spellId == 9003U;
            }), "Denken's tornado must switch to its evolved visual after three seconds");
}

bool revolteParryActivatesWithoutWindup()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {20.0F, 576.0F};
    request.enemySpawn = {1000.0F, 544.0F};
    request.enemyArchetype = arcane::game::EnemyArchetype::Revolte;
    request.enemyContactDamage = 0;
    arcane::game::CombatSession combat(request);
    combat.update({}, 2.4F);
    advanceDialogue(combat);
    combat.update({}, 4.51F);
    const auto state = combat.enemyState();
    return expect(state.skillVariant == 3 && state.attackActive && !state.windingUp,
        "Revolte parry must become active immediately without a windup state");
}

bool revolteWavesCrossTheArenaAndSecondPhaseAddsCrossExecution()
{
    arcane::game::CombatRequest waveRequest;
    waveRequest.playerSpawn = {160.0F, 576.0F};
    waveRequest.enemySpawn = {450.0F, 544.0F};
    waveRequest.enemyArchetype = arcane::game::EnemyArchetype::Revolte;
    waveRequest.enemyContactDamage = 0;
    waveRequest.enemyAttackDamage = 0;
    arcane::game::CombatSession wave(waveRequest);
    wave.update({}, 2.4F);
    advanceDialogue(wave);
    wave.update({}, 3.51F);
    wave.update({}, 0.33F);
    const auto waveEffects = wave.spellEffects();
    const auto slash = std::find_if(waveEffects.begin(), waveEffects.end(), [](const auto& effect) {
        return effect.spellId == 9101U;
    });
    if (!expect(slash != waveEffects.end() && slash->duration > 1.52F
            && slash->duration < 1.53F,
            "Revolte's light sword wave must retain a finite 480-pixel path at 315 px/s"))
        return false;
    const float initialWaveTop = slash->bounds.top;
    arcane::game::PlayerIntent jump;
    jump.jumpPressed = true;
    wave.update(jump, 0.10F);
    const auto armingEffects = wave.spellEffects();
    const auto armingSlash = std::find_if(armingEffects.begin(), armingEffects.end(),
        [](const auto& effect) { return effect.spellId == 9101U; });
    if (!expect(armingSlash != armingEffects.end()
            && std::abs(armingSlash->bounds.top - initialWaveTop) < 0.5F
            && std::abs(armingSlash->rotationDegrees) < 0.5F,
            "Revolte's sword wave must fly straight during its 0.15-second arming window"))
        return false;
    wave.update({}, 0.25F);
    const auto trackingEffects = wave.spellEffects();
    const auto trackingSlash = std::find_if(trackingEffects.begin(), trackingEffects.end(),
        [](const auto& effect) { return effect.spellId == 9101U; });
    if (!expect(trackingSlash != trackingEffects.end()
            && trackingSlash->bounds.top < initialWaveTop - 8.0F
            && std::abs(trackingSlash->rotationDegrees) > 5.0F
            && std::abs(trackingSlash->rotationDegrees) <= 16.0F,
            "Revolte's sword wave must curve toward a jump without turning instantly")) return false;

    arcane::game::CombatRequest phaseRequest;
    phaseRequest.playerSpawn = {500.0F, 576.0F};
    phaseRequest.playerMaximumHealth = 500;
    phaseRequest.playerCurrentHealth = 500;
    phaseRequest.enemySpawn = {450.0F, 544.0F};
    phaseRequest.enemyArchetype = arcane::game::EnemyArchetype::Revolte;
    phaseRequest.enemyMaximumHealth = 30;
    phaseRequest.enemyContactDamage = 0;
    phaseRequest.enemyAttackDamage = 0;
    arcane::game::CombatSession phase(phaseRequest);
    phase.update({}, 2.4F);
    advanceDialogue(phase);
    arcane::game::PlayerIntent faceLeft;
    faceLeft.moveAxis = -1.0F;
    phase.update(faceLeft, 0.01F);
    arcane::game::PlayerIntent attack;
    attack.attackPressed = true;
    phase.update(attack, 0.01F);
    phase.update({}, 0.5F);
    phase.update(attack, 0.01F);
    if (!expect(phase.dialogueLine().has_value(),
            "the cross-execution test must reach Revolte's second phase")) return false;
    advanceDialogue(phase);
    arcane::game::PlayerIntent faceRight;
    faceRight.moveAxis = 1.0F;
    phase.update(faceRight, 0.01F);

    bool sawFirstWarning = false;
    bool sawFirstSlash = false;
    bool sawSecondWarning = false;
    bool sawSecondSlash = false;
    bool sawCrossImpact = false;
    bool sawSlashGap = false;
    bool issuedEscapeDash = false;
    std::optional<float> firstImpactCenterX;
    std::optional<float> secondImpactCenterX;
    for (int step = 0; step < 220 && !sawSecondSlash; ++step)
    {
        arcane::game::PlayerIntent movement;
        if (sawFirstWarning && !sawFirstSlash)
        {
            movement.moveAxis = 1.0F;
            if (!issuedEscapeDash)
            {
                movement.dashPressed = true;
                issuedEscapeDash = true;
            }
        }
        phase.update(movement, 0.1F);
        const auto state = phase.enemyState();
        const auto effects = phase.spellEffects();
        if (state.skillVariant == 5 && state.windingUp)
        {
            const std::uint32_t warningId = sawFirstSlash
                ? arcane::game::RevolteCrossSlashSecondTelegraphVisualId
                : arcane::game::RevolteCrossSlashFirstTelegraphVisualId;
            std::vector<const arcane::game::SpellEffectView*> warnings;
            for (const auto& effect : effects)
                if (effect.spellId == warningId) warnings.push_back(&effect);
            if (warnings.size() == 2U
                && warnings[0]->bounds.height == 52.0F
                && warnings[1]->bounds.height == 52.0F
                && warnings[0]->rotationDegrees * warnings[1]->rotationDegrees < 0.0F)
            {
                if (!sawFirstSlash) sawFirstWarning = true;
                else sawSecondWarning = true;
            }
        }

        const auto slashCount = std::count_if(effects.begin(), effects.end(), [](const auto& effect) {
            return effect.spellId == arcane::game::RevolteCrossSlashVisualId
                && effect.bounds.height == 52.0F;
        });
        if (sawFirstSlash && slashCount == 0) sawSlashGap = true;
        if (slashCount == 2)
        {
            if (sawSecondWarning && sawSlashGap) sawSecondSlash = true;
            else if (!sawFirstSlash) sawFirstSlash = true;
        }
        const auto crossImpact = std::find_if(effects.begin(), effects.end(),
            [](const auto& effect) {
                return effect.spellId == arcane::game::RevolteCrossSlashImpactVisualId;
            });
        if (crossImpact != effects.end())
        {
            sawCrossImpact = true;
            const float centerX = crossImpact->bounds.left + crossImpact->bounds.width * 0.5F;
            if (!firstImpactCenterX) firstImpactCenterX = centerX;
            else if (sawSecondSlash) secondImpactCenterX = centerX;
        }
    }
    if (!(expect(sawFirstWarning,
            "Cross Execution must expose two opposite diagonal warnings for its first X")
        && expect(sawFirstSlash && sawCrossImpact,
            "Cross Execution must resolve the first X with two slash visuals and a center burst")
        && expect(sawSecondWarning
                && firstImpactCenterX && secondImpactCenterX
                && std::abs(*secondImpactCenterX - *firstImpactCenterX) > 40.0F,
            "Cross Execution must retarget the moved player before warning its second X")
        && expect(sawSecondSlash,
            "Cross Execution must resolve a second pair of crossing diagonal slashes")))
        return false;

    arcane::game::CombatSession bladePhase(phaseRequest);
    bladePhase.update({}, 2.4F);
    advanceDialogue(bladePhase);
    bladePhase.update(faceLeft, 0.01F);
    bladePhase.update(attack, 0.01F);
    bladePhase.update({}, 0.5F);
    bladePhase.update(attack, 0.01F);
    if (!expect(bladePhase.dialogueLine().has_value(),
            "the flying-blade test must reach Revolte's second phase")) return false;
    advanceDialogue(bladePhase);

    bool sawThreeBladeWarnings = false;
    bool sawThreeFlyingBlades = false;
    bool movedDuringBladeWarning = false;
    bool escapedBladeLanes = false;
    std::optional<std::array<float, 3>> firstBladeRotations;
    std::optional<std::array<float, 3>> secondBladeRotations;
    std::optional<std::array<arcane::game::Vec2, 3>> firstBladeCenters;
    std::optional<std::array<arcane::game::Vec2, 3>> secondBladeCenters;
    for (int step = 0; step < 300 && !secondBladeCenters; ++step)
    {
        arcane::game::PlayerIntent movement;
        if (sawThreeBladeWarnings)
        {
            movement.moveAxis = 1.0F;
            if (!escapedBladeLanes)
            {
                movement.dashPressed = true;
                escapedBladeLanes = true;
            }
            movedDuringBladeWarning = true;
        }
        bladePhase.update(movement, 0.1F);
        const auto effects = bladePhase.spellEffects();
        std::vector<const arcane::game::SpellEffectView*> warnings;
        std::vector<const arcane::game::SpellEffectView*> blades;
        for (const auto& effect : effects)
        {
            if (effect.spellId == arcane::game::RevolteFlyingBladeTelegraphVisualId)
                warnings.push_back(&effect);
            else if (effect.spellId == arcane::game::RevolteFlyingBladeVisualId)
                blades.push_back(&effect);
        }
        if (warnings.size() == 3U)
            sawThreeBladeWarnings = std::all_of(warnings.begin(), warnings.end(),
                [&](const auto* warning) {
                    return warning->bounds.left >= phaseRequest.worldBounds.left
                        && warning->bounds.left <= phaseRequest.worldBounds.right
                        && warning->bounds.top >= 0.0F
                        && warning->bounds.top <= phaseRequest.worldBounds.groundTop;
                }) && std::abs(warnings[0]->rotationDegrees
                    - warnings[1]->rotationDegrees) > 1.0F
                && std::abs(warnings[1]->rotationDegrees
                    - warnings[2]->rotationDegrees) > 1.0F;

        if (blades.size() != 3U) continue;
        sawThreeFlyingBlades = true;
        const std::array<arcane::game::Vec2, 3> centers {{
            {blades[0]->bounds.left + blades[0]->bounds.width * 0.5F,
                blades[0]->bounds.top + blades[0]->bounds.height * 0.5F},
            {blades[1]->bounds.left + blades[1]->bounds.width * 0.5F,
                blades[1]->bounds.top + blades[1]->bounds.height * 0.5F},
            {blades[2]->bounds.left + blades[2]->bounds.width * 0.5F,
                blades[2]->bounds.top + blades[2]->bounds.height * 0.5F}}};
        const std::array<float, 3> rotations {
            blades[0]->rotationDegrees, blades[1]->rotationDegrees,
            blades[2]->rotationDegrees};
        if (!firstBladeCenters)
        {
            firstBladeCenters = centers;
            firstBladeRotations = rotations;
        }
        else if (std::abs(centers[0].x - (*firstBladeCenters)[0].x)
                    + std::abs(centers[0].y - (*firstBladeCenters)[0].y) > 1.0F
                && std::abs(centers[1].x - (*firstBladeCenters)[1].x)
                    + std::abs(centers[1].y - (*firstBladeCenters)[1].y) > 1.0F
                && std::abs(centers[2].x - (*firstBladeCenters)[2].x)
                    + std::abs(centers[2].y - (*firstBladeCenters)[2].y) > 1.0F)
        {
            secondBladeCenters = centers;
            secondBladeRotations = rotations;
        }
    }
    return expect(sawThreeBladeWarnings && movedDuringBladeWarning,
            "Revolte's three blades must reveal three distinct movement-blocking aim lines")
        && expect(sawThreeFlyingBlades && firstBladeCenters && secondBladeCenters,
            "all three warned swords must launch as visible two-dimensional projectiles")
        && expect(firstBladeRotations && secondBladeRotations
                && std::abs((*firstBladeRotations)[0] - (*secondBladeRotations)[0]) < 0.01F
                && std::abs((*firstBladeRotations)[1] - (*secondBladeRotations)[1]) < 0.01F
                && std::abs((*firstBladeRotations)[2] - (*secondBladeRotations)[2]) < 0.01F,
            "all three flying blades must keep their locked directions after launch");
}

bool newLateActEnemiesExposeFogProjectileAndFlightRules()
{
    arcane::game::CombatRequest fogRequest;
    fogRequest.playerSpawn = {20.0F, 576.0F};
    fogRequest.enemies = {{arcane::game::EnemyArchetype::Heimon, {1000.0F, 0.0F}}};
    arcane::game::CombatSession fog(fogRequest);
    const arcane::game::PlayerIntent idle;
    fog.update(idle, 1.5F);
    fog.update(idle, 0.5F);
    if (!expect(fog.enemyState().specialAttackActive,
        "Heimon must briefly expose the fog-summoning pose after the shared windup")) return false;
    fog.update(idle, 0.6F);
    fog.update(idle, 0.81F);
    const auto hidden = fog.enemyState();
    const auto fogEffects = fog.spellEffects();
    if (!expect(hidden.maximumHealth == 125 && hidden.width == 42.0F && hidden.height == 72.0F,
            "Heimon must expose the configured health and collision box")
        || !expect(hidden.concealmentProgress >= 0.99F,
            "an enemy remaining in Heimon's fog must fully conceal after 0.8 seconds")
        || !expect(std::any_of(fogEffects.begin(), fogEffects.end(), [](const auto& effect) {
                return effect.spellId == 9100U && effect.bounds.width == 640.0F
                    && effect.bounds.height == 96.0F;
            }), "Heimon's one-shot fog must persist with authoritative bounds")) return false;
    const auto fogArea = *std::find_if(fogEffects.begin(), fogEffects.end(), [](const auto& effect) {
        return effect.spellId == 9100U;
    });
    fog.update(idle, 0.25F);
    const auto movedFogEffects = fog.spellEffects();
    const auto movedFog = std::find_if(movedFogEffects.begin(), movedFogEffects.end(),
        [](const auto& effect) { return effect.spellId == 9100U; });
    if (!expect(movedFog != movedFogEffects.end()
            && std::abs(movedFog->bounds.left - fogArea.bounds.left) < 0.01F,
            "Heimon's fog must remain anchored after the caster moves")) return false;

    arcane::game::CombatRequest warriorRequest;
    warriorRequest.playerSpawn = {250.0F, 576.0F};
    warriorRequest.enemies = {{arcane::game::EnemyArchetype::DemonWarrior, {360.0F, 0.0F}}};
    arcane::game::CombatSession warrior(warriorRequest);
    warrior.update(idle, 2.5F);
    warrior.update(idle, 0.5F);
    warrior.update(idle, 0.5F);
    const auto warriorEffects = warrior.spellEffects();
    if (!expect(warrior.enemyState().maximumHealth == 150,
            "Demon Warrior must use its revised 150 HP")
        || !expect(std::any_of(warriorEffects.begin(), warriorEffects.end(),
            [](const auto& effect) { return effect.spellId == 9101U
                && effect.bounds.width == 32.0F && effect.bounds.height == 42.0F
                && std::abs(effect.duration - 0.72F) < 0.01F
                && effect.facingDirection < 0.0F; }),
            "Demon Warrior must emit its moving slash when the melee windup ends")) return false;

    arcane::game::CombatRequest birdRequest;
    birdRequest.enemies = {{arcane::game::EnemyArchetype::LargeBirdDemon, {800.0F, 0.0F}}};
    arcane::game::CombatSession bird(birdRequest);
    const auto birdState = bird.enemyState();
    return expect(birdState.maximumHealth == 100 && birdState.width == 54.0F
            && birdState.height == 42.0F,
            "Large Bird Demon must expose its configured health and collision box")
        && expect(std::abs(birdState.position.y - 454.0F) < 0.01F,
            "Large Bird Demon must start 144 pixels above the ground");
}

bool richterUsesSixSecondEarthMagicCooldown()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {300.0F, 576.0F};
    request.enemies = {{arcane::game::EnemyArchetype::Richter, {700.0F, 0.0F}}};
    request.enemyContactDamage = 0;
    arcane::game::CombatSession combat(request);
    combat.update({}, 3.01F);
    if (!expect(combat.enemyState().specialWindingUp,
        "Richter must begin the first earth-magic windup after half of the six-second cooldown"))
        return false;
    combat.update({}, 0.50F);
    const auto effects = combat.spellEffects();
    return expect(std::any_of(effects.begin(), effects.end(), [](const auto& effect) {
            return effect.spellId == 9001U && effect.bounds.width == 64.0F
                && effect.bounds.height == 108.0F && effect.duration == 4.0F;
        }), "Richter must expose the four-second summoned pillar entity");
}

bool gargoyleActivatesBeforeFiringAnAngledLaser()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {350.0F, 576.0F};
    request.enemies = {{arcane::game::EnemyArchetype::Gargoyle, {400.0F, 0.0F}}};
    request.enemyContactDamage = 0;
    arcane::game::CombatSession combat(request);
    const float groundPosition = combat.enemyState().position.y;
    combat.update({}, 0.50F);
    if (!expect(combat.enemyState().position.y < groundPosition
            && combat.enemyState().position.y > 454.0F,
        "Gargoyle must rise at 80 pixels per second after the player enters 320 pixels"))
        return false;
    combat.update({}, 1.30F);
    if (!expect(std::abs(combat.enemyState().position.y - 454.0F) < 0.01F,
        "Gargoyle must stop 144 pixels above the ground")) return false;
    combat.update({}, 2.01F);
    combat.update({}, 0.50F);
    const auto effects = combat.spellEffects();
    return expect(combat.playerState().currentHealth == 80,
            "Gargoyle laser must deal twenty damage along its locked diagonal")
        && expect(std::any_of(effects.begin(), effects.end(), [](const auto& effect) {
                return effect.spellId == 9200U && effect.bounds.width <= 240.01F
                    && effect.bounds.width > 100.0F
                    && effect.bounds.height == 18.0F
                    && std::abs(effect.rotationDegrees) > 1.0F;
            }), "Gargoyle must expose a ground-clipped 240-pixel laser visual");
}

bool threeHeadedDemonHealsToThePreviousStateThreshold()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {160.0F, 576.0F};
    request.enemies = {{arcane::game::EnemyArchetype::ThreeHeadedDemon, {205.0F, 0.0F}}};
    request.enemyContactDamage = 0;
    arcane::game::CombatSession combat(request);
    if (!expect(combat.enemyState().width == 84.0F
            && combat.enemyState().height == 64.0F,
        "Three-Headed Demon must use its revised 84 by 64 collision box")) return false;
    for (int attackIndex = 0; attackIndex < 5; ++attackIndex)
    {
        arcane::game::PlayerIntent attack;
        attack.attackPressed = true;
        combat.update(attack, 0.01F);
        combat.update({}, 0.49F);
    }
    if (!expect(combat.enemyState().currentHealth == 105,
        "Three-Headed Demon must enter its two-head state below 120 HP")) return false;
    combat.update({}, 1.01F);
    combat.update({}, 0.50F);
    combat.update({}, 0.50F);
    return expect(combat.enemyState().currentHealth == 120,
        "self-heal must restore the previous state's minimum health threshold");
}

bool swordDemonMagicHitImmediatelyTriggersFlashStep()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {160.0F, 576.0F};
    request.enemies = {{arcane::game::EnemyArchetype::SwordDemon, {300.0F, 0.0F}}};
    request.enemyContactDamage = 0;
    request.equippedSpellIds[0] = 1004U;
    arcane::game::CombatSession combat(request);
    arcane::game::PlayerIntent cast;
    cast.spellPressed[0] = true;
    combat.update(cast, 0.01F);
    const auto hit = combat.enemyState();
    combat.update({}, 144.0F / 320.0F);
    const auto afterFlash = combat.enemyState();
    return expect(hit.currentHealth == 80,
            "player spell damage must be applied before Sword Demon retaliates")
        && expect(std::abs(afterFlash.position.x - hit.position.x) >= 143.9F,
            "Sword Demon must immediately flash-step 144 pixels after player spell damage");
}

bool swordDemonSlashUsesOneInstantDamageWindow()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {200.0F, 576.0F};
    request.enemies = {{arcane::game::EnemyArchetype::SwordDemon, {280.0F, 0.0F}}};
    request.enemyContactDamage = 0;
    arcane::game::CombatSession combat(request);
    combat.update({}, 1.51F);
    if (!expect(combat.enemyState().windingUp,
        "Sword Demon slash must retain its warning windup")) return false;
    combat.update({}, 0.50F);
    if (!expect(combat.playerState().currentHealth == 80,
        "Sword Demon slash must deal twenty damage when its windup ends")) return false;
    combat.update({}, 0.60F);
    return expect(combat.playerState().currentHealth == 80,
        "Sword Demon slash attack pose must not repeat its instant damage check");
}

bool revolteRetargetsBetweenConsecutiveReadySkills()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {400.0F, 576.0F};
    request.enemySpawn = {500.0F, 544.0F};
    request.enemyArchetype = arcane::game::EnemyArchetype::Revolte;
    request.enemyMaximumHealth = 300;
    arcane::game::CombatSession combat(request);
    combat.update({}, 2.4F);
    advanceDialogue(combat);
    combat.update({}, 3.51F);
    combat.update({}, 0.30F);
    arcane::game::PlayerIntent crossBehind;
    crossBehind.moveAxis = 1.0F;
    combat.update(crossBehind, 0.60F);
    const auto firstSlash = combat.enemyStates().front();
    if (!expect(firstSlash.skillVariant == 0 && firstSlash.specialAttackActive,
            "Revolte must finish the extended first slash before selecting another skill")) {
        return false;
    }
    combat.update({}, 0.42F);
    combat.update({}, 0.01F);
    const auto state = combat.enemyStates().front();
    return expect(state.skillVariant == 1 && state.windingUp,
            "Revolte must select the other ready slash after the first attack finishes")
        && expect(state.skillEffectBounds
                && state.skillEffectBounds->left >= state.position.x + state.width,
            "Revolte must retarget the next consecutive skill after the player crosses behind");
}

bool waterMirrorBossOpensWithCopiesAndRejectsDirectDamage()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {740.0F, 576.0F};
    request.enemySpawn = {800.0F, 568.0F};
    request.enemyArchetype = arcane::game::EnemyArchetype::WaterMirrorDemon;
    request.enemyMaximumHealth = 200;
    arcane::game::CombatSession combat(request);
    const auto enemies = combat.enemyStates();
    if (!expect(enemies.size() == 3U,
            "Water Mirror Demon must open with the Stark and Fern copies")
        || !expect(enemies[0].archetype == arcane::game::EnemyArchetype::WaterMirrorDemon
                && enemies[0].currentHealth == 200 && enemies[0].width == 84.0F
                && enemies[0].height == 84.0F && enemies[0].position.x == 598.0F
                && enemies[0].position.y == 412.0F,
            "the final boss must expose its authoritative two hundred HP")
        || !expect(enemies[1].archetype == arcane::game::EnemyArchetype::StarkCopy
                && enemies[1].currentHealth == 300,
            "the Stark copy must open with three hundred HP")
        || !expect(enemies[2].archetype == arcane::game::EnemyArchetype::FernCopy
                && enemies[2].currentHealth == 180,
            "the Fern copy must open with one hundred eighty HP")
        || !expect(combat.bossIntro() && combat.bossIntro()->name == "WATER MIRROR DEMON",
            "the final boss must expose its name introduction")) return false;
    combat.update({}, 2.4F);
    advanceDialogue(combat);
    arcane::game::PlayerIntent attack;
    attack.attackPressed = true;
    combat.update(attack, 0.01F);
    return expect(combat.enemyStates()[0].currentHealth == 200,
        "the Water Mirror Demon must reject direct player damage");
}

bool starkCopyWhirlwindSlashHitsBehindOnce()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {240.0F, 576.0F};
    request.enemies = {{arcane::game::EnemyArchetype::StarkCopy, {300.0F, 0.0F}}};
    arcane::game::CombatSession combat(request);
    combat.update({}, 2.01F);
    arcane::game::PlayerIntent crossBehind;
    crossBehind.moveAxis = 1.0F;
    combat.update(crossBehind, 0.50F);
    if (!expect(combat.playerState().position.x > combat.enemyState().position.x
            + combat.enemyState().width,
            "the whirlwind regression setup must move the player behind Stark")
        || !expect(combat.playerState().currentHealth == 80,
            "Stark's whirlwind slash must hit within the rear fifty-four-pixel range"))
        return false;
    combat.update({}, 0.30F);
    return expect(combat.playerState().currentHealth == 80,
        "the whirlwind attack pose must not repeat its instant damage check");
}

bool starkCopyLandingExposesStretchedLinieEffect()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {20.0F, 576.0F};
    request.enemies = {{arcane::game::EnemyArchetype::StarkCopy, {700.0F, 0.0F}}};
    arcane::game::CombatSession combat(request);
    combat.update({}, 4.01F);
    combat.update({}, 0.80F);
    combat.update({}, 1.05F);
    const auto effects = combat.spellEffects();
    const auto shockwaveCount = std::count_if(effects.begin(), effects.end(), [](const auto& effect) {
        return effect.spellId == 9302U && effect.bounds.width == 32.0F
            && effect.bounds.height == 220.0F;
    });
    return expect(std::any_of(effects.begin(), effects.end(), [](const auto& effect) {
        return effect.spellId == 9303U && effect.bounds.width == 288.0F
            && effect.bounds.height == 72.0F;
    }), "Stark's landing must expose the stretched Linie landing-effect bounds")
        && expect(shockwaveCount == 2,
            "Stark's landing must launch one 220-pixel shockwave in each direction")
        && expect(std::any_of(effects.begin(), effects.end(), [](const auto& effect) {
                return effect.spellId == 9302U && effect.facingDirection < 0.0F;
            }) && std::any_of(effects.begin(), effects.end(), [](const auto& effect) {
                return effect.spellId == 9302U && effect.facingDirection > 0.0F;
            }), "Stark's paired shockwaves must travel left and right");
}

bool copyMageSpellsExposeLockedWarningsAndFernCapsHerVolley()
{
    arcane::game::CombatRequest fernRequest;
    fernRequest.seed = 0U;
    fernRequest.playerSpawn = {180.0F, 576.0F};
    fernRequest.playerMaximumHealth = 1000;
    fernRequest.playerCurrentHealth = 1000;
    fernRequest.enemies = {{arcane::game::EnemyArchetype::FernCopy, {700.0F, 0.0F}}};
    arcane::game::CombatSession fern(fernRequest);
    bool sawLockedWarning = false;
    bool sawExtendedFernRange = false;
    bool movedAfterLock = false;
    bool previousBeam = false;
    bool sawBeamAtCaster = false;
    bool sawBeamExtend = false;
    bool beamStayedCompact = true;
    int beamCount = 0;
    std::optional<float> lockedRotation;
    for (int step = 0; step < 55; ++step)
    {
        arcane::game::PlayerIntent movement;
        if (lockedRotation && beamCount == 0)
        {
            movement.moveAxis = -1.0F;
            movedAfterLock = true;
        }
        fern.update(movement, 0.1F);
        const auto effects = fern.spellEffects();
        const auto warning = std::find_if(effects.begin(), effects.end(), [](const auto& effect) {
            return effect.spellId == arcane::game::CopyBeamTelegraphVisualId;
        });
        if (warning != effects.end() && beamCount == 0)
        {
            if (warning->bounds.width == 1600.0F) sawExtendedFernRange = true;
            if (!lockedRotation) lockedRotation = warning->rotationDegrees;
            else if (movedAfterLock
                && std::abs(warning->rotationDegrees - *lockedRotation) < 0.01F)
                sawLockedWarning = true;
        }
        const auto beamEffect = std::find_if(effects.begin(), effects.end(), [](const auto& effect) {
            return effect.spellId == arcane::game::FrierenCopyBeamVisualId;
        });
        const bool beam = beamEffect != effects.end();
        if (beam)
        {
            if (beamEffect->bounds.width < 2.0F) sawBeamAtCaster = true;
            if (sawBeamAtCaster && beamEffect->bounds.width > 120.0F) sawBeamExtend = true;
            if (beamEffect->bounds.width > 144.1F) beamStayedCompact = false;
        }
        if (beam && !previousBeam) ++beamCount;
        previousBeam = beam;
    }
    if (!expect(sawLockedWarning,
            "Fern must expose a fixed aim line that does not follow movement during windup")
        || !expect(sawExtendedFernRange,
            "Fern's warning must extend along the full off-screen beam path")
        || !expect(beamCount >= 1 && beamCount <= 3,
            "Fern's deterministic opening volley must stop after at most three beams")
        || !expect(sawBeamAtCaster && sawBeamExtend,
            "Copy beams must visibly propagate outward from the caster")
        || !expect(beamStayedCompact,
            "travelling copy beams must retain their compact 144-pixel visual length"))
        return false;

    arcane::game::CombatRequest travellingBeamRequest;
    travellingBeamRequest.seed = 0U;
    travellingBeamRequest.playerSpawn = {20.0F, 576.0F};
    travellingBeamRequest.playerMaximumHealth = 100;
    travellingBeamRequest.playerCurrentHealth = 100;
    travellingBeamRequest.enemies = {
        {arcane::game::EnemyArchetype::FernCopy, {1100.0F, 0.0F}}};
    arcane::game::CombatSession travellingBeam(travellingBeamRequest);
    bool foundTravellingBeam = false;
    for (int step = 0; step < 120 && !foundTravellingBeam; ++step)
    {
        travellingBeam.update({}, 0.05F);
        const auto effects = travellingBeam.spellEffects();
        foundTravellingBeam = std::any_of(effects.begin(), effects.end(), [](const auto& effect) {
            return effect.spellId == arcane::game::FrierenCopyBeamVisualId;
        });
    }
    if (!expect(foundTravellingBeam && travellingBeam.playerState().currentHealth == 100,
            "Copy beam damage must not arrive before its visible leading edge")) return false;
    travellingBeam.update({}, 0.30F);
    if (!expect(travellingBeam.playerState().currentHealth == 85,
            "Copy beam must keep travelling beyond the former range and hit along its swept path"))
        return false;

    arcane::game::CombatRequest frierenRequest;
    frierenRequest.playerSpawn = {180.0F, 576.0F};
    frierenRequest.playerMaximumHealth = 1000;
    frierenRequest.playerCurrentHealth = 1000;
    frierenRequest.enemies = {{arcane::game::EnemyArchetype::FrierenCopy, {760.0F, 0.0F}}};
    arcane::game::CombatSession frieren(frierenRequest);
    bool sawBeamWarning = false;
    bool sawStrengthenedFrierenBeam = false;
    bool sawLightningWarning = false;
    bool sawRaisedLightning = false;
    bool sawGroundFireWarning = false;
    for (int step = 0; step < 120
        && !(sawBeamWarning && sawLightningWarning && sawGroundFireWarning); ++step)
    {
        frieren.update({}, 0.1F);
        const auto effects = frieren.spellEffects();
        sawBeamWarning = sawBeamWarning || std::any_of(effects.begin(), effects.end(),
            [](const auto& effect) {
                return effect.spellId == arcane::game::CopyBeamTelegraphVisualId;
            });
        sawStrengthenedFrierenBeam = sawStrengthenedFrierenBeam
            || std::any_of(effects.begin(), effects.end(), [](const auto& effect) {
                return effect.spellId == arcane::game::CopyBeamTelegraphVisualId
                    && effect.bounds.width == 1600.0F && effect.duration == 0.55F;
            });
        sawLightningWarning = sawLightningWarning || std::any_of(effects.begin(), effects.end(),
            [](const auto& effect) {
                return effect.spellId == arcane::game::FrierenCopyLightningVisualId;
            });
        sawRaisedLightning = sawRaisedLightning || std::any_of(effects.begin(), effects.end(),
            [](const auto& effect) {
                return effect.spellId == arcane::game::FrierenCopyLightningVisualId
                    && effect.bounds.top == 400.0F && effect.bounds.height == 240.0F;
            });
        sawGroundFireWarning = sawGroundFireWarning || std::any_of(effects.begin(), effects.end(),
            [](const auto& effect) {
                return effect.spellId
                    == arcane::game::FrierenCopyGroundFireTelegraphVisualId;
            });
    }
    return expect(sawBeamWarning,
            "Frieren's direct beam must expose a locked pre-fire aim line")
        && expect(sawStrengthenedFrierenBeam,
            "Frieren's direct beam must use the strengthened windup and range")
        && expect(sawLightningWarning,
            "Frieren's lightning must retain a visible locked strike warning")
        && expect(sawRaisedLightning,
            "Frieren's lightning must extend above the raised final-boss platforms")
        && expect(sawGroundFireWarning,
            "Frieren's full-floor fire must warn the ground before ignition");
}

bool waterMirrorBossAdvancesThroughBothCopyPhases()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {660.0F, 576.0F};
    request.enemySpawn = {1100.0F, 568.0F};
    request.enemyArchetype = arcane::game::EnemyArchetype::WaterMirrorDemon;
    request.enemyMaximumHealth = 2;
    request.playerMaximumHealth = 10000;
    request.playerCurrentHealth = 10000;
    arcane::game::CombatSession combat(request);
    combat.update({}, 2.4F);
    advanceDialogue(combat);

    const auto defeat = [&](const arcane::game::EnemyArchetype archetype) {
        for (int step = 0; step < 800; ++step)
        {
            const auto enemies = combat.enemyStates();
            const auto target = std::find_if(enemies.begin(), enemies.end(), [&](const auto& enemy) {
                return enemy.archetype == archetype && enemy.alive;
            });
            if (target == enemies.end()) return true;
            const auto player = combat.playerState();
            const float playerCenter = player.position.x + 21.0F;
            const float targetCenter = target->position.x + target->width * 0.5F;
            arcane::game::PlayerIntent intent;
            if (std::abs(targetCenter - playerCenter) > 72.0F)
                intent.moveAxis = targetCenter > playerCenter ? 1.0F : -1.0F;
            else
            {
                intent.moveAxis = targetCenter > playerCenter ? 1.0F : -1.0F;
                intent.attackPressed = true;
                if (archetype == arcane::game::EnemyArchetype::FrierenCopy)
                    intent.jumpPressed = true;
            }
            combat.update(intent, 0.05F);
        }
        return false;
    };

    if (!expect(defeat(arcane::game::EnemyArchetype::StarkCopy)
            && defeat(arcane::game::EnemyArchetype::FernCopy),
        "defeating both opening copies must be possible in the deterministic preview")) return false;
    if (!expect(combat.dialogueLine().has_value(),
            "defeating both opening copies must enter the second-phase dialogue")) return false;
    const auto secondPhase = combat.enemyStates();
    const auto mirror = std::find_if(secondPhase.begin(), secondPhase.end(), [](const auto& enemy) {
        return enemy.archetype == arcane::game::EnemyArchetype::WaterMirrorDemon;
    });
    const auto frierenCopy = std::find_if(secondPhase.begin(), secondPhase.end(), [](const auto& enemy) {
        return enemy.archetype == arcane::game::EnemyArchetype::FrierenCopy;
    });
    if (!expect(mirror != secondPhase.end() && mirror->currentHealth == 1,
            "the phase transition must halve the Water Mirror Demon's health")
        || !expect(frierenCopy != secondPhase.end() && frierenCopy->position.y == 408.0F,
            "the Frieren copy must hover 160 pixels above the arena floor")) return false;
    advanceDialogue(combat);
    if (!expect(defeat(arcane::game::EnemyArchetype::FrierenCopy),
            "the Frieren copy must be the second-phase target")) return false;
    if (!expect(combat.dialogueLine().has_value(),
            "defeating the Frieren copy must enter the final dialogue")) return false;
    advanceDialogue(combat);
    return expect(combat.result() && combat.result()->outcome == arcane::game::CombatOutcome::Victory,
        "the final dialogue must resolve the Water Mirror Demon victory");
}

bool defeatingFinalEnemyClearsItsPersistentMagicEffects()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {500.0F, 576.0F};
    request.enemySpawn = {700.0F, 568.0F};
    request.enemyArchetype = arcane::game::EnemyArchetype::Richter;
    request.enemyMaximumHealth = 1;
    request.enemyContactDamage = 0;
    request.equippedSpellIds[0] = 1004U;
    arcane::game::CombatSession combat(request);
    combat.update({}, 3.01F);
    arcane::game::PlayerIntent evadePillar;
    evadePillar.moveAxis = 1.0F;
    combat.update(evadePillar, 0.50F);
    const auto pillarEffects = combat.spellEffects();
    if (!expect(std::any_of(pillarEffects.begin(), pillarEffects.end(),
            [](const auto& effect) { return effect.spellId == 9001U; }),
            "Richter must have a persistent pillar before the cleanup check")) return false;

    arcane::game::PlayerIntent kill;
    kill.spellPressed[0] = true;
    combat.update(kill, 0.01F);
    const auto effects = combat.spellEffects();
    const auto enemy = combat.enemyState();
    return expect(combat.result() && combat.result()->outcome
                == arcane::game::CombatOutcome::Victory,
            "the cleanup scenario must defeat its final enemy")
        && expect(effects.empty(),
            "ending combat must remove both enemy and player magic visuals")
        && expect(!enemy.specialWindingUp && !enemy.specialAttackActive,
            "defeated enemies must clear frozen casting state");
}

bool tacticalSpawnsPreserveElevatedLanesAndFlyingAltitude()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {120.0F, 576.0F};
    request.enemyContactDamage = 0;
    request.enemies = {
        {arcane::game::EnemyArchetype::Richter, {620.0F, 480.0F},
            arcane::game::WorldBounds {500.0F, 800.0F, 480.0F}, false},
        {arcane::game::EnemyArchetype::Laufen, {860.0F, 640.0F}},
        {arcane::game::EnemyArchetype::BirdDemon, {700.0F, 360.0F}, std::nullopt, true}
    };
    arcane::game::CombatSession combat(request);
    auto enemies = combat.enemyStates();
    if (!expect(enemies[0].position.y == 408.0F,
            "elevated caster feet must align to the platform surface")
        || !expect(enemies[1].position.y == 576.0F,
            "ground melee enemies must remain on the base floor")
        || !expect(enemies[2].position.y == 344.0F,
            "flying placement must use the map-provided aerial anchor"))
        return false;

    for (int step = 0; step < 240; ++step) combat.update({}, 1.0F / 60.0F);
    enemies = combat.enemyStates();
    return expect(enemies[0].position.x >= 500.0F
            && enemies[0].position.x + enemies[0].width <= 800.0F,
            "elevated caster movement and knockback must stay inside its platform lane")
        && expect(enemies[0].position.y == 408.0F,
            "elevated caster must not drift vertically while pursuing the player");
}
}

int main()
{
    const bool passed = healthClampsDamageAndHealing()
        && damageResolverReportsAppliedAndLethalDamage()
        && damageResolverDeduplicatesPerSource()
        && damageResolverCentralizesModifiersAndBlocking()
        && oneAttackHitsOnlyOnce()
        && playerViewSequencesOnlyAdvanceForSuccessfulActions()
        && postHitInvulnerabilityBlocksOverlappingDamage()
        && repeatedAttacksProduceVictoryResult()
        && enemyAttackHasWindupAndHitsOnce()
        && zeroHealthProducesDefeatResult()
        && enemyChaseIsDeltaTimeStableAndStopsWhenDead()
        && hitStunSuppressesInputAndAppliesKnockback()
        && spellSlotsHaveIndependentCooldowns()
        && spellCooldownIsDeltaTimeStable()
        && regularSpellCatalogHasDefinitionsAndAuthoritativeRanges()
        && innateActionsExposeSpellEffectPlaceholders()
        && collectibleSpellsEmitEffectPlaceholders()
        && everySpellEffectKeepsItsCompleteVisualTimeline()
        && secondarySpellEffectsKeepTheirCompleteVisualTimelines()
        && statefulSpellVisualsUseTheirMatchingTransitionPhases()
        && flameBurstUsesOneShotVisualAndSeparateBurningField()
        && newCombatSpellsApplyTheirCoreInteractions()
        && lightningStaffEnhancesExactlyThreeBasicAttacks()
        && frostLanceCanFreezeAfterItsOwnBaseCooldown()
        && ultimateSlotUsesSharedCooldownAndRejectsRegularSpells()
        && bossRewardDescriptionsExposeImplementedEffects()
        && selectedBossSpellCatalogIsDefinedAndOmitsRejectedChoices()
        && goddessSpearsDealsThreeHitsAndHealsOnFullHit()
        && delayedAndSustainedBossSpellsResolveOverTime()
        && mirrorArrayRepeatsTheNextRegularDamageSpell()
        && directBossSpellsApplyTheirDistinctRules()
        && severingMagicGainsExecuteDamageBelowTwentyPercent()
        && combatSessionCastsUltimateWithDedicatedInput()
        && brokenHighSpeedFormulaDamagesCrossedEnemiesAndReducesCooldown()
        && multiEnemyEncounterExposesConfiguredContentAndStartsOnCooldown()
        && chestMimicTriggersFromTheFrontAfterHalfCooldown()
        && enemyChasePreservesConfiguredBodyGap()
        && frostWolfClawUsesAnInstantHitWindow()
        && chestMimicBiteStunsWithoutKnockback()
        && linieUsesGroundedMediumImpactArea()
        && displacementSkillTriggersWhenPlayerEdgeEntersExtendedRange()
        && auraOpensWithTwoHeadlessKnights()
        && auraDominationUsesTelegraphedFrontRange()
        && auraSoulGuillotineLocksPositionAndCanBeSidestepped()
        && defeatingAuraClearsHerArmyAndStartsDefeatDialogue()
        && revolteLocksAtFiveAndHealsForSecondPhase()
        && revolteParryActivatesWithoutWindup()
        && revolteRetargetsBetweenConsecutiveReadySkills()
        && revolteWavesCrossTheArenaAndSecondPhaseAddsCrossExecution()
        && redMirrorDragonBreathRespectsPostHitInvulnerability()
        && enemyDirectionLocksWhenWindupBegins()
        && secondActEnemiesExposeConfiguredContent()
        && qualKillingMagicUsesRestrictedEffectHeight()
        && lugnerBloodMagicUsesRaisedSpriteHeightArea()
        && chaosFlowerSleepReducesOutgoingDamageWithoutWindup()
        && lateSecondActEnemiesExposeConfiguredContent()
        && denkenExposesCremationPoseWhenTornadoEvolves()
        && richterUsesSixSecondEarthMagicCooldown()
        && laufenTeleportsBehindAndImmediatelySideKicks()
        && swordRelicsModifyCollisionDamage()
        && expandedSpellAndRelicCatalogIsComplete()
        && newSpellsApplyTargetingShieldWindAndGravity()
        && combatRelicsModifyCooldownBloodUltimateAndGold()
        && newLateActEnemiesExposeFogProjectileAndFlightRules()
        && gargoyleActivatesBeforeFiringAnAngledLaser()
        && threeHeadedDemonHealsToThePreviousStateThreshold()
        && swordDemonMagicHitImmediatelyTriggersFlashStep()
        && swordDemonSlashUsesOneInstantDamageWindow()
        && waterMirrorBossOpensWithCopiesAndRejectsDirectDamage()
        && starkCopyWhirlwindSlashHitsBehindOnce()
        && starkCopyLandingExposesStretchedLinieEffect()
        && copyMageSpellsExposeLockedWarningsAndFernCapsHerVolley()
        && waterMirrorBossAdvancesThroughBothCopyPhases()
        && defeatingFinalEnemyClearsItsPersistentMagicEffects()
        && tacticalSpawnsPreserveElevatedLanesAndFlyingAltitude()
        && combatSessionAppliesEquippedSpellDamage()
        && spellDamageTargetsEveryEnemyInsideTheAuthoritativeArea()
        && bloodMagicUsesCurrentHealth()
        && flowerFieldHealsAndMoonFlowerAutoCasts()
        && goddessBlessingBoostsDamageAndPreventsControl()
        && darkDragonHornAppliesRiskAndReward();

    if (!passed)
    {
        return 1;
    }

    std::cout << "All combat session tests passed.\n";
    return 0;
}
