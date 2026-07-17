#pragma once

#include "app/run/TowerSession.hpp"
#include "common/binding/Bindings.hpp"
#include "common/events/UiEvent.hpp"
#include "common/input/InputState.hpp"
#include "common/ui/UiStates.hpp"
#include "presentation/viewmodel/LoadoutViewModel.hpp"
#include "presentation/viewmodel/SpellAcquisitionViewModel.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace arcane::presentation::viewmodel
{
using ApplicationScreen = common::ui::ApplicationScreen;
using PauseMenuItem = common::ui::PauseMenuItem;
using ApplicationSnapshot = common::ui::ApplicationState;

class ApplicationViewModel final : public common::binding::ICommandBinding<common::FrameCommand>
{
public:
    explicit ApplicationViewModel(game::run::Seed seed, app::TowerSessionConfig config = {});

    [[nodiscard]] bool canExecute(const common::FrameCommand& command) const noexcept override;
    void execute(const common::FrameCommand& command) override;

    [[nodiscard]] const common::binding::IReadOnlyProperty<ApplicationSnapshot>&
        stateBinding() const noexcept;
    [[nodiscard]] common::binding::IEventBinding<common::UiEvent>& eventBinding() noexcept;

private:
    void update(const common::InputState& intent, float deltaSeconds);
    [[nodiscard]] std::optional<RewardViewModel> reward() const;
    [[nodiscard]] std::optional<BreakthroughViewModel> breakthrough() const noexcept;
    [[nodiscard]] std::optional<MerchantViewModel> merchant() const;
    [[nodiscard]] std::optional<EquippedSlotsViewModel> equippedSlots() const noexcept;

    void handleStart(const game::PlayerIntent& intent);
    void handlePlaying(const game::PlayerIntent& intent, float deltaSeconds);
    void handlePause(const game::PlayerIntent& intent);
    void handleResult(const game::PlayerIntent& intent);
    void startNewRun();
    void startEventPreview();
    void startMerchantPreview();
    void startSpellAcquisitionPreview();
    void startBossPreview(std::size_t bossIndex);
    void refreshState();

    game::run::Seed seed_;
    app::TowerSessionConfig config_;
    std::optional<app::TowerSession> model_;
    LoadoutViewModel loadout_;
    SpellAcquisitionViewModel spellAcquisition_;
    ApplicationScreen screen_ {ApplicationScreen::Start};
    PauseMenuItem pauseMenuItem_ {PauseMenuItem::ReplayCurrentFloor};
    bool victory_ {};
    common::binding::ObservableProperty<ApplicationSnapshot> state_;
    common::binding::EventChannel<common::UiEvent> events_;
};
}
