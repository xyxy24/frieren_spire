#include "game/combat/CombatSession.hpp"
#include "platform/SfmlInputMapper.hpp"

#include <SFML/Graphics.hpp>

#include <algorithm>

namespace
{
constexpr unsigned int WindowWidth = 1280;
constexpr unsigned int WindowHeight = 720;
constexpr float GroundTop = 640.0F;
constexpr float MaximumFrameTime = 0.05F;

void drawHealthBar(
    sf::RenderTarget& target,
    const sf::Vector2f position,
    const sf::Vector2f size,
    const int current,
    const int maximum,
    const sf::Color fillColor)
{
    sf::RectangleShape background(size);
    background.setPosition(position);
    background.setFillColor(sf::Color {44, 45, 54});
    background.setOutlineColor(sf::Color {215, 215, 225});
    background.setOutlineThickness(2.0F);
    target.draw(background);

    const float ratio = maximum > 0
        ? std::clamp(static_cast<float>(current) / static_cast<float>(maximum), 0.0F, 1.0F)
        : 0.0F;
    sf::RectangleShape fill({size.x * ratio, size.y});
    fill.setPosition(position);
    fill.setFillColor(fillColor);
    target.draw(fill);
}
}

int main()
{
    sf::RenderWindow window(
        sf::VideoMode({WindowWidth, WindowHeight}),
        "Arcane Spire - A/D Move | Space Jump | J Attack");
    window.setVerticalSyncEnabled(true);

    arcane::platform::SfmlInputMapper inputMapper;
    arcane::game::CombatRequest request;
    request.worldBounds = {0.0F, static_cast<float>(WindowWidth), GroundTop};
    arcane::game::CombatSession combat(request);

    sf::RectangleShape ground({static_cast<float>(WindowWidth), static_cast<float>(WindowHeight) - GroundTop});
    ground.setPosition({0.0F, GroundTop});
    ground.setFillColor(sf::Color {64, 72, 88});

    sf::RectangleShape playerShape({arcane::game::PlayerController::Width, arcane::game::PlayerController::Height});
    playerShape.setFillColor(sf::Color {232, 232, 242});
    playerShape.setOutlineColor(sf::Color {142, 115, 200});
    playerShape.setOutlineThickness(3.0F);

    sf::RectangleShape enemyShape({
        arcane::game::CombatSession::EnemyWidth,
        arcane::game::CombatSession::EnemyHeight
    });
    enemyShape.setFillColor(sf::Color {176, 70, 78});
    enemyShape.setOutlineColor(sf::Color {245, 176, 129});
    enemyShape.setOutlineThickness(3.0F);

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
        combat.update(intent, deltaSeconds);

        const arcane::game::PlayerStateView player = combat.playerState();
        const arcane::game::EnemyStateView enemy = combat.enemyState();
        playerShape.setPosition({player.position.x, player.position.y});
        playerShape.setFillColor(player.attackActive
            ? sf::Color {255, 231, 153}
            : sf::Color {232, 232, 242});
        enemyShape.setPosition({enemy.position.x, enemy.position.y});

        sf::Color backgroundColor {18, 20, 28};
        if (combat.result().has_value())
        {
            backgroundColor = combat.result()->outcome == arcane::game::CombatOutcome::Victory
                ? sf::Color {20, 45, 39}
                : sf::Color {52, 23, 29};
        }

        window.clear(backgroundColor);
        window.draw(ground);
        window.draw(playerShape);

        if (enemy.alive)
        {
            window.draw(enemyShape);
        }

        if (player.attackActive)
        {
            const arcane::game::Aabb bounds = combat.attackBounds();
            sf::RectangleShape attackShape({bounds.width, bounds.height});
            attackShape.setPosition({bounds.left, bounds.top});
            attackShape.setFillColor(sf::Color {255, 196, 92, 105});
            attackShape.setOutlineColor(sf::Color {255, 218, 145});
            attackShape.setOutlineThickness(2.0F);
            window.draw(attackShape);
        }

        drawHealthBar(
            window,
            {32.0F, 28.0F},
            {300.0F, 22.0F},
            player.currentHealth,
            player.maximumHealth,
            sf::Color {108, 206, 126});
        drawHealthBar(
            window,
            {static_cast<float>(WindowWidth) - 332.0F, 28.0F},
            {300.0F, 22.0F},
            enemy.currentHealth,
            enemy.maximumHealth,
            sf::Color {218, 92, 103});
        window.display();
    }

    return 0;
}
