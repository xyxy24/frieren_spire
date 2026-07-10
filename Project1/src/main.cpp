#include "app/run/AppFlowController.hpp"
#include "platform/SfmlInputMapper.hpp"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>

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

sf::Color colorForContent(const arcane::game::run::ContentId id)
{
    const auto red = static_cast<std::uint8_t>(90U + (id * 47U) % 130U);
    const auto green = static_cast<std::uint8_t>(80U + (id * 71U) % 140U);
    const auto blue = static_cast<std::uint8_t>(110U + (id * 29U) % 120U);
    return {red, green, blue};
}

void drawCard(
    sf::RenderTarget& target,
    const arcane::game::run::ContentId id,
    const sf::Vector2f position,
    const sf::Vector2f size,
    const bool selected)
{
    sf::RectangleShape card(size);
    card.setPosition(position);
    card.setFillColor(colorForContent(id));
    card.setOutlineColor(selected ? sf::Color {255, 231, 145} : sf::Color {210, 210, 225});
    card.setOutlineThickness(selected ? 6.0F : 3.0F);
    target.draw(card);

    sf::CircleShape sigil(size.x * 0.18F, 6U);
    sigil.setOrigin({sigil.getRadius(), sigil.getRadius()});
    sigil.setPosition({position.x + size.x * 0.5F, position.y + size.y * 0.42F});
    sigil.setFillColor(sf::Color {245, 245, 255, 190});
    target.draw(sigil);
}

void drawEquippedSlots(sf::RenderTarget& target, const arcane::game::run::PlayerProgress& player)
{
    constexpr float SlotSize = 66.0F;
    constexpr float Gap = 18.0F;
    const float startX = (static_cast<float>(WindowWidth) - (SlotSize * 3.0F + Gap * 2.0F)) * 0.5F;

    for (std::size_t index = 0; index < player.equippedSpells.size(); ++index)
    {
        sf::RectangleShape slot({SlotSize, SlotSize});
        slot.setPosition({startX + static_cast<float>(index) * (SlotSize + Gap), 630.0F});
        slot.setFillColor(player.equippedSpells[index]
            ? colorForContent(*player.equippedSpells[index])
            : sf::Color {38, 41, 52});
        slot.setOutlineColor(sf::Color {175, 164, 214});
        slot.setOutlineThickness(3.0F);
        target.draw(slot);
    }
}

void drawRewardScreen(sf::RenderTarget& target, const arcane::app::TowerSession& tower)
{
    const auto candidates = tower.rewardCandidates();
    if (!candidates) return;

    constexpr std::array<float, 3> CardX {270.0F, 535.0F, 800.0F};
    for (std::size_t index = 0; index < candidates->size(); ++index)
    {
        drawCard(target, (*candidates)[index], {CardX[index], 205.0F}, {210.0F, 280.0F}, false);
    }
}

void drawMerchantScreen(sf::RenderTarget& target, const arcane::app::TowerSession& tower)
{
    constexpr std::array<float, 3> CardX {270.0F, 535.0F, 800.0F};
    const auto& stock = tower.merchantStock();
    for (std::size_t index = 0U; index < stock.size() && index < CardX.size(); ++index)
        drawCard(target, stock[index].id, {CardX[index], 205.0F}, {210.0F, 280.0F}, stock[index].sold);
}

void drawEventScreen(sf::RenderTarget& target, const arcane::app::TowerSession& tower)
{
    if (tower.eventFloorState() == arcane::app::EventFloorState::Result)
    {
        if (tower.eventResultChoice())
            drawCard(target, *tower.eventResultChoice(), {535.0F, 205.0F}, {210.0F, 280.0F}, true);
        return;
    }
    constexpr std::array<float, 2> CardX {400.0F, 670.0F};
    const auto choices = tower.eventChoices();
    for (std::size_t index = 0U; index < choices.size() && index < CardX.size(); ++index)
        drawCard(target, choices[index].id, {CardX[index], 205.0F}, {210.0F, 280.0F}, false);
}

