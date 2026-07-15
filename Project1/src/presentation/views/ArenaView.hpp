#pragma once

#include "game/floors/ArenaLayout.hpp"

#include <SFML/Graphics.hpp>

#include <array>
#include <optional>
#include <string_view>

namespace arcane::presentation::views
{
struct ArenaTextures
{
    std::array<std::optional<sf::Texture>, 3> backgrounds;
    std::array<std::optional<sf::Texture>, 3> platforms;
};

[[nodiscard]] ArenaTextures loadArenaTextures(std::string_view directory);
void drawArena(sf::RenderTarget& target, const game::floors::ArenaLayout& layout,
    float groundTop, const ArenaTextures& textures);
}
