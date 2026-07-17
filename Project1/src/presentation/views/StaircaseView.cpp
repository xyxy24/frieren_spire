#include "presentation/views/StaircaseView.hpp"

#include "presentation/views/UiPrimitives.hpp"

#include <filesystem>
#include <string>
#include <utility>

namespace arcane::presentation::views
{
namespace
{
[[nodiscard]] std::size_t themeIndex(const common::ui::ArenaTheme theme) noexcept
{
    switch (theme)
    {
    case common::ui::ArenaTheme::AuraOccupation: return 0U;
    case common::ui::ArenaTheme::NorthernFrontier: return 1U;
    case common::ui::ArenaTheme::MageExam: return 2U;
    }
    return 0U;
}
}

StaircaseTextures loadStaircaseTextures(const std::string_view directory)
{
    StaircaseTextures textures;
    const std::filesystem::path root {directory};
    for (std::size_t index = 0U; index < textures.size(); ++index)
    {
        sf::Texture texture;
        const auto path = root / ("staircase-act" + std::to_string(index + 1U) + ".png");
        if (!texture.loadFromFile(path)) continue;
        texture.setSmooth(false);
        textures[index] = std::move(texture);
    }
    return textures;
}

void drawStaircase(sf::RenderTarget& target, const common::RectF interactionBounds,
    const bool unlocked, const common::ui::ArenaTheme theme,
    const StaircaseTextures& textures)
{
    const auto& texture = textures[themeIndex(theme)];
    if (!texture) return;

    const sf::Vector2u textureSize = texture->getSize();
    if (textureSize.x == 0U || textureSize.y == 0U) return;
    const float displayHeight = interactionBounds.height * 1.55F;
    const float scale = displayHeight / static_cast<float>(textureSize.y);
    const sf::Vector2f anchor {
        interactionBounds.left + interactionBounds.width * 0.5F,
        interactionBounds.top + interactionBounds.height
    };

    sf::Sprite staircase(*texture);
    staircase.setOrigin({static_cast<float>(textureSize.x) * 0.5F,
        static_cast<float>(textureSize.y)});
    staircase.setPosition(anchor);
    staircase.setScale({scale, scale});

    if (unlocked)
    {
        sf::Sprite glow = staircase;
        glow.setScale({scale * 1.045F, scale * 1.045F});
        glow.setColor(sf::Color {255, 215, 132, 72});
        target.draw(glow, sf::RenderStates {sf::BlendAdd});
        staircase.setColor(sf::Color::White);
    }
    else
        staircase.setColor(sf::Color {108, 111, 124, 225});
    target.draw(staircase);

    const std::string_view label = unlocked ? "E  ASCEND" : "SEALED";
    const sf::Color labelColor = unlocked
        ? sf::Color {255, 226, 155, 235} : sf::Color {134, 137, 151, 210};
    drawPixelText(target, label,
        {anchor.x - (unlocked ? 40.0F : 24.0F), anchor.y + 7.0F},
        0.72F, labelColor);
}
}
