#pragma once

#include "common/ui/UiStates.hpp"

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
void drawArena(sf::RenderTarget& target, const common::ui::ArenaState& layout,
    float groundTop, const ArenaTextures& textures);
}
