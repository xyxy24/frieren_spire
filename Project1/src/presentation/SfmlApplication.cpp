#include "presentation/SfmlApplication.hpp"

#include "platform/SfmlInputMapper.hpp"
#include "game/player/PlayerController.hpp"
#include "presentation/LootBookAnimator.hpp"
#include "presentation/PlayerAnimator.hpp"
#include "presentation/ShadeChargeAnimator.hpp"
#include "presentation/SpellCardArt.hpp"
#include "presentation/SpellEffectAnimator.hpp"
#include "presentation/viewmodel/ApplicationViewModel.hpp"
#include "presentation/viewmodel/CombatFeedbackViewModel.hpp"
#include "presentation/views/CombatView.hpp"
#include "presentation/views/ScreenViews.hpp"
#include "presentation/views/SpellAcquisitionView.hpp"
#include "presentation/views/UiPrimitives.hpp"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <optional>
#include <random>
#include <string>

namespace ui = arcane::presentation::viewmodel;
using namespace arcane::presentation::views;

namespace
{
constexpr float MaximumFrameTime = 0.05F;

arcane::game::run::Seed makeRuntimeSeed()
{
    std::random_device device;
    const auto entropy = (static_cast<std::uint64_t>(device()) << 32U)
        ^ static_cast<std::uint64_t>(device());
    const auto clock = static_cast<std::uint64_t>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());
    return entropy ^ clock;
}

std::string makeWindowTitle(const ui::ApplicationViewModel& app)
{
    const auto* towerModel = app.model();
    if (!towerModel) return "Arcane Spire";
    const auto& tower = *towerModel;
    const auto& run = tower.run();
    const auto& player = run.player();
    std::string title = "Arcane Spire | Floor " + std::to_string(run.context().floorIndex + 1U)
        + " | Seed " + std::to_string(run.context().runSeed)
        + " | HP " + std::to_string(player.currentHp) + "/" + std::to_string(player.maxHp)
        + " | Gold " + std::to_string(player.gold)
        + " | Boss " + std::to_string(run.context().bossesDefeated) + "/3 | ";

    if (const auto acquisition = app.spellAcquisition().snapshot())
        return title + std::string {acquisition->bossSpell ? "ULTIMATE SPELL ACQUIRED - "
            : "SPELL ACQUIRED - "} + std::string {acquisition->content.summary.name}
            + (acquisition->canDismiss ? " | Enter Continue"
                : acquisition->canSkip ? " | Space Skip" : " | Unlocking...");

    const auto& loadout = app.loadout();
    if (loadout.open())
    {
        const auto loadoutModel = loadout.snapshot(player);
        if (loadout.page() == ui::LoadoutPage::Relics)
        {
            const std::string selected = loadoutModel.selectedDetail
                ? std::string {loadoutModel.selectedDetail->summary.name} : std::string {"None"};
            return title + "RELIC COLLECTION - Selected " + selected
                + " | A/D Select, Q/E Spell Page, Tab Close";
        }
        const std::string selected = loadoutModel.selectedDetail
            ? std::string {loadoutModel.selectedDetail->summary.name} : std::string {"None"};
        const bool bossSection = loadout.spellSection() == ui::SpellSection::Boss;
        return title + (bossSection ? "BOSS SPELL LOADOUT - Selected " : "REGULAR SPELL LOADOUT - Selected ")
            + selected + (bossSection ? " | A/D Select, R Equip Ultimate" : " | A/D Select, U/I/O Equip Slot")
            + ", W/S Switch Spell Group, Q/E Relic Page, Tab Close";
    }

    switch (run.phase())
    {
    case arcane::game::run::RunPhase::InEncounter:
        if (tower.currentFloorType() == arcane::game::run::FloorType::Merchant)
        {
            if (!tower.specialPanelOpen()) return title + "MERCHANT ROOM - Meet NPC, E Trade | Rear Staircase Exits";
            return title + "MERCHANT - WASD Select | Enter Buy | E Close";
        }
        if (tower.currentFloorType() == arcane::game::run::FloorType::Event)
        {
            if (!tower.specialPanelOpen())
                return title + (tower.eventFloorState() == arcane::app::EventFloorState::Result
                    ? "EVENT RESOLVED - E Review NPC Result | Rear Staircase Exits"
                    : "EVENT ROOM - Meet NPC, E Interact");
            if (tower.eventFloorState() == arcane::app::EventFloorState::Result)
                return title + "EVENT RESULT - E Close";
            return title + "EVENT CHOICE - U I O Select | E Close";
        }
        return title + "A/D Move, Space Jump, S+Space Drop, J Attack, K Dash, U/I/O Spells, R Ultimate, Tab Loadout";
    case arcane::game::run::RunPhase::LootPending:
        return title + "ENEMY DROP - Move To Drop, E Inspect Reward | Tab Loadout";
    case arcane::game::run::RunPhase::Reward:
    {
        const auto candidates = tower.rewardCandidates();
        if (!candidates) return title + "Reward";
        const auto name = [](const arcane::game::run::ContentId id) {
            const auto* definition = arcane::game::spells::findDefinition(id);
            return definition ? std::string {definition->name} : std::to_string(id);
        };
        return title + "Choose U=" + name((*candidates)[0])
            + " I=" + name((*candidates)[1])
            + " O=" + name((*candidates)[2]) + " | Tab Loadout";
    }
    case arcane::game::run::RunPhase::FloorComplete:
        return title + "Move Into Staircase, E Climb (+50% Missing HP), Tab Loadout";
    case arcane::game::run::RunPhase::Victory:
        return title + "VICTORY - Third Boss Defeated";
    case arcane::game::run::RunPhase::Defeat:
        return title + "DEFEAT";
    case arcane::game::run::RunPhase::FloorLoading:
        return title + "Loading";
    }

    return title;
}

