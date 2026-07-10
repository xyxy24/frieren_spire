#include "game/player/PlayerController.hpp"
#include "platform/SfmlInputMapper.hpp"

#include <SFML/Graphics.hpp>

#include <algorithm>

namespace
{
constexpr unsigned int WindowWidth = 1280;
constexpr unsigned int WindowHeight = 720;
constexpr float GroundTop = 640.0F;
constexpr float MaximumFrameTime = 0.05F;
}

int main()
{
    sf::RenderWindow window(sf::VideoMode({WindowWidth, WindowHeight}), "Arcane Spire - Combat Probe");
    window.setVerticalSyncEnabled(true);

    arcane::platform::SfmlInputMapper inputMapper;
    arcane::game::PlayerController player;
    const arcane::game::WorldBounds worldBounds {0.0F, static_cast<float>(WindowWidth), GroundTop};

    sf::RectangleShape ground({static_cast<float>(WindowWidth), static_cast<float>(WindowHeight) - GroundTop});
    ground.setPosition({0.0F, GroundTop});
    ground.setFillColor(sf::Color {64, 72, 88});

    sf::RectangleShape playerShape({arcane::game::PlayerController::Width, arcane::game::PlayerController::Height});
    playerShape.setFillColor(sf::Color {232, 232, 242});
    playerShape.setOutlineColor(sf::Color {142, 115, 200});
    playerShape.setOutlineThickness(3.0F);

    sf::Clock frameClock;

    while (window.isOpen())
    {
        while (const auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
        }

        const float deltaSeconds = std::min(frameClock.restart().asSeconds(), MaximumFrameTime);
        const arcane::game::PlayerIntent intent = inputMapper.sample();
        player.update(intent, deltaSeconds, worldBounds);

        const arcane::game::Vec2 playerPosition = player.position();
        playerShape.setPosition({playerPosition.x, playerPosition.y});

        window.clear(sf::Color {18, 20, 28});
        window.draw(ground);
        window.draw(playerShape);
        window.display();
    }

    return 0;
}

