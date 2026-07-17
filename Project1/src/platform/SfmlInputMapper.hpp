#pragma once

#include "common/input/InputState.hpp"

#include <SFML/Window/Event.hpp>

namespace arcane::platform
{
class SfmlInputMapper
{
public:
    void handleEvent(const sf::Event& event);
    [[nodiscard]] common::InputState sample();

private:
    common::InputState pendingIntent_;
};
}
