#pragma once

#include "game/contracts/PlayerIntent.hpp"

#include <SFML/Window/Event.hpp>

namespace arcane::platform
{
class SfmlInputMapper
{
public:
    void handleEvent(const sf::Event& event);
    [[nodiscard]] game::PlayerIntent sample();

private:
    game::PlayerIntent pendingIntent_;
};
}