std::optional<arcane::presentation::PlayerVisualState> updatePresentationState(
    const ui::ApplicationViewModel& app, arcane::presentation::PlayerAnimator& animator,
    const float deltaSeconds)
{
    std::optional<arcane::presentation::PlayerVisualState> state;
    if (const auto* tower = app.model())
    {
        if (const auto* combat = tower->combat()) state = makePlayerVisualState(combat->playerState());
        else if (const auto* player = tower->explorationPlayer())
            state = makePlayerVisualState(*player, tower->run().player().currentHp);
    }
    const auto application = app.snapshot();
    if (state && (application.screen == ui::ApplicationScreen::Playing
        || application.screen == ui::ApplicationScreen::Result))
    {
        const std::optional<arcane::presentation::PlayerAnimation> presentationOverride =
            app.spellAcquisition().active()
            ? std::optional {arcane::presentation::PlayerAnimation::Pickup}
            : std::nullopt;
        animator.update(*state, deltaSeconds, presentationOverride);
    }
    return state;
}

void updateWindowTitle(sf::RenderWindow& window, const ui::ApplicationViewModel& app)
{
    const auto model = app.snapshot();
    if (model.screen == ui::ApplicationScreen::Start)
        window.setTitle(model.canContinue
            ? "Arcane Spire | CONTINUE - Enter | F2 Event | F3 Shop | F4 Spell"
            : "Arcane Spire | START - Enter | F2 Event | F3 Shop | F4 Spell");
    else if (model.screen == ui::ApplicationScreen::Pause)
        window.setTitle("Arcane Spire | PAUSE - W/S Select | Enter Confirm | Esc Resume");
    else if (model.screen == ui::ApplicationScreen::Result)
        window.setTitle(model.victory ? "Arcane Spire | WIN - Enter Start"
            : "Arcane Spire | DEFEAT - Enter Start");
    else if (app.model()) window.setTitle(makeWindowTitle(app) + " | Esc Pause");
}