void drawStaircase(sf::RenderTarget& target, arcane::game::Aabb bounds, bool unlocked);

void drawSpecialFloor(sf::RenderTarget& target, const arcane::app::TowerSession& tower)
{
    sf::RectangleShape ground({static_cast<float>(WindowWidth), static_cast<float>(WindowHeight) - GroundTop});
    ground.setPosition({0.0F, GroundTop});
    ground.setFillColor(sf::Color {64, 72, 88});
    target.draw(ground);
    if (const auto* player = tower.explorationPlayer())
    {
        sf::RectangleShape shape({arcane::game::PlayerController::Width, arcane::game::PlayerController::Height});
        const auto position = player->position();
        shape.setPosition({position.x, position.y});
        shape.setFillColor(sf::Color {232, 232, 242});
        target.draw(shape);
    }
    const auto npc = tower.npcBounds();
    sf::RectangleShape npcShape({npc.width, npc.height});
    npcShape.setPosition({npc.left, npc.top});
    npcShape.setFillColor(tower.currentFloorType() == arcane::game::run::FloorType::Merchant
        ? sf::Color {205, 159, 75} : sf::Color {116, 83, 170});
    npcShape.setOutlineColor(sf::Color {235, 225, 190});
    npcShape.setOutlineThickness(3.0F);
    target.draw(npcShape);
    drawStaircase(target, tower.staircaseBounds(), tower.staircaseUnlocked());
}

void drawLoadoutOverlay(sf::RenderTarget& target, const arcane::app::TowerSession& tower)
{
    sf::RectangleShape overlay({static_cast<float>(WindowWidth), static_cast<float>(WindowHeight)});
    overlay.setFillColor(sf::Color {14, 12, 25, 235});
    target.draw(overlay);

    const auto& learned = tower.run().player().learnedSpells;
    constexpr std::size_t Columns = 8U;
    constexpr float CardWidth = 90.0F;
    constexpr float CardHeight = 95.0F;
    constexpr float HorizontalGap = 20.0F;
    constexpr float VerticalGap = 20.0F;
    constexpr float StartX = 190.0F;
    constexpr float StartY = 90.0F;

    for (std::size_t index = 0; index < learned.size(); ++index)
    {
        const std::size_t column = index % Columns;
        const std::size_t row = index / Columns;
        const sf::Vector2f position {
            StartX + static_cast<float>(column) * (CardWidth + HorizontalGap),
            StartY + static_cast<float>(row) * (CardHeight + VerticalGap)
        };
        drawCard(
            target,
            learned[index],
            position,
            {CardWidth, CardHeight},
            tower.selectedLearnedSpell() == learned[index]);
    }

    drawEquippedSlots(target, tower.run().player());
}

void drawStaircase(
    sf::RenderTarget& target,
    const arcane::game::Aabb bounds,
    const bool unlocked)
{
    sf::RectangleShape interactionZone({bounds.width, bounds.height});
    interactionZone.setPosition({bounds.left, bounds.top});
    interactionZone.setFillColor(sf::Color::Transparent);
    interactionZone.setOutlineColor(unlocked ? sf::Color {245, 219, 145} : sf::Color {95, 96, 110});
    interactionZone.setOutlineThickness(3.0F);
    target.draw(interactionZone);

    constexpr int StepCount = 5;
    const float stepHeight = bounds.height / static_cast<float>(StepCount);
    for (int step = 0; step < StepCount; ++step)
    {
        const float width = bounds.width * static_cast<float>(step + 1) / static_cast<float>(StepCount);
        sf::RectangleShape shape({width, stepHeight});
        shape.setPosition({bounds.left, bounds.top + bounds.height - stepHeight * static_cast<float>(step + 1)});
        shape.setFillColor(unlocked ? sf::Color {146, 124, 174} : sf::Color {68, 69, 80});
        shape.setOutlineColor(unlocked ? sf::Color {226, 207, 244} : sf::Color {105, 106, 118});
        shape.setOutlineThickness(1.5F);
        target.draw(shape);
    }
}

