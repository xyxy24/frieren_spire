#include "presentation/viewmodel/ApplicationViewModel.hpp"
#include "game/relics/RelicSystem.hpp"
#include "game/spells/SpellSystem.hpp"
#include "presentation/viewmodel/UiViewModels.hpp"
#include "presentation/viewmodel/CombatFeedbackViewModel.hpp"

#include <algorithm>
#include <iostream>
#include <array>
#include <cmath>
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

const arcane::common::ui::ApplicationState& applicationState(
    const ui::ApplicationViewModel& application) noexcept;
void updateApplication(ui::ApplicationViewModel& application,
    const arcane::common::InputState& input, float deltaSeconds);

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

void collectNearbyLoot(arcane::app::TowerSession& tower, const bool jumpBeforeInteract = false)
{
    arcane::game::PlayerIntent moveRight;
    moveRight.moveAxis = 1.0F;
    tower.update(moveRight, 0.10F);
    if (jumpBeforeInteract)
    {
        arcane::game::PlayerIntent jump;
        jump.jumpPressed = true;
        tower.update(jump, 0.01F);
    }
    interact(tower);
}

void choosePowerBreakthrough(arcane::app::TowerSession& tower)
{
    if (tower.run().phase() != arcane::game::run::RunPhase::Breakthrough) return;
    arcane::game::PlayerIntent intent;
    intent.spellPressed[0] = true;
    tower.update(intent, 0.01F);
}

bool combatRewardLoadoutAndStairsFormOneFlow()
{
    arcane::app::TowerSession tower(0xC0FFEEU, fastFlowConfig());
    ui::LoadoutViewModel loadout;
    attackOnce(tower);
    if (!expect(tower.run().phase() == arcane::game::run::RunPhase::LootPending,
            "enemy defeat must create a loot drop")
        || !expect(tower.lootDropBounds().has_value(), "loot drop must exist at enemy death")) return false;
    collectNearbyLoot(tower, true);
    if (!expect(tower.run().phase() == arcane::game::run::RunPhase::Reward,
            "loot interaction must enter the reward phase")
        || !expect(tower.rewardCandidates().has_value(), "reward phase must expose three candidates")
        || !expect(tower.combat() && tower.combat()->playerState().grounded
                && std::abs(tower.combat()->playerState().position.y - 576.0F) < 0.01F
                && tower.combat()->playerState().velocity.x == 0.0F
                && tower.combat()->playerState().velocity.y == 0.0F,
            "opening an airborne reward must settle the player on the floor")) return false;

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
        choosePowerBreakthrough(tower);
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
    updateApplication(app, confirm, 0.01F);
    if (!expect(applicationState(app).screen == ui::ApplicationScreen::Playing
            && applicationState(app).run.has_value(),
        "start confirmation must create a playable run")) return false;

    arcane::game::PlayerIntent attack;
    attack.attackPressed = true;
    updateApplication(app, attack, 0.01F);
    if (!expect(applicationState(app).run->phase == arcane::common::ui::GameplayPhase::LootPending,
        "test must mutate the current floor before restart")) return false;
    arcane::game::PlayerIntent pause;
    pause.pausePressed = true;
    updateApplication(app, pause, 0.01F);
    if (!expect(applicationState(app).pauseMenuItem == ui::PauseMenuItem::ReplayCurrentFloor,
            "pause menu must initially select replay current floor")) return false;

    arcane::game::PlayerIntent down;
    down.menuDownPressed = true;
    updateApplication(app, down, 0.01F);
    if (!expect(applicationState(app).pauseMenuItem == ui::PauseMenuItem::SaveAndExit,
            "down input must select save and exit")) return false;
    updateApplication(app, confirm, 0.01F);
    if (!expect(applicationState(app).screen == ui::ApplicationScreen::Start
            && applicationState(app).canContinue
            && applicationState(app).run->phase == arcane::common::ui::GameplayPhase::LootPending,
        "save and exit must retain the current run on a continue-capable start screen")) return false;
    updateApplication(app, confirm, 0.01F);
    if (!expect(applicationState(app).screen == ui::ApplicationScreen::Playing
            && applicationState(app).run->phase == arcane::common::ui::GameplayPhase::LootPending,
        "continue must resume the retained run instead of creating another one")) return false;

    updateApplication(app, pause, 0.01F);
    if (!expect(applicationState(app).pauseMenuItem == ui::PauseMenuItem::ReplayCurrentFloor,
            "reopening pause must reset selection to replay")) return false;
    updateApplication(app, confirm, 0.01F);
    if (!expect(applicationState(app).screen == ui::ApplicationScreen::Playing
            && applicationState(app).run->phase == arcane::common::ui::GameplayPhase::InEncounter,
            "confirming replay must restore the floor-start phase")
        || !expect(applicationState(app).run->gold == 0,
            "replay must roll back gold earned on the current floor")) return false;
    arcane::common::InputState toggle;
    toggle.toggleLoadoutPressed = true;
    updateApplication(app, toggle, 0.01F);
    return expect(applicationState(app).loadout
            && applicationState(app).loadout->regularSpells.empty(),
        "replay must roll back spells earned on the current floor");
}

