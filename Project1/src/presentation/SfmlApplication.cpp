#include "presentation/SfmlApplication.hpp"

#include "platform/SfmlInputMapper.hpp"
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
#include <iostream>
#include <optional>
#include <random>
#include <span>
#include <string>
#include <vector>

namespace ui = arcane::presentation::viewmodel;
using namespace arcane::presentation::views;

namespace
{
constexpr float MaximumFrameTime = 0.05F;

arcane::common::ui::Seed makeRuntimeSeed()
{
    std::random_device device;
    const auto entropy = (static_cast<std::uint64_t>(device()) << 32U)
        ^ static_cast<std::uint64_t>(device());
    const auto clock = static_cast<std::uint64_t>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());
    return entropy ^ clock;
}

std::optional<arcane::presentation::PlayerVisualState> updatePresentationState(
    const arcane::common::ui::ApplicationState& appState,
    arcane::presentation::PlayerAnimator& animator,
    const float deltaSeconds)
{
    std::optional<arcane::presentation::PlayerVisualState> visual;
    if (appState.combat)
    {
        const auto& player = appState.combat->player;
        visual = arcane::presentation::PlayerVisualState {player.position, player.velocity,
            player.currentHealth, player.grounded, player.facingDirection,
            player.attackSequence, player.castSequence, player.hurtSequence,
            player.dashRemaining, player.shadowDashChargeRemaining,
            player.shadowDashing, player.stunned};
    }
    else if (appState.specialFloor && appState.specialFloor->player)
        visual = *appState.specialFloor->player;
    if (visual && (appState.screen == arcane::common::ui::ApplicationScreen::Playing
        || appState.screen == arcane::common::ui::ApplicationScreen::Result))
    {
        const std::optional<arcane::presentation::PlayerAnimation> presentationOverride =
            appState.spellAcquisition
            ? std::optional {arcane::presentation::PlayerAnimation::Pickup}
            : std::nullopt;
        animator.update(*visual, deltaSeconds, presentationOverride);
    }
    return visual;
}

void updateWindowTitle(sf::RenderWindow& window, const ui::ApplicationViewModel& app,
    std::string& displayedTitle)
{
    std::string title = app.stateBinding().value().windowTitle;
    if (app.stateBinding().value().screen == arcane::common::ui::ApplicationScreen::Playing)
        title += " | Esc Pause";
    if (title == displayedTitle) return;
    window.setTitle(title);
    displayedTitle = std::move(title);
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
    const EnemyStateTextures& gargoyle;
    const std::array<std::optional<sf::Texture>, 2>& gargoyleSkill;
    const EnemyStateTextures& swordDemon;
    const EnemyStateTextures& threeHeadedDemon;
    const EnemyStateTextures& richter;
    const std::array<std::optional<sf::Texture>, 3>& pillar;
    const EnemyStateTextures& laufen;
    const EnemyStateTextures& starkCopy;
    const std::optional<sf::Texture>& starkSlash;
    const EnemyStateTextures& frierenCopy;
    const EnemyStateTextures& fernCopy;
    const EnemyStateTextures& waterMirror;
    const std::array<std::optional<sf::Texture>, 2>& frierenBeam;
    const std::array<std::optional<sf::Texture>, 2>& frierenLightning;
    const std::optional<sf::Texture>& frierenFire;
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
    const StaircaseTextures& staircases;
    const std::array<std::optional<sf::Texture>, 3>& meteorCards;
    const std::optional<sf::Texture>& meteorNpc;
    const std::array<std::optional<sf::Texture>, 3>& ordenCards;
    const std::optional<sf::Texture>& ordenNpc;
    const std::array<std::optional<sf::Texture>, 3>& swordVillageCards;
    const std::optional<sf::Texture>& swordVillageNpc;
    const std::array<std::optional<sf::Texture>, 3>& southernHeroCards;
    const std::optional<sf::Texture>& southernHeroNpc;
    const std::optional<sf::Texture>& merchantNpc;
    const std::optional<sf::Texture>& startMenuBackground;
    const arcane::presentation::EnemyAnimator& enemyAnimator;
    const arcane::presentation::LootBookAnimator& lootBookAnimator;
    const arcane::presentation::PlayerAnimator& playerAnimator;
    const arcane::presentation::ShadeChargeAnimator& shadeChargeAnimator;
    const arcane::presentation::SpellCardArt& spellCards;
    const arcane::presentation::SpellEffectAnimator& spellAnimator;
};