void drawCombat(sf::RenderTarget& target, const arcane::app::TowerSession& tower)
{
    const arcane::game::CombatSession* combat = tower.combat();
    if (!combat) return;

    sf::RectangleShape ground({static_cast<float>(WindowWidth), static_cast<float>(WindowHeight) - GroundTop});
    ground.setPosition({0.0F, GroundTop});
    ground.setFillColor(sf::Color {64, 72, 88});
    target.draw(ground);

    const arcane::game::PlayerStateView player = combat->playerState();
    sf::RectangleShape playerShape({arcane::game::PlayerController::Width, arcane::game::PlayerController::Height});
    playerShape.setPosition({player.position.x, player.position.y});
    playerShape.setFillColor(player.attackActive ? sf::Color {255, 231, 153} : sf::Color {232, 232, 242});
    playerShape.setOutlineColor(sf::Color {142, 115, 200});
    playerShape.setOutlineThickness(3.0F);
    target.draw(playerShape);

    const arcane::game::EnemyStateView enemy = combat->enemyState();
    if (enemy.alive)
    {
        sf::RectangleShape enemyShape({
            arcane::game::CombatSession::EnemyWidth,
            arcane::game::CombatSession::EnemyHeight
        });
        enemyShape.setPosition({enemy.position.x, enemy.position.y});
        enemyShape.setFillColor(tower.currentFloorType() == arcane::game::run::FloorType::Boss
            ? sf::Color {129, 68, 172}
            : sf::Color {176, 70, 78});
        enemyShape.setOutlineColor(sf::Color {245, 176, 129});
        enemyShape.setOutlineThickness(tower.currentFloorType() == arcane::game::run::FloorType::Boss ? 6.0F : 3.0F);
        target.draw(enemyShape);

        drawHealthBar(
            target,
            {static_cast<float>(WindowWidth) - 332.0F, 28.0F},
            {300.0F, 22.0F},
            enemy.currentHealth,
            enemy.maximumHealth,
            sf::Color {218, 92, 103});
    }

    if (player.attackActive)
    {
        const arcane::game::Aabb bounds = combat->attackBounds();
        sf::RectangleShape attackShape({bounds.width, bounds.height});
        attackShape.setPosition({bounds.left, bounds.top});
        attackShape.setFillColor(sf::Color {255, 196, 92, 105});
        attackShape.setOutlineColor(sf::Color {255, 218, 145});
        attackShape.setOutlineThickness(2.0F);
        target.draw(attackShape);
    }
}