bool startMenuEventPreviewOpensAnInteractiveEventFloor()
{
    arcane::app::TowerSessionConfig config;
    config.npcBounds = {150.0F, 550.0F, 100.0F, 90.0F};
    config.enableSpecialFloors = false;
    ui::ApplicationViewModel app(909U, config);

    arcane::game::PlayerIntent preview;
    preview.debugEventPreviewPressed = true;
    updateApplication(app, preview, 0.01F);
    if (!expect(applicationState(app).screen == ui::ApplicationScreen::Playing
            && applicationState(app).run
            && applicationState(app).run->floorKind == arcane::common::ui::FloorKind::Event,
        "F2 preview must start directly on a deterministic event floor")) return false;

    arcane::game::PlayerIntent interactWithNpc;
    interactWithNpc.interactPressed = true;
    updateApplication(app, interactWithNpc, 0.01F);
    return expect(applicationState(app).specialFloor
            && applicationState(app).specialFloor->event.open
            && applicationState(app).specialFloor->event.state
                == arcane::common::ui::EventFloorState::Choosing,
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
    updateApplication(app, preview, 0.01F);
    if (!expect(applicationState(app).screen == ui::ApplicationScreen::Playing
            && applicationState(app).run
            && applicationState(app).run->floorKind == arcane::common::ui::FloorKind::Merchant,
        "F3 preview must start directly on a deterministic merchant floor")
        || !expect(applicationState(app).run->gold >= 100,
            "merchant preview must provide enough gold to test purchases")) return false;

    arcane::game::PlayerIntent interactWithNpc;
    interactWithNpc.interactPressed = true;
    updateApplication(app, interactWithNpc, 0.01F);
    return expect(applicationState(app).run->specialPanelOpen,
            "merchant preview must expose the real spatial shop interaction page")
        && expect(applicationState(app).merchant
                && applicationState(app).merchant->items.size() == 5U,
            "opened merchant binding must expose the real spell and relic rows");
}

bool startMenuSpellPreviewOpensTheRegistrationPresentation()
{
    ui::ApplicationViewModel app(911U);
    arcane::game::PlayerIntent preview;
    preview.debugSpellAcquisitionPreviewPressed = true;
    updateApplication(app, preview, 0.01F);
    const auto acquisition = applicationState(app).spellAcquisition;
    return expect(applicationState(app).screen == ui::ApplicationScreen::Playing
            && applicationState(app).run && acquisition
            && acquisition->content.summary.id == 1001U,
        "F4 preview must start a real run under the spell-registration presentation");
}

bool startMenuBossPreviewsOpenTheRealActBosses()
{
    constexpr std::array archetypes {
        arcane::game::EnemyArchetype::Aura,
        arcane::game::EnemyArchetype::Revolte,
        arcane::game::EnemyArchetype::WaterMirrorDemon};
    for (std::size_t index = 0U; index < archetypes.size(); ++index)
    {
        ui::ApplicationViewModel app(1200U + index);
        arcane::game::PlayerIntent preview;
        preview.debugBossPreviewPressed[index] = true;
        updateApplication(app, preview, 0.01F);
        const auto& state = applicationState(app);
        if (!expect(state.screen == ui::ApplicationScreen::Playing && state.run && state.combat,
                "F5-F7 preview must create a playable tower model")
            || !expect(state.run->floorKind == arcane::common::ui::FloorKind::Boss,
                "boss preview must enter the real boss combat floor")
            || !expect(!state.combat->enemies.empty()
                    && state.combat->enemies[0].archetype
                        == static_cast<arcane::common::ui::EnemyArchetype>(archetypes[index]),
                "each boss preview key must select its matching boss")
            || !expect(state.run->floorIndex == index * 5U + 4U
                    && state.run->act == index + 1U
                    && state.run->bossesDefeated == index,
                "boss preview must use the matching run progression")
            || !expect(state.combat->arena.id == (index + 1U) * 100U + 15U,
                "boss preview must use the matching act boss arena")
            || !expect(state.equippedSlots
                    && state.equippedSlots->regular[0]->id == 1004U
                    && state.equippedSlots->regular[1]->id == 1005U
                    && state.equippedSlots->regular[2]->id == 1006U,
                "boss preview must provide the deterministic regular test loadout")) return false;

        const bool shouldHaveUltimate = index > 0U;
        if (!expect(state.equippedSlots->ultimateUnlocked == shouldHaveUltimate
                && state.equippedSlots->ultimate.has_value() == shouldHaveUltimate,
            "later boss previews must expose an equipped ultimate without unlocking it early"))
            return false;
    }
    return true;
}

bool defeatResultCanStartANewRun()
{
    auto config = fastFlowConfig();
    config.enemySpawn = config.playerSpawn;
    config.normalEnemyHealth = 1000;
    ui::ApplicationViewModel app(321U, config);
    arcane::game::PlayerIntent confirm;
    confirm.menuConfirmPressed = true;
    updateApplication(app, confirm, 0.01F);
    for (int step = 0; step < 1000 && applicationState(app).screen == ui::ApplicationScreen::Playing; ++step)
        updateApplication(app, arcane::game::PlayerIntent {}, 0.10F);
    if (!expect(applicationState(app).screen == ui::ApplicationScreen::Result && !applicationState(app).victory,
        "player death must enter the defeat result screen")) return false;
    updateApplication(app, confirm, 0.01F);
    return expect(applicationState(app).screen == ui::ApplicationScreen::Playing
            && applicationState(app).run->currentHp == applicationState(app).run->maximumHp,
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

bool combatFeedbackTracksAuthoritativeHealthDeltas()
{
    ui::CombatFeedbackViewModel feedback;
    arcane::game::PlayerStateView player;
    player.position = {100.0F, 500.0F};
    player.currentHealth = 100;
    arcane::game::EnemyStateView enemy;
    enemy.position = {300.0F, 500.0F};
    enemy.currentHealth = 80;
    enemy.maximumHealth = 80;
    enemy.alive = true;
    std::array enemies {enemy};
    feedback.update(player, enemies, 0.01F);
    if (!expect(feedback.snapshot().damageNumbers.empty(),
        "feedback initialization must not invent damage events")) return false;

    player.currentHealth = 88;
    enemies[0].currentHealth = 55;
    feedback.update(player, enemies, 0.01F);
    const auto& active = feedback.snapshot();
    if (!expect(active.damageNumbers.size() == 2U,
            "player and enemy health deltas must each create one damage number")
        || !expect(active.impactBursts.size() == 2U,
            "each applied health delta must create a short impact burst")
        || !expect(active.playerFlashRatio > 0.0F && active.enemyFlashRatios[0] > 0.0F,
            "health loss must start target-specific hit flashes")
        || !expect(std::abs(active.enemyImpactOffsets[0]) > 0.01F,
            "enemy damage must create a presentation-only impact kick")
        || !expect(active.hitStopRemaining > 0.08F
                && feedback.combatDeltaSeconds(0.016F) == 0.0F,
            "medium damage must request hit stop without advancing combat time")
        || !expect(std::abs(active.cameraOffset.x) > 0.01F
                || std::abs(active.cameraOffset.y) > 0.01F,
            "damage feedback must expose deterministic camera shake")) return false;

    feedback.update(player, enemies, 1.0F);
    return expect(feedback.snapshot().damageNumbers.empty(),
            "damage numbers must expire using presentation delta time")
        && expect(feedback.snapshot().playerFlashRatio == 0.0F,
            "hit flashes must expire without mutating combat state")
        && expect(std::abs(feedback.combatDeltaSeconds(0.016F) - 0.016F) < 0.0001F,
            "combat delta time must resume after hit stop expires");
}

const arcane::common::ui::ApplicationState& applicationState(
    const ui::ApplicationViewModel& application) noexcept
{
    return application.stateBinding().value();
}

void updateApplication(ui::ApplicationViewModel& application,
    const arcane::common::InputState& input, const float deltaSeconds)
{
    application.execute({input, deltaSeconds});
}

bool bossHitStopKeepsImpactWithoutLookingLikeAFrameHitch()
{
    ui::CombatFeedbackViewModel feedback;
    arcane::game::PlayerStateView player;
    player.position = {100.0F, 500.0F};
    player.currentHealth = 100;
    arcane::game::EnemyStateView boss;
    boss.archetype = arcane::game::EnemyArchetype::Aura;
    boss.position = {700.0F, 500.0F};
    boss.currentHealth = 225;
    boss.maximumHealth = 225;
    boss.alive = true;
    std::array enemies {boss};
    feedback.update(player, enemies, 0.01F);

    enemies[0].currentHealth = 155;
    feedback.update(player, enemies, 0.01F);
    const float remaining = feedback.snapshot().hitStopRemaining;
    return expect(remaining > 0.0F,
            "damaging a boss must retain a brief hit-stop response")
        && expect(remaining <= 0.0551F,
            "boss hit stop must be capped so repeated attacks do not look like frame hitches");
}

bool applicationViewModelSelectsNewRewardWithoutAutoEquipping()
{
    auto config = fastFlowConfig();
    config.initialPlayer.learnedSpells = {1004U};
    ui::ApplicationViewModel app(809U, config);
    arcane::game::PlayerIntent confirm;
    confirm.menuConfirmPressed = true;
    updateApplication(app, confirm, 0.01F);

    arcane::game::PlayerIntent attack;
    attack.attackPressed = true;
    updateApplication(app, attack, 0.01F);
    arcane::game::PlayerIntent moveRight;
    moveRight.moveAxis = 1.0F;
    updateApplication(app, moveRight, 0.10F);
    arcane::game::PlayerIntent interactIntent;
    interactIntent.interactPressed = true;
    updateApplication(app, interactIntent, 0.01F);
    if (!expect(applicationState(app).reward.has_value(),
            "application view model must expose a reward binding in the reward phase")) return false;

    const auto chosen = applicationState(app).reward->cards[0].summary.id;
    arcane::game::PlayerIntent choose;
    choose.spellPressed[0] = true;
    updateApplication(app, choose, 0.01F);
    if (!expect(applicationState(app).spellAcquisition
            && applicationState(app).spellAcquisition->content.summary.id == chosen,
            "reward command must publish the learned spell through the acquisition state")
        || !expect(applicationState(app).equippedSlots
                && !applicationState(app).equippedSlots->regular[0],
            "selecting a reward must not bypass the explicit equip command")
        || !expect(applicationState(app).spellAcquisition.has_value(),
            "selecting a reward must start the acquisition presentation for that spell"))
        return false;

    const auto completedPhase = applicationState(app).run->phase;
    arcane::game::PlayerIntent blockedInput;
    blockedInput.toggleLoadoutPressed = true;
    blockedInput.pausePressed = true;
    blockedInput.interactPressed = true;
    blockedInput.spellPressed[0] = true;
    updateApplication(app, blockedInput, 0.25F);
    if (!expect(applicationState(app).spellAcquisition
            && !applicationState(app).loadout,
            "the acquisition presentation must consume pause, loadout, and world input")
        || !expect(applicationState(app).screen == ui::ApplicationScreen::Playing
                && applicationState(app).run->phase == completedPhase,
            "the acquisition presentation must freeze the underlying application flow"))
        return false;

    updateApplication(app, confirm, 0.01F);
    if (!expect(applicationState(app).spellAcquisition.has_value(),
            "confirmation must not dismiss the acquisition before its minimum display time"))
        return false;

    arcane::game::PlayerIntent idle;
    updateApplication(app, idle, ui::SpellAcquisitionViewModel::MinimumDisplaySeconds);
    const auto revealed = applicationState(app).spellAcquisition;
    if (!expect(revealed && revealed->registrationProgress == 1.0F
                && revealed->circulationProgress == 1.0F
                && revealed->burstProgress == 1.0F
                && revealed->revealProgress == 1.0F && revealed->canDismiss,
            "the spell-registration timeline must deterministically finish its reveal"))
        return false;
    updateApplication(app, confirm, 0.01F);
    if (!expect(!applicationState(app).spellAcquisition,
            "confirmation after the reveal must close the acquisition presentation")) return false;

    arcane::game::PlayerIntent toggle;
    toggle.toggleLoadoutPressed = true;
    updateApplication(app, toggle, 0.01F);
    const auto loadout = applicationState(app).loadout;
    return expect(loadout && loadout->selectedDetail
            && loadout->selectedDetail->summary.id == chosen,
        "view must receive the selected reward through the loadout snapshot");
}

bool spellAcquisitionReproducesRegistrationCadence()
{
    ui::SpellAcquisitionViewModel acquisition;
    acquisition.start(2001U);
    arcane::game::PlayerIntent confirm;
    confirm.menuConfirmPressed = true;
    acquisition.update(confirm,
        ui::SpellAcquisitionViewModel::CirculationStartSeconds - 0.01F);
    const auto analysis = acquisition.snapshot();
    if (!expect(analysis && analysis->bossSpell
            && analysis->registrationProgress > 0.0F
            && analysis->circulationProgress == 0.0F
            && analysis->burstProgress == 0.0F && !analysis->canDismiss,
        "the registration must begin with diagnostics before the circulation network"))
        return false;
    if (!expect(analysis->referenceElapsedSeconds > analysis->elapsedSeconds
            && ui::SpellAcquisitionViewModel::MinimumDisplaySeconds < 6.0F,
        "the complete reference sequence must play at the accelerated reward pace"))
        return false;
    acquisition.update({}, 0.02F);
    const auto circulating = acquisition.snapshot();
    if (!expect(circulating && circulating->circulationProgress > 0.0F
            && circulating->burstProgress == 0.0F,
        "crossing reference frame 384 must begin the expanding circulation network"))
        return false;
    acquisition.update({}, ui::SpellAcquisitionViewModel::RegistrationSeconds
        - ui::SpellAcquisitionViewModel::CirculationStartSeconds);
    const auto bursting = acquisition.snapshot();
    if (!expect(bursting && bursting->registrationProgress == 1.0F
            && bursting->burstProgress > 0.0F && bursting->revealProgress == 0.0F,
        "finishing the 713-frame registration must begin the closing starburst"))
        return false;
    acquisition.update({}, ui::SpellAcquisitionViewModel::MinimumDisplaySeconds);
    if (!expect(acquisition.snapshot()->canDismiss,
            "the registration must become confirmable after its final information reveal"))
        return false;
    acquisition.update(confirm, 0.0F);
    if (!expect(!acquisition.active(),
            "the spell-registration presentation must close after its minimum display time"))
        return false;

    ui::SpellAcquisitionViewModel skippable;
    skippable.start(1001U);
    arcane::game::PlayerIntent skip;
    skip.jumpPressed = true;
    skippable.update(skip, ui::SpellAcquisitionViewModel::SkipUnlockSeconds - 0.01F);
    if (!expect(skippable.snapshot() && !skippable.snapshot()->canDismiss,
            "the skip action must have a short accidental-input lockout"))
        return false;
    skippable.update(skip, 0.02F);
    const auto skipped = skippable.snapshot();
    return expect(skipped && skipped->registrationProgress == 1.0F
            && skipped->circulationProgress == 1.0F
            && skipped->burstProgress == 1.0F
            && skipped->revealProgress == 1.0F && skipped->canDismiss,
        "Space must skip only the animation and preserve the complete result page");
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

    auto comboPlayer = player;
    comboPlayer.learnedSpells.push_back(1001U);
    comboPlayer.learnedSpells.push_back(1016U);
    const std::array<arcane::game::run::ContentId, 3> comboCandidates {
        1006U, 1028U, 2007U};
    const auto comboReward = ui::makeRewardViewModel(comboCandidates, comboPlayer);
    const auto hasPartner = [](const auto& hints, const auto id) {
        return std::any_of(hints.begin(), hints.end(),
            [id](const auto& hint) { return hint.ownedSpellId == id; });
    };
    if (!expect(comboReward.cards[0].synergies.size() == 2U
            && hasPartner(comboReward.cards[0].synergies, 1001U)
            && hasPartner(comboReward.cards[0].synergies, 1005U),
            "reward cards must list every relevant owned combo for Flame Burst")
        || !expect(std::any_of(comboReward.cards[0].synergies.begin(),
                comboReward.cards[0].synergies.end(), [](const auto& hint) {
                    return hint.description.find("2 second") != std::string_view::npos;
                }),
            "combo hints must explain the implemented trigger order and result")
        || !expect(comboReward.cards[1].synergies.empty(),
            "reward cards must not invent a combo for unrelated owned magic")
        || !expect(comboReward.cards[2].synergies.size() == 1U
                && comboReward.cards[2].synergies[0].ownedSpellId == 1016U
                && comboReward.cards[2].synergies[0].description.find("40 percent")
                    != std::string_view::npos,
            "Boss reward cards must explain interactions with owned regular magic")) return false;

    const std::array<arcane::game::run::ContentId, 3> setupCandidates {
        1002U, 1023U, 2012U};
    const auto setupReward = ui::makeRewardViewModel(setupCandidates, player);
    if (!expect(!setupReward.cards[0].synergies.empty()
            && setupReward.cards[0].synergies[0].ownedSpellId == 1004U,
            "a newly offered damage buff must identify compatible owned attacks")
        || !expect(!setupReward.cards[1].synergies.empty()
                && setupReward.cards[1].synergies[0].ownedSpellId == 1004U,
            "a newly offered Flight Magic must explain its first-spell combo")
        || !expect(!setupReward.cards[2].synergies.empty()
                && setupReward.cards[2].synergies[0].description.find("90 percent")
                    != std::string_view::npos,
            "a newly offered Mirror Array must explain copied owned spells")) return false;

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
        && spellAcquisitionReproducesRegistrationCadence()
        && combatFeedbackTracksAuthoritativeHealthDeltas()
        && bossHitStopKeepsImpactWithoutLookingLikeAFrameHitch()
        && contactDefeatEndsTheTowerFlow()
        && thirdBossEndsInVictory()
        && firstBossIsAlwaysAura()
        && specialFloorsAdvanceWithoutCombat()
        && purchasedMerchantSpellCanBeEquippedAndCast()
        && pauseCanContinueOrRestartFloorSnapshot()
        && startMenuEventPreviewOpensAnInteractiveEventFloor()
        && startMenuMerchantPreviewOpensAnInteractiveShop()
        && startMenuSpellPreviewOpensTheRegistrationPresentation()
        && startMenuBossPreviewsOpenTheRealActBosses()
        && defeatResultCanStartANewRun()
        && heldSpellCannotBypassLootInteraction()
        && loadoutEquipsAndCastsUnlockedUltimateSpell()
        && viewModelsProjectAuthoritativeContentWithoutRenderingRules();
    if (!passed) return 1;
    std::cout << "All tower session tests passed.\n";
    return 0;
}
