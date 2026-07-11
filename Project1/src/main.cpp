#include "app/run/AppFlowController.hpp"
#include "game/relics/RelicSystem.hpp"
#include "game/spells/SpellSystem.hpp"
#include "platform/SfmlInputMapper.hpp"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <cctype>

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

std::array<std::uint8_t, 7> glyphRows(const char value)
{
    switch (static_cast<char>(std::toupper(static_cast<unsigned char>(value))))
    {
    case 'A': return {14, 17, 17, 31, 17, 17, 17}; case 'B': return {30, 17, 17, 30, 17, 17, 30};
    case 'C': return {14, 17, 16, 16, 16, 17, 14}; case 'D': return {30, 17, 17, 17, 17, 17, 30};
    case 'E': return {31, 16, 16, 30, 16, 16, 31}; case 'F': return {31, 16, 16, 30, 16, 16, 16};
    case 'G': return {14, 17, 16, 23, 17, 17, 15}; case 'H': return {17, 17, 17, 31, 17, 17, 17};
    case 'I': return {31, 4, 4, 4, 4, 4, 31}; case 'J': return {7, 2, 2, 2, 18, 18, 12};
    case 'K': return {17, 18, 20, 24, 20, 18, 17}; case 'L': return {16, 16, 16, 16, 16, 16, 31};
    case 'M': return {17, 27, 21, 21, 17, 17, 17}; case 'N': return {17, 25, 21, 19, 17, 17, 17};
    case 'O': return {14, 17, 17, 17, 17, 17, 14}; case 'P': return {30, 17, 17, 30, 16, 16, 16};
    case 'Q': return {14, 17, 17, 17, 21, 18, 13}; case 'R': return {30, 17, 17, 30, 20, 18, 17};
    case 'S': return {15, 16, 16, 14, 1, 1, 30}; case 'T': return {31, 4, 4, 4, 4, 4, 4};
    case 'U': return {17, 17, 17, 17, 17, 17, 14}; case 'V': return {17, 17, 17, 17, 17, 10, 4};
    case 'W': return {17, 17, 17, 21, 21, 21, 10}; case 'X': return {17, 17, 10, 4, 10, 17, 17};
    case 'Y': return {17, 17, 10, 4, 4, 4, 4}; case 'Z': return {31, 1, 2, 4, 8, 16, 31};
    case '0': return {14, 17, 19, 21, 25, 17, 14}; case '1': return {4, 12, 4, 4, 4, 4, 14};
    case '2': return {14, 17, 1, 2, 4, 8, 31}; case '3': return {30, 1, 1, 14, 1, 1, 30};
    case '4': return {2, 6, 10, 18, 31, 2, 2}; case '5': return {31, 16, 16, 30, 1, 1, 30};
    case '6': return {14, 16, 16, 30, 17, 17, 14}; case '7': return {31, 1, 2, 4, 8, 8, 8};
    case '8': return {14, 17, 17, 14, 17, 17, 14}; case '9': return {14, 17, 17, 15, 1, 1, 14};
    case '+': return {0, 4, 4, 31, 4, 4, 0}; case '-': return {0, 0, 0, 31, 0, 0, 0};
    case '%': return {17, 2, 4, 8, 17, 0, 0}; case ':': return {0, 4, 4, 0, 4, 4, 0};
    default: return {};
    }
}

