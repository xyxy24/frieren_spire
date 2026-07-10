#include "game/combat/CombatSession.hpp"
#include "game/combat/Health.hpp"
#include "game/spells/SpellSystem.hpp"
#include "game/ai/EnemyController.hpp"

#include <iostream>
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

    return expect(healthAfterFirstHit == 75, "a basic attack should deal 25 damage")
        && expect(combat.enemyState().currentHealth == 75, "an active attack must hit an enemy only once");
}

bool fourAttacksProduceVictoryResult()
{
    arcane::game::CombatRequest request = adjacentEnemyRequest();
    request.encounterId = 42;
    request.goldReward = 17;
    request.enemyContactDamage = 0;
    arcane::game::CombatSession combat(request);

    for (int attackNumber = 0; attackNumber < 4; ++attackNumber)
    {
        arcane::game::PlayerIntent attack;
        attack.attackPressed = true;
        combat.update(attack, 0.01F);

        if (attackNumber < 3)
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
    arcane::game::CombatSession combat(request);

    combat.update(arcane::game::PlayerIntent {}, 0.01F);
    combat.update(arcane::game::PlayerIntent {}, 0.40F);
    if (!expect(combat.playerState().currentHealth == 100,
        "enemy windup must not deal early damage")) return false;
    combat.update(arcane::game::PlayerIntent {}, 0.06F);
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

bool enemyChaseIsDeltaTimeStableAndStopsWhenDead()
{
    const arcane::game::Aabb player {160.0F, 576.0F, 42.0F, 64.0F};
    const arcane::game::WorldBounds bounds {0.0F, 1280.0F, 640.0F};
    arcane::game::ai::EnemyController oneStep({800.0F, 576.0F});
    arcane::game::ai::EnemyController manySteps({800.0F, 576.0F});
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
    std::array<std::optional<std::uint32_t>, 3> equipped {1001U, 1002U, std::nullopt};
    arcane::game::spells::SpellSystem spells(equipped);
    const arcane::game::Aabb target {260.0F, 576.0F, 48.0F, 64.0F};
    const auto first = spells.tryCast(0U, {160.0F, 576.0F}, 1.0F, target);
    const auto blocked = spells.tryCast(0U, {160.0F, 576.0F}, 1.0F, target);
    const auto otherSlot = spells.tryCast(1U, {160.0F, 576.0F}, 1.0F, target);
    const auto emptySlot = spells.tryCast(2U, {160.0F, 576.0F}, 1.0F, target);
    spells.update(1.20F);
    const auto readyAgain = spells.tryCast(0U, {160.0F, 576.0F}, 1.0F, target);
    return expect(first.cast && first.hit && first.damage == 25, "equipped spell must cast and hit")
        && expect(!blocked.cast, "spell must be blocked during its own cooldown")
        && expect(otherSlot.cast, "one slot cooldown must not block another slot")
        && expect(!emptySlot.cast, "empty spell slot must not cast")
        && expect(readyAgain.cast, "spell must become available after its cooldown");
}

bool spellCooldownIsDeltaTimeStable()
{
    std::array<std::optional<std::uint32_t>, 3> equipped {1001U, std::nullopt, std::nullopt};
    arcane::game::spells::SpellSystem oneStep(equipped);
    arcane::game::spells::SpellSystem manySteps(equipped);
    const arcane::game::Aabb target {260.0F, 576.0F, 48.0F, 64.0F};
    static_cast<void>(oneStep.tryCast(0U, {160.0F, 576.0F}, 1.0F, target));
    static_cast<void>(manySteps.tryCast(0U, {160.0F, 576.0F}, 1.0F, target));
    oneStep.update(1.20F);
    for (int frame = 0; frame < 12; ++frame) manySteps.update(0.10F);
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
    request.equippedSpellIds[0] = 1001U;
    arcane::game::CombatSession combat(request);
    arcane::game::PlayerIntent cast;
    cast.spellPressed[0] = true;
    combat.update(cast, 0.01F);
    return expect(combat.result().has_value()
            && combat.result()->outcome == arcane::game::CombatOutcome::Victory,
        "spell damage must use combat health and produce victory");
}
}

int main()
{
    const bool passed = healthClampsDamageAndHealing()
        && oneAttackHitsOnlyOnce()
        && fourAttacksProduceVictoryResult()
        && enemyAttackHasWindupAndHitsOnce()
        && zeroHealthProducesDefeatResult()
        && enemyChaseIsDeltaTimeStableAndStopsWhenDead()
        && hitStunSuppressesInputAndAppliesKnockback()
        && spellSlotsHaveIndependentCooldowns()
        && spellCooldownIsDeltaTimeStable()
        && combatSessionAppliesEquippedSpellDamage();

    if (!passed)
    {
        return 1;
    }

    std::cout << "All combat session tests passed.\n";
    return 0;
}
