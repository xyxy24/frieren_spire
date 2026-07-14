#include "presentation/viewmodel/ApplicationViewModel.hpp"
#include "game/relics/RelicSystem.hpp"
#include "game/spells/SpellSystem.hpp"
#include "presentation/viewmodel/UiViewModels.hpp"

#include <iostream>
#include <optional>
#include <string_view>

namespace
{
namespace ui = arcane::presentation::viewmodel;

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

void confirmMenu(arcane::app::TowerSession& tower)
{
    arcane::game::PlayerIntent intent;
    intent.menuConfirmPressed = true;
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
    ui::LoadoutViewModel loadout;
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
    static_cast<void>(loadout.handleInput(openLoadout, tower));
    arcane::game::PlayerIntent equipFirstSlot;
    equipFirstSlot.spellPressed[0] = true;
    static_cast<void>(loadout.handleInput(equipFirstSlot, tower));
    static_cast<void>(loadout.handleInput(openLoadout, tower));
    if (!expect(tower.run().player().equippedSpells[0] == selectedReward,
            "loadout input must equip a learned spell")) return false;

    interact(tower);
    return expect(tower.run().context().floorIndex == 1U, "stair interaction must advance exactly one floor")
        && expect(tower.run().phase() == arcane::game::run::RunPhase::InEncounter,
            "the next combat must start after climbing")
        && expect(tower.run().player().gold == 10, "victory gold must be awarded once");
}

bool loadoutSeparatesSpellAndRelicPages()
{
    auto config = fastFlowConfig();
    config.initialPlayer.learnedSpells = {1004U};
    config.initialPlayer.relics = {4001U, 4002U};
    arcane::app::TowerSession tower(808U, config);
    ui::LoadoutViewModel loadout;
    const auto initialPhase = tower.run().phase();

    arcane::game::PlayerIntent toggle;
    toggle.toggleLoadoutPressed = true;
    static_cast<void>(loadout.handleInput(toggle, tower));
    if (!expect(loadout.open() && loadout.page() == ui::LoadoutPage::Spells,
            "Tab must open the spell page first")) return false;

    arcane::game::PlayerIntent nextPage;
    nextPage.menuPageNextPressed = true;
    static_cast<void>(loadout.handleInput(nextPage, tower));
    if (!expect(loadout.page() == ui::LoadoutPage::Relics,
            "page input must switch to the separate relic page")
        || !expect(loadout.selectedRelic(tower.run().player()) == 4001U,
            "relic page must select the first acquired relic")) return false;

    arcane::game::PlayerIntent nextRelic;
    nextRelic.menuNextPressed = true;
    static_cast<void>(loadout.handleInput(nextRelic, tower));
    if (!expect(loadout.selectedRelic(tower.run().player()) == 4002U,
            "A/D navigation must select acquired relics on the relic page")) return false;

    arcane::game::PlayerIntent spellKey;
    spellKey.spellPressed[0] = true;
    static_cast<void>(loadout.handleInput(spellKey, tower));
    if (!expect(!tower.run().player().equippedSpells[0],
            "spell slot keys must not equip anything from the relic page")) return false;

    arcane::game::PlayerIntent previousPage;
    previousPage.menuPagePreviousPressed = true;
    static_cast<void>(loadout.handleInput(previousPage, tower));
    static_cast<void>(loadout.handleInput(spellKey, tower));
    if (!expect(loadout.page() == ui::LoadoutPage::Spells
            && tower.run().player().equippedSpells[0] == 1004U,
            "returning to the spell page must preserve spell equipment controls")) return false;

    static_cast<void>(loadout.handleInput(toggle, tower));
    return expect(!loadout.open() && tower.run().phase() == initialPhase,
        "closing either collection page must restore the original run phase");
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

bool firstBossIsAlwaysAura()
{
    arcane::app::TowerSessionConfig config;
    config.floorsPerBoss = 1U;
    config.bossEnemyHealth = 225;
    for (const arcane::game::run::Seed seed : {1ULL, 2ULL, 77ULL, 918ULL})
    {
        arcane::app::TowerSession tower(seed, config);
        if (!expect(tower.combat() != nullptr
                && tower.combat()->enemyState().archetype == arcane::game::EnemyArchetype::Aura,
            "first boss must remain Aura for every run seed while Red Mirror Dragon is deferred"))
            return false;
    }
    return true;
}

bool specialFloorsAdvanceWithoutCombat()
{
    arcane::app::TowerSessionConfig config;
    config.npcBounds = {150.0F, 550.0F, 100.0F, 90.0F};
    config.staircaseBounds = {300.0F, 550.0F, 100.0F, 90.0F};
    config.initialPlayer.gold = 30;
    config.firstFloorTypeOverride = arcane::game::run::FloorType::Event;

    std::optional<arcane::game::run::Seed> aldenSeed;
    for (arcane::game::run::Seed seed = 1U; seed < 1000U && !aldenSeed; ++seed)
    {
        arcane::app::TowerSession candidate(seed, config);
        if (candidate.eventKind() == arcane::app::EventKind::AldenBall) aldenSeed = seed;
    }
    if (!expect(aldenSeed.has_value(), "a deterministic Alden Ball event seed must exist")) return false;
    arcane::app::TowerSession eventTower(*aldenSeed, config);
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

    config.firstFloorTypeOverride = arcane::game::run::FloorType::Merchant;
    arcane::app::TowerSession merchantTower(2U, config);
    if (!expect(merchantTower.merchantStock().size() == 5U,
        "merchant floor must expose separate spell and relic rows"))
        return false;
    interact(merchantTower);
    const auto purchasedId = merchantTower.merchantStock()[0].id;
    confirmMenu(merchantTower);
    if (!expect(merchantTower.merchantStock().size() == 4U,
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

bool purchasedMerchantSpellCanBeEquippedAndCast()
{
    arcane::app::TowerSessionConfig config;
    config.npcBounds = {150.0F, 550.0F, 100.0F, 90.0F};
    config.initialPlayer.gold = 100;
    config.firstFloorTypeOverride = arcane::game::run::FloorType::Merchant;

    std::optional<arcane::game::run::Seed> merchantSeed;
    std::size_t spellStockIndex = 0U;
    for (arcane::game::run::Seed seed = 1U; seed < 1000U && !merchantSeed; ++seed)
    {
        arcane::app::TowerSession candidate(seed, config);
        if (candidate.currentFloorType() != arcane::game::run::FloorType::Merchant) continue;
        const auto& stock = candidate.merchantStock();
        for (std::size_t index = 0U; index < stock.size(); ++index)
        {
            if (stock[index].kind == arcane::game::economy::ItemKind::Spell)
            {
                merchantSeed = seed;
                spellStockIndex = index;
                break;
            }
        }
    }
    if (!expect(merchantSeed.has_value(), "a deterministic merchant stock with a spell must exist")) return false;

    arcane::app::TowerSession tower(*merchantSeed, config);
    interact(tower);
    const auto purchasedSpell = tower.merchantStock()[spellStockIndex].id;
    for (std::size_t index = 0U; index < spellStockIndex; ++index)
    {
        arcane::game::PlayerIntent next;
        next.menuNextPressed = true;
        tower.update(next, 0.01F);
    }
    confirmMenu(tower);
    if (!expect(tower.run().player().learnedSpells.size() == 1U
            && tower.run().player().learnedSpells[0] == purchasedSpell,
            "purchased merchant spell must enter the learned list")
        || !expect(arcane::game::spells::findDefinition(purchasedSpell) != nullptr,
            "purchased merchant spell must have a combat definition")) return false;

    arcane::game::PlayerIntent toggleLoadout;
    toggleLoadout.toggleLoadoutPressed = true;
    ui::LoadoutViewModel loadout;
    static_cast<void>(loadout.handleInput(toggleLoadout, tower));
    arcane::game::PlayerIntent equipFirstSlot;
    equipFirstSlot.spellPressed[0] = true;
    static_cast<void>(loadout.handleInput(equipFirstSlot, tower));
    static_cast<void>(loadout.handleInput(toggleLoadout, tower));

    arcane::game::spells::SpellSystem spells(tower.run().player().equippedSpells);
    const auto cast = spells.tryCast(0U, {160.0F, 576.0F}, 1.0F, {220.0F, 576.0F, 64.0F, 64.0F});
    return expect(tower.run().player().equippedSpells[0] == purchasedSpell,
            "purchased merchant spell must equip into a combat slot")
        && expect(cast.cast,
            "equipped merchant spell must cast even when it is a non-damaging support spell");
}

bool pauseCanContinueOrRestartFloorSnapshot()
{
    auto config = fastFlowConfig();
    ui::ApplicationViewModel app(123U, config);
    arcane::game::PlayerIntent confirm;
    confirm.menuConfirmPressed = true;
    app.update(confirm, 0.01F);
    if (!expect(app.snapshot().screen == ui::ApplicationScreen::Playing && app.model(),
        "start confirmation must create a playable run")) return false;

    arcane::game::PlayerIntent attack;
    attack.attackPressed = true;
    app.update(attack, 0.01F);
    if (!expect(app.model()->run().phase() == arcane::game::run::RunPhase::LootPending,
        "test must mutate the current floor before restart")) return false;
    arcane::game::PlayerIntent pause;
    pause.pausePressed = true;
    app.update(pause, 0.01F);
    if (!expect(app.snapshot().pauseMenuItem == ui::PauseMenuItem::ReplayCurrentFloor,
            "pause menu must initially select replay current floor")) return false;

    arcane::game::PlayerIntent down;
    down.menuDownPressed = true;
    app.update(down, 0.01F);
    if (!expect(app.snapshot().pauseMenuItem == ui::PauseMenuItem::SaveAndExit,
            "down input must select save and exit")) return false;
    app.update(confirm, 0.01F);
    if (!expect(app.snapshot().screen == ui::ApplicationScreen::Start && app.snapshot().canContinue
            && app.model()->run().phase() == arcane::game::run::RunPhase::LootPending,
        "save and exit must retain the current run on a continue-capable start screen")) return false;
    app.update(confirm, 0.01F);
    if (!expect(app.snapshot().screen == ui::ApplicationScreen::Playing
            && app.model()->run().phase() == arcane::game::run::RunPhase::LootPending,
        "continue must resume the retained run instead of creating another one")) return false;

    app.update(pause, 0.01F);
    if (!expect(app.snapshot().pauseMenuItem == ui::PauseMenuItem::ReplayCurrentFloor,
            "reopening pause must reset selection to replay")) return false;
    app.update(confirm, 0.01F);
    return expect(app.snapshot().screen == ui::ApplicationScreen::Playing
            && app.model()->run().phase() == arcane::game::run::RunPhase::InEncounter,
            "confirming replay must restore the floor-start phase")
        && expect(app.model()->run().player().gold == 0 && app.model()->run().player().learnedSpells.empty(),
            "replay must roll back rewards earned on the current floor");
}

bool startMenuEventPreviewOpensAnInteractiveEventFloor()
{
    arcane::app::TowerSessionConfig config;
    config.npcBounds = {150.0F, 550.0F, 100.0F, 90.0F};
    config.enableSpecialFloors = false;
    ui::ApplicationViewModel app(909U, config);

    arcane::game::PlayerIntent preview;
    preview.debugEventPreviewPressed = true;
    app.update(preview, 0.01F);
    if (!expect(app.snapshot().screen == ui::ApplicationScreen::Playing && app.model()
            && app.model()->currentFloorType() == arcane::game::run::FloorType::Event,
        "F2 preview must start directly on a deterministic event floor")) return false;

    arcane::game::PlayerIntent interactWithNpc;
    interactWithNpc.interactPressed = true;
    app.update(interactWithNpc, 0.01F);
    return expect(app.model()->specialPanelOpen()
            && app.model()->eventFloorState() == arcane::app::EventFloorState::Choosing,
        "event preview must expose the real spatial NPC interaction page");
}

bool startMenuMerchantPreviewOpensAnInteractiveShop()
{
    arcane::app::TowerSessionConfig config;
    config.npcBounds = {150.0F, 550.0F, 100.0F, 90.0F};
    config.enableSpecialFloors = false;
    ui::ApplicationViewModel app(910U, config);

    arcane::game::PlayerIntent preview;
    preview.debugMerchantPreviewPressed = true;
    app.update(preview, 0.01F);
    if (!expect(app.snapshot().screen == ui::ApplicationScreen::Playing && app.model()
            && app.model()->currentFloorType() == arcane::game::run::FloorType::Merchant,
        "F3 preview must start directly on a deterministic merchant floor")
        || !expect(app.model()->run().player().gold >= 100,
            "merchant preview must provide enough gold to test purchases")
        || !expect(app.model()->merchantStock().size() == 5U,
            "merchant preview must generate the real spell and relic rows")) return false;

    arcane::game::PlayerIntent interactWithNpc;
    interactWithNpc.interactPressed = true;
    app.update(interactWithNpc, 0.01F);
    return expect(app.model()->specialPanelOpen(),
        "merchant preview must expose the real spatial shop interaction page");
}

bool defeatResultCanStartANewRun()
{
    auto config = fastFlowConfig();
    config.enemySpawn = config.playerSpawn;
    config.normalEnemyHealth = 1000;
    ui::ApplicationViewModel app(321U, config);
    arcane::game::PlayerIntent confirm;
    confirm.menuConfirmPressed = true;
    app.update(confirm, 0.01F);
    for (int step = 0; step < 1000 && app.snapshot().screen == ui::ApplicationScreen::Playing; ++step)
        app.update(arcane::game::PlayerIntent {}, 0.10F);
    if (!expect(app.snapshot().screen == ui::ApplicationScreen::Result && !app.snapshot().victory,
        "player death must enter the defeat result screen")) return false;
    app.update(confirm, 0.01F);
    return expect(app.snapshot().screen == ui::ApplicationScreen::Playing
            && app.model()->run().player().currentHp == app.model()->run().player().maxHp,
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

bool loadoutEquipsAndCastsUnlockedUltimateSpell()
{
    auto config = fastFlowConfig();
    config.normalEnemyHealth = 70;
    config.enemySpawn = {300.0F, 576.0F};
    config.initialPlayer.learnedBossSpells = {2001U};
    config.initialPlayer.ultimateSpellUnlocked = true;
    arcane::app::TowerSession tower(778U, config);
    ui::LoadoutViewModel loadout;

    arcane::game::PlayerIntent open;
    open.toggleLoadoutPressed = true;
    static_cast<void>(loadout.handleInput(open, tower));
    arcane::game::PlayerIntent selectBossSection;
    selectBossSection.menuDownPressed = true;
    static_cast<void>(loadout.handleInput(selectBossSection, tower));
    if (!expect(loadout.spellSection() == ui::SpellSection::Boss,
            "spell page must expose a separate boss spell section")) return false;
    arcane::game::PlayerIntent equipUltimate;
    equipUltimate.ultimateSpellPressed = true;
    static_cast<void>(loadout.handleInput(equipUltimate, tower));
    if (!expect(tower.run().player().equippedUltimateSpell == 2001U,
            "R on the boss spell section must equip the ultimate slot")) return false;
    static_cast<void>(loadout.handleInput(open, tower));

    arcane::game::PlayerIntent cast;
    cast.ultimateSpellPressed = true;
    tower.update(cast, 0.01F);
    return expect(tower.run().phase() == arcane::game::run::RunPhase::LootPending,
        "equipped ultimate spell must cast through the tower combat flow");
}

bool applicationViewModelSelectsNewRewardWithoutAutoEquipping()
{
    auto config = fastFlowConfig();
    config.initialPlayer.learnedSpells = {1004U};
    ui::ApplicationViewModel app(809U, config);
    arcane::game::PlayerIntent confirm;
    confirm.menuConfirmPressed = true;
    app.update(confirm, 0.01F);

    arcane::game::PlayerIntent attack;
    attack.attackPressed = true;
    app.update(attack, 0.01F);
    arcane::game::PlayerIntent moveRight;
    moveRight.moveAxis = 1.0F;
    app.update(moveRight, 0.10F);
    arcane::game::PlayerIntent interactIntent;
    interactIntent.interactPressed = true;
    app.update(interactIntent, 0.01F);
    if (!expect(app.reward().has_value(),
            "application view model must expose a reward binding in the reward phase")) return false;

    const auto chosen = app.reward()->cards[0].summary.id;
    arcane::game::PlayerIntent choose;
    choose.spellPressed[0] = true;
    app.update(choose, 0.01F);
    if (!expect(app.model()->run().player().learnedSpells.back() == chosen,
            "reward command must update the model's learned spell collection")
        || !expect(app.loadout().selectedSpell(app.model()->run().player()) == chosen,
            "loadout view model must select the newly learned spell")
        || !expect(!app.model()->run().player().equippedSpells[0],
            "selecting a reward must not bypass the explicit equip command")) return false;

    arcane::game::PlayerIntent toggle;
    toggle.toggleLoadoutPressed = true;
    app.update(toggle, 0.01F);
    const auto loadout = app.loadout().snapshot(app.model()->run().player());
    return expect(app.loadout().open() && loadout.selectedDetail
            && loadout.selectedDetail->summary.id == chosen,
        "view must receive the selected reward through the loadout snapshot");
}

bool viewModelsProjectAuthoritativeContentWithoutRenderingRules()
{
    arcane::game::run::PlayerProgress player;
    player.learnedSpells = {1004U, 1005U};
    player.equippedSpells = {1004U, std::nullopt, 1005U};
    player.learnedBossSpells = {2001U};
    player.equippedUltimateSpell = 2001U;
    player.ultimateSpellUnlocked = true;
    player.relics = {arcane::game::relics::FirstClassBadgeId,
        arcane::game::relics::DarkDragonHornId};

    const std::array<arcane::game::run::ContentId, 3> candidates {1004U, 1005U, 2001U};
    const auto reward = ui::makeRewardViewModel(candidates, player);
    if (!expect(reward.showRerollHint,
            "reward view model must expose the owned reroll relic hint")
        || !expect(reward.cards[0].summary.name == "Zoltraak"
                && reward.cards[0].spell.has_value(),
            "reward view model must resolve spell details from the authoritative registry")
        || !expect(reward.cards[2].summary.bossSpell,
            "reward view model must identify boss spells")) return false;

    const std::array stock {
        arcane::game::economy::StockItem {1004U,
            arcane::game::economy::ItemKind::Spell, 20, false},
        arcane::game::economy::StockItem {arcane::game::relics::DarkDragonHornId,
            arcane::game::economy::ItemKind::Relic, 45, false}
    };
    const auto merchant = ui::makeMerchantViewModel(
        stock, arcane::game::relics::DarkDragonHornId);
    if (!expect(merchant.items.size() == 2U && merchant.items[1].content.selected,
            "merchant view model must preserve stock order and selection")
        || !expect(merchant.selectedDetail
                && merchant.selectedDetail->summary.kind == ui::ContentKind::Relic
                && merchant.selectedDetail->description.find("damage") != std::string_view::npos,
            "merchant view model must expose the selected relic description")) return false;

    const auto loadout = ui::makeLoadoutSnapshot(player, ui::LoadoutPage::Spells,
        ui::SpellSection::Regular, 1005U, arcane::game::relics::DarkDragonHornId);
    return expect(loadout.regularSpells.size() == 2U
            && !loadout.regularSpells[0].selected && loadout.regularSpells[1].selected,
        "loadout view model must select only the active regular spell")
        && expect(loadout.bossSpells.size() == 1U && !loadout.bossSpells[0].selected,
            "inactive boss section must not show a selected boss spell")
        && expect(loadout.equipped.regular[0]
                && loadout.equipped.regular[0]->id == 1004U
                && loadout.equipped.ultimate
                && loadout.equipped.ultimate->id == 2001U
                && loadout.equipped.ultimateUnlocked,
            "loadout view model must project regular and ultimate equipment");
}
}

int main()
{
    const bool passed = combatRewardLoadoutAndStairsFormOneFlow()
        && loadoutSeparatesSpellAndRelicPages()
        && applicationViewModelSelectsNewRewardWithoutAutoEquipping()
        && contactDefeatEndsTheTowerFlow()
        && thirdBossEndsInVictory()
        && firstBossIsAlwaysAura()
        && specialFloorsAdvanceWithoutCombat()
        && purchasedMerchantSpellCanBeEquippedAndCast()
        && pauseCanContinueOrRestartFloorSnapshot()
        && startMenuEventPreviewOpensAnInteractiveEventFloor()
        && startMenuMerchantPreviewOpensAnInteractiveShop()
        && defeatResultCanStartANewRun()
        && heldSpellCannotBypassLootInteraction()
        && loadoutEquipsAndCastsUnlockedUltimateSpell()
        && viewModelsProjectAuthoritativeContentWithoutRenderingRules();
    if (!passed) return 1;
    std::cout << "All tower session tests passed.\n";
    return 0;
}