void drawPixelText(sf::RenderTarget& target, const std::string_view text, const sf::Vector2f origin,
    const float scale = 2.0F, const sf::Color color = sf::Color {235, 235, 245})
{
    sf::Vector2f cursor = origin;
    for (const char character : text)
    {
        if (character == '\n') { cursor.x = origin.x; cursor.y += 9.0F * scale; continue; }
        const auto rows = glyphRows(character);
        for (std::size_t row = 0U; row < rows.size(); ++row)
            for (std::size_t column = 0U; column < 5U; ++column)
                if ((rows[row] & (1U << (4U - column))) != 0U)
                {
                    sf::RectangleShape pixel({scale, scale});
                    pixel.setPosition({cursor.x + static_cast<float>(column) * scale,
                        cursor.y + static_cast<float>(row) * scale});
                    pixel.setFillColor(color);
                    target.draw(pixel);
                }
        cursor.x += 6.0F * scale;
    }
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

void drawEquippedSlots(sf::RenderTarget& target, const arcane::game::run::PlayerProgress& player,
    const arcane::game::PlayerStateView* combatView = nullptr)
{
    constexpr float SlotSize = 66.0F;
    constexpr float Gap = 18.0F;
    constexpr float UltimateGap = 34.0F;
    const float totalWidth = SlotSize * 4.0F + Gap * 2.0F + UltimateGap;
    const float startX = (static_cast<float>(WindowWidth) - totalWidth) * 0.5F;

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
        if (combatView && combatView->spellSlots[index].cooldownDuration > 0.0F)
        {
            const float ratio = std::clamp(combatView->spellSlots[index].cooldownRemaining
                / combatView->spellSlots[index].cooldownDuration, 0.0F, 1.0F);
            sf::RectangleShape cooldown({SlotSize, SlotSize * ratio});
            cooldown.setPosition({startX + static_cast<float>(index) * (SlotSize + Gap), 630.0F});
            cooldown.setFillColor(sf::Color {8, 10, 18, 190});
            target.draw(cooldown);
        }
    }

    const float ultimateX = startX + SlotSize * 3.0F + Gap * 2.0F + UltimateGap;
    sf::RectangleShape ultimateSlot({SlotSize, SlotSize});
    ultimateSlot.setPosition({ultimateX, 630.0F});
    ultimateSlot.setFillColor(player.equippedUltimateSpell
        ? colorForContent(*player.equippedUltimateSpell)
        : sf::Color {38, 41, 52});
    ultimateSlot.setOutlineColor(player.ultimateSpellUnlocked
        ? sf::Color {255, 205, 92}
        : sf::Color {85, 85, 98});
    ultimateSlot.setOutlineThickness(4.0F);
    target.draw(ultimateSlot);
    if (combatView && combatView->ultimateSpellSlot.cooldownDuration > 0.0F)
    {
        const float ratio = std::clamp(combatView->ultimateSpellSlot.cooldownRemaining
            / combatView->ultimateSpellSlot.cooldownDuration, 0.0F, 1.0F);
        sf::RectangleShape cooldown({SlotSize, SlotSize * ratio});
        cooldown.setPosition({ultimateX, 630.0F});
        cooldown.setFillColor(sf::Color {8, 10, 18, 205});
        target.draw(cooldown);
    }
    drawPixelText(target, player.ultimateSpellUnlocked ? "R" : "LOCK",
        {ultimateX + (player.ultimateSpellUnlocked ? 27.0F : 10.0F), 607.0F}, 1.0F,
        player.ultimateSpellUnlocked ? sf::Color {255, 221, 130} : sf::Color {105, 105, 118});
}

void drawRewardScreen(sf::RenderTarget& target, const arcane::app::TowerSession& tower)
{
    const auto candidates = tower.rewardCandidates();
    if (!candidates) return;

    constexpr std::array<float, 3> CardX {270.0F, 535.0F, 800.0F};
    for (std::size_t index = 0; index < candidates->size(); ++index)
    {
        drawCard(target, (*candidates)[index], {CardX[index], 205.0F}, {210.0F, 280.0F}, false);
        if (const auto* definition = arcane::game::spells::findDefinition((*candidates)[index]))
            drawPixelText(target, definition->name, {CardX[index], 505.0F}, 1.5F);
    }
}

