#include "presentation/views/ScreenViews.hpp"

#include "presentation/views/CombatView.hpp"
#include "presentation/views/UiPrimitives.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <string_view>

namespace arcane::presentation::views
{
namespace ui = viewmodel;

void drawRewardScreen(sf::RenderTarget& target, const ui::RewardViewModel& model,
    const arcane::presentation::SpellCardArt& spellCards)
{
    drawPixelText(target, "CHOOSE ONE SPELLBOOK", {465.0F, 70.0F}, 2.0F,
        sf::Color {255, 231, 145});
    drawPixelText(target, "READ THE EFFECT THEN PRESS U I OR O", {390.0F, 110.0F}, 1.3F,
        sf::Color {182, 174, 215});
    if (model.showRerollHint)
        drawPixelText(target, "R REROLL - ONCE PER ACT", {510.0F, 132.0F}, 0.9F,
            sf::Color {145, 218, 255});

    constexpr std::array<float, 3> CardX {270.0F, 535.0F, 800.0F};
    constexpr std::array<std::string_view, 3> Keys {"U", "I", "O"};
    for (std::size_t index = 0; index < model.cards.size(); ++index)
    {
        const auto& card = model.cards[index];
        drawPixelText(target, Keys[index], {CardX[index] + 96.0F, 150.0F}, 1.5F,
            sf::Color {255, 231, 145});
        drawCard(target, card.summary, {CardX[index], 180.0F}, {210.0F, 340.0F},
            false, &spellCards);
        if (card.spell)
        {
            sf::RectangleShape informationPanel({194.0F, 154.0F});
            informationPanel.setPosition({CardX[index] + 8.0F, 358.0F});
            informationPanel.setFillColor(sf::Color {15, 17, 29, 220});
            informationPanel.setOutlineColor(sf::Color {225, 220, 240, 180});
            informationPanel.setOutlineThickness(2.0F);
            target.draw(informationPanel);

            drawPixelText(target, card.summary.name, {CardX[index] + 8.0F, 370.0F}, 1.05F,
                sf::Color {255, 238, 173});
            drawPixelText(target, wrapPixelText(card.description, 38U),
                {CardX[index] + 8.0F, 394.0F}, 0.80F);
            drawPixelText(target, "CD " + formatTenths(card.spell->cooldownSeconds) + " SEC",
                {CardX[index] + 8.0F, 472.0F}, 0.95F, sf::Color {145, 218, 255});
            drawPixelText(target, spellRangeText(*card.spell),
                {CardX[index] + 8.0F, 492.0F}, 0.85F, sf::Color {174, 242, 184});
        }
    }
}

void drawMerchantScreen(sf::RenderTarget& target, const ui::MerchantViewModel& model,
    const arcane::presentation::SpellCardArt& spellCards)
{
    sf::RectangleShape panel({1120.0F, 650.0F});
    panel.setPosition({80.0F, 45.0F});
    panel.setFillColor(sf::Color {18, 20, 34, 242});
    panel.setOutlineColor(sf::Color {205, 159, 75});
    panel.setOutlineThickness(4.0F);
    target.draw(panel);
    drawPixelText(target, "MERCHANT", {535.0F, 65.0F}, 2.0F, sf::Color {255, 225, 145});
    drawPixelText(target, "DETAILS", {130.0F, 125.0F}, 1.35F, sf::Color {255, 225, 145});
    drawPixelText(target, "SPELLBOOKS", {320.0F, 125.0F}, 1.35F);
    drawPixelText(target, "RELICS", {320.0F, 405.0F}, 1.35F);
    drawPixelText(target, "WASD SELECT   ENTER BUY   E CLOSE", {390.0F, 660.0F}, 1.15F,
        sf::Color {175, 164, 214});

    sf::RectangleShape detailPanel({190.0F, 450.0F});
    detailPanel.setPosition({100.0F, 165.0F});
    detailPanel.setFillColor(sf::Color {12, 14, 25, 225});
    detailPanel.setOutlineColor(sf::Color {205, 159, 75});
    detailPanel.setOutlineThickness(2.0F);
    target.draw(detailPanel);

    if (model.selectedDetail)
    {
        const auto& detail = *model.selectedDetail;
        if (detail.summary.kind == ui::ContentKind::Spell && detail.spell)
        {
            drawPixelText(target, "SPELL", {115.0F, 185.0F}, 1.0F,
                sf::Color {145, 218, 255});
            drawPixelText(target, wrapPixelText(detail.summary.name, 25U),
                {115.0F, 215.0F}, 0.95F, sf::Color {255, 238, 173});
            drawPixelText(target, wrapPixelText(detail.description, 34U),
                {115.0F, 275.0F}, 0.78F);
            drawPixelText(target, "CD " + formatTenths(detail.spell->cooldownSeconds) + " SEC",
                {115.0F, 520.0F}, 0.90F, sf::Color {145, 218, 255});
            drawPixelText(target, wrapPixelText(spellRangeText(*detail.spell), 30U),
                {115.0F, 550.0F}, 0.82F, sf::Color {174, 242, 184});
        }
        else if (detail.summary.kind == ui::ContentKind::Relic)
        {
            drawPixelText(target, "RELIC", {115.0F, 185.0F}, 1.0F,
                sf::Color {220, 177, 255});
            drawPixelText(target, wrapPixelText(detail.summary.name, 25U),
                {115.0F, 215.0F}, 0.95F, sf::Color {255, 238, 173});
            drawPixelText(target, wrapPixelText(detail.description, 34U),
                {115.0F, 275.0F}, 0.78F);
        }
    }

    constexpr std::array<float, 3> CardX {320.0F, 590.0F, 860.0F};
    std::size_t spellColumn = 0U;
    std::size_t relicColumn = 0U;
    for (const auto& item : model.items)
    {
        const bool spellItem = item.content.kind == ui::ContentKind::Spell;
        const std::size_t column = spellItem ? spellColumn++ : relicColumn++;
        if (column >= CardX.size()) continue;
        const float y = spellItem ? 145.0F : 425.0F;
        drawCard(target, item.content, {CardX[column], y}, {165.0F, 175.0F},
            item.content.selected,
            &spellCards);
        drawPixelText(target, item.content.name, {CardX[column], y + 185.0F}, 1.0F);
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
    const bool meteor = tower.eventKind() == arcane::app::EventKind::HalfCenturyMeteorShower;
    const bool swordVillage = tower.eventKind() == arcane::app::EventKind::SwordVillage;
    const std::string_view title = swordVillage ? "VILLAGE OF THE SWORD"
        : (meteor ? "HALF CENTURY METEOR SHOWER" : "LORD ORDEN'S BALL");
    drawPixelText(target, title, {swordVillage ? 405.0F : (meteor ? 315.0F : 430.0F), 70.0F},
        2.1F, sf::Color {255, 231, 145});
    drawPixelText(target, swordVillage
        ? "A HERO'S SWORD LIES SEALED HERE. ONLY A TRUE HERO MAY DRAW IT."
        : (meteor ? "A METEOR SHOWER SEEN ONCE IN FIFTY YEARS PASSES ABOVE."
            : "LORD ORDEN ASKS STARK TO POSE AS HIS SON AT A SOCIAL BALL."),
        {175.0F, 115.0F}, 1.15F);
    drawPixelText(target, swordVillage
        ? "HIMMEL COULD NOT DRAW IT. WHAT DO YOU THINK?"
        : (meteor ? "TO YOU IT IS NOT SO RARE. HOW WILL YOU SPEND THE NIGHT?"
            : "HE PROMISES YOU A REWARD. YOUR CHOICE IS:"),
        {245.0F, 145.0F}, 1.15F);

    const std::array<std::string_view, 3> effects = swordVillage
        ? std::array<std::string_view, 3> {"GAIN HERO SWORD", "GAIN TRUE HERO SWORD", "+50 GOLD"}
        : (meteor
            ? std::array<std::string_view, 3> {"GAIN A RANDOM SPELLBOOK", "RESTORE HP TO MAX", "50 PERCENT CHANCE OF 99 GOLD"}
            : std::array<std::string_view, 3> {"+30 MAX HP", "GAIN A RANDOM SPELLBOOK", "+50 GOLD"});
    if (tower.eventFloorState() == arcane::app::EventFloorState::Result)
    {
        if (tower.eventResultChoice())
        {
            drawCard(target, *tower.eventResultChoice(), {535.0F, 205.0F}, {210.0F, 280.0F}, true);
            const auto base = swordVillage ? 5201U : (meteor ? 5101U : 5001U);
            const auto choiceIndex = static_cast<std::size_t>(*tower.eventResultChoice() - base);
            if (choiceIndex < effects.size())
            {
                std::string resultText {effects[choiceIndex]};
                if (meteor && choiceIndex == 2U)
                    resultText = tower.eventChoices()[2].goldDelta == 99
                        ? "WISH GRANTED +99 GOLD" : "THE WISH RECEIVED NO ANSWER";
                drawPixelText(target, resultText, {440.0F, 515.0F}, 1.4F,
                    sf::Color {255, 231, 145});
            }
        }
        return;
    }
    constexpr std::array<float, 3> CardX {270.0F, 535.0F, 800.0F};
    const auto choices = tower.eventChoices();
    for (std::size_t index = 0U; index < choices.size() && index < CardX.size(); ++index)
        drawCard(target, choices[index].id, {CardX[index], 205.0F}, {210.0F, 280.0F}, false);
    for (std::size_t index = 0U; index < choices.size() && index < effects.size(); ++index)
        drawPixelText(target, effects[index], {CardX[index], 505.0F}, 1.25F);
}

void drawStaircase(sf::RenderTarget& target, arcane::game::Aabb bounds, bool unlocked);

void drawPlayer(sf::RenderTarget& target, const arcane::presentation::PlayerAnimator& animator,
    const arcane::presentation::PlayerVisualState& player, const sf::Color fallbackColor,
    const sf::Color spriteTint)
{
    const sf::Vector2f bottomCenter {
        player.position.x + arcane::game::PlayerController::Width * 0.5F,
        player.position.y + arcane::game::PlayerController::Height
    };
    if (animator.draw(target, bottomCenter, player.facingDirection, spriteTint)) return;

    sf::RectangleShape shape(
        {arcane::game::PlayerController::Width, arcane::game::PlayerController::Height});
    shape.setPosition({player.position.x, player.position.y});
    sf::Color tintedFallback = fallbackColor;
    tintedFallback.a = spriteTint.a;
    if (spriteTint.r == 255 && spriteTint.g < 255)
        tintedFallback = sf::Color {255, spriteTint.g, spriteTint.b, spriteTint.a};
    shape.setFillColor(tintedFallback);
    shape.setOutlineColor(sf::Color {142, 115, 200});
    shape.setOutlineThickness(3.0F);
    target.draw(shape);
}

void drawSpecialFloor(sf::RenderTarget& target, const arcane::app::TowerSession& tower,
    const arcane::presentation::PlayerAnimator& playerAnimator)
{
    sf::RectangleShape ground({static_cast<float>(WindowWidth), static_cast<float>(WindowHeight) - GroundTop});
    ground.setPosition({0.0F, GroundTop});
    ground.setFillColor(sf::Color {64, 72, 88});
    target.draw(ground);
    if (const auto* player = tower.explorationPlayer())
    {
        drawPlayer(target, playerAnimator,
            makePlayerVisualState(*player, tower.run().player().currentHp),
            sf::Color {232, 232, 242});
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

void drawLoadoutOverlay(sf::RenderTarget& target, const ui::LoadoutSnapshot& model,
    const arcane::presentation::SpellCardArt& spellCards)
{
    sf::RectangleShape overlay({static_cast<float>(WindowWidth), static_cast<float>(WindowHeight)});
    overlay.setFillColor(sf::Color {14, 12, 25, 235});
    target.draw(overlay);

    const bool spellPage = model.page == ui::LoadoutPage::Spells;
    drawPixelText(target, "SPELLS PAGE 1 OF 2", {120.0F, 30.0F}, 1.6F,
        spellPage ? sf::Color {255, 231, 145} : sf::Color {125, 126, 145});
    drawPixelText(target, "Q E SWITCH PAGE", {510.0F, 32.0F}, 1.4F, sf::Color {182, 174, 215});
    drawPixelText(target, "RELICS PAGE 2 OF 2", {875.0F, 30.0F}, 1.6F,
        spellPage ? sf::Color {125, 126, 145} : sf::Color {255, 231, 145});

    if (!spellPage)
    {
        const auto& relics = model.relics;
        constexpr std::size_t Columns = 8U;
        constexpr float CardWidth = 110.0F;
        constexpr float CardHeight = 72.0F;
        constexpr float HorizontalGap = 20.0F;
        constexpr float VerticalGap = 38.0F;
        constexpr float StartX = 120.0F;
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
                relics[index].selected);
            if (relics[index].kind == ui::ContentKind::Relic)
                drawPixelText(target, wrapPixelText(relics[index].name, 18U),
                    {position.x, position.y + CardHeight + 7.0F}, 0.78F);
            else
                drawPixelText(target, "UNKNOWN RELIC", {position.x, position.y + CardHeight + 10.0F}, 1.0F);
        }

        if (model.selectedDetail)
        {
            const auto& detail = *model.selectedDetail;
            if (detail.summary.kind == ui::ContentKind::Relic)
            {
                drawPixelText(target, detail.summary.name, {120.0F, 470.0F}, 1.6F,
                    sf::Color {255, 231, 145});
                drawPixelText(target, wrapPixelText(detail.description, 115U),
                    {120.0F, 505.0F}, 0.95F);
            }
            else
            {
                drawPixelText(target, "RELIC DEFINITION MISSING", {157.5F, 535.0F}, 1.5F,
                    sf::Color {235, 105, 105});
            }
        }
        return;
    }

    const auto& learned = model.regularSpells;
    const auto& learnedBoss = model.bossSpells;
    constexpr std::size_t Columns = 10U;
    constexpr float CardWidth = 90.0F;
    constexpr float CardHeight = 95.0F;
    constexpr float HorizontalGap = 15.0F;
    constexpr float VerticalGap = 20.0F;
    constexpr float StartX = 120.0F;
    constexpr float StartY = 110.0F;

    drawPixelText(target, "REGULAR SPELLS", {190.0F, 78.0F}, 1.3F,
        model.spellSection == ui::SpellSection::Regular
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
            learned[index].selected,
            &spellCards);
    }

    drawPixelText(target, model.equipped.ultimateUnlocked ? "BOSS SPELLS - R EQUIP ULTIMATE"
            : "BOSS SPELLS - DEFEAT BOSS 1 TO UNLOCK",
        {120.0F, 330.0F}, 1.25F,
        model.spellSection == ui::SpellSection::Boss
            ? sf::Color {255, 205, 92} : sf::Color {130, 130, 150});
    for (std::size_t index = 0; index < learnedBoss.size(); ++index)
    {
        const sf::Vector2f position {
            StartX + static_cast<float>(index % Columns) * (CardWidth + HorizontalGap),
            365.0F + static_cast<float>(index / Columns) * (CardHeight + VerticalGap)
        };
        drawCard(target, learnedBoss[index], position, {CardWidth, CardHeight},
            learnedBoss[index].selected, &spellCards);
    }

    drawEquippedSlots(target, model.equipped, spellCards);
    if (model.selectedDetail)
    {
        const auto& detail = *model.selectedDetail;
        if (detail.summary.kind == ui::ContentKind::Spell && detail.spell)
        {
            drawPixelText(target, detail.summary.name, {120.0F, 500.0F}, 1.5F,
                sf::Color {255, 231, 145});
            drawPixelText(target, wrapPixelText(detail.description, 105U),
                {120.0F, 528.0F}, 0.85F);
            drawPixelText(target, "CD " + formatTenths(detail.spell->cooldownSeconds) + " SEC",
                {120.0F, 590.0F}, 0.95F, sf::Color {145, 218, 255});
            drawPixelText(target, spellRangeText(*detail.spell),
                {420.0F, 590.0F}, 0.90F, sf::Color {174, 242, 184});
        }
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
}
