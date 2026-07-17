#include "presentation/views/ArenaView.hpp"

#include "presentation/views/UiPrimitives.hpp"

#include <array>
#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>

namespace arcane::presentation::views
{
namespace
{
constexpr std::size_t ThemeCount = 3U;

[[nodiscard]] std::size_t themeIndex(const common::ui::ArenaTheme theme)
{
    switch (theme)
    {
    case common::ui::ArenaTheme::AuraOccupation: return 0U;
    case common::ui::ArenaTheme::NorthernFrontier: return 1U;
    case common::ui::ArenaTheme::MageExam: return 2U;
    }
    return 0U;
}

[[nodiscard]] std::optional<sf::Texture> loadArenaTexture(
    const std::filesystem::path& path)
{
    sf::Texture texture;
    if (!texture.loadFromFile(path)) return std::nullopt;
    texture.setSmooth(false);
    return texture;
}

void drawRectangle(sf::RenderTarget& target, const sf::Vector2f position,
    const sf::Vector2f size, const sf::Color color)
{
    sf::RectangleShape shape(size);
    shape.setPosition(position);
    shape.setFillColor(color);
    target.draw(shape);
}

void drawAuraOccupation(sf::RenderTarget& target, const common::ui::ContentId arenaId)
{
    drawRectangle(target, {}, {static_cast<float>(WindowWidth), static_cast<float>(WindowHeight)},
        sf::Color {24, 19, 39});
    drawRectangle(target, {0.0F, 170.0F}, {1280.0F, 470.0F}, sf::Color {38, 28, 52});
    drawRectangle(target, {0.0F, 330.0F}, {1280.0F, 310.0F}, sf::Color {49, 38, 58});

    sf::CircleShape moon(54.0F, 24U);
    moon.setPosition({1018.0F, 76.0F});
    moon.setFillColor(sf::Color {205, 190, 184, 205});
    target.draw(moon);

    for (std::size_t index = 0U; index < 8U; ++index)
    {
        const float x = static_cast<float>(index) * 184.0F - 38.0F;
        const float height = 108.0F + static_cast<float>((index + arenaId) % 3U) * 34.0F;
        drawRectangle(target, {x, 640.0F - height}, {138.0F, height}, sf::Color {42, 34, 49});
        drawRectangle(target, {x + 18.0F, 640.0F - height - 42.0F},
            {102.0F, 42.0F}, sf::Color {36, 28, 45});
        for (std::size_t window = 0U; window < 3U; ++window)
            drawRectangle(target, {x + 24.0F + static_cast<float>(window) * 36.0F,
                640.0F - height + 34.0F}, {12.0F, 28.0F}, sf::Color {103, 62, 94});
    }

    for (const float x : std::array {340.0F, 746.0F})
    {
        drawRectangle(target, {x, 240.0F}, {8.0F, 210.0F}, sf::Color {83, 70, 82});
        drawRectangle(target, {x + 8.0F, 252.0F}, {54.0F, 102.0F}, sf::Color {105, 35, 57});
        drawRectangle(target, {x + 18.0F, 268.0F}, {34.0F, 6.0F}, sf::Color {166, 74, 89});
    }
}

void drawNorthernFrontier(sf::RenderTarget& target, const common::ui::ContentId arenaId)
{
    drawRectangle(target, {}, {1280.0F, 720.0F}, sf::Color {19, 35, 55});
    drawRectangle(target, {0.0F, 220.0F}, {1280.0F, 420.0F}, sf::Color {35, 57, 76});

    const auto drawMountain = [&target](const float x, const float base, const float width,
        const float height, const sf::Color color)
    {
        sf::ConvexShape mountain(3U);
        mountain.setPoint(0U, {x, base});
        mountain.setPoint(1U, {x + width * 0.5F, base - height});
        mountain.setPoint(2U, {x + width, base});
        mountain.setFillColor(color);
        target.draw(mountain);
    };
    drawMountain(-120.0F, 575.0F, 580.0F, 300.0F, sf::Color {55, 76, 94});
    drawMountain(250.0F, 590.0F, 640.0F, 390.0F, sf::Color {47, 70, 90});
    drawMountain(720.0F, 580.0F, 680.0F, 330.0F, sf::Color {58, 82, 99});
    drawMountain(375.0F, 424.0F, 260.0F, 168.0F, sf::Color {174, 195, 207});

    for (std::size_t index = 0U; index < 46U; ++index)
    {
        const auto seed = static_cast<std::uint32_t>(arenaId * 37U + index * 97U);
        const float x = static_cast<float>((seed * 29U) % 1280U);
        const float y = 52.0F + static_cast<float>((seed * 53U) % 510U);
        const float size = index % 5U == 0U ? 3.0F : 2.0F;
        drawRectangle(target, {x, y}, {size, size}, sf::Color {216, 232, 238, 190});
    }

    for (const float x : std::array {92.0F, 1110.0F})
    {
        drawRectangle(target, {x, 400.0F}, {70.0F, 240.0F}, sf::Color {45, 52, 59});
        drawRectangle(target, {x - 16.0F, 384.0F}, {102.0F, 20.0F}, sf::Color {79, 91, 98});
        drawRectangle(target, {x + 14.0F, 430.0F}, {42.0F, 12.0F}, sf::Color {218, 230, 231});
    }
}

void drawMageExam(sf::RenderTarget& target, const common::ui::ContentId arenaId)
{
    drawRectangle(target, {}, {1280.0F, 720.0F}, sf::Color {17, 23, 38});
    drawRectangle(target, {0.0F, 120.0F}, {1280.0F, 520.0F}, sf::Color {24, 34, 50});

    for (std::size_t index = 0U; index < 7U; ++index)
    {
        const float x = 46.0F + static_cast<float>(index) * 198.0F;
        drawRectangle(target, {x, 230.0F}, {46.0F, 410.0F}, sf::Color {49, 58, 74});
        drawRectangle(target, {x - 14.0F, 214.0F}, {74.0F, 18.0F}, sf::Color {72, 77, 92});
        drawRectangle(target, {x - 8.0F, 620.0F}, {62.0F, 20.0F}, sf::Color {66, 72, 87});
    }
    drawRectangle(target, {110.0F, 286.0F}, {1060.0F, 12.0F}, sf::Color {52, 70, 79});

    const sf::Color rune {84, 204, 181, 145};
    for (std::size_t index = 0U; index < 5U; ++index)
    {
        const float x = 226.0F + static_cast<float>(index) * 210.0F;
        const float y = 350.0F + static_cast<float>((index + arenaId) % 2U) * 58.0F;
        drawRectangle(target, {x, y}, {52.0F, 5.0F}, rune);
        drawRectangle(target, {x + 23.0F, y - 23.0F}, {5.0F, 52.0F}, rune);
        drawRectangle(target, {x + 8.0F, y - 15.0F}, {36.0F, 5.0F}, rune);
        drawRectangle(target, {x + 8.0F, y + 15.0F}, {36.0F, 5.0F}, rune);
    }
}

void drawGround(sf::RenderTarget& target, const common::ui::ArenaTheme theme,
    const float groundTop)
{
    sf::Color body;
    sf::Color cap;
    sf::Color seam;
    switch (theme)
    {
    case common::ui::ArenaTheme::AuraOccupation:
        body = sf::Color {63, 55, 67}; cap = sf::Color {126, 107, 117}; seam = sf::Color {48, 42, 52};
        break;
    case common::ui::ArenaTheme::NorthernFrontier:
        body = sf::Color {55, 68, 78}; cap = sf::Color {218, 232, 235}; seam = sf::Color {41, 55, 66};
        break;
    case common::ui::ArenaTheme::MageExam:
        body = sf::Color {48, 55, 69}; cap = sf::Color {109, 139, 139}; seam = sf::Color {34, 41, 55};
        break;
    }
    drawRectangle(target, {0.0F, groundTop}, {1280.0F, 720.0F - groundTop}, body);
    drawRectangle(target, {0.0F, groundTop}, {1280.0F, 8.0F}, cap);
    for (std::size_t index = 0U; index < 16U; ++index)
        drawRectangle(target, {static_cast<float>(index) * 84.0F, groundTop + 34.0F},
            {2.0F, 46.0F}, seam);
}

void drawPlatform(sf::RenderTarget& target, const common::RectF& platform,
    const common::ui::ArenaTheme theme)
{
    sf::Color body;
    sf::Color top;
    sf::Color shadow;
    switch (theme)
    {
    case common::ui::ArenaTheme::AuraOccupation:
        body = sf::Color {83, 68, 82}; top = sf::Color {150, 119, 125}; shadow = sf::Color {42, 34, 46};
        break;
    case common::ui::ArenaTheme::NorthernFrontier:
        body = sf::Color {67, 87, 101}; top = sf::Color {224, 238, 239}; shadow = sf::Color {35, 52, 65};
        break;
    case common::ui::ArenaTheme::MageExam:
        body = sf::Color {62, 72, 88}; top = sf::Color {96, 205, 180}; shadow = sf::Color {31, 39, 55};
        break;
    }
    drawRectangle(target, {platform.left + 6.0F, platform.top + 9.0F},
        {platform.width, platform.height}, shadow);
    drawRectangle(target, {platform.left, platform.top},
        {platform.width, platform.height}, body);
    drawRectangle(target, {platform.left, platform.top}, {platform.width, 7.0F}, top);
    for (float x = platform.left + 30.0F; x < platform.left + platform.width; x += 54.0F)
        drawRectangle(target, {x, platform.top + 13.0F}, {18.0F, 4.0F}, shadow);
}

void drawTextureInRect(sf::RenderTarget& target, const sf::Texture& texture,
    const sf::Vector2f position, const sf::Vector2f size)
{
    const sf::Vector2u textureSize = texture.getSize();
    if (textureSize.x == 0U || textureSize.y == 0U) return;
    sf::Sprite sprite(texture);
    sprite.setPosition(position);
    sprite.setScale({size.x / static_cast<float>(textureSize.x),
        size.y / static_cast<float>(textureSize.y)});
    target.draw(sprite);
}

[[nodiscard]] float platformVisualHeight(const common::ui::ArenaTheme theme,
    const float width)
{
    switch (theme)
    {
    case common::ui::ArenaTheme::AuraOccupation:
        return std::clamp(width * 0.15625F, 42.0F, 84.0F);
    case common::ui::ArenaTheme::NorthernFrontier:
        return std::clamp(width * 0.11523F, 34.0F, 68.0F);
    case common::ui::ArenaTheme::MageExam:
        return std::clamp(width * 0.14258F, 40.0F, 76.0F);
    }
    return 48.0F;
}
}

ArenaTextures loadArenaTextures(const std::string_view directory)
{
    const std::filesystem::path root {std::string {directory}};
    ArenaTextures textures;
    constexpr std::array<std::string_view, ThemeCount> ActNames {"act1", "act2", "act3"};
    for (std::size_t index = 0U; index < ThemeCount; ++index)
    {
        const std::string prefix {ActNames[index]};
        textures.backgrounds[index] = loadArenaTexture(root / (prefix + "-background.png"));
        textures.platforms[index] = loadArenaTexture(root / (prefix + "-platform.png"));
    }
    return textures;
}

void drawArena(sf::RenderTarget& target, const common::ui::ArenaState& layout,
    const float groundTop, const ArenaTextures& textures)
{
    const std::size_t index = themeIndex(layout.theme);
    if (textures.backgrounds[index])
    {
        drawTextureInRect(target, *textures.backgrounds[index], {},
            {static_cast<float>(WindowWidth), static_cast<float>(WindowHeight)});
    }
    else
    {
        switch (layout.theme)
        {
        case common::ui::ArenaTheme::AuraOccupation:
            drawAuraOccupation(target, layout.id);
            break;
        case common::ui::ArenaTheme::NorthernFrontier:
            drawNorthernFrontier(target, layout.id);
            break;
        case common::ui::ArenaTheme::MageExam:
            drawMageExam(target, layout.id);
            break;
        }
    }

    if (textures.platforms[index])
    {
        drawTextureInRect(target, *textures.platforms[index], {0.0F, groundTop},
            {static_cast<float>(WindowWidth), static_cast<float>(WindowHeight) - groundTop});
        for (const common::RectF& platform : layout.oneWayPlatforms)
        {
            drawTextureInRect(target, *textures.platforms[index],
                {platform.left, platform.top},
                {platform.width, platformVisualHeight(layout.theme, platform.width)});
        }
    }
    else
    {
        drawGround(target, layout.theme, groundTop);
        for (const common::RectF& platform : layout.oneWayPlatforms)
            drawPlatform(target, platform, layout.theme);
    }
}
}
