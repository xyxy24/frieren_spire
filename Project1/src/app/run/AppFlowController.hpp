#pragma once

#include "app/run/TowerSession.hpp"

#include <optional>

namespace arcane::app
{
enum class AppScreen : std::uint8_t { Start, Playing, Pause, Result };

class AppFlowController
{
public:
    explicit AppFlowController(game::run::Seed seed, TowerSessionConfig config = {});
    void update(const game::PlayerIntent& intent, float deltaSeconds);
    [[nodiscard]] AppScreen screen() const noexcept;
    [[nodiscard]] bool canContinue() const noexcept;
    [[nodiscard]] bool victory() const noexcept;
    [[nodiscard]] const TowerSession* tower() const noexcept;

private:
    void startNewRun();
    game::run::Seed seed_;
    TowerSessionConfig config_;
    std::optional<TowerSession> tower_;
    AppScreen screen_ {AppScreen::Start};
    bool victory_ {};
};
}