void renderApplicationFrame(sf::RenderWindow& window,
    const arcane::common::ui::ApplicationState& application,
    const std::optional<arcane::presentation::PlayerVisualState>& playerVisualState,
    const RenderResources& resources,
    const arcane::common::ui::CombatFeedbackState& feedback)
{
    sf::Color background {18, 20, 28};
    if (application.screen == arcane::common::ui::ApplicationScreen::Result)
        background = application.victory ? sf::Color {20, 67, 49} : sf::Color {68, 24, 31};
    window.clear(background);
    if (application.screen == arcane::common::ui::ApplicationScreen::Start)
    {
        if (resources.startMenuBackground)
        {
            const sf::Vector2u textureSize = resources.startMenuBackground->getSize();
            sf::Sprite backdrop(*resources.startMenuBackground);
            backdrop.setScale({static_cast<float>(WindowWidth) / static_cast<float>(textureSize.x),
                static_cast<float>(WindowHeight) / static_cast<float>(textureSize.y)});
            window.draw(backdrop);
            sf::RectangleShape readabilityVeil({static_cast<float>(WindowWidth),
                static_cast<float>(WindowHeight)});
            readabilityVeil.setFillColor(sf::Color {5, 8, 18, 38});
            window.draw(readabilityVeil);
        }
        drawStartMenu(window, application.canContinue);
    }
    else if (application.screen == arcane::common::ui::ApplicationScreen::Pause)
        drawPauseMenu(window, application.pauseMenuItem);
    else if (application.screen == arcane::common::ui::ApplicationScreen::Result)
    {
        drawResultMenu(window, application.victory);
        if (!application.victory && playerVisualState)
            static_cast<void>(resources.playerAnimator.draw(window,
                {static_cast<float>(WindowWidth) * 0.5F, GroundTop},
                playerVisualState->facingDirection));
    }
    else if (application.run)
    {
        const auto& run = *application.run;
        if (application.combat)
        {
            const sf::View interfaceView = window.getView();
            sf::View worldView = interfaceView;
            worldView.move({feedback.cameraOffset.x, feedback.cameraOffset.y});
            window.setView(worldView);
            drawCombat(window, *application.combat,
                resources.headless, resources.mimic, resources.bird,
                resources.frostWolf, resources.chaosFlower, resources.qual, resources.qualSkill,
                resources.heimon, resources.heimonSkill, resources.heimonFog,
                resources.demonWarrior, resources.largeBirdDemon,
                resources.gargoyle, resources.gargoyleSkill,
                resources.swordDemon,
                resources.threeHeadedDemon,
                resources.richter, resources.pillar,
                resources.laufen,
                resources.starkCopy, resources.starkSlash,
                resources.frierenCopy, resources.fernCopy, resources.waterMirror,
                resources.frierenBeam,
                resources.frierenLightning, resources.frierenFire,
                resources.slash, resources.largeSlash,
                resources.lugner, resources.lugnerSkill,
                resources.linie, resources.linieSkill,
                resources.draht, resources.aura, resources.revolte,
                resources.denken, resources.tornado, resources.arena,
                resources.staircases,
                resources.enemyAnimator, resources.playerAnimator,
                resources.shadeChargeAnimator, resources.spellAnimator, feedback);
            if (application.combat->lootDrop)
                drawLootDrop(window, *application.combat->lootDrop,
                    resources.lootBookAnimator);
            window.setView(interfaceView);
        }
        if (application.reward)
            drawRewardScreen(window, *application.reward, resources.spellCards);
        if (application.breakthrough)
            drawBreakthroughScreen(window, *application.breakthrough);
        if (application.specialFloor)
        {
            drawSpecialFloor(window, *application.specialFloor, resources.playerAnimator,
                resources.shadeChargeAnimator, resources.arena, resources.staircases,
                resources.meteorNpc, resources.ordenNpc, resources.swordVillageNpc,
                resources.southernHeroNpc, resources.merchantNpc);
            if (application.merchant)
                drawMerchantScreen(window, *application.merchant, resources.spellCards);
            if (application.specialFloor->event.open
                && application.specialFloor->floorKind == arcane::common::ui::FloorKind::Event)
                drawEventScreen(window, application.specialFloor->event,
                    resources.meteorCards, resources.ordenCards,
                    resources.swordVillageCards, resources.southernHeroCards);
        }
        const auto* combatView = application.combat ? &application.combat->player : nullptr;
        drawHealthBar(window, {32.0F, 28.0F}, {300.0F, 22.0F}, run.currentHp,
            run.maximumHp, sf::Color {108, 206, 126});
        if (combatView && combatView->shield > 0)
            drawPixelText(window, "SHIELD " + std::to_string(combatView->shield),
                {345.0F, 32.0F}, 1.0F, sf::Color {140, 190, 255});
        drawGold(window, run.gold);
        if (!application.loadout)
        {
            if (application.equippedSlots)
                drawEquippedSlots(window, *application.equippedSlots,
                    resources.spellCards, combatView);
        }
        else
            drawLoadoutOverlay(window, *application.loadout, resources.spellCards);
        if (application.combat)
            drawCombatOverlay(window, *application.combat, resources.portraits);
        if (application.spellAcquisition)
        {
            sf::Vector2f focusPosition {
                static_cast<float>(WindowWidth) * 0.5F, GroundTop - 145.0F};
            if (playerVisualState)
            {
                focusPosition = {
                    playerVisualState->position.x
                        + arcane::common::ui::PlayerWidth * 0.5F,
                    playerVisualState->position.y - 60.0F};
            }
            drawSpellAcquisition(window, *application.spellAcquisition,
                resources.spellCards,
                focusPosition);
        }
    }
}

