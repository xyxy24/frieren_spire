#include "platform/SfmlInputMapper.hpp"

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>

#include <iostream>
#include <string_view>

namespace
{
bool expect(const bool condition, const std::string_view message)
{
    if (!condition) std::cerr << "FAILED: " << message << '\n';
    return condition;
}

sf::Event keyPressed(const sf::Keyboard::Key key)
{
    return sf::Event {sf::Event::KeyPressed {.code = key}};
}

sf::Event keyReleased(const sf::Keyboard::Key key)
{
    return sf::Event {sf::Event::KeyReleased {.code = key}};
}

bool buffersShortSpellPressUntilNextSample()
{
    arcane::platform::SfmlInputMapper mapper;
    mapper.handleEvent(keyPressed(sf::Keyboard::Key::U));
    mapper.handleEvent(keyReleased(sf::Keyboard::Key::U));

    const auto first = mapper.sample();
    const auto second = mapper.sample();
    return expect(first.spellPressed[0],
            "a spell tap must survive a press and release between rendered frames")
        && expect(!second.spellPressed[0],
            "a buffered spell tap must be consumed exactly once");
}

bool preservesSharedPhysicalKeyBindings()
{
    arcane::platform::SfmlInputMapper mapper;
    mapper.handleEvent(keyPressed(sf::Keyboard::Key::E));
    mapper.handleEvent(keyPressed(sf::Keyboard::Key::R));

    const auto intent = mapper.sample();
    return expect(intent.interactPressed && intent.menuPageNextPressed,
            "E must remain both interact and next-page input")
        && expect(intent.ultimateSpellPressed && intent.menuSecondaryPressed,
            "R must remain both ultimate-spell and secondary-menu input");
}

bool ignoresReleaseEvents()
{
    arcane::platform::SfmlInputMapper mapper;
    mapper.handleEvent(keyReleased(sf::Keyboard::Key::Enter));
    return expect(!mapper.sample().menuConfirmPressed,
        "releasing a key must not create a one-shot action");
}

bool mapsSpellAcquisitionPreview()
{
    arcane::platform::SfmlInputMapper mapper;
    mapper.handleEvent(keyPressed(sf::Keyboard::Key::F4));
    const auto intent = mapper.sample();
    return expect(intent.debugSpellAcquisitionPreviewPressed,
        "F4 must expose the spell-acquisition presentation preview");
}
}

int main()
{
    const bool passed = buffersShortSpellPressUntilNextSample()
        && preservesSharedPhysicalKeyBindings()
        && mapsSpellAcquisitionPreview()
        && ignoresReleaseEvents();

    if (!passed) return 1;
    std::cout << "All SFML input mapper tests passed.\n";
    return 0;
}