struct RenderResources
{
    const EnemyStateTextures& headless;
    const EnemyStateTextures& mimic;
    const EnemyStateTextures& bird;
    const EnemyStateTextures& frostWolf;
    const EnemyStateTextures& chaosFlower;
    const EnemyStateTextures& qual;
    const std::array<std::optional<sf::Texture>, 3>& qualSkill;
    const EnemyStateTextures& heimon;
    const std::array<std::optional<sf::Texture>, 2>& heimonSkill;
    const std::optional<sf::Texture>& heimonFog;
    const EnemyStateTextures& demonWarrior;
    const EnemyStateTextures& largeBirdDemon;
    const std::optional<sf::Texture>& slash;
    const std::optional<sf::Texture>& largeSlash;
    const EnemyStateTextures& lugner;
    const std::array<std::optional<sf::Texture>, 3>& lugnerSkill;
    const EnemyStateTextures& linie;
    const std::array<std::optional<sf::Texture>, 2>& linieSkill;
    const EnemyStateTextures& draht;
    const EnemyStateTextures& aura;
    const EnemyStateTextures& revolte;
    const EnemyStateTextures& denken;
    const std::array<std::optional<sf::Texture>, 2>& tornado;
    const DialoguePortraitTextures& portraits;
    const ArenaTextures& arena;
    const arcane::presentation::LootBookAnimator& lootBookAnimator;
    const arcane::presentation::PlayerAnimator& playerAnimator;
    const arcane::presentation::ShadeChargeAnimator& shadeChargeAnimator;
    const arcane::presentation::SpellCardArt& spellCards;
    const arcane::presentation::SpellEffectAnimator& spellAnimator;
};

