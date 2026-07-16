#pragma once

#include "game/combat/Aabb.hpp"
#include "game/floors/ArenaLayout.hpp"

#include <SFML/Graphics.hpp>

#include <array>
#include <optional>
#include <string_view>

namespace arcane::presentation::views
{
using StaircaseTextures = std::array<std::optional<sf::Texture>, 3>;

[[nodiscard]] StaircaseTextures loadStaircaseTextures(std::string_view directory);
void drawStaircase(sf::RenderTarget& target, game::Aabb interactionBounds,
    bool unlocked, game::floors::ArenaTheme theme,
    const StaircaseTextures& textures);
}
