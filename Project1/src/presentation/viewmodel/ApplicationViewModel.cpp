#include "presentation/viewmodel/ApplicationViewModel.hpp"

#include <algorithm>
#include <iterator>
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
}

ApplicationViewModel::ApplicationViewModel(
    const game::run::Seed seed, app::TowerSessionConfig config)
    : seed_(seed), config_(std::move(config)) {}

void ApplicationViewModel::update(
    const game::PlayerIntent& intent, const float deltaSeconds)
{
    switch (screen_)
    {
    case ApplicationScreen::Start: handleStart(intent); break;
    case ApplicationScreen::Playing: handlePlaying(intent, deltaSeconds); break;
    case ApplicationScreen::Pause: handlePause(intent); break;
    case ApplicationScreen::Result: handleResult(intent); break;
    }
}

ApplicationSnapshot ApplicationViewModel::snapshot() const noexcept
{
    return {screen_, pauseMenuItem_, screen_ == ApplicationScreen::Start && model_.has_value(),
        victory_};
}

const app::TowerSession* ApplicationViewModel::model() const noexcept
{
    return model_ ? &*model_ : nullptr;
}

const LoadoutViewModel& ApplicationViewModel::loadout() const noexcept
{
    return loadout_;
}

const SpellAcquisitionViewModel& ApplicationViewModel::spellAcquisition() const noexcept
{
    return spellAcquisition_;
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
    }
    else if (player.learnedSpells.size() > regularCount)
    {
        loadout_.selectNewestRegular(player);
        spellAcquisition_.start(player.learnedSpells.back());
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
            spellAcquisition_.start(upgraded->spellId, upgraded->rank);
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
}