void drawMerchantScreen(sf::RenderTarget& target, const arcane::app::TowerSession& tower)
{
    sf::RectangleShape panel({1120.0F, 650.0F});
    panel.setPosition({80.0F, 45.0F});
    panel.setFillColor(sf::Color {18, 20, 34, 242});
    panel.setOutlineColor(sf::Color {205, 159, 75});
    panel.setOutlineThickness(4.0F);
    target.draw(panel);
    drawPixelText(target, "MERCHANT", {535.0F, 65.0F}, 2.0F, sf::Color {255, 225, 145});
    drawPixelText(target, "SPELLBOOKS", {125.0F, 125.0F}, 1.35F);
    drawPixelText(target, "RELICS", {125.0F, 405.0F}, 1.35F);
    drawPixelText(target, "WASD SELECT   ENTER BUY   E CLOSE", {390.0F, 660.0F}, 1.15F,
        sf::Color {175, 164, 214});

    constexpr std::array<float, 3> CardX {320.0F, 590.0F, 860.0F};
    const auto& stock = tower.merchantStock();
    std::size_t spellColumn = 0U;
    std::size_t relicColumn = 0U;
    for (const auto& item : stock)
    {
        const bool spellItem = item.kind == arcane::game::economy::ItemKind::Spell;
        const std::size_t column = spellItem ? spellColumn++ : relicColumn++;
        if (column >= CardX.size()) continue;
        const float y = spellItem ? 145.0F : 425.0F;
        const bool selected = tower.selectedMerchantItem() == item.id;
        drawCard(target, item.id, {CardX[column], y}, {165.0F, 175.0F}, selected);
        if (const auto* spell = arcane::game::spells::findDefinition(item.id))
            drawPixelText(target, spell->name, {CardX[column], y + 185.0F}, 1.0F);
        else if (const auto* relic = arcane::game::relics::findDefinition(item.id))
            drawPixelText(target, relic->name, {CardX[column], y + 185.0F}, 1.0F);
        drawPixelText(target, std::to_string(item.price) + " GOLD",
            {CardX[column], y + 210.0F}, 1.15F, sf::Color {255, 225, 145});
    }
}