void renderApplicationFrame(sf::RenderWindow& window, const ui::ApplicationViewModel& app,
    const std::optional<arcane::presentation::PlayerVisualState>& playerVisualState,
    const RenderResources& resources, const ui::CombatFeedbackSnapshot& feedback)
{
    sf::Color background {18, 20, 28};
    const auto application = app.snapshot();
    if (application.screen == ui::ApplicationScreen::Result)
        background = application.victory ? sf::Color {20, 67, 49} : sf::Color {68, 24, 31};
    window.clear(background);
    if (application.screen == ui::ApplicationScreen::Start) drawStartMenu(window, application.canContinue);
    else if (application.screen == ui::ApplicationScreen::Pause) drawPauseMenu(window, application.pauseMenuItem);
    else if (application.screen == ui::ApplicationScreen::Result)
    {
        drawResultMenu(window, application.victory);
        if (!application.victory && playerVisualState)
            static_cast<void>(resources.playerAnimator.draw(window,
                {static_cast<float>(WindowWidth) * 0.5F, GroundTop},
                playerVisualState->facingDirection));
    }
    else if (const auto* tower = app.model())
    {
        const auto phase = tower->run().phase();
        if ((phase == arcane::game::run::RunPhase::InEncounter
            || phase == arcane::game::run::RunPhase::LootPending
            || phase == arcane::game::run::RunPhase::FloorComplete) && tower->combat())
        {
            const sf::View interfaceView = window.getView();
            sf::View worldView = interfaceView;
            worldView.move({feedback.cameraOffset.x, feedback.cameraOffset.y});
            window.setView(worldView);
            drawCombat(window, *tower, resources.headless, resources.mimic, resources.bird,
                resources.frostWolf, resources.chaosFlower, resources.qual, resources.qualSkill,
                resources.heimon, resources.heimonSkill, resources.heimonFog,
                resources.demonWarrior, resources.largeBirdDemon,
                resources.slash, resources.largeSlash,
                resources.lugner, resources.lugnerSkill,
                resources.linie, resources.linieSkill,
                resources.draht, resources.aura, resources.revolte,
                resources.denken, resources.tornado, resources.arena,
                resources.playerAnimator,
                resources.shadeChargeAnimator, resources.spellAnimator, feedback);
            drawStaircase(window, tower->staircaseBounds(), tower->staircaseUnlocked());
            if (const auto loot = tower->lootDropBounds())
                drawLootDrop(window, *loot, resources.lootBookAnimator);
            window.setView(interfaceView);
        }
        if (phase == arcane::game::run::RunPhase::Reward)
        {
            if (const auto model = app.reward())
                drawRewardScreen(window, *model, resources.spellCards);
        }
        if (phase == arcane::game::run::RunPhase::InEncounter
            && (tower->currentFloorType() == arcane::game::run::FloorType::Merchant
                || tower->currentFloorType() == arcane::game::run::FloorType::Event))
        {
            drawSpecialFloor(window, *tower, resources.playerAnimator,
                resources.shadeChargeAnimator, resources.arena);
            if (tower->specialPanelOpen()
                && tower->currentFloorType() == arcane::game::run::FloorType::Merchant)
            {
                if (const auto model = app.merchant())
                    drawMerchantScreen(window, *model, resources.spellCards);
            }
            if (tower->specialPanelOpen()
                && tower->currentFloorType() == arcane::game::run::FloorType::Event)
                drawEventScreen(window, *tower);
        }
        int health = tower->run().player().currentHp;
        std::optional<arcane::game::PlayerStateView> combatView;
        if (tower->combat()) { combatView = tower->combat()->playerState(); health = combatView->currentHealth; }
        drawHealthBar(window, {32.0F, 28.0F}, {300.0F, 22.0F}, health,
            tower->run().player().maxHp, sf::Color {108, 206, 126});
        if (combatView && combatView->shield > 0)
            drawPixelText(window, "SHIELD " + std::to_string(combatView->shield),
                {345.0F, 32.0F}, 1.0F, sf::Color {140, 190, 255});
        drawGold(window, tower->run().player().gold);
        if (!app.loadout().open())
        {
            if (const auto model = app.equippedSlots())
                drawEquippedSlots(window, *model, resources.spellCards,
                    combatView ? &*combatView : nullptr);
        }
        else
        {
            const auto model = app.loadout().snapshot(tower->run().player());
            drawLoadoutOverlay(window, model, resources.spellCards);
        }
        if (tower->combat()) drawCombatOverlay(window, *tower->combat(), resources.portraits);
        if (const auto acquisition = app.spellAcquisition().snapshot())
        {
            sf::Vector2f focusPosition {
                static_cast<float>(WindowWidth) * 0.5F, GroundTop - 145.0F};
            if (playerVisualState)
            {
                focusPosition = {
                    playerVisualState->position.x
                        + arcane::game::PlayerController::Width * 0.5F,
                    playerVisualState->position.y - 60.0F};
            }
            drawSpellAcquisition(window, *acquisition, resources.spellCards,
                focusPosition);
        }
    }
    window.display();
}
}

