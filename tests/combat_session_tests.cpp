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
    return expect(combat.playerState().currentHealth == 90, "blood magic must cost 10% current HP")
        && expect(combat.enemyState().currentHealth == 55,
            "blood magic damage must equal 50% of HP remaining after its cost");
}

bool flowerFieldHealsAndMoonFlowerAutoCasts()
{
    auto request = adjacentEnemyRequest();
    request.playerCurrentHealth = 50;
    request.enemySpawn = {500.0F, 576.0F};
    request.relicIds = {arcane::game::relics::MoonGrassFlowerId};
    request.enemyContactDamage = 0;
    arcane::game::CombatSession combat(request);
    if (!expect(combat.playerState().flowerFieldRemaining == 4.0F,
        "moon grass flower must cast the field at combat start")) return false;
    combat.update(arcane::game::PlayerIntent {}, 1.0F);
    return expect(combat.playerState().currentHealth == 55, "flower field must heal 5 HP each second")
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
    return expect(combat.playerState().blessingRemaining > 7.0F, "blessing must last eight seconds")
        && expect(!combat.playerState().stunned, "blessing must prevent control effects")
        && expect(combat.enemyState().currentHealth == 76, "blessing must increase outgoing damage by 20%");
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
    spells.update(0.80F);
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
    oneStep.update(0.80F);
    for (int frame = 0; frame < 8; ++frame) manySteps.update(0.10F);
    return expect(oneStep.tryCast(0U, {160.0F, 576.0F}, 1.0F, target).cast,
            "single-step cooldown must finish")
        && expect(manySteps.tryCast(0U, {160.0F, 576.0F}, 1.0F, target).cast,
            "partitioned delta time must finish the same cooldown");
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
        && expect(initial[1].currentHealth == 60 && initial[1].width == 42.0F && initial[1].height == 58.0F,
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
    if (!expect(combat.enemyState().currentHealth == 100, "Linie must have one hundred HP")) return false;
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
        && ultimateSlotUsesSharedCooldownAndRejectsRegularSpells()
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
        && combatSessionAppliesEquippedSpellDamage()
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
