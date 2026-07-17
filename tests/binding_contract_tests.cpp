#include "common/binding/Bindings.hpp"
#include "common/events/UiEvent.hpp"
#include "common/input/InputState.hpp"
#include "common/ui/UiStates.hpp"
#include "presentation/viewmodel/ApplicationViewModel.hpp"

#include <algorithm>
#include <iostream>
#include <string_view>

namespace
{
bool expect(const bool condition, const std::string_view message)
{
    if (!condition) std::cerr << "FAILED: " << message << '\n';
    return condition;
}

bool propertyBindingPublishesReadOnlyState()
{
    arcane::common::binding::ObservableProperty<int> source {7};
    const arcane::common::binding::IReadOnlyProperty<int>& binding = source;
    if (!expect(binding.value() == 7 && binding.revision() == 0U,
            "property binding must expose its initial value without a write API")) return false;

    source.publish(11);
    return expect(binding.value() == 11 && binding.revision() == 1U,
        "property binding must publish a new immutable snapshot and revision");
}

bool applicationUsesCommandAndEventBindings()
{
    using arcane::common::FrameCommand;
    using arcane::common::UiEventKind;
    using arcane::common::ui::ApplicationScreen;
    arcane::presentation::viewmodel::ApplicationViewModel application {0xB1D1A6U};

    const auto& property = application.stateBinding();
    if (!expect(property.value().screen == ApplicationScreen::Start,
            "application state property must initially expose the start screen")) return false;
    const auto initialRevision = property.revision();

    arcane::common::binding::ICommandBinding<FrameCommand>& commands = application;
    FrameCommand start;
    start.input.menuConfirmPressed = true;
    start.deltaSeconds = 0.01F;
    if (!expect(commands.canExecute(start), "start command must be executable")) return false;
    commands.execute(start);

    if (!expect(property.revision() == initialRevision + 1U
            && property.value().screen == ApplicationScreen::Playing
            && property.value().run.has_value(),
            "command binding must update the published application state")) return false;

    auto& events = application.eventBinding();
    const auto pending = events.pending();
    const auto hasEvent = [pending](const UiEventKind kind) {
        return std::any_of(pending.begin(), pending.end(), [kind](const auto& event) {
            return event.kind == kind;
        });
    };
    if (!expect(hasEvent(UiEventKind::ScreenChanged) && hasEvent(UiEventKind::FloorChanged),
            "event binding must announce screen and floor transitions")) return false;

    events.acknowledge();
    return expect(events.pending().empty(),
        "event binding must remove notifications only after acknowledgement");
}
}

int main()
{
    if (!propertyBindingPublishesReadOnlyState() || !applicationUsesCommandAndEventBindings())
        return 1;
    std::cout << "All binding contract tests passed.\n";
    return 0;
}
