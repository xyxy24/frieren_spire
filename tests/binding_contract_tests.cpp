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

    std::size_t notificationCount = 0U;
    std::uint64_t notifiedRevision = 0U;
    {
        auto connection = binding.subscribe([&](const std::uint64_t revision) {
            ++notificationCount;
            notifiedRevision = revision;
        });
        if (!expect(connection.connected(), "property subscription must return a live connection"))
            return false;
        source.publish(11);
        if (!expect(binding.value() == 11 && binding.revision() == 1U,
                "property binding must publish a new immutable snapshot and revision"))
            return false;
        if (!expect(notificationCount == 1U && notifiedRevision == 1U,
                "property subscribers must be notified with the published revision"))
            return false;
    }

    source.publish(13);
    return expect(notificationCount == 1U && binding.value() == 13,
        "destroying a binding connection must automatically unsubscribe");
}

bool applicationUsesCommandAndEventBindings()
{
    using arcane::common::FrameCommand;
    using arcane::common::UiAction;
    using arcane::common::UiCommand;
    using arcane::common::UiEventKind;
    using arcane::common::ui::ApplicationScreen;
    arcane::presentation::viewmodel::ApplicationViewModel application {0xB1D1A6U};

    const auto& property = application.stateBinding();
    if (!expect(property.value().screen == ApplicationScreen::Start,
            "application state property must initially expose the start screen")) return false;
    const auto initialRevision = property.revision();

    arcane::common::binding::ICommandBinding<FrameCommand>& commands = application;
    arcane::common::binding::ICommandBinding<UiCommand>& uiCommands = application;
    const UiCommand start {UiAction::Confirm};
    if (!expect(uiCommands.canExecute(start), "start UI command must be executable")) return false;
    uiCommands.execute(start);
    commands.execute(FrameCommand {{}, 0.01F});

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