void drawEventScreen(sf::RenderTarget& target, const arcane::app::TowerSession& tower)
{
    sf::RectangleShape panel({1120.0F, 650.0F});
    panel.setPosition({80.0F, 45.0F});
    panel.setFillColor(sf::Color {22, 15, 38, 244});
    panel.setOutlineColor(sf::Color {148, 105, 205});
    panel.setOutlineThickness(4.0F);
    target.draw(panel);
    drawPixelText(target, "LORD ORDEN'S BALL", {430.0F, 70.0F}, 2.1F, sf::Color {255, 231, 145});
    drawPixelText(target, "LORD ORDEN ASKS STARK TO POSE AS HIS SON AT A SOCIAL BALL.",
        {175.0F, 115.0F}, 1.15F);
    drawPixelText(target, "HE PROMISES YOU A REWARD. YOUR CHOICE IS:",
        {300.0F, 145.0F}, 1.15F);

    constexpr std::array<std::string_view, 3> Effects {
        "+30 MAX HP", "GAIN A RANDOM SPELLBOOK", "+50 GOLD"
    };
    if (tower.eventFloorState() == arcane::app::EventFloorState::Result)
    {
        if (tower.eventResultChoice())
        {
            drawCard(target, *tower.eventResultChoice(), {535.0F, 205.0F}, {210.0F, 280.0F}, true);
            const auto choiceIndex = static_cast<std::size_t>(*tower.eventResultChoice() - 5001U);
            if (choiceIndex < Effects.size())
                drawPixelText(target, Effects[choiceIndex], {500.0F, 515.0F}, 1.4F,
                    sf::Color {255, 231, 145});
        }
        return;
    }
    constexpr std::array<float, 3> CardX {270.0F, 535.0F, 800.0F};
    const auto choices = tower.eventChoices();
    for (std::size_t index = 0U; index < choices.size() && index < CardX.size(); ++index)
        drawCard(target, choices[index].id, {CardX[index], 205.0F}, {210.0F, 280.0F}, false);
    for (std::size_t index = 0U; index < choices.size() && index < Effects.size(); ++index)
        drawPixelText(target, Effects[index], {CardX[index], 505.0F}, 1.25F);
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

    const bool spellPage = tower.loadoutPage() == arcane::app::LoadoutPage::Spells;
    drawPixelText(target, "SPELLS PAGE 1 OF 2", {120.0F, 30.0F}, 1.6F,
        spellPage ? sf::Color {255, 231, 145} : sf::Color {125, 126, 145});
    drawPixelText(target, "Q E SWITCH PAGE", {510.0F, 32.0F}, 1.4F, sf::Color {182, 174, 215});
    drawPixelText(target, "RELICS PAGE 2 OF 2", {875.0F, 30.0F}, 1.6F,
        spellPage ? sf::Color {125, 126, 145} : sf::Color {255, 231, 145});

    if (!spellPage)
    {
        const auto& relics = tower.run().player().relics;
        constexpr std::size_t Columns = 6U;
        constexpr float CardWidth = 140.0F;
        constexpr float CardHeight = 90.0F;
        constexpr float HorizontalGap = 25.0F;
        constexpr float VerticalGap = 50.0F;
        constexpr float StartX = 157.5F;
        constexpr float StartY = 90.0F;

        if (relics.empty())
        {
            drawPixelText(target, "NO RELICS ACQUIRED", {465.0F, 300.0F}, 2.0F,
                sf::Color {160, 160, 180});
            return;
        }

        for (std::size_t index = 0U; index < relics.size(); ++index)
        {
            const std::size_t column = index % Columns;
            const std::size_t row = index / Columns;
            const sf::Vector2f position {
                StartX + static_cast<float>(column) * (CardWidth + HorizontalGap),
                StartY + static_cast<float>(row) * (CardHeight + VerticalGap)
            };
            drawCard(target, relics[index], position, {CardWidth, CardHeight},
                tower.selectedRelic() == relics[index]);
            if (const auto* definition = arcane::game::relics::findDefinition(relics[index]))
                drawPixelText(target, definition->name, {position.x, position.y + CardHeight + 10.0F}, 1.0F);
            else
                drawPixelText(target, "UNKNOWN RELIC", {position.x, position.y + CardHeight + 10.0F}, 1.0F);
        }

        if (tower.selectedRelic())
        {
            if (const auto* definition = arcane::game::relics::findDefinition(*tower.selectedRelic()))
            {
                drawPixelText(target, definition->name, {157.5F, 535.0F}, 1.8F,
                    sf::Color {255, 231, 145});
                drawPixelText(target, definition->description, {157.5F, 570.0F}, 1.25F);
            }
            else
            {
                drawPixelText(target, "RELIC DEFINITION MISSING", {157.5F, 535.0F}, 1.5F,
                    sf::Color {235, 105, 105});
            }
        }
        return;
    }

    const auto& player = tower.run().player();
    const auto& learned = player.learnedSpells;
    const auto& learnedBoss = player.learnedBossSpells;
    constexpr std::size_t Columns = 8U;
    constexpr float CardWidth = 90.0F;
    constexpr float CardHeight = 95.0F;
    constexpr float HorizontalGap = 20.0F;
    constexpr float VerticalGap = 20.0F;
    constexpr float StartX = 190.0F;
    constexpr float StartY = 110.0F;

    drawPixelText(target, "REGULAR SPELLS", {190.0F, 78.0F}, 1.3F,
        tower.spellLoadoutSection() == arcane::app::SpellLoadoutSection::Regular
            ? sf::Color {255, 231, 145} : sf::Color {130, 130, 150});

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

    drawPixelText(target, player.ultimateSpellUnlocked ? "BOSS SPELLS - R EQUIP ULTIMATE"
            : "BOSS SPELLS - DEFEAT BOSS 1 TO UNLOCK",
        {190.0F, 330.0F}, 1.25F,
        tower.spellLoadoutSection() == arcane::app::SpellLoadoutSection::Boss
            ? sf::Color {255, 205, 92} : sf::Color {130, 130, 150});
    for (std::size_t index = 0; index < learnedBoss.size(); ++index)
    {
        const sf::Vector2f position {
            StartX + static_cast<float>(index % Columns) * (CardWidth + HorizontalGap),
            365.0F + static_cast<float>(index / Columns) * (CardHeight + VerticalGap)
        };
        drawCard(target, learnedBoss[index], position, {CardWidth, CardHeight},
            tower.spellLoadoutSection() == arcane::app::SpellLoadoutSection::Boss
                && tower.selectedLearnedSpell() == learnedBoss[index]);
    }

    drawEquippedSlots(target, tower.run().player());
    if (tower.selectedLearnedSpell())
        if (const auto* definition = arcane::game::spells::findDefinition(*tower.selectedLearnedSpell()))
        {
            drawPixelText(target, definition->name, {190.0F, 520.0F}, 1.8F, sf::Color {255, 231, 145});
            drawPixelText(target, definition->description, {190.0F, 548.0F}, 1.15F);
        }
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
        const float width = bounds.width * static_cast<float>(StepCount - step) / static_cast<float>(StepCount);
        sf::RectangleShape shape({width, stepHeight});
        shape.setPosition({bounds.left + bounds.width - width,
            bounds.top + bounds.height - stepHeight * static_cast<float>(step + 1)});
        shape.setFillColor(unlocked ? sf::Color {146, 124, 174} : sf::Color {68, 69, 80});
        shape.setOutlineColor(unlocked ? sf::Color {226, 207, 244} : sf::Color {105, 106, 118});
        shape.setOutlineThickness(1.5F);
        target.draw(shape);
    }
}

