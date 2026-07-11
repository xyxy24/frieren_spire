#include "app/run/AppFlowController.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace arcane::app
{
AppFlowController::AppFlowController(const game::run::Seed seed, TowerSessionConfig config)
    : seed_(seed), config_(std::move(config)) {}

void AppFlowController::update(const game::PlayerIntent& intent, const float deltaSeconds)
{
    switch (screen_)
    {
    case AppScreen::Start:
        if (intent.debugMerchantPreviewPressed)
        {
            startMerchantPreview();
            screen_ = AppScreen::Playing;
        }
        else if (intent.debugEventPreviewPressed)
        {
            startEventPreview();
            screen_ = AppScreen::Playing;
        }
        else if (intent.menuConfirmPressed)
        {
            if (!tower_) startNewRun();
            screen_ = AppScreen::Playing;
        }
        break;
    case AppScreen::Playing:
        if (intent.pausePressed)
        {
            pauseMenuItem_ = PauseMenuItem::ReplayCurrentFloor;
            screen_ = AppScreen::Pause;
            break;
        }
        if (!tower_) throw std::logic_error("playing screen requires a tower session");
        tower_->update(intent, deltaSeconds);
        if (tower_->run().phase() == game::run::RunPhase::Victory
            || tower_->run().phase() == game::run::RunPhase::Defeat)
        {
            victory_ = tower_->run().phase() == game::run::RunPhase::Victory;
            screen_ = AppScreen::Result;
        }
        break;
    case AppScreen::Pause:
        if (intent.pausePressed) screen_ = AppScreen::Playing;
        else if (intent.menuUpPressed || intent.menuDownPressed)
        {
            pauseMenuItem_ = pauseMenuItem_ == PauseMenuItem::ReplayCurrentFloor
                ? PauseMenuItem::SaveAndExit
                : PauseMenuItem::ReplayCurrentFloor;
        }
        else if (intent.menuConfirmPressed)
        {
            if (pauseMenuItem_ == PauseMenuItem::ReplayCurrentFloor)
            {
                if (!tower_) throw std::logic_error("pause screen requires a tower session");
                tower_->restartCurrentFloor();
                screen_ = AppScreen::Playing;
            }
            else
                screen_ = AppScreen::Start;
        }
        else if (intent.menuSecondaryPressed)
        {
            if (!tower_) throw std::logic_error("pause screen requires a tower session");
            tower_->restartCurrentFloor();
            screen_ = AppScreen::Playing;
        }
        break;
    case AppScreen::Result:
        if (intent.menuConfirmPressed)
        {
            startNewRun();
            screen_ = AppScreen::Playing;
        }
        break;
    }
}

AppScreen AppFlowController::screen() const noexcept { return screen_; }
bool AppFlowController::canContinue() const noexcept { return screen_ == AppScreen::Start && tower_.has_value(); }
bool AppFlowController::victory() const noexcept { return victory_; }
PauseMenuItem AppFlowController::pauseMenuItem() const noexcept { return pauseMenuItem_; }
const TowerSession* AppFlowController::tower() const noexcept { return tower_ ? &*tower_ : nullptr; }

void AppFlowController::startNewRun()
{
    tower_.emplace(seed_, config_);
    victory_ = false;
}

void AppFlowController::startEventPreview()
{
    TowerSessionConfig previewConfig = config_;
    previewConfig.firstFloorTypeOverride = game::run::FloorType::Event;
    tower_.emplace(seed_, std::move(previewConfig));
    victory_ = false;
}

void AppFlowController::startMerchantPreview()
{
    TowerSessionConfig previewConfig = config_;
    previewConfig.firstFloorTypeOverride = game::run::FloorType::Merchant;
    previewConfig.initialPlayer.gold = std::max(previewConfig.initialPlayer.gold, 100);
    tower_.emplace(seed_, std::move(previewConfig));
    victory_ = false;
}
}