std::string makeWindowTitle(const arcane::app::TowerSession& tower)
{
    const auto& run = tower.run();
    const auto& player = run.player();
    std::string title = "Arcane Spire | Floor " + std::to_string(run.context().floorIndex + 1U)
        + " | HP " + std::to_string(player.currentHp) + "/" + std::to_string(player.maxHp)
        + " | Gold " + std::to_string(player.gold)
        + " | Boss " + std::to_string(run.context().bossesDefeated) + "/3 | ";

    if (tower.loadoutOpen())
    {
        const std::string selected = tower.selectedLearnedSpell()
            ? std::to_string(*tower.selectedLearnedSpell())
            : std::string {"None"};
        return title + "SPELL LOADOUT - Selected " + selected
            + " | A/D Select, U/I/O Equip Slot, Tab Close";
    }

    switch (run.phase())
    {
    case arcane::game::run::RunPhase::InEncounter:
        if (tower.currentFloorType() == arcane::game::run::FloorType::Merchant)
        {
            if (!tower.specialPanelOpen()) return title + "MERCHANT ROOM - Meet NPC, E Trade | Rear Staircase Exits";
            const auto& stock = tower.merchantStock();
            std::string items;
            constexpr std::array<char, 3> keys {'U', 'I', 'O'};
            for (std::size_t index = 0U; index < stock.size() && index < keys.size(); ++index)
            {
                if (!items.empty()) items += " | ";
                items += keys[index] + std::string {"="} + std::to_string(stock[index].id)
                    + (stock[index].sold ? " SOLD" : " " + std::to_string(stock[index].price) + "g");
            }
            return title + "MERCHANT - " + items + " | E Close";
        }
        if (tower.currentFloorType() == arcane::game::run::FloorType::Event)
        {
            if (!tower.specialPanelOpen())
                return title + (tower.eventFloorState() == arcane::app::EventFloorState::Result
                    ? "EVENT RESOLVED - E Review NPC Result | Rear Staircase Exits"
                    : "EVENT ROOM - Meet NPC, E Interact");
            if (tower.eventFloorState() == arcane::app::EventFloorState::Result)
                return title + "EVENT RESULT - E Close";
            return title + "EVENT - U Gain 10 Gold, I Trade 20 HP For Relic, E Close";
        }
        return title + "A/D Move, Space Jump, J Attack, Tab Loadout";
    case arcane::game::run::RunPhase::Reward:
    {
        const auto candidates = tower.rewardCandidates();
        if (!candidates) return title + "Reward";
        return title + "Choose U=" + std::to_string((*candidates)[0])
            + " I=" + std::to_string((*candidates)[1])
            + " O=" + std::to_string((*candidates)[2]) + " | Tab Loadout";
    }
    case arcane::game::run::RunPhase::FloorComplete:
        return title + "Move Into Staircase, E Climb (+50% Missing HP), Tab Loadout";
    case arcane::game::run::RunPhase::Victory:
        return title + "VICTORY - Third Boss Defeated";
    case arcane::game::run::RunPhase::Defeat:
        return title + "DEFEAT";
    case arcane::game::run::RunPhase::FloorLoading:
        return title + "Loading";
    }

    return title;
}

void drawMenuPanels(sf::RenderTarget& target, const std::size_t count, const sf::Color color)
{
    for (std::size_t index = 0U; index < count; ++index)
    {
        sf::RectangleShape panel({420.0F, 82.0F});
        panel.setPosition({430.0F, 270.0F + static_cast<float>(index) * 110.0F});
        panel.setFillColor(color);
        panel.setOutlineColor(sf::Color {225, 220, 240});
        panel.setOutlineThickness(3.0F);
        target.draw(panel);
    }
}

void drawDigit(sf::RenderTarget& target, const int digit, const sf::Vector2f origin)
{
    constexpr std::array<std::uint8_t, 10> masks {0x3FU, 0x06U, 0x5BU, 0x4FU, 0x66U,
        0x6DU, 0x7DU, 0x07U, 0x7FU, 0x6FU};
    constexpr std::array<sf::Vector2f, 7> positions {{{4.0F, 0.0F}, {20.0F, 4.0F},
        {20.0F, 24.0F}, {4.0F, 40.0F}, {0.0F, 24.0F}, {0.0F, 4.0F}, {4.0F, 20.0F}}};
    constexpr std::array<sf::Vector2f, 7> sizes {{{16.0F, 4.0F}, {4.0F, 16.0F},
        {4.0F, 16.0F}, {16.0F, 4.0F}, {4.0F, 16.0F}, {4.0F, 16.0F}, {16.0F, 4.0F}}};
    for (std::size_t segment = 0U; segment < positions.size(); ++segment)
    {
        if ((masks[static_cast<std::size_t>(digit)] & (1U << segment)) == 0U) continue;
        sf::RectangleShape bar(sizes[segment]);
        bar.setPosition(origin + positions[segment]);
        bar.setFillColor(sf::Color {246, 201, 82});
        target.draw(bar);
    }
}

void drawGold(sf::RenderTarget& target, const int gold)
{
    sf::CircleShape coin(10.0F);
    coin.setPosition({32.0F, 68.0F});
    coin.setFillColor(sf::Color {246, 201, 82});
    target.draw(coin);
    const std::string digits = std::to_string(std::max(0, gold));
    for (std::size_t index = 0U; index < digits.size(); ++index)
        drawDigit(target, digits[index] - '0', {62.0F + static_cast<float>(index) * 30.0F, 58.0F});
}
}

