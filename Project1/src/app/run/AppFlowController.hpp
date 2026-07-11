#pragma once

#include "app/run/TowerSession.hpp"

#include <optional>

namespace arcane::app
{
enum class AppScreen : std::uint8_t { Start, Playing, Pause, Result };
enum class PauseMenuItem : std::uint8_t { ReplayCurrentFloor, SaveAndExit };

class AppFlowController
{
public:
    explicit AppFlowController(game::run::Seed seed, TowerSessionConfig config = {});
    void update(const game::PlayerIntent& intent, float deltaSeconds);
    [[nodiscard]] AppScreen screen() const noexcept;
    [[nodiscard]] bool canContinue() const noexcept;
    [[nodiscard]] bool victory() const noexcept;
    [[nodiscard]] PauseMenuItem pauseMenuItem() const noexcept;
    [[nodiscard]] const TowerSession* tower() const noexcept;

private:
    void startNewRun();
    void startEventPreview();
    void startMerchantPreview();
    game::run::Seed seed_;
    TowerSessionConfig config_;
    std::optional<TowerSession> tower_;
    AppScreen screen_ {AppScreen::Start};
    PauseMenuItem pauseMenuItem_ {PauseMenuItem::ReplayCurrentFloor};
    bool victory_ {};
};
}
