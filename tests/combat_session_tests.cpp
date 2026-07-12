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
    if (!expect(arcane::game::spells::findDefinition(1012U) == nullptr
            && arcane::game::spells::findDefinition(1013U) == nullptr
            && arcane::game::spells::findDefinition(1014U) == nullptr
            && arcane::game::spells::findDefinition(1015U) == nullptr,
        "retired spells 1012 through 1015 must not remain in the catalog")) return false;

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
            "innate guard and dash must not occupy learned spell slots");
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
    if (!expect(dashEffects.size() == 1U && dashEffects.front().spellId == 1010U
            && dashEffects.front().bounds.width == arcane::game::PlayerController::DashDistance,
        "K dash must expose its 150px effect placeholder")) return false;

    arcane::game::CombatSession guardCombat(request);
    arcane::game::PlayerIntent guard;
    guard.guardPressed = true;
    guardCombat.update(guard, 0.01F);
    const auto guardEffects = guardCombat.spellEffects();
    return expect(guardEffects.size() == 1U && guardEffects.front().spellId == 1007U,
        "L guard must expose its self effect placeholder");
}

bool collectibleSpellsEmitEffectPlaceholders()
{
    constexpr std::array ids {
        1001U, 1002U, 1003U, 1004U, 1005U, 1006U, 1008U, 1009U, 1011U, 1016U,
        1017U, 1018U, 1019U, 1020U, 1021U, 1022U, 1023U, 1024U, 1025U, 1026U
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
    return expect(combat.enemyState().currentHealth == 22,
        "Lightning Staff must add three seven-damage hits and one twelve-damage burst");
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

bool guardBlocksDamageAndUsesItsOwnCooldown()
{
    auto request = adjacentEnemyRequest();
    request.enemyArchetype = arcane::game::EnemyArchetype::ChestMimic;
    request.enemySpawn = {180.0F, 576.0F};
    request.enemyAttackDamage = 0;
    request.enemyContactDamage = 10;
    arcane::game::CombatSession combat(request);
    arcane::game::PlayerIntent guard;
    guard.guardPressed = true;
    combat.update(guard, 0.01F);
    const auto guarded = combat.playerState();
    if (!expect(guarded.currentHealth == 100 && guarded.guarding,
            "innate guard must block overlapping enemy contact damage")
        || !expect(guarded.guardCooldownRemaining == arcane::game::CombatSession::GuardCooldownSeconds,
            "guard must start its independent two-second cooldown")) return false;

    combat.update({}, 1.01F);
    return expect(combat.playerState().currentHealth == 90,
        "contact damage must apply again after the guard window expires");
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
    combat.update({}, 0.80F);
    const auto area = combat.enemyState().skillEffectBounds;
    return expect(area.has_value(), "Linie landing must expose its impact rectangle")
        && expect(area->top == 616.0F && area->height == 24.0F,
            "Linie impact rectangle must sit on the ground and be twenty-four pixels high")
        && expect(area->width == 186.0F,
            "Linie impact must extend seventy-two pixels on both sides");
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

bool auraDominationUsesNinetySixPixelRange()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {160.0F, 576.0F};
    request.enemyArchetype = arcane::game::EnemyArchetype::Aura;
    request.enemySpawn = {257.0F, 576.0F};
    request.enemyMaximumHealth = 225;
    arcane::game::CombatSession combat(request);
    combat.update({}, 5.01F);
    const auto winding = combat.enemyState();
    if (!expect(winding.windingUp && !winding.skillEffectBounds.has_value(),
        "Aura domination must show windup without drawing a range rectangle")) return false;
    combat.update({}, 0.50F);
    combat.update({}, 0.01F);
    return expect(combat.playerState().stunRemaining > 1.4F,
        "domination must reach and stun a target inside ninety-six pixels");
}

bool redMirrorDragonBreathTicksThreeTimes()
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
    return expect(combat.playerState().currentHealth == 55,
        "dragon flame breath must deal fifteen damage every half second for one and a half seconds");
}

bool enemyDirectionLocksWhenWindupBegins()
{
    arcane::game::ai::EnemyConfig config;
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

bool chaosFlowerSleepAppliesStackBasedStun()
{
    arcane::game::CombatRequest request;
    request.playerSpawn = {160.0F, 576.0F};
    request.enemies = {{arcane::game::EnemyArchetype::ChaosFlower, {223.0F, 0.0F}}};
    arcane::game::CombatSession combat(request);
    combat.update({}, 3.51F);
    combat.update({}, 0.50F);
    return expect(combat.playerState().stunRemaining > 0.49F,
        "first sleep stack must stun for half a second");
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
}

int main()
{
    const bool passed = healthClampsDamageAndHealing()
        && damageResolverReportsAppliedAndLethalDamage()
        && damageResolverDeduplicatesPerSource()
        && damageResolverCentralizesModifiersAndBlocking()
        && oneAttackHitsOnlyOnce()
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
        && newCombatSpellsApplyTheirCoreInteractions()
        && lightningStaffEnhancesExactlyThreeBasicAttacks()
        && ultimateSlotUsesSharedCooldownAndRejectsRegularSpells()
        && bossRewardDescriptionsExposeImplementedEffects()
        && selectedBossSpellCatalogIsDefinedAndOmitsRejectedChoices()
        && goddessSpearsDealsThreeHitsAndHealsOnFullHit()
        && delayedAndSustainedBossSpellsResolveOverTime()
        && mirrorArrayRepeatsTheNextRegularDamageSpell()
        && directBossSpellsApplyTheirDistinctRules()
        && severingMagicGainsExecuteDamageBelowTwentyPercent()
        && combatSessionCastsUltimateWithDedicatedInput()
        && guardBlocksDamageAndUsesItsOwnCooldown()
        && multiEnemyEncounterExposesConfiguredContentAndStartsOnCooldown()
        && chestMimicTriggersFromTheFrontAfterHalfCooldown()
        && enemyChasePreservesConfiguredBodyGap()
        && chestMimicBiteStunsWithoutKnockback()
        && linieUsesGroundedMediumImpactArea()
        && auraOpensWithTwoHeadlessKnights()
        && auraDominationUsesNinetySixPixelRange()
        && redMirrorDragonBreathTicksThreeTimes()
        && enemyDirectionLocksWhenWindupBegins()
        && secondActEnemiesExposeConfiguredContent()
        && chaosFlowerSleepAppliesStackBasedStun()
        && swordRelicsModifyCollisionDamage()
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