int main()
{
    sf::RenderWindow window(sf::VideoMode({WindowWidth, WindowHeight}), "Arcane Spire");
    window.setVerticalSyncEnabled(true);

    arcane::platform::SfmlInputMapper inputMapper;
    arcane::app::TowerSessionConfig config;
    config.worldBounds = {0.0F, static_cast<float>(WindowWidth), GroundTop};
    arcane::app::AppFlowController app(0xC0FFEEU, config);
    sf::Clock frameClock;

    while (window.isOpen())
    {
        while (const auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>()) window.close();
        }

        const float deltaSeconds = std::min(frameClock.restart().asSeconds(), MaximumFrameTime);
        app.update(inputMapper.sample(), deltaSeconds);
        if (app.screen() == arcane::app::AppScreen::Start)
            window.setTitle(app.canContinue() ? "Arcane Spire | CONTINUE - Enter" : "Arcane Spire | START - Enter");
        else if (app.screen() == arcane::app::AppScreen::Pause)
            window.setTitle("Arcane Spire | PAUSE - Esc Resume | Enter Exit To Start | R Restart Floor");
        else if (app.screen() == arcane::app::AppScreen::Result)
            window.setTitle(app.victory() ? "Arcane Spire | WIN - Enter Start" : "Arcane Spire | DEFEAT - Enter Start");
        else if (app.tower()) window.setTitle(makeWindowTitle(*app.tower()) + " | Esc Pause");

        sf::Color background {18, 20, 28};
        if (app.screen() == arcane::app::AppScreen::Result)
            background = app.victory() ? sf::Color {20, 67, 49} : sf::Color {68, 24, 31};
        window.clear(background);

        if (app.screen() == arcane::app::AppScreen::Start)
            drawMenuPanels(window, 1U, app.canContinue() ? sf::Color {55, 105, 130} : sf::Color {63, 116, 82});
        else if (app.screen() == arcane::app::AppScreen::Pause)
            drawMenuPanels(window, 2U, sf::Color {74, 66, 105});
        else if (app.screen() == arcane::app::AppScreen::Result)
            drawMenuPanels(window, 1U, app.victory() ? sf::Color {63, 116, 82} : sf::Color {116, 63, 72});
        else if (const auto* tower = app.tower())
        {
            const auto phase = tower->run().phase();
            if (phase == arcane::game::run::RunPhase::InEncounter
                || phase == arcane::game::run::RunPhase::FloorComplete)
            {
                if (tower->combat())
                {
                    drawCombat(window, *tower);
                    drawStaircase(window, tower->staircaseBounds(), tower->staircaseUnlocked());
                }
            }
            if (phase == arcane::game::run::RunPhase::Reward) drawRewardScreen(window, *tower);
            if (phase == arcane::game::run::RunPhase::InEncounter
                && (tower->currentFloorType() == arcane::game::run::FloorType::Merchant
                    || tower->currentFloorType() == arcane::game::run::FloorType::Event))
            {
                drawSpecialFloor(window, *tower);
                if (tower->specialPanelOpen() && tower->currentFloorType() == arcane::game::run::FloorType::Merchant)
                    drawMerchantScreen(window, *tower);
                if (tower->specialPanelOpen() && tower->currentFloorType() == arcane::game::run::FloorType::Event)
                    drawEventScreen(window, *tower);
            }
            int displayedHealth = tower->run().player().currentHp;
            if (tower->combat()) displayedHealth = tower->combat()->playerState().currentHealth;
            drawHealthBar(window, {32.0F, 28.0F}, {300.0F, 22.0F}, displayedHealth,
                tower->run().player().maxHp, sf::Color {108, 206, 126});
            drawGold(window, tower->run().player().gold);
            if (!tower->loadoutOpen()) drawEquippedSlots(window, tower->run().player());
            else drawLoadoutOverlay(window, *tower);
        }

        window.display();
    }

    return 0;
}