bool visualSmokeStateMatches(const std::size_t step,
    const arcane::common::ui::ApplicationState& state,
    const std::optional<float> initialPlayerX)
{
    using arcane::common::ui::ApplicationScreen;
    using arcane::common::ui::PauseMenuItem;
    switch (step)
    {
    case 0U: return state.screen == ApplicationScreen::Start && !state.canContinue;
    case 1U: return state.screen == ApplicationScreen::Playing && state.combat.has_value();
    case 2U: return state.combat && initialPlayerX
            && state.combat->player.position.x > *initialPlayerX + 30.0F;
    case 3U: return state.combat && (!state.combat->player.grounded
            || state.combat->player.velocity.y < 0.0F);
    case 4U: return state.combat && state.combat->player.attackSequence > 0U;
    case 5U: return state.combat && (state.combat->player.dashRemaining > 0.0F
            || state.combat->player.shadowDashing);
    case 6U: return state.screen == ApplicationScreen::Pause
            && state.pauseMenuItem == PauseMenuItem::ReplayCurrentFloor;
    case 7U: return state.screen == ApplicationScreen::Pause
            && state.pauseMenuItem == PauseMenuItem::SaveAndExit;
    case 8U: return state.screen == ApplicationScreen::Start && state.canContinue;
    case 9U: return state.screen == ApplicationScreen::Playing && state.combat.has_value();
    case 10U: return state.screen == ApplicationScreen::Playing && state.loadout.has_value();
    case 11U: return state.screen == ApplicationScreen::Playing && !state.loadout.has_value();
    default: return false;
    }
}

void drawVisualSmokeStatus(sf::RenderTarget& target, const std::size_t step,
    const bool finished, const bool passed)
{
    constexpr std::array<std::string_view, 12> Names {"START SCREEN", "START TO PLAYING",
        "MOVE RIGHT", "JUMP", "BASIC ATTACK", "DASH", "PLAYING TO PAUSE",
        "SELECT SAVE AND EXIT", "EXIT TO CONTINUE SCREEN", "CONTINUE TO PLAYING",
        "OPEN LOADOUT OVERLAY", "CLOSE LOADOUT OVERLAY"};
    sf::RectangleShape status({static_cast<float>(WindowWidth), 76.0F});
    status.setFillColor(sf::Color {7, 9, 18, 225});
    target.draw(status);
    drawPixelText(target, "REAL VIEW UI SMOKE TEST", {22.0F, 12.0F}, 1.35F,
        sf::Color {145, 218, 255});
    const std::string message = finished
        ? (passed ? "PASS - REAL GAME UI VERIFIED" : "FAIL - UI STATE MISMATCH")
        : "STEP " + std::to_string(step + 1U) + "/12 - " + std::string {Names[step]};
    drawPixelText(target, message, {22.0F, 43.0F}, 1.05F,
        passed ? sf::Color {125, 232, 155} : sf::Color {240, 100, 110});
}
}

