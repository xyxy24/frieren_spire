#pragma once

#include "common/ui/UiStates.hpp"

#include <SFML/Graphics.hpp>

#include <array>
#include <optional>
#include <string_view>

namespace arcane::presentation::views
{
using StaircaseTextures = std::array<std::optional<sf::Texture>, 3>;

[[nodiscard]] StaircaseTextures loadStaircaseTextures(std::string_view directory);
void drawStaircase(sf::RenderTarget& target, common::RectF interactionBounds,
    bool unlocked, common::ui::ArenaTheme theme,
    const StaircaseTextures& textures);
}
