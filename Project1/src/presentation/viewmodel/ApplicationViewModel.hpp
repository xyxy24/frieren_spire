#pragma once

#include "app/run/TowerSession.hpp"
#include "presentation/viewmodel/LoadoutViewModel.hpp"
#include "presentation/viewmodel/SpellAcquisitionViewModel.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace arcane::presentation::viewmodel
{
enum class ApplicationScreen : std::uint8_t { Start, Playing, Pause, Result };
enum class PauseMenuItem : std::uint8_t { ReplayCurrentFloor, SaveAndExit };

struct ApplicationSnapshot
{
    ApplicationScreen screen {ApplicationScreen::Start};
    PauseMenuItem pauseMenuItem {PauseMenuItem::ReplayCurrentFloor};
    bool canContinue {};
    bool victory {};
};

class ApplicationViewModel
{
public:
    explicit ApplicationViewModel(game::run::Seed seed, app::TowerSessionConfig config = {});

    void update(const game::PlayerIntent& intent, float deltaSeconds);

    [[nodiscard]] ApplicationSnapshot snapshot() const noexcept;
    [[nodiscard]] const app::TowerSession* model() const noexcept;
    [[nodiscard]] const LoadoutViewModel& loadout() const noexcept;
    [[nodiscard]] const SpellAcquisitionViewModel& spellAcquisition() const noexcept;
    [[nodiscard]] std::optional<RewardViewModel> reward() const;
    [[nodiscard]] std::optional<BreakthroughViewModel> breakthrough() const noexcept;
    [[nodiscard]] std::optional<MerchantViewModel> merchant() const;
    [[nodiscard]] std::optional<EquippedSlotsViewModel> equippedSlots() const noexcept;

private:
    void handleStart(const game::PlayerIntent& intent);
    void handlePlaying(const game::PlayerIntent& intent, float deltaSeconds);
    void handlePause(const game::PlayerIntent& intent);
    void handleResult(const game::PlayerIntent& intent);
    void startNewRun();
    void startEventPreview();
    void startMerchantPreview();
    void startSpellAcquisitionPreview();
    void startBossPreview(std::size_t bossIndex);

    game::run::Seed seed_;
    app::TowerSessionConfig config_;
    std::optional<app::TowerSession> model_;
    LoadoutViewModel loadout_;
    SpellAcquisitionViewModel spellAcquisition_;
    ApplicationScreen screen_ {ApplicationScreen::Start};
    PauseMenuItem pauseMenuItem_ {PauseMenuItem::ReplayCurrentFloor};
    bool victory_ {};
};
}