int arcane::presentation::SfmlApplication::run()
{
    return runImpl(false);
}

int arcane::presentation::SfmlApplication::runVisualSmokeTest()
{
    return runImpl(true);
}

int arcane::presentation::SfmlApplication::runImpl(const bool visualSmokeTest)
{
    sf::RenderWindow window(sf::VideoMode({WindowWidth, WindowHeight}), "Arcane Spire");
    window.setVerticalSyncEnabled(true);
    window.setKeyRepeatEnabled(false);
    std::string displayedWindowTitle = "Arcane Spire | Loading assets...";
    window.setTitle(displayedWindowTitle);
    window.clear(sf::Color {18, 20, 28});
    drawPixelText(window, "LOADING ASSETS...",
        {static_cast<float>(WindowWidth) * 0.5F - 154.0F,
            static_cast<float>(WindowHeight) * 0.5F - 20.0F},
        3.0F, sf::Color {220, 206, 255});
    drawPixelText(window, "PREPARING THE TOWER",
        {static_cast<float>(WindowWidth) * 0.5F - 120.0F,
            static_cast<float>(WindowHeight) * 0.5F + 26.0F},
        1.5F, sf::Color {137, 171, 205});
    window.display();

    arcane::platform::SfmlInputMapper inputMapper;
    arcane::app::TowerSessionConfig config;
    config.worldBounds = {0.0F, static_cast<float>(WindowWidth), GroundTop};
    ui::ApplicationViewModel app(visualSmokeTest ? 0x51A7E123U : makeRuntimeSeed(), config);
    const EnemyStateTextures headlessKnightTextures = loadEnemyStateTextures(
        "assets/enemies/headless_knight/", false, false, true, false, true);
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
    EnemyStateTextures gargoyleTextures = loadEnemyStateTextures(
        "assets/enemies/gargoyle/");
    gargoyleTextures.jump = loadTexture("assets/enemies/gargoyle/fly.png");
    const std::array<std::optional<sf::Texture>, 2> gargoyleSkillTextures {
        loadTexture("assets/enemies/gargoyle/skill1.png"),
        loadTexture("assets/enemies/gargoyle/skill2.png")
    };
    EnemyStateTextures swordDemonTextures = loadEnemyStateTextures(
        "assets/enemies/sword_demon/");
    swordDemonTextures.dash = loadTexture("assets/enemies/sword_demon/dash.png");
    EnemyStateTextures threeHeadedDemonTextures;
    for (std::size_t index = 0U; index < threeHeadedDemonTextures.idleVariants.size(); ++index)
    {
        const std::string suffix = std::to_string(index + 1U) + ".png";
        threeHeadedDemonTextures.idleVariants[index] = loadTexture(
            "assets/enemies/three_headed_demon/idle" + suffix);
        threeHeadedDemonTextures.skillWindups[index] = loadTexture(
            "assets/enemies/three_headed_demon/windup" + suffix);
        threeHeadedDemonTextures.skillAttacks[index] = loadTexture(
            "assets/enemies/three_headed_demon/attack" + suffix);
    }
    const EnemyStateTextures richterTextures = loadEnemyStateTextures(
        "assets/enemies/richter/", false, false, false);
    const std::array<std::optional<sf::Texture>, 3> pillarTextures {
        loadTexture("assets/enemies/richter/skill1.png"),
        loadTexture("assets/enemies/richter/skill2.png"),
        loadTexture("assets/enemies/richter/skill3.png")
    };
    EnemyStateTextures laufenTextures = loadEnemyStateTextures(
        "assets/enemies/laufen/");
    laufenTextures.preJump = loadTexture("assets/enemies/laufen/windup_attack.png");
    EnemyStateTextures starkCopyTextures;
    starkCopyTextures.idle = loadTexture("assets/enemies/stark_copy/idle.png");
    starkCopyTextures.skillWindups[0] = loadTexture("assets/enemies/stark_copy/windup1.png");
    starkCopyTextures.jump = loadTexture("assets/enemies/stark_copy/jump.png");
    starkCopyTextures.skillAttacks[0] = loadTexture("assets/enemies/stark_copy/attack1.png");
    starkCopyTextures.skillWindups[1] = loadTexture("assets/enemies/stark_copy/windup2.png");
    starkCopyTextures.skillAttacks[1] = loadTexture("assets/enemies/stark_copy/attack2.png");
    const std::optional<sf::Texture> starkSlashTexture =
        loadTexture("assets/enemies/stark_copy/slash.png");
    EnemyStateTextures frierenCopyTextures;
    frierenCopyTextures.idle = loadTexture("assets/enemies/frieren_copy/idle.png");
    frierenCopyTextures.attack = loadTexture("assets/enemies/frieren_copy/attack.png");
    frierenCopyTextures.windup = loadTexture("assets/enemies/frieren_copy/windup_attack.png");
    frierenCopyTextures.skillWindups[0] = loadTexture(
        "assets/enemies/frieren_copy/lightning-windup.png");
    frierenCopyTextures.skillWindups[1] = loadTexture(
        "assets/enemies/frieren_copy/fire_windup.png");
    const std::array<std::optional<sf::Texture>, 2> frierenBeamTextures {
        loadTexture("assets/enemies/frieren_copy/skill1.png"),
        loadTexture("assets/enemies/frieren_copy/skill2.png")};
    const std::array<std::optional<sf::Texture>, 2> frierenLightningTextures {
        loadTexture("assets/enemies/frieren_copy/lightning1.png"),
        loadTexture("assets/enemies/frieren_copy/lightning2.png")};
    const std::optional<sf::Texture> frierenFireTexture =
        loadTexture("assets/enemies/frieren_copy/fire.png");
    EnemyStateTextures fernCopyTextures;
    fernCopyTextures.idle = loadTexture("assets/enemies/fern_copy/idle.png");
    fernCopyTextures.windup = loadTexture("assets/enemies/fern_copy/windup.png");
    fernCopyTextures.attack = loadTexture("assets/enemies/fern_copy/attack.png");
    EnemyStateTextures waterMirrorTextures;
    waterMirrorTextures.idle = loadTexture("assets/enemies/water_mirror_demon/idle.png");
    waterMirrorTextures.die = loadTexture("assets/enemies/water_mirror_demon/die.png");
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
    EnemyStateTextures auraTextures = loadEnemyStateTextures(
        "assets/enemies/aura/", false, true, false);
    auraTextures.domination = loadTexture("assets/enemies/aura/domination.png");
    auraTextures.guillotineFrame = loadTexture(
        "assets/enemies/aura/soul-guillotine-frame.png");
    auraTextures.guillotineBlade = loadTexture(
        "assets/enemies/aura/soul-guillotine-blade.png");
    EnemyStateTextures revolteTextures;
    revolteTextures.idle = loadTexture("assets/enemies/revolte/idle.png");
    revolteTextures.attack = loadTexture("assets/enemies/revolte/attack.png");
    revolteTextures.die = loadTexture("assets/enemies/revolte/die.png");
    revolteTextures.parry = loadTexture("assets/enemies/revolte/parry.png");
    revolteTextures.dash = loadTexture("assets/enemies/revolte/dash.png");
    revolteTextures.executionSlash =
        loadTexture("assets/enemies/revolte/execution-slash.png");
    revolteTextures.executionCrossImpact =
        loadTexture("assets/enemies/revolte/execution-cross-impact.png");
    revolteTextures.flyingBlade =
        loadTexture("assets/enemies/revolte/flying-blade.png");
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
        loadTexture("assets/portraits/frieren_copy/avatar.png"),
        loadTexture("assets/portraits/water_mirror_demon/avatar.png"),
        loadTexture("assets/portraits/water_mirror_demon/die.png"),
        loadTexture("assets/portraits/aura/initial.png"),
        loadTexture("assets/portraits/aura/idle.png"),
        loadTexture("assets/portraits/aura/windup.png"),
        loadTexture("assets/portraits/aura/die.png"),
        {loadTexture("assets/portraits/revolte/avatar1.png"),
            loadTexture("assets/portraits/revolte/avatar2.png"),
            loadTexture("assets/portraits/revolte/avatar3.png")}
    };
    const ArenaTextures arenaTextures = loadArenaTextures("assets/environments/processed");
    const StaircaseTextures staircaseTextures = loadStaircaseTextures("assets/props");
    const std::array<std::optional<sf::Texture>, 3> meteorCardTextures {
        loadTexture("assets/events/half_century_meteor_shower/5101.png"),
        loadTexture("assets/events/half_century_meteor_shower/5102.png"),
        loadTexture("assets/events/half_century_meteor_shower/5103.png")};
    const std::optional<sf::Texture> meteorNpcTexture =
        loadTexture("assets/npcs/meteor_observer.png");
    const std::array<std::optional<sf::Texture>, 3> ordenCardTextures {
        loadTexture("assets/events/lord_orden_ball/5001.png"),
        loadTexture("assets/events/lord_orden_ball/5002.png"),
        loadTexture("assets/events/lord_orden_ball/5003.png")};
    const std::optional<sf::Texture> ordenNpcTexture =
        loadTexture("assets/npcs/lord_orden.png");
    const std::array<std::optional<sf::Texture>, 3> swordVillageCardTextures {
        loadTexture("assets/events/village_of_the_sword/5201.png"),
        loadTexture("assets/events/village_of_the_sword/5202.png"),
        loadTexture("assets/events/village_of_the_sword/5203.png")};
    const std::optional<sf::Texture> swordVillageNpcTexture =
        loadTexture("assets/npcs/sword_village_keeper.png");
    const std::array<std::optional<sf::Texture>, 3> southernHeroCardTextures {
        loadTexture("assets/events/hero_of_the_south/5301.png"),
        loadTexture("assets/events/hero_of_the_south/5302.png"),
        loadTexture("assets/events/hero_of_the_south/5303.png")};
    const std::optional<sf::Texture> southernHeroNpcTexture =
        loadTexture("assets/npcs/hero_of_the_south.png");
    const std::optional<sf::Texture> merchantNpcTexture =
        loadTexture("assets/npcs/merchant.png");
    const std::optional<sf::Texture> startMenuBackgroundTexture =
        loadTexture("assets/ui/start-menu-background.png");
    arcane::presentation::EnemyAnimator enemyAnimator;
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
        demonWarriorTextures, largeBirdDemonTextures,
        gargoyleTextures, gargoyleSkillTextures, swordDemonTextures,
        threeHeadedDemonTextures,
        richterTextures, pillarTextures,
        laufenTextures,
        starkCopyTextures, starkSlashTexture,
        frierenCopyTextures, fernCopyTextures, waterMirrorTextures,
        frierenBeamTextures, frierenLightningTextures,
        frierenFireTexture,
        slashTexture, largeSlashTexture,
        lugnerTextures, lugnerSkillTextures, linieTextures,
        linieSkillTextures, drahtTextures, auraTextures, revolteTextures,
        denkenTextures, tornadoTextures, dialoguePortraits,
        arenaTextures, staircaseTextures, meteorCardTextures, meteorNpcTexture,
        ordenCardTextures, ordenNpcTexture,
        swordVillageCardTextures, swordVillageNpcTexture,
        southernHeroCardTextures, southernHeroNpcTexture,
        merchantNpcTexture,
        startMenuBackgroundTexture,
        enemyAnimator, lootBookAnimator, playerAnimator,
        shadeChargeAnimator, spellCards,
        spellEffectAnimator};
    sf::Clock frameClock;
    ui::CombatFeedbackViewModel combatFeedback;
    arcane::common::binding::ICommandBinding<arcane::common::FrameCommand>& commands = app;
    auto& events = app.eventBinding();
    constexpr std::size_t SmokeStepCount = 12U;
    std::size_t smokeStep = 0U;
    bool smokePassed = !visualSmokeTest || visualSmokeStateMatches(0U,
        app.stateBinding().value(), std::nullopt);
    bool smokeStepObserved = smokePassed;
    bool smokeFirstFrame = false;
    bool smokeFinished = false;
    std::optional<float> smokeInitialPlayerX;
    sf::Clock smokeClock;
    sf::Clock smokeFinishedClock;

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
        const auto& beforeUpdate = app.stateBinding().value();
        if (beforeUpdate.screen == arcane::common::ui::ApplicationScreen::Playing
            && beforeUpdate.combat && beforeUpdate.run
            && beforeUpdate.run->phase == arcane::common::ui::GameplayPhase::InEncounter)
            simulationDeltaSeconds = combatFeedback.combatDeltaSeconds(deltaSeconds);
        arcane::common::InputState frameInput = inputMapper.sample();
        if (visualSmokeTest)
        {
            frameInput = {};
            if (!smokeFinished && smokeClock.getElapsedTime().asSeconds() >= 1.25F)
            {
                if (!smokeStepObserved)
                    std::cerr << "Visual UI smoke test failed at step "
                        << (smokeStep + 1U) << '.\n';
                smokePassed = smokePassed && smokeStepObserved;
                if (smokeStep + 1U < SmokeStepCount)
                {
                    ++smokeStep;
                    smokeStepObserved = false;
                    smokeFirstFrame = true;
                    smokeClock.restart();
                }
                else
                {
                    smokeFinished = true;
                    smokeFinishedClock.restart();
                }
            }
            if (smokeStep == 2U) frameInput.moveAxis = 1.0F;
            if (smokeStep == 3U) frameInput.jumpPressed = true;
            if (smokeStep == 4U) frameInput.attackPressed = true;
            if (smokeStep == 5U) frameInput.dashPressed = true;
            if (smokeFirstFrame)
            {
                if (smokeStep == 1U || smokeStep == 8U || smokeStep == 9U)
                    frameInput.menuConfirmPressed = true;
                else if (smokeStep == 6U) frameInput.pausePressed = true;
                else if (smokeStep == 7U) frameInput.menuDownPressed = true;
                else if (smokeStep == 10U || smokeStep == 11U)
                    frameInput.toggleLoadoutPressed = true;
                smokeFirstFrame = false;
            }
        }
        commands.execute({frameInput, simulationDeltaSeconds});
        if (visualSmokeTest && !smokeFinished)
        {
            if (smokeStep == 1U && !smokeInitialPlayerX
                && app.stateBinding().value().combat)
                smokeInitialPlayerX = app.stateBinding().value().combat->player.position.x;
            smokeStepObserved = smokeStepObserved || visualSmokeStateMatches(smokeStep,
                app.stateBinding().value(), smokeInitialPlayerX);
        }
        const auto& application = app.stateBinding().value();
        for (const auto& event : events.pending())
        {
            if (event.kind == arcane::common::UiEventKind::FloorChanged)
            {
                combatFeedback.reset();
                enemyAnimator.reset();
            }
        }
        if (application.screen == arcane::common::ui::ApplicationScreen::Playing
            && application.combat)
        {
            const float feedbackDelta = !application.loadout ? deltaSeconds : 0.0F;
            combatFeedback.update(application.combat->player,
                application.combat->enemies, feedbackDelta);
            const bool enemyAnimationAdvances = !application.loadout
                && !application.spellAcquisition
                && !application.combat->dialogue
                && !application.combat->bossIntro;
            enemyAnimator.update(application.combat->enemies,
                enemyAnimationAdvances ? simulationDeltaSeconds : 0.0F);
        }
        else
        {
            combatFeedback.reset();
            enemyAnimator.reset();
        }
        const auto presentationState = updatePresentationState(
            application, playerAnimator, simulationDeltaSeconds);
        if (presentationState)
        {
            const bool shadeTimeAdvances = application.screen
                    == arcane::common::ui::ApplicationScreen::Playing
                && !application.loadout && !application.spellAcquisition;
            shadeChargeAnimator.update(*presentationState,
                shadeTimeAdvances ? deltaSeconds : 0.0F);
        }
        else
            shadeChargeAnimator.reset();
        updateWindowTitle(window, app, displayedWindowTitle);
        renderApplicationFrame(window, application, presentationState, renderResources,
            combatFeedback.snapshot());
        if (visualSmokeTest)
            drawVisualSmokeStatus(window, smokeStep, smokeFinished, smokePassed);
        window.display();
        events.acknowledge();
        if (visualSmokeTest && smokeFinished
            && smokeFinishedClock.getElapsedTime().asSeconds() >= 3.0F)
            window.close();
    }

    return !visualSmokeTest || (smokePassed && smokeFinished) ? 0 : 1;
}
