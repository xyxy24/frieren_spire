#include "presentation/views/UiPrimitives.hpp"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{
using arcane::common::ui::PauseMenuItem;
using arcane::presentation::views::WindowHeight;
using arcane::presentation::views::WindowWidth;

constexpr sf::Color BackgroundColor {18, 20, 28};
constexpr int ChannelTolerance = 8;
constexpr unsigned int TileWidth = WindowWidth / 2U;
constexpr unsigned int TileHeight = WindowHeight / 2U;
// Channel tolerance absorbs tiny driver blending differences. More than nine
// changed pixels at 1280x720 still fails, so even a single glyph regression is visible.
constexpr double AllowedDifferentPixelRatio = 0.00001;

static_assert(WindowWidth % TileWidth == 0U);
static_assert(WindowHeight % TileHeight == 0U);
static_assert(TileWidth <= 1024U && TileHeight <= 1024U);

enum class Scene
{
    StartMenu,
    PauseReplay,
    PauseSaveAndExit,
};

struct Scenario
{
    std::string_view name;
    Scene scene;
};

struct Comparison
{
    std::size_t differentPixels {0};
    std::size_t totalPixels {0};
    int maximumChannelDelta {0};
};

constexpr std::array Scenarios {
    Scenario {"start_menu_new_run", Scene::StartMenu},
    Scenario {"pause_replay_selected", Scene::PauseReplay},
    Scenario {"pause_save_selected", Scene::PauseSaveAndExit},
};

void drawScene(sf::RenderTarget& target, Scene scene)
{
    using namespace arcane::presentation::views;
    switch (scene)
    {
    case Scene::StartMenu:
        drawStartMenu(target, false);
        break;
    case Scene::PauseReplay:
        drawPauseMenu(target, PauseMenuItem::ReplayCurrentFloor);
        break;
    case Scene::PauseSaveAndExit:
        drawPauseMenu(target, PauseMenuItem::SaveAndExit);
        break;
    }
}

[[nodiscard]] sf::Image render(Scene scene)
{
    sf::Image result({WindowWidth, WindowHeight}, BackgroundColor);
    constexpr unsigned int ColumnCount = WindowWidth / TileWidth;
    constexpr unsigned int RowCount = WindowHeight / TileHeight;
    for (unsigned int row = 0U; row < RowCount; ++row)
    {
        for (unsigned int column = 0U; column < ColumnCount; ++column)
        {
            sf::RenderTexture target;
            if (!target.resize({TileWidth, TileHeight}))
                throw std::runtime_error("failed to create a UI render tile");

            const unsigned int logicalLeft = column * TileWidth;
            const unsigned int logicalTop = row * TileHeight;
            target.setView(sf::View {sf::FloatRect {
                {static_cast<float>(logicalLeft), static_cast<float>(logicalTop)},
                {static_cast<float>(TileWidth), static_cast<float>(TileHeight)}}});
            target.clear(BackgroundColor);
            drawScene(target, scene);
            target.display();

            const sf::Image tile = target.getTexture().copyToImage();
            if (tile.getSize() != sf::Vector2u {TileWidth, TileHeight})
                throw std::runtime_error("UI render tile returned an unexpected image size");
            for (unsigned int y = 0U; y < TileHeight; ++y)
            {
                for (unsigned int x = 0U; x < TileWidth; ++x)
                    result.setPixel(
                        {logicalLeft + x, logicalTop + y}, tile.getPixel({x, y}));
            }
        }
    }
    return result;
}

[[nodiscard]] int channelDelta(sf::Color left, sf::Color right)
{
    const auto delta = [](std::uint8_t first, std::uint8_t second) {
        return std::abs(static_cast<int>(first) - static_cast<int>(second));
    };
    return std::max({delta(left.r, right.r), delta(left.g, right.g),
        delta(left.b, right.b), delta(left.a, right.a)});
}