int arcane::presentation::SfmlApplication::run()
{
    sf::RenderWindow window(sf::VideoMode({WindowWidth, WindowHeight}), "Arcane Spire");
    window.setVerticalSyncEnabled(true);
    window.setKeyRepeatEnabled(false);

    arcane::platform::SfmlInputMapper inputMapper;
    arcane::app::TowerSessionConfig config;
    config.worldBounds = {0.0F, static_cast<float>(WindowWidth), GroundTop};
    ui::ApplicationViewModel app(makeRuntimeSeed(), config);
    const EnemyStateTextures headlessKnightTextures = loadEnemyStateTextures(
        "assets/enemies/headless_knight/");
    const EnemyStateTextures chestMimicTextures = loadEnemyStateTextures(
        "assets/enemies/chest_mimic/");
    const EnemyStateTextures birdDemonTextures = loadEnemyStateTextures(
        "assets/enemies/bird_demon/");
    const EnemyStateTextures frostWolfTextures = loadEnemyStateTextures(
        "assets/enemies/frost_wolf/", true, false, true, true);
    const EnemyStateTextures chaosFlowerTextures = loadEnemyStateTextures(
        "assets/enemies/chaos_flower/");
    const EnemyStateTextures qualTextures = loadEnemyStateTextures(
        "assets/enemies/qual/");
    const std::array<std::optional<sf::Texture>, 3> qualSkillTextures {
        loadTexture("assets/enemies/qual/skill1.png"),
        loadTexture("assets/enemies/qual/skill2.png"),
        loadTexture("assets/enemies/qual/skill3.png")
    };
    EnemyStateTextures heimonTextures = loadEnemyStateTextures(
        "assets/enemies/heimon/");
    heimonTextures.summon = loadTexture("assets/enemies/heimon/summon.png");
    const std::array<std::optional<sf::Texture>, 2> heimonSkillTextures {
        loadTexture("assets/enemies/heimon/skill1.png"),
        loadTexture("assets/enemies/heimon/skill2.png")
    };
    const std::optional<sf::Texture> heimonFogTexture =
        loadTexture("assets/enemies/heimon/fog.png");
    const EnemyStateTextures demonWarriorTextures = loadEnemyStateTextures(
        "assets/enemies/demon_warrior/");
    const EnemyStateTextures largeBirdDemonTextures = loadEnemyStateTextures(
        "assets/enemies/large_bird_demon/");
    const std::optional<sf::Texture> slashTexture =
        loadTexture("assets/enemies/shared/slash.png");
    const std::optional<sf::Texture> largeSlashTexture =
        loadTexture("assets/enemies/shared/large_slash.png");
    const EnemyStateTextures lugnerTextures = loadEnemyStateTextures(
        "assets/enemies/lugner/");
    const std::array<std::optional<sf::Texture>, 3> lugnerSkillTextures {
        loadTexture("assets/enemies/lugner/skill1.png"),
        loadTexture("assets/enemies/lugner/skill2.png"),
        loadTexture("assets/enemies/lugner/skill3.png")
    };
    const EnemyStateTextures linieTextures = loadEnemyStateTextures(
        "assets/enemies/linie/", true);
    const std::array<std::optional<sf::Texture>, 2> linieSkillTextures {
        loadTexture("assets/enemies/linie/skill1.png"),
        loadTexture("assets/enemies/linie/skill2.png")
    };
    const EnemyStateTextures drahtTextures = loadEnemyStateTextures(
        "assets/enemies/draht/");
    const EnemyStateTextures auraTextures = loadEnemyStateTextures(
        "assets/enemies/aura/", false, true, false);
    EnemyStateTextures revolteTextures = loadEnemyStateTextures(
        "assets/enemies/revolte/", false, false, true);
    revolteTextures.die = loadTexture("assets/enemies/revolte/die.png");
    revolteTextures.parry = loadTexture("assets/enemies/revolte/parry.png");
    revolteTextures.dash = loadTexture("assets/enemies/revolte/dash.png");
    for (std::size_t index = 0U; index < revolteTextures.skillWindups.size(); ++index)
        revolteTextures.skillWindups[index] = loadTexture("assets/enemies/revolte/windup"
            + std::to_string(index + 1U) + ".png");
    for (std::size_t index = 0U; index < revolteTextures.skillAttacks.size(); ++index)
        revolteTextures.skillAttacks[index] = loadTexture(index == 0U
            ? "assets/enemies/revolte/attack.png"
            : "assets/enemies/revolte/attack" + std::to_string(index + 1U) + ".png");
    EnemyStateTextures denkenTextures = loadEnemyStateTextures(
        "assets/enemies/denken/", false, false, false);
    denkenTextures.skillWindups[1] = loadTexture("assets/enemies/denken/windup2.png");
    const std::array<std::optional<sf::Texture>, 2> tornadoTextures {
        loadTexture("assets/enemies/denken/tornado.png"),
        loadTexture("assets/enemies/denken/tornado2.png")
    };
    const DialoguePortraitTextures dialoguePortraits {
        loadTexture("assets/portraits/frieren/idle.png"),
        loadTexture("assets/portraits/aura/initial.png"),
        loadTexture("assets/portraits/aura/idle.png"),
        loadTexture("assets/portraits/aura/windup.png"),
        loadTexture("assets/portraits/aura/die.png"),
        {loadTexture("assets/portraits/revolte/avatar1.png"),
            loadTexture("assets/portraits/revolte/avatar2.png"),
            loadTexture("assets/portraits/revolte/avatar3.png")}
    };
    const ArenaTextures arenaTextures = loadArenaTextures("assets/environments/processed");
    arcane::presentation::PlayerAnimator playerAnimator;
    static_cast<void>(playerAnimator.loadFromDirectory("assets/player"));
    arcane::presentation::ShadeChargeAnimator shadeChargeAnimator;
    arcane::presentation::LootBookAnimator lootBookAnimator;
    static_cast<void>(lootBookAnimator.load("assets/loot/magic-book.png"));
    arcane::presentation::SpellCardArt spellCards;
    static_cast<void>(spellCards.loadFromDirectory("assets/spell_cards"));
    arcane::presentation::SpellEffectAnimator spellEffectAnimator;
    static_cast<void>(spellEffectAnimator.loadFromDirectory("assets/spells"));
    const RenderResources renderResources {headlessKnightTextures, chestMimicTextures,
        birdDemonTextures, frostWolfTextures, chaosFlowerTextures, qualTextures, qualSkillTextures,
        heimonTextures, heimonSkillTextures, heimonFogTexture,
        demonWarriorTextures, largeBirdDemonTextures, slashTexture, largeSlashTexture,
        lugnerTextures, lugnerSkillTextures, linieTextures,
        linieSkillTextures, drahtTextures, auraTextures, revolteTextures,
        denkenTextures, tornadoTextures, dialoguePortraits,
        arenaTextures, lootBookAnimator, playerAnimator, shadeChargeAnimator, spellCards,
        spellEffectAnimator};
    sf::Clock frameClock;
    ui::CombatFeedbackViewModel combatFeedback;
    std::optional<std::uint32_t> feedbackFloor;

    while (window.isOpen())
    {
        while (const auto event = window.pollEvent())
        {
            inputMapper.handleEvent(*event);
            if (event->is<sf::Event::Closed>()) window.close();
        }

        const float deltaSeconds = std::min(frameClock.restart().asSeconds(), MaximumFrameTime);
        lootBookAnimator.update(deltaSeconds);
        float simulationDeltaSeconds = deltaSeconds;
        if (const auto* tower = app.model(); app.snapshot().screen == ui::ApplicationScreen::Playing
            && tower && tower->combat()
            && tower->run().phase() == arcane::game::run::RunPhase::InEncounter)
            simulationDeltaSeconds = combatFeedback.combatDeltaSeconds(deltaSeconds);
        app.update(inputMapper.sample(), simulationDeltaSeconds);
        const auto application = app.snapshot();
        if (const auto* tower = app.model(); application.screen == ui::ApplicationScreen::Playing
            && tower && tower->combat())
        {
            const std::uint32_t floor = tower->run().context().floorIndex;
            if (!feedbackFloor || *feedbackFloor != floor)
            {
                combatFeedback.reset();
                feedbackFloor = floor;
            }
            const auto enemies = tower->combat()->enemyStates();
            const float feedbackDelta = !app.loadout().open() ? deltaSeconds : 0.0F;
            combatFeedback.update(tower->combat()->playerState(), enemies, feedbackDelta);
        }
        else
        {
            combatFeedback.reset();
            feedbackFloor.reset();
        }
        const auto presentationState = updatePresentationState(
            app, playerAnimator, simulationDeltaSeconds);
        if (presentationState)
        {
            const bool shadeTimeAdvances = application.screen == ui::ApplicationScreen::Playing
                && !app.loadout().open() && !app.spellAcquisition().active();
            shadeChargeAnimator.update(*presentationState,
                shadeTimeAdvances ? deltaSeconds : 0.0F);
        }
        else
            shadeChargeAnimator.reset();
        updateWindowTitle(window, app);
        renderApplicationFrame(window, app, presentationState, renderResources,
            combatFeedback.snapshot());
    }

    return 0;
}
