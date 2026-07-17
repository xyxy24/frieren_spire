#include "common/input/InputState.hpp"
#include "common/ui/UiStates.hpp"
#include "presentation/viewmodel/ApplicationViewModel.hpp"

#include <iostream>
#include <string_view>

namespace
{
using arcane::common::FrameCommand;
using arcane::common::ui::ApplicationScreen;
using arcane::common::ui::FloorKind;
using arcane::common::ui::PauseMenuItem;
using arcane::presentation::viewmodel::ApplicationViewModel;

bool expect(const bool condition, const std::string_view message)
{
    if (!condition) std::cerr << "FAILED: " << message << '\n';
    return condition;
}

void send(ApplicationViewModel& application, arcane::common::InputState input,
    const float deltaSeconds = 0.01F)
{
    application.execute(FrameCommand {input, deltaSeconds});
}

bool startPauseExitAndContinueFlowIsProjected()
{
    ApplicationViewModel application {0x51A7E123U};
    const auto& state = application.stateBinding();

    if (!expect(state.value().screen == ApplicationScreen::Start
            && !state.value().canContinue && !state.value().run,
            "a fresh application must show START without a resumable run")) return false;

    arcane::common::InputState input;
    input.menuConfirmPressed = true;
    send(application, input);
    const auto startedFloor = state.value().run ? state.value().run->floorIndex : 999U;
    const auto startedSeed = state.value().run ? state.value().run->runSeed : 0U;
    if (!expect(state.value().screen == ApplicationScreen::Playing
            && state.value().run && state.value().combat,
            "confirming START must publish a playable combat scene")) return false;

    input = {};
    input.pausePressed = true;
    send(application, input);
    if (!expect(state.value().screen == ApplicationScreen::Pause
            && state.value().pauseMenuItem == PauseMenuItem::ReplayCurrentFloor,
            "pause must open with replay-current-floor selected")) return false;

    input = {};
    input.menuDownPressed = true;
    send(application, input);
    if (!expect(state.value().pauseMenuItem == PauseMenuItem::SaveAndExit,
            "pause navigation must select save-and-exit")) return false;

    input = {};
    input.menuConfirmPressed = true;
    send(application, input);
    if (!expect(state.value().screen == ApplicationScreen::Start
            && state.value().canContinue && state.value().run,
            "save-and-exit must return to a CONTINUE-capable start screen")) return false;

    send(application, input);
    return expect(state.value().screen == ApplicationScreen::Playing
            && state.value().run && state.value().run->floorIndex == startedFloor
            && state.value().run->runSeed == startedSeed,
        "CONTINUE must preserve the current run and floor");
}

bool specialFloorPreviewsExposeTheCorrectUiModel()
{
    ApplicationViewModel eventApplication {0xE7E17U};
    arcane::common::InputState input;
    input.debugEventPreviewPressed = true;
    send(eventApplication, input);
    const auto& eventState = eventApplication.stateBinding().value();
    if (!expect(eventState.screen == ApplicationScreen::Playing && eventState.run
            && eventState.run->floorKind == FloorKind::Event && eventState.specialFloor
            && eventState.specialFloor->floorKind == FloorKind::Event && !eventState.combat,
            "event preview must expose event-room UI state without a combat scene")) return false;

    ApplicationViewModel merchantApplication {0x5A0FU};
    input = {};
    input.debugMerchantPreviewPressed = true;
    send(merchantApplication, input);
    const auto& merchantState = merchantApplication.stateBinding().value();
    return expect(merchantState.screen == ApplicationScreen::Playing && merchantState.run
            && merchantState.run->floorKind == FloorKind::Merchant
            && merchantState.specialFloor
            && merchantState.specialFloor->floorKind == FloorKind::Merchant
            && merchantState.run->gold >= 100 && !merchantState.combat,
        "merchant preview must expose merchant-room UI state and preview funds");
}

bool loadoutOverlayTogglesWithoutReplacingGameplayState()
{
    ApplicationViewModel application {0x10AD0U};
    arcane::common::InputState input;
    input.menuConfirmPressed = true;
    send(application, input);

    input = {};
    input.toggleLoadoutPressed = true;
    send(application, input);
    const auto& openState = application.stateBinding().value();
    if (!expect(openState.screen == ApplicationScreen::Playing && openState.combat
            && openState.loadout && openState.equippedSlots,
            "opening loadout must layer its UI model over the active gameplay scene")) return false;

    send(application, input);
    const auto& closedState = application.stateBinding().value();
    return expect(closedState.screen == ApplicationScreen::Playing && closedState.combat
            && !closedState.loadout && closedState.equippedSlots,
        "closing loadout must restore gameplay UI while retaining equipped-slot HUD state");
}
}

int main()
{
    if (!startPauseExitAndContinueFlowIsProjected()
        || !specialFloorPreviewsExposeTheCorrectUiModel()
        || !loadoutOverlayTogglesWithoutReplacingGameplayState()) return 1;
    std::cout << "All UI view-model tests passed.\n";
    return 0;
}