[[nodiscard]] Comparison compareImages(
    const sf::Image& expected, const sf::Image& actual, sf::Image& difference)
{
    const sf::Vector2u size = actual.getSize();
    difference = sf::Image(size, sf::Color::Black);

    Comparison result;
    result.totalPixels = static_cast<std::size_t>(size.x) * size.y;
    for (unsigned int y = 0; y < size.y; ++y)
    {
        for (unsigned int x = 0; x < size.x; ++x)
        {
            const sf::Vector2u position {x, y};
            const sf::Color actualPixel = actual.getPixel(position);
            const int delta = channelDelta(expected.getPixel(position), actualPixel);
            result.maximumChannelDelta = std::max(result.maximumChannelDelta, delta);
            if (delta > ChannelTolerance)
            {
                ++result.differentPixels;
                difference.setPixel(position, sf::Color {255, 30, 80});
            }
            else
            {
                difference.setPixel(position, sf::Color {
                    static_cast<std::uint8_t>(actualPixel.r / 4U),
                    static_cast<std::uint8_t>(actualPixel.g / 4U),
                    static_cast<std::uint8_t>(actualPixel.b / 4U),
                    255});
            }
        }
    }
    return result;
}

[[nodiscard]] bool saveImage(const sf::Image& image, const std::filesystem::path& path)
{
    if (image.saveToFile(path))
        return true;
    std::cerr << "Failed to save image: " << path << '\n';
    return false;
}
}

int main(int argc, char** argv)
{
    if (argc < 3 || argc > 4)
    {
        std::cerr << "Usage: ui_visual_regression_tests <baseline-dir> <artifact-dir> "
                     "[--update-baselines]\n";
        return 2;
    }

    const std::filesystem::path baselineDirectory = argv[1];
    const std::filesystem::path artifactDirectory = argv[2];
    const bool updateBaselines = argc == 4 && std::string_view {argv[3]} == "--update-baselines";
    if (argc == 4 && !updateBaselines)
    {
        std::cerr << "Unknown option: " << argv[3] << '\n';
        return 2;
    }

    std::filesystem::create_directories(
        updateBaselines ? baselineDirectory : artifactDirectory);

    bool allPassed = true;
    try
    {
        for (const Scenario& scenario : Scenarios)
        {
            const sf::Image actual = render(scenario.scene);
            const std::filesystem::path baselinePath =
                baselineDirectory / (std::string {scenario.name} + ".png");

            if (updateBaselines)
            {
                allPassed = saveImage(actual, baselinePath) && allPassed;
                std::cout << "Updated baseline: " << baselinePath << '\n';
                continue;
            }

            sf::Image expected;
            if (!expected.loadFromFile(baselinePath))
            {
                std::cerr << "Missing UI baseline: " << baselinePath
                          << "\nRegenerate it with the --update-baselines option.\n";
                allPassed = false;
                continue;
            }
            if (expected.getSize() != actual.getSize())
            {
                std::cerr << scenario.name << ": image size changed from "
                          << expected.getSize().x << 'x' << expected.getSize().y << " to "
                          << actual.getSize().x << 'x' << actual.getSize().y << '\n';
                static_cast<void>(saveImage(
                    actual, artifactDirectory / (std::string {scenario.name} + "_actual.png")));
                allPassed = false;
                continue;
            }

            sf::Image difference;
            const Comparison comparison = compareImages(expected, actual, difference);
            const double ratio = static_cast<double>(comparison.differentPixels)
                / static_cast<double>(comparison.totalPixels);
            if (ratio > AllowedDifferentPixelRatio)
            {
                const std::string prefix {scenario.name};
                static_cast<void>(saveImage(actual,
                    artifactDirectory / (prefix + "_actual.png")));
                static_cast<void>(saveImage(difference,
                    artifactDirectory / (prefix + "_diff.png")));
                std::cerr << scenario.name << ": " << comparison.differentPixels << " / "
                          << comparison.totalPixels << " pixels differ ("
                          << ratio * 100.0 << "%, allowed "
                          << AllowedDifferentPixelRatio * 100.0 << "%), max channel delta "
                          << comparison.maximumChannelDelta << '\n';
                allPassed = false;
            }
            else
            {
                std::cout << scenario.name << ": passed (" << comparison.differentPixels
                          << " tolerant pixel differences)\n";
            }
        }
    }
    catch (const std::exception& exception)
    {
        std::cerr << "UI visual regression rendering failed: " << exception.what() << '\n';
        return 1;
    }

    return allPassed ? 0 : 1;
}