void drawLootDrop(sf::RenderTarget& target, const arcane::game::Aabb bounds)
{
    sf::CircleShape glow(bounds.width * 0.65F, 32U);
    glow.setOrigin({glow.getRadius(), glow.getRadius()});
    glow.setPosition({bounds.left + bounds.width * 0.5F, bounds.top + bounds.height * 0.5F});
    glow.setFillColor(sf::Color {116, 188, 255, 70});
    target.draw(glow);

    sf::CircleShape drop(bounds.width * 0.5F, 4U);
    drop.setPosition({bounds.left, bounds.top});
    drop.setFillColor(sf::Color {116, 188, 255});
    drop.setOutlineColor(sf::Color {232, 246, 255});
    drop.setOutlineThickness(3.0F);
    target.draw(drop);
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
    playerShape.setFillColor(player.stunned ? sf::Color {112, 180, 235}
        : (player.attackActive ? sf::Color {255, 231, 153} : sf::Color {232, 232, 242}));
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
        sf::Color enemyColor = tower.currentFloorType() == arcane::game::run::FloorType::Boss
            ? sf::Color {129, 68, 172} : sf::Color {176, 70, 78};
        if (enemy.windingUp) enemyColor = sf::Color {242, 154, 69};
        if (enemy.attackActive) enemyColor = sf::Color {248, 222, 105};
        enemyShape.setFillColor(enemyColor);
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
        if (tower.loadoutPage() == arcane::app::LoadoutPage::Relics)
        {
            const std::string selected = tower.selectedRelic()
                ? (arcane::game::relics::findDefinition(*tower.selectedRelic())
                    ? arcane::game::relics::findDefinition(*tower.selectedRelic())->name
                    : std::to_string(*tower.selectedRelic()))
                : std::string {"None"};
            return title + "RELIC COLLECTION - Selected " + selected
                + " | A/D Select, Q/E Spell Page, Tab Close";
        }
        const std::string selected = tower.selectedLearnedSpell()
            ? (arcane::game::spells::findDefinition(*tower.selectedLearnedSpell())
                ? arcane::game::spells::findDefinition(*tower.selectedLearnedSpell())->name
                : std::to_string(*tower.selectedLearnedSpell()))
            : std::string {"None"};
        const bool bossSection = tower.spellLoadoutSection() == arcane::app::SpellLoadoutSection::Boss;
        return title + (bossSection ? "BOSS SPELL LOADOUT - Selected " : "REGULAR SPELL LOADOUT - Selected ")
            + selected + (bossSection ? " | A/D Select, R Equip Ultimate" : " | A/D Select, U/I/O Equip Slot")
            + ", W/S Switch Spell Group, Q/E Relic Page, Tab Close";
    }

    switch (run.phase())
    {
    case arcane::game::run::RunPhase::InEncounter:
        if (tower.currentFloorType() == arcane::game::run::FloorType::Merchant)
        {
            if (!tower.specialPanelOpen()) return title + "MERCHANT ROOM - Meet NPC, E Trade | Rear Staircase Exits";
            return title + "MERCHANT - WASD Select | Enter Buy | E Close";
        }
        if (tower.currentFloorType() == arcane::game::run::FloorType::Event)
        {
            if (!tower.specialPanelOpen())
                return title + (tower.eventFloorState() == arcane::app::EventFloorState::Result
                    ? "EVENT RESOLVED - E Review NPC Result | Rear Staircase Exits"
                    : "EVENT ROOM - Meet NPC, E Interact");
            if (tower.eventFloorState() == arcane::app::EventFloorState::Result)
                return title + "EVENT RESULT - E Close";
            return title + "ALDEN BALL - U Dessert(+30 MaxHP), I Grimoire(Random Spell), O Commission(+50 Gold), E Close";
        }
        return title + "A/D Move, Space Jump, J Attack, U/I/O Spells, R Ultimate, Tab Loadout";
    case arcane::game::run::RunPhase::LootPending:
        return title + "ENEMY DROP - Move To Drop, E Inspect Reward | Tab Loadout";
    case arcane::game::run::RunPhase::Reward:
    {
        const auto candidates = tower.rewardCandidates();
        if (!candidates) return title + "Reward";
        const auto name = [](const arcane::game::run::ContentId id) {
            const auto* definition = arcane::game::spells::findDefinition(id);
            return definition ? std::string {definition->name} : std::to_string(id);
        };
        return title + "Choose U=" + name((*candidates)[0])
            + " I=" + name((*candidates)[1])
            + " O=" + name((*candidates)[2]) + " | Tab Loadout";
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

void drawMenuPanel(sf::RenderTarget& target, const float y, const sf::Color color,
    const std::string_view label, const bool selected)
{
    sf::RectangleShape panel({520.0F, 82.0F});
    panel.setPosition({380.0F, y});
    panel.setFillColor(selected ? sf::Color {
        static_cast<std::uint8_t>(std::min(255, static_cast<int>(color.r) + 35)),
        static_cast<std::uint8_t>(std::min(255, static_cast<int>(color.g) + 35)),
        static_cast<std::uint8_t>(std::min(255, static_cast<int>(color.b) + 35))
    } : color);
    panel.setOutlineColor(selected ? sf::Color {255, 231, 145} : sf::Color {175, 170, 195});
    panel.setOutlineThickness(selected ? 6.0F : 3.0F);
    target.draw(panel);
    drawPixelText(target, label, {410.0F, y + 28.0F}, 2.0F,
        selected ? sf::Color {255, 241, 184} : sf::Color {225, 220, 240});
}

void drawStartMenu(sf::RenderTarget& target, const bool canContinue)
{
    drawPixelText(target, "ARCANE SPIRE", {485.0F, 150.0F}, 2.5F, sf::Color {232, 218, 255});
    drawMenuPanel(target, 270.0F, canContinue ? sf::Color {55, 105, 130} : sf::Color {63, 116, 82},
        canContinue ? "CONTINUE" : "START NEW RUN", true);
    drawPixelText(target, "ENTER CONFIRM", {520.0F, 380.0F}, 1.5F);
    drawPixelText(target, "F2 EVENT PREVIEW", {500.0F, 430.0F}, 1.5F, sf::Color {190, 174, 225});
    drawPixelText(target, "F3 SHOP PREVIEW", {510.0F, 470.0F}, 1.5F, sf::Color {222, 190, 125});
}

void drawPauseMenu(sf::RenderTarget& target, const arcane::app::PauseMenuItem selection)
{
    drawPixelText(target, "PAUSED", {555.0F, 150.0F}, 2.5F, sf::Color {232, 218, 255});
    drawMenuPanel(target, 250.0F, sf::Color {74, 66, 105}, "REPLAY CURRENT FLOOR",
        selection == arcane::app::PauseMenuItem::ReplayCurrentFloor);
    drawMenuPanel(target, 370.0F, sf::Color {74, 66, 105}, "SAVE AND EXIT",
        selection == arcane::app::PauseMenuItem::SaveAndExit);
    drawPixelText(target, "W S OR UP DOWN SELECT", {445.0F, 500.0F}, 1.4F);
    drawPixelText(target, "ENTER CONFIRM   ESC RESUME", {420.0F, 540.0F}, 1.4F);
    drawPixelText(target, "SAVE IS RETAINED UNTIL APP CLOSES", {380.0F, 590.0F}, 1.2F,
        sf::Color {170, 164, 190});
}

void drawResultMenu(sf::RenderTarget& target, const bool victory)
{
    drawPixelText(target, victory ? "VICTORY" : "DEFEAT", victory
        ? sf::Vector2f {535.0F, 170.0F} : sf::Vector2f {545.0F, 170.0F}, 2.5F,
        sf::Color {242, 232, 190});
    drawMenuPanel(target, 300.0F, victory ? sf::Color {63, 116, 82} : sf::Color {116, 63, 72},
        "START NEW RUN", true);
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
            window.setTitle(app.canContinue()
                ? "Arcane Spire | CONTINUE - Enter | F2 Event | F3 Shop"
                : "Arcane Spire | START - Enter | F2 Event | F3 Shop");
        else if (app.screen() == arcane::app::AppScreen::Pause)
            window.setTitle("Arcane Spire | PAUSE - W/S Select | Enter Confirm | Esc Resume");
        else if (app.screen() == arcane::app::AppScreen::Result)
            window.setTitle(app.victory() ? "Arcane Spire | WIN - Enter Start" : "Arcane Spire | DEFEAT - Enter Start");
        else if (app.tower()) window.setTitle(makeWindowTitle(*app.tower()) + " | Esc Pause");

        sf::Color background {18, 20, 28};
        if (app.screen() == arcane::app::AppScreen::Result)
            background = app.victory() ? sf::Color {20, 67, 49} : sf::Color {68, 24, 31};
        window.clear(background);

        if (app.screen() == arcane::app::AppScreen::Start)
            drawStartMenu(window, app.canContinue());
        else if (app.screen() == arcane::app::AppScreen::Pause)
            drawPauseMenu(window, app.pauseMenuItem());
        else if (app.screen() == arcane::app::AppScreen::Result)
            drawResultMenu(window, app.victory());
        else if (const auto* tower = app.tower())
        {
            const auto phase = tower->run().phase();
            if (phase == arcane::game::run::RunPhase::InEncounter
                || phase == arcane::game::run::RunPhase::LootPending
                || phase == arcane::game::run::RunPhase::FloorComplete)
            {
                if (tower->combat())
                {
                    drawCombat(window, *tower);
                    drawStaircase(window, tower->staircaseBounds(), tower->staircaseUnlocked());
                    if (const auto loot = tower->lootDropBounds()) drawLootDrop(window, *loot);
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
            std::optional<arcane::game::PlayerStateView> combatView;
            if (tower->combat())
            {
                combatView = tower->combat()->playerState();
                displayedHealth = combatView->currentHealth;
            }
            drawHealthBar(window, {32.0F, 28.0F}, {300.0F, 22.0F}, displayedHealth,
                tower->run().player().maxHp, sf::Color {108, 206, 126});
            drawGold(window, tower->run().player().gold);
            if (!tower->loadoutOpen()) drawEquippedSlots(window, tower->run().player(),
                combatView ? &*combatView : nullptr);
            else drawLoadoutOverlay(window, *tower);
        }

        window.display();
    }

    return 0;
}
