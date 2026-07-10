#include "app/run/AppFlowController.hpp"

#include <iostream>
#include <optional>
#include <string_view>

namespace
{
bool expect(const bool condition, const std::string_view message)
{
    if (!condition) std::cerr << "FAILED: " << message << '\n';
    return condition;
}

arcane::app::TowerSessionConfig fastFlowConfig()
{
    arcane::app::TowerSessionConfig config;
    config.playerSpawn = {160.0F, 576.0F};
    config.enemySpawn = {210.0F, 576.0F};
    config.staircaseBounds = {150.0F, 550.0F, 100.0F, 90.0F};
    config.normalEnemyHealth = 15;
    config.bossEnemyHealth = 15;
    config.enableSpecialFloors = false;
    return config;
}

void attackOnce(arcane::app::TowerSession& tower)
{
    arcane::game::PlayerIntent intent;
    intent.attackPressed = true;
    tower.update(intent, 0.01F);
}

void chooseLeftReward(arcane::app::TowerSession& tower)
{
    arcane::game::PlayerIntent intent;
    intent.spellPressed[0] = true;
    tower.update(intent, 0.01F);
}

void interact(arcane::app::TowerSession& tower)
{
    arcane::game::PlayerIntent intent;
    intent.interactPressed = true;
    tower.update(intent, 0.01F);
}

void collectNearbyLoot(arcane::app::TowerSession& tower)
{
    arcane::game::PlayerIntent moveRight;
    moveRight.moveAxis = 1.0F;
    tower.update(moveRight, 0.10F);
    interact(tower);
}

bool combatRewardLoadoutAndStairsFormOneFlow()
{
    arcane::app::TowerSession tower(0xC0FFEEU, fastFlowConfig());
    attackOnce(tower);
    if (!expect(tower.run().phase() == arcane::game::run::RunPhase::LootPending,
            "enemy defeat must create a loot drop")
        || !expect(tower.lootDropBounds().has_value(), "loot drop must exist at enemy death")) return false;
    collectNearbyLoot(tower);
    if (!expect(tower.run().phase() == arcane::game::run::RunPhase::Reward,
            "loot interaction must enter the reward phase")
        || !expect(tower.rewardCandidates().has_value(), "reward phase must expose three candidates")) return false;

    const auto selectedReward = (*tower.rewardCandidates())[0];
    chooseLeftReward(tower);
    if (!expect(tower.run().phase() == arcane::game::run::RunPhase::FloorComplete,
            "choosing a reward must complete the floor")
        || !expect(tower.run().player().learnedSpells.size() == 1U
                && tower.run().player().learnedSpells[0] == selectedReward,
            "chosen reward must enter the learned spell list")) return false;

    arcane::game::PlayerIntent openLoadout;
    openLoadout.toggleLoadoutPressed = true;
    tower.update(openLoadout, 0.01F);
    arcane::game::PlayerIntent equipFirstSlot;
    equipFirstSlot.spellPressed[0] = true;
    tower.update(equipFirstSlot, 0.01F);
    tower.update(openLoadout, 0.01F);
    if (!expect(tower.run().player().equippedSpells[0] == selectedReward,
            "loadout input must equip a learned spell")) return false;

    interact(tower);
    return expect(tower.run().context().floorIndex == 1U, "stair interaction must advance exactly one floor")
        && expect(tower.run().phase() == arcane::game::run::RunPhase::InEncounter,
            "the next combat must start after climbing")
        && expect(tower.run().player().gold == 10, "victory gold must be awarded once");
}

bool contactDefeatEndsTheTowerFlow()
{
    auto config = fastFlowConfig();
    config.enemySpawn = config.playerSpawn;
    config.normalEnemyHealth = 1000;
    arcane::app::TowerSession tower(7U, config);
    for (int step = 0; step < 1000 && tower.run().phase() == arcane::game::run::RunPhase::InEncounter; ++step)
        tower.update(arcane::game::PlayerIntent {}, 0.10F);
    return expect(tower.run().phase() == arcane::game::run::RunPhase::Defeat,
            "lethal contact damage must enter defeat")
        && expect(tower.run().player().currentHp == 0, "defeat must commit zero run HP");
}

bool thirdBossEndsInVictory()
{
    auto config = fastFlowConfig();
    config.floorsPerBoss = 1U;
    arcane::app::TowerSession tower(99U, config);
    for (int boss = 0; boss < 3; ++boss)
    {
        attackOnce(tower);
        if (!expect(tower.run().phase() == arcane::game::run::RunPhase::LootPending,
                "each boss defeat must create loot")) return false;
        collectNearbyLoot(tower);
        if (!expect(tower.run().phase() == arcane::game::run::RunPhase::Reward,
                "each boss loot interaction must enter reward")) return false;
        chooseLeftReward(tower);
        interact(tower);
    }
    return expect(tower.run().context().bossesDefeated == 3U, "exactly three bosses must be counted")
        && expect(tower.run().phase() == arcane::game::run::RunPhase::Victory,
            "climbing after the third boss must enter victory");
}

std::optional<arcane::game::run::Seed> seedForFirstFloorType(
    const arcane::game::run::FloorType wanted, const arcane::app::TowerSessionConfig& config)
{
    for (arcane::game::run::Seed seed = 1U; seed < 1000U; ++seed)
    {
        arcane::app::TowerSession tower(seed, config);
        if (tower.currentFloorType() == wanted) return seed;
    }
    return std::nullopt;
}

bool specialFloorsAdvanceWithoutCombat()
{
    arcane::app::TowerSessionConfig config;
    config.npcBounds = {150.0F, 550.0F, 100.0F, 90.0F};
    config.staircaseBounds = {300.0F, 550.0F, 100.0F, 90.0F};
    config.initialPlayer.gold = 30;
    const auto eventSeed = seedForFirstFloorType(arcane::game::run::FloorType::Event, config);
    const auto merchantSeed = seedForFirstFloorType(arcane::game::run::FloorType::Merchant, config);
    if (!expect(eventSeed.has_value() && merchantSeed.has_value(),
        "fixed seeds must be discoverable for both special floor types")) return false;

    arcane::app::TowerSession eventTower(*eventSeed, config);
    interact(eventTower);
    if (!expect(eventTower.specialPanelOpen()
            && eventTower.eventFloorState() == arcane::app::EventFloorState::Choosing,
        "event must wait for NPC interaction before showing choices")) return false;
    chooseLeftReward(eventTower);
    if (!expect(eventTower.eventFloorState() == arcane::app::EventFloorState::Result,
            "event choice must persist a result state")
        || !expect(eventTower.run().player().maxHp == 130 && eventTower.run().player().gold == 30,
            "dessert choice must increase maximum HP by thirty without changing gold")) return false;
    interact(eventTower);
    interact(eventTower);
    if (!expect(eventTower.specialPanelOpen()
            && eventTower.eventFloorState() == arcane::app::EventFloorState::Result
            && eventTower.run().player().maxHp == 130 && eventTower.run().player().gold == 30,
        "reopening an event NPC must show its result without applying it twice")) return false;
    interact(eventTower);
    arcane::game::PlayerIntent moveRight;
    moveRight.moveAxis = 1.0F;
    eventTower.update(moveRight, 0.5F);
    interact(eventTower);
    if (!expect(eventTower.run().context().floorIndex == 1U, "resolved event staircase must advance")) return false;

    arcane::app::TowerSession merchantTower(*merchantSeed, config);
    if (!expect(merchantTower.merchantStock().size() == 3U, "merchant floor must expose three stock items"))
        return false;
    interact(merchantTower);
    const auto purchasedId = merchantTower.merchantStock()[0].id;
    chooseLeftReward(merchantTower);
    if (!expect(merchantTower.merchantStock().size() == 2U,
            "purchased stock must disappear from the merchant panel")
        || !expect(merchantTower.merchantStock()[0].id != purchasedId,
            "sold content cannot remain purchasable")) return false;
    interact(merchantTower);
    interact(merchantTower);
    if (!expect(merchantTower.specialPanelOpen(), "merchant NPC must support repeated interaction")) return false;
    interact(merchantTower);
    merchantTower.update(moveRight, 0.5F);
    interact(merchantTower);
    return expect(merchantTower.run().context().floorIndex == 1U, "completed merchant must advance");
}

bool pauseCanContinueOrRestartFloorSnapshot()
{
    auto config = fastFlowConfig();
    arcane::app::AppFlowController app(123U, config);
    arcane::game::PlayerIntent confirm;
    confirm.menuConfirmPressed = true;
    app.update(confirm, 0.01F);
    if (!expect(app.screen() == arcane::app::AppScreen::Playing && app.tower(),
        "start confirmation must create a playable run")) return false;

    arcane::game::PlayerIntent attack;
    attack.attackPressed = true;
    app.update(attack, 0.01F);
    if (!expect(app.tower()->run().phase() == arcane::game::run::RunPhase::LootPending,
        "test must mutate the current floor before restart")) return false;
    arcane::game::PlayerIntent pause;
    pause.pausePressed = true;
    app.update(pause, 0.01F);
    arcane::game::PlayerIntent restart;
    restart.menuSecondaryPressed = true;
    app.update(restart, 0.01F);
    if (!expect(app.screen() == arcane::app::AppScreen::Playing
            && app.tower()->run().phase() == arcane::game::run::RunPhase::InEncounter,
        "restart must restore the floor-start phase")
        || !expect(app.tower()->run().player().gold == 0 && app.tower()->run().player().learnedSpells.empty(),
            "restart must roll back rewards earned on the current floor")) return false;

    app.update(pause, 0.01F);
    app.update(confirm, 0.01F);
    if (!expect(app.screen() == arcane::app::AppScreen::Start && app.canContinue(),
        "temporary exit must return to a continue-capable start screen")) return false;
    app.update(confirm, 0.01F);
    return expect(app.screen() == arcane::app::AppScreen::Playing
            && app.tower()->run().context().floorIndex == 0U,
        "continue must resume the retained run instead of creating another one");
}

bool defeatResultCanStartANewRun()
{
    auto config = fastFlowConfig();
    config.enemySpawn = config.playerSpawn;
    config.normalEnemyHealth = 1000;
    arcane::app::AppFlowController app(321U, config);
    arcane::game::PlayerIntent confirm;
    confirm.menuConfirmPressed = true;
    app.update(confirm, 0.01F);
    for (int step = 0; step < 1000 && app.screen() == arcane::app::AppScreen::Playing; ++step)
        app.update(arcane::game::PlayerIntent {}, 0.10F);
    if (!expect(app.screen() == arcane::app::AppScreen::Result && !app.victory(),
        "player death must enter the defeat result screen")) return false;
    app.update(confirm, 0.01F);
    return expect(app.screen() == arcane::app::AppScreen::Playing
            && app.tower()->run().player().currentHp == app.tower()->run().player().maxHp,
        "result confirmation must create a fresh run");
}

bool heldSpellCannotBypassLootInteraction()
{
    auto config = fastFlowConfig();
    config.initialPlayer.learnedSpells = {1006U};
    config.initialPlayer.equippedSpells[0] = 1006U;
    arcane::app::TowerSession tower(777U, config);

    arcane::game::PlayerIntent heldSpell;
    heldSpell.spellPressed[0] = true;
    tower.update(heldSpell, 0.01F);
    if (!expect(tower.run().phase() == arcane::game::run::RunPhase::LootPending,
            "lethal spell must create loot instead of opening reward")
        || !expect(tower.run().player().learnedSpells.size() == 1U,
            "lethal spell input must not grant another spell")) return false;

    tower.update(heldSpell, 0.01F);
    return expect(tower.run().phase() == arcane::game::run::RunPhase::LootPending,
            "continued spell input must not bypass loot interaction")
        && expect(tower.run().player().learnedSpells.size() == 1U,
            "continued spell input must not select a reward");
}
}

int main()
{
    const bool passed = combatRewardLoadoutAndStairsFormOneFlow()
        && contactDefeatEndsTheTowerFlow()
        && thirdBossEndsInVictory()
        && specialFloorsAdvanceWithoutCombat()
        && pauseCanContinueOrRestartFloorSnapshot()
        && defeatResultCanStartANewRun()
        && heldSpellCannotBypassLootInteraction();
    if (!passed) return 1;
    std::cout << "All tower session tests passed.\n";
    return 0;
}
