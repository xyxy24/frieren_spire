#include "presentation/viewmodel/ApplicationViewModel.hpp"

#include <algorithm>
#include <iterator>
#include <string>
#include <stdexcept>
#include <utility>

namespace arcane::presentation::viewmodel
{
namespace
{
void configureBossPreviewPlayer(game::run::PlayerProgress& player, const std::size_t bossIndex)
{
    player.learnedSpells = {1004U, 1005U, 1006U};
    if (bossIndex >= 1U)
        player.learnedSpells.insert(player.learnedSpells.end(), {1016U, 1017U, 1028U});
    if (bossIndex >= 2U)
        player.learnedSpells.insert(player.learnedSpells.end(), {1024U, 1025U, 1026U});
    player.equippedSpells = {1004U, 1005U, 1006U};
    player.spellMasteries.clear();

    player.learnedBossSpells.clear();
    player.equippedUltimateSpell.reset();
    player.ultimateSpellUnlocked = bossIndex > 0U;
    if (bossIndex >= 1U)
    {
        player.learnedBossSpells.push_back(2001U);
        player.equippedUltimateSpell = 2001U;
    }
    if (bossIndex >= 2U)
    {
        player.learnedBossSpells.push_back(2007U);
        player.equippedUltimateSpell = 2007U;
    }
}

common::ui::GameplayPhase toGameplayPhase(const game::run::RunPhase phase) noexcept
{
    using Source = game::run::RunPhase;
    using Target = common::ui::GameplayPhase;
    switch (phase)
    {
    case Source::FloorLoading: return Target::FloorLoading;
    case Source::InEncounter: return Target::InEncounter;
    case Source::LootPending: return Target::LootPending;
    case Source::Reward: return Target::Reward;
    case Source::Breakthrough: return Target::Breakthrough;
    case Source::FloorComplete: return Target::FloorComplete;
    case Source::Defeat: return Target::Defeat;
    case Source::Victory: return Target::Victory;
    }
    return Target::FloorLoading;
}

common::ui::FloorKind toFloorKind(const game::run::FloorType type) noexcept
{
    using Source = game::run::FloorType;
    using Target = common::ui::FloorKind;
    switch (type)
    {
    case Source::Combat: return Target::Combat;
    case Source::Event: return Target::Event;
    case Source::Merchant: return Target::Merchant;
    case Source::Boss: return Target::Boss;
    }
    return Target::Combat;
}

common::ui::EventKind toEventKind(const app::EventKind kind) noexcept
{
    using Source = app::EventKind;
    using Target = common::ui::EventKind;
    switch (kind)
    {
    case Source::AldenBall: return Target::AldenBall;
    case Source::HalfCenturyMeteorShower: return Target::HalfCenturyMeteorShower;
    case Source::SwordVillage: return Target::SwordVillage;
    case Source::SouthernHero: return Target::SouthernHero;
    }
    return Target::AldenBall;
}

common::ui::EventFloorState toEventFloorState(const app::EventFloorState state) noexcept
{
    using Source = app::EventFloorState;
    using Target = common::ui::EventFloorState;
    switch (state)
    {
    case Source::Untriggered: return Target::Untriggered;
    case Source::Choosing: return Target::Choosing;
    case Source::Result: return Target::Result;
    }
    return Target::Untriggered;
}

common::ui::PlayerVisualState makeExplorationVisual(
    const game::PlayerController& player, const int health) noexcept
{
    return {player.position(), player.velocity(), health, player.isGrounded(),
        player.facingDirection(), 0U, 0U, 0U, player.dashRemaining(),
        player.shadowDashChargeRemaining(), player.isShadowDashing(), player.isStunned()};
}

std::string makeBoundWindowTitle(const common::ui::ApplicationState& state)
{
    using common::ui::ApplicationScreen;
    if (state.screen == ApplicationScreen::Start)
        return state.canContinue
            ? "Arcane Spire | CONTINUE - Enter | F2 Event | F3 Shop | F4 Spell | F5-F7 Boss"
            : "Arcane Spire | START - Enter | F2 Event | F3 Shop | F4 Spell | F5-F7 Boss";
    if (state.screen == ApplicationScreen::Pause)
        return "Arcane Spire | PAUSE - W/S Select | Enter Confirm | Esc Resume";
    if (state.screen == ApplicationScreen::Result)
        return state.victory ? "Arcane Spire | WIN - Enter Start"
            : "Arcane Spire | DEFEAT - Enter Start";
    if (!state.run) return "Arcane Spire";

    const auto& run = *state.run;
    std::string title = "Arcane Spire | Floor " + std::to_string(run.floorIndex + 1U)
        + " | Seed " + std::to_string(run.runSeed)
        + " | HP " + std::to_string(run.currentHp) + "/" + std::to_string(run.maximumHp)
        + " | Gold " + std::to_string(run.gold)
        + " | Boss " + std::to_string(run.bossesDefeated) + "/3 | ";
    if (state.spellAcquisition)
    {
        const auto& acquisition = *state.spellAcquisition;
        return title + (acquisition.bossSpell ? "ULTIMATE SPELL ACQUIRED - " : "SPELL ACQUIRED - ")
            + std::string {acquisition.content.summary.name}
            + (acquisition.canDismiss ? " | Enter Continue"
                : acquisition.canSkip ? " | Space Skip" : " | Unlocking...");
    }
    if (state.loadout)
    {
        const auto& loadout = *state.loadout;
        const std::string selected = loadout.selectedDetail
            ? std::string {loadout.selectedDetail->summary.name} : std::string {"None"};
        if (loadout.page == common::ui::LoadoutPage::Relics)
            return title + "RELIC COLLECTION - Selected " + selected
                + " | A/D Select, Q/E Spell Page, Tab Close";
        const bool boss = loadout.spellSection == common::ui::SpellSection::Boss;
        return title + (boss ? "BOSS SPELL LOADOUT - Selected " : "REGULAR SPELL LOADOUT - Selected ")
            + selected + (boss ? " | A/D Select, R Equip Ultimate"
                : " | A/D Select, U/I/O Equip Slot")
            + ", W/S Switch Spell Group, Q/E Relic Page, Tab Close";
    }
    using common::ui::GameplayPhase;
    switch (run.phase)
    {
    case GameplayPhase::InEncounter:
        if (run.floorKind == common::ui::FloorKind::Merchant)
            return title + (run.specialPanelOpen
                ? "MERCHANT - WASD Select | Enter Buy | E Close"
                : "MERCHANT ROOM - Meet NPC, E Trade | Rear Staircase Exits");
        if (run.floorKind == common::ui::FloorKind::Event)
            return title + (run.specialPanelOpen ? "EVENT CHOICE OR RESULT - U I O Select | E Close"
                : "EVENT ROOM - Meet NPC, E Interact");
        return title + "A/D Move, Space Jump, S+Space Drop, J Attack, K Dash, U/I/O Spells, R Ultimate, Tab Loadout";
    case GameplayPhase::LootPending:
        return title + "ENEMY DROP - Move To Drop, E Inspect Reward | Tab Loadout";
    case GameplayPhase::Reward:
        return title + "CHOOSE ONE SPELL - U I O | Tab Loadout";
    case GameplayPhase::Breakthrough:
        return title + "Choose Mana Breakthrough: U Power, I Haste, O Defense";
    case GameplayPhase::FloorComplete:
        return title + "Move Into Staircase, E Climb (+50% Missing HP), Tab Loadout";
    case GameplayPhase::Victory: return title + "VICTORY - Third Boss Defeated";
    case GameplayPhase::Defeat: return title + "DEFEAT";
    case GameplayPhase::FloorLoading: return title + "Loading";
    }
    return title;
}
}

ApplicationViewModel::ApplicationViewModel(
    const game::run::Seed seed, app::TowerSessionConfig config)
    : seed_(seed), config_(std::move(config))
{
    refreshState();
}

void ApplicationViewModel::update(
    const common::InputState& intent, const float deltaSeconds)
{
    const ApplicationScreen previousScreen = screen_;
    const auto previousFloor = model_
        ? std::optional {model_->run().context().floorIndex} : std::nullopt;
    const auto previousPhase = model_
        ? std::optional {model_->run().phase()} : std::nullopt;
    switch (screen_)
    {
    case ApplicationScreen::Start: handleStart(intent); break;
    case ApplicationScreen::Playing: handlePlaying(intent, deltaSeconds); break;
    case ApplicationScreen::Pause: handlePause(intent); break;
    case ApplicationScreen::Result: handleResult(intent); break;
    }
    if (screen_ != previousScreen)
        events_.publish({common::UiEventKind::ScreenChanged,
            static_cast<std::uint32_t>(screen_)});
    if (model_ && (!previousFloor || *previousFloor != model_->run().context().floorIndex))
        events_.publish({common::UiEventKind::FloorChanged,
            model_->run().context().floorIndex});
    if (model_ && (!previousPhase || *previousPhase != model_->run().phase())
        && (model_->run().phase() == game::run::RunPhase::Victory
            || model_->run().phase() == game::run::RunPhase::Defeat))
        events_.publish({common::UiEventKind::RunEnded,
            model_->run().phase() == game::run::RunPhase::Victory ? 1U : 0U});
    refreshState();
}

bool ApplicationViewModel::canExecute(const common::FrameCommand&) const noexcept
{
    return true;
}

void ApplicationViewModel::execute(const common::FrameCommand& command)
{
    common::InputState combined = command.input;
    combined.toggleLoadoutPressed = combined.toggleLoadoutPressed
        || pendingUiInput_.toggleLoadoutPressed;
    combined.menuPreviousPressed = combined.menuPreviousPressed
        || pendingUiInput_.menuPreviousPressed;
    combined.menuNextPressed = combined.menuNextPressed
        || pendingUiInput_.menuNextPressed;
    combined.menuPagePreviousPressed = combined.menuPagePreviousPressed
        || pendingUiInput_.menuPagePreviousPressed;
    combined.menuPageNextPressed = combined.menuPageNextPressed
        || pendingUiInput_.menuPageNextPressed;
    combined.menuUpPressed = combined.menuUpPressed || pendingUiInput_.menuUpPressed;
    combined.menuDownPressed = combined.menuDownPressed || pendingUiInput_.menuDownPressed;
    combined.pausePressed = combined.pausePressed || pendingUiInput_.pausePressed;
    combined.menuConfirmPressed = combined.menuConfirmPressed
        || pendingUiInput_.menuConfirmPressed;
    combined.menuSecondaryPressed = combined.menuSecondaryPressed
        || pendingUiInput_.menuSecondaryPressed;
    combined.debugEventPreviewPressed = combined.debugEventPreviewPressed
        || pendingUiInput_.debugEventPreviewPressed;
    combined.debugMerchantPreviewPressed = combined.debugMerchantPreviewPressed
        || pendingUiInput_.debugMerchantPreviewPressed;
    combined.debugSpellAcquisitionPreviewPressed = combined.debugSpellAcquisitionPreviewPressed
        || pendingUiInput_.debugSpellAcquisitionPreviewPressed;
    for (std::size_t index = 0; index < combined.debugBossPreviewPressed.size(); ++index)
    {
        combined.debugBossPreviewPressed[index] = combined.debugBossPreviewPressed[index]
            || pendingUiInput_.debugBossPreviewPressed[index];
    }
    pendingUiInput_ = {};
    update(combined, command.deltaSeconds);
}

bool ApplicationViewModel::canExecute(const common::UiCommand&) const noexcept
{
    return true;
}

void ApplicationViewModel::execute(const common::UiCommand& command)
{
    using common::UiAction;
    switch (command.action)
    {
    case UiAction::ToggleLoadout: pendingUiInput_.toggleLoadoutPressed = true; break;
    case UiAction::SelectPrevious: pendingUiInput_.menuPreviousPressed = true; break;
    case UiAction::SelectNext: pendingUiInput_.menuNextPressed = true; break;
    case UiAction::PreviousPage: pendingUiInput_.menuPagePreviousPressed = true; break;
    case UiAction::NextPage: pendingUiInput_.menuPageNextPressed = true; break;
    case UiAction::SelectUp: pendingUiInput_.menuUpPressed = true; break;
    case UiAction::SelectDown: pendingUiInput_.menuDownPressed = true; break;
    case UiAction::TogglePause: pendingUiInput_.pausePressed = true; break;
    case UiAction::Confirm: pendingUiInput_.menuConfirmPressed = true; break;
    case UiAction::Secondary: pendingUiInput_.menuSecondaryPressed = true; break;
    case UiAction::PreviewEvent: pendingUiInput_.debugEventPreviewPressed = true; break;
    case UiAction::PreviewMerchant: pendingUiInput_.debugMerchantPreviewPressed = true; break;
    case UiAction::PreviewSpellAcquisition:
        pendingUiInput_.debugSpellAcquisitionPreviewPressed = true;
        break;
    case UiAction::PreviewBossOne: pendingUiInput_.debugBossPreviewPressed[0] = true; break;
    case UiAction::PreviewBossTwo: pendingUiInput_.debugBossPreviewPressed[1] = true; break;
    case UiAction::PreviewBossThree: pendingUiInput_.debugBossPreviewPressed[2] = true; break;
    }
}

const common::binding::IReadOnlyProperty<ApplicationSnapshot>&
ApplicationViewModel::stateBinding() const noexcept
{
    return state_;
}

common::binding::IEventBinding<common::UiEvent>& ApplicationViewModel::eventBinding() noexcept
{
    return events_;
}

std::optional<RewardViewModel> ApplicationViewModel::reward() const
{
    if (!model_) return std::nullopt;
    const auto candidates = model_->rewardCandidates();
    if (!candidates) return std::nullopt;
    return makeRewardViewModel(*candidates, model_->run().player());
}

std::optional<BreakthroughViewModel> ApplicationViewModel::breakthrough() const noexcept
{
    if (!model_ || model_->run().phase() != game::run::RunPhase::Breakthrough)
        return std::nullopt;
    return makeBreakthroughViewModel(model_->run().player());
}

std::optional<MerchantViewModel> ApplicationViewModel::merchant() const
{
    if (!model_ || model_->currentFloorType() != game::run::FloorType::Merchant
        || !model_->specialPanelOpen()) return std::nullopt;
    return makeMerchantViewModel(model_->merchantStock(), model_->selectedMerchantItem());
}

std::optional<EquippedSlotsViewModel> ApplicationViewModel::equippedSlots() const noexcept
{
    if (!model_) return std::nullopt;
    return makeEquippedSlotsViewModel(model_->run().player());
}

void ApplicationViewModel::handleStart(const game::PlayerIntent& intent)
{
    if (intent.debugSpellAcquisitionPreviewPressed)
    {
        startSpellAcquisitionPreview();
        screen_ = ApplicationScreen::Playing;
    }
    else if (const auto boss = std::find(intent.debugBossPreviewPressed.begin(),
                 intent.debugBossPreviewPressed.end(), true);
        boss != intent.debugBossPreviewPressed.end())
    {
        startBossPreview(static_cast<std::size_t>(
            std::distance(intent.debugBossPreviewPressed.begin(), boss)));
        screen_ = ApplicationScreen::Playing;
    }
    else if (intent.debugMerchantPreviewPressed)
    {
        startMerchantPreview();
        screen_ = ApplicationScreen::Playing;
    }
    else if (intent.debugEventPreviewPressed)
    {
        startEventPreview();
        screen_ = ApplicationScreen::Playing;
    }
    else if (intent.menuConfirmPressed)
    {
        if (!model_) startNewRun();
        screen_ = ApplicationScreen::Playing;
    }
}

void ApplicationViewModel::handlePlaying(
    const game::PlayerIntent& intent, const float deltaSeconds)
{
    if (spellAcquisition_.active())
    {
        spellAcquisition_.update(intent, deltaSeconds);
        return;
    }
    if (intent.pausePressed)
    {
        pauseMenuItem_ = PauseMenuItem::ReplayCurrentFloor;
        screen_ = ApplicationScreen::Pause;
        return;
    }
    if (!model_) throw std::logic_error("playing screen requires a tower model");
    if (loadout_.handleInput(intent, *model_)) return;

    const auto regularCount = model_->run().player().learnedSpells.size();
    const auto bossCount = model_->run().player().learnedBossSpells.size();
    const auto masteriesBefore = model_->run().player().spellMasteries;
    model_->update(intent, deltaSeconds);
    const auto& player = model_->run().player();
    if (player.learnedBossSpells.size() > bossCount)
    {
        loadout_.selectNewestBoss(player);
        spellAcquisition_.start(player.learnedBossSpells.back());
        events_.publish({common::UiEventKind::SpellAcquired, player.learnedBossSpells.back()});
    }
    else if (player.learnedSpells.size() > regularCount)
    {
        loadout_.selectNewestRegular(player);
        spellAcquisition_.start(player.learnedSpells.back());
        events_.publish({common::UiEventKind::SpellAcquired, player.learnedSpells.back()});
    }
    else
    {
        const auto previousRank = [&](const game::run::ContentId id) {
            const auto found = std::find_if(masteriesBefore.begin(), masteriesBefore.end(),
                [id](const auto& mastery) { return mastery.spellId == id; });
            return found == masteriesBefore.end() ? std::uint8_t {1U} : found->rank;
        };
        const auto upgraded = std::find_if(player.spellMasteries.begin(),
            player.spellMasteries.end(), [&](const auto& mastery) {
                return mastery.rank > previousRank(mastery.spellId);
            });
        if (upgraded != player.spellMasteries.end())
        {
            spellAcquisition_.start(upgraded->spellId, upgraded->rank);
            events_.publish({common::UiEventKind::SpellAcquired, upgraded->spellId});
        }
    }

    if (model_->run().phase() == game::run::RunPhase::Victory
        || model_->run().phase() == game::run::RunPhase::Defeat)
    {
        victory_ = model_->run().phase() == game::run::RunPhase::Victory;
        screen_ = ApplicationScreen::Result;
    }
}

void ApplicationViewModel::handlePause(const game::PlayerIntent& intent)
{
    if (intent.pausePressed)
    {
        screen_ = ApplicationScreen::Playing;
        return;
    }
    if (intent.menuUpPressed || intent.menuDownPressed)
    {
        pauseMenuItem_ = pauseMenuItem_ == PauseMenuItem::ReplayCurrentFloor
            ? PauseMenuItem::SaveAndExit : PauseMenuItem::ReplayCurrentFloor;
        return;
    }
    if (intent.menuConfirmPressed)
    {
        if (pauseMenuItem_ == PauseMenuItem::SaveAndExit)
        {
            screen_ = ApplicationScreen::Start;
            return;
        }
        if (!model_) throw std::logic_error("pause screen requires a tower model");
        model_->restartCurrentFloor();
        screen_ = ApplicationScreen::Playing;
        return;
    }
    if (intent.menuSecondaryPressed)
    {
        if (!model_) throw std::logic_error("pause screen requires a tower model");
        model_->restartCurrentFloor();
        screen_ = ApplicationScreen::Playing;
    }
}

void ApplicationViewModel::handleResult(const game::PlayerIntent& intent)
{
    if (!intent.menuConfirmPressed) return;
    startNewRun();
    screen_ = ApplicationScreen::Playing;
}

void ApplicationViewModel::startNewRun()
{
    model_.emplace(seed_, config_);
    loadout_.reset();
    spellAcquisition_.reset();
    victory_ = false;
}

void ApplicationViewModel::startEventPreview()
{
    app::TowerSessionConfig previewConfig = config_;
    previewConfig.firstFloorTypeOverride = game::run::FloorType::Event;
    model_.emplace(seed_, std::move(previewConfig));
    loadout_.reset();
    spellAcquisition_.reset();
    victory_ = false;
}

void ApplicationViewModel::startMerchantPreview()
{
    app::TowerSessionConfig previewConfig = config_;
    previewConfig.firstFloorTypeOverride = game::run::FloorType::Merchant;
    previewConfig.initialPlayer.gold = std::max(previewConfig.initialPlayer.gold, 100);
    model_.emplace(seed_, std::move(previewConfig));
    loadout_.reset();
    spellAcquisition_.reset();
    victory_ = false;
}

void ApplicationViewModel::startSpellAcquisitionPreview()
{
    startNewRun();
    spellAcquisition_.start(1001U);
}

void ApplicationViewModel::startBossPreview(const std::size_t bossIndex)
{
    if (bossIndex >= 3U) throw std::out_of_range("boss preview index is outside the tower");

    app::TowerSessionConfig previewConfig = config_;
    previewConfig.enableSpecialFloors = false;
    previewConfig.firstFloorTypeOverride.reset();
    previewConfig.startPosition = {
        static_cast<std::uint32_t>((bossIndex + 1U) * previewConfig.floorsPerBoss - 1U),
        static_cast<std::uint32_t>(bossIndex + 1U),
        static_cast<std::uint32_t>(bossIndex)};
    configureBossPreviewPlayer(previewConfig.initialPlayer, bossIndex);
    model_.emplace(seed_, std::move(previewConfig));
    loadout_.reset();
    spellAcquisition_.reset();
    victory_ = false;
}

void ApplicationViewModel::refreshState()
{
    ApplicationSnapshot next;
    next.screen = screen_;
    next.pauseMenuItem = pauseMenuItem_;
    next.canContinue = screen_ == ApplicationScreen::Start && model_.has_value();
    next.victory = victory_;
    if (!model_)
    {
        next.windowTitle = makeBoundWindowTitle(next);
        state_.publish(std::move(next));
        return;
    }

    const auto& tower = *model_;
    const auto& run = tower.run();
    const auto& player = run.player();
    next.run = common::ui::RunHeaderState {run.context().runSeed,
        run.context().floorIndex, run.context().act, run.context().bossesDefeated,
        toGameplayPhase(run.phase()), toFloorKind(tower.currentFloorType()),
        player.currentHp, player.maxHp, player.gold, tower.specialPanelOpen()};
    next.equippedSlots = makeEquippedSlotsViewModel(player);
    if (loadout_.open()) next.loadout = loadout_.snapshot(player);
    if (const auto acquisition = spellAcquisition_.snapshot())
        next.spellAcquisition = *acquisition;
    if (const auto rewardState = reward()) next.reward = *rewardState;
    if (const auto breakthroughState = breakthrough()) next.breakthrough = *breakthroughState;
    if (const auto merchantState = merchant()) next.merchant = *merchantState;

    if (const auto* combat = tower.combat())
    {
        common::ui::CombatSceneState scene;
        scene.arena = tower.arenaLayout();
        scene.staircase = {tower.staircaseBounds(), tower.staircaseUnlocked(),
            tower.arenaLayout().theme};
        scene.player = combat->playerState();
        combat->populateEnemyStates(scene.enemies);
        combat->populateSpellEffects(scene.spellEffects);
        scene.attackBounds = combat->attackBounds();
        scene.dialogue = combat->dialogueLine();
        scene.bossIntro = combat->bossIntro();
        scene.lootDrop = tower.lootDropBounds();
        next.run->currentHp = scene.player.currentHealth;
        next.combat = std::move(scene);
    }
    else if (tower.currentFloorType() == game::run::FloorType::Merchant
        || tower.currentFloorType() == game::run::FloorType::Event)
    {
        common::ui::SpecialFloorState special;
        special.arena = tower.arenaLayout();
        special.staircase = {tower.staircaseBounds(), tower.staircaseUnlocked(),
            tower.arenaLayout().theme};
        if (const auto* exploration = tower.explorationPlayer())
            special.player = makeExplorationVisual(*exploration, player.currentHp);
        special.npcBounds = tower.npcBounds();
        special.floorKind = toFloorKind(tower.currentFloorType());
        special.event.open = tower.specialPanelOpen();
        special.event.kind = toEventKind(tower.eventKind());
        special.event.state = toEventFloorState(tower.eventFloorState());
        special.event.resultChoice = tower.eventResultChoice();
        const auto choices = tower.eventChoices();
        for (std::size_t index = 0U; index < special.event.choices.size(); ++index)
            special.event.choices[index] = {choices[index].id, choices[index].goldDelta};
        next.specialFloor = std::move(special);
    }
    next.windowTitle = makeBoundWindowTitle(next);
    state_.publish(std::move(next));
}
}
