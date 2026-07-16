#include "presentation/views/ScreenViews.hpp"

#include "presentation/views/ArenaView.hpp"
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
namespace
{
float drawWrappedBlock(sf::RenderTarget& target, const std::string_view text,
    const sf::Vector2f position, const std::size_t width, const float scale,
    const sf::Color color = sf::Color::White)
{
    const auto wrapped = wrapPixelText(text, width);
    drawPixelText(target, wrapped, position, scale, color);
    const auto lines = 1U + static_cast<std::size_t>(
        std::count(wrapped.begin(), wrapped.end(), '\n'));
    return position.y + static_cast<float>(lines) * 9.0F * scale;
}
}

void drawRewardScreen(sf::RenderTarget& target, const ui::RewardViewModel& model,
    const arcane::presentation::SpellCardArt& spellCards)
{
    drawPixelText(target, "CHOOSE NEW MAGIC OR MASTERY", {390.0F, 54.0F}, 2.0F,
        sf::Color {255, 231, 145});
    drawPixelText(target, "READ THE COMPLETE EFFECT THEN PRESS U I OR O", {335.0F, 94.0F}, 1.2F,
        sf::Color {182, 174, 215});
    if (model.showRerollHint)
        drawPixelText(target, "R REROLL - ONCE PER ACT", {510.0F, 132.0F}, 0.9F,
            sf::Color {145, 218, 255});

    constexpr std::array<float, 3> CardX {270.0F, 535.0F, 800.0F};
    constexpr std::array<std::string_view, 3> Keys {"U", "I", "O"};
    for (std::size_t index = 0; index < model.cards.size(); ++index)
    {
        const auto& card = model.cards[index];
        drawPixelText(target, Keys[index], {CardX[index] + 96.0F, 134.0F}, 1.5F,
            sf::Color {255, 231, 145});
        drawCard(target, card.summary, {CardX[index], 145.0F}, {210.0F, 475.0F},
            false, &spellCards);
        if (card.spell)
        {
            sf::RectangleShape informationPanel({194.0F, 310.0F});
            informationPanel.setPosition({CardX[index] + 8.0F, 302.0F});
            informationPanel.setFillColor(sf::Color {15, 17, 29, 220});
            informationPanel.setOutlineColor(sf::Color {225, 220, 240, 180});
            informationPanel.setOutlineThickness(2.0F);
            target.draw(informationPanel);

            drawPixelText(target, card.summary.name, {CardX[index] + 8.0F, 312.0F}, 0.9F,
                sf::Color {255, 238, 173});
            drawPixelText(target, card.upgradeReward
                    ? "MASTERY -> RANK " + std::to_string(card.summary.rank)
                    : "NEW SPELL - RANK I",
                {CardX[index] + 8.0F, 330.0F}, 0.68F,
                card.upgradeReward ? sf::Color {220, 177, 255} : sf::Color {174, 242, 184});
            float nextY = drawWrappedBlock(target, card.description,
                {CardX[index] + 8.0F, 349.0F}, 44U, 0.56F) + 4.0F;
            if (!card.masteryDescription.empty())
            {
                drawPixelText(target, "MASTERY", {CardX[index] + 8.0F, nextY}, 0.56F,
                    sf::Color {220, 177, 255});
                nextY = drawWrappedBlock(target, card.masteryDescription,
                    {CardX[index] + 8.0F, nextY + 8.0F}, 47U, 0.49F,
                    sf::Color {220, 177, 255}) + 3.0F;
            }
            if (!card.synergies.empty())
            {
                drawPixelText(target, "COMBOS WITH OWNED MAGIC",
                    {CardX[index] + 8.0F, nextY}, 0.56F, sf::Color {255, 198, 105});
                nextY += 9.0F;
                for (const auto& synergy : card.synergies)
                {
                    drawPixelText(target, "+ " + std::string(synergy.ownedSpellName),
                        {CardX[index] + 8.0F, nextY}, 0.52F, sf::Color {255, 220, 138});
                    nextY = drawWrappedBlock(target, synergy.description,
                        {CardX[index] + 8.0F, nextY + 7.0F}, 50U, 0.43F,
                        sf::Color {232, 224, 205}) + 2.0F;
                }
            }
            drawPixelText(target, "CD " + formatTenths(card.spell->cooldownSeconds) + " SEC",
                {CardX[index] + 8.0F, 581.0F}, 0.72F, sf::Color {145, 218, 255});
            drawPixelText(target, spellRangeText(*card.spell),
                {CardX[index] + 8.0F, 596.0F}, 0.63F, sf::Color {174, 242, 184});
        }
    }
}

void drawBreakthroughScreen(sf::RenderTarget& target,
    const ui::BreakthroughViewModel& model)
{
    sf::RectangleShape backdrop({1120.0F, 610.0F});
    backdrop.setPosition({80.0F, 55.0F});
    backdrop.setFillColor(sf::Color {12, 13, 28, 246});
    backdrop.setOutlineColor(sf::Color {188, 147, 255});
    backdrop.setOutlineThickness(4.0F);
    target.draw(backdrop);
    drawPixelText(target, "MANA BREAKTHROUGH", {430.0F, 82.0F}, 2.2F,
        sf::Color {255, 231, 145});
    drawPixelText(target, "BOSS KNOWLEDGE BECOMES PERMANENT POWER - PRESS U I OR O",
        {270.0F, 128.0F}, 1.05F, sf::Color {182, 174, 215});

    constexpr std::array<float, 3> CardX {150.0F, 475.0F, 800.0F};
    constexpr std::array<std::string_view, 3> Keys {"U", "I", "O"};
    constexpr std::array<sf::Color, 3> Colors {
        sf::Color {225, 101, 112}, sf::Color {106, 202, 255}, sf::Color {124, 224, 168}};
    for (std::size_t index = 0U; index < model.cards.size(); ++index)
    {
        const auto& card = model.cards[index];
        sf::RectangleShape panel({280.0F, 360.0F});
        panel.setPosition({CardX[index], 190.0F});
        panel.setFillColor(sf::Color {24, 26, 45, 245});
        panel.setOutlineColor(Colors[index]);
        panel.setOutlineThickness(4.0F);
        target.draw(panel);
        drawPixelText(target, Keys[index], {CardX[index] + 128.0F, 160.0F}, 1.5F,
            sf::Color {255, 231, 145});
        drawPixelText(target, card.name, {CardX[index] + 25.0F, 230.0F}, 1.25F,
            Colors[index]);
        drawPixelText(target, "CURRENT RANK " + std::to_string(card.currentRank),
            {CardX[index] + 25.0F, 278.0F}, 0.95F,
            sf::Color {182, 174, 215});
        drawPixelText(target, wrapPixelText(card.description, 34U),
            {CardX[index] + 25.0F, 330.0F}, 0.95F);
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

void drawEventScreen(sf::RenderTarget& target, const arcane::app::TowerSession& tower,
    const std::array<std::optional<sf::Texture>, 3>& meteorCards,
    const std::array<std::optional<sf::Texture>, 3>& ordenCards,
    const std::array<std::optional<sf::Texture>, 3>& swordVillageCards,
    const std::array<std::optional<sf::Texture>, 3>& southernHeroCards)
{
    sf::RectangleShape panel({1120.0F, 650.0F});
    panel.setPosition({80.0F, 45.0F});
    panel.setFillColor(sf::Color {22, 15, 38, 244});
    panel.setOutlineColor(sf::Color {148, 105, 205});
    panel.setOutlineThickness(4.0F);
    target.draw(panel);
    const bool meteor = tower.eventKind() == arcane::app::EventKind::HalfCenturyMeteorShower;
    const bool swordVillage = tower.eventKind() == arcane::app::EventKind::SwordVillage;
    const bool southernHero = tower.eventKind() == arcane::app::EventKind::SouthernHero;
    const std::string_view title = southernHero ? "HERO OF THE SOUTH"
        : (swordVillage ? "VILLAGE OF THE SWORD"
            : (meteor ? "HALF CENTURY METEOR SHOWER" : "LORD ORDEN'S BALL"));
    drawPixelText(target, title, {southernHero ? 430.0F
            : (swordVillage ? 405.0F : (meteor ? 315.0F : 430.0F)), 70.0F},
        2.1F, sf::Color {255, 231, 145});
    drawPixelText(target, southernHero
        ? "THE ADVENTURER BEFORE YOU CLAIMS HE CAN SEE THE FUTURE."
        : (swordVillage
        ? "A HERO'S SWORD LIES SEALED HERE. ONLY A TRUE HERO MAY DRAW IT."
        : (meteor ? "A METEOR SHOWER SEEN ONCE IN FIFTY YEARS PASSES ABOVE."
            : "LORD ORDEN ASKS STARK TO POSE AS HIS SON AT A SOCIAL BALL.")),
        {175.0F, 115.0F}, 1.15F);
    drawPixelText(target, southernHero
        ? "HE OFFERS TO ANSWER ONE QUESTION. WHAT WILL YOU ASK?"
        : (swordVillage
        ? "HIMMEL COULD NOT DRAW IT. WHAT DO YOU THINK?"
        : (meteor ? "TO YOU IT IS NOT SO RARE. HOW WILL YOU SPEND THE NIGHT?"
            : "HE PROMISES YOU A REWARD. YOUR CHOICE IS:")),
        {245.0F, 145.0F}, 1.15F);

    const std::array<std::string_view, 3> effects = southernHero
        ? std::array<std::string_view, 3> {"FINAL BOSS DAMAGE +25%",
            "ALL OWNED SPELL RANKS +1", "+50 MAX HP"}
        : (swordVillage
        ? std::array<std::string_view, 3> {"GAIN HERO SWORD", "GAIN TRUE HERO SWORD", "+50 GOLD"}
        : (meteor
            ? std::array<std::string_view, 3> {"GAIN A RANDOM SPELLBOOK", "RESTORE HP TO MAX", "50 PERCENT CHANCE OF 99 GOLD"}
            : std::array<std::string_view, 3> {"+30 MAX HP", "GAIN A RANDOM SPELLBOOK", "+50 GOLD"}));
    if (tower.eventFloorState() == arcane::app::EventFloorState::Result)
    {
        if (tower.eventResultChoice())
        {
            const auto base = southernHero ? 5301U
                : (swordVillage ? 5201U : (meteor ? 5101U : 5001U));
            const auto choiceIndex = static_cast<std::size_t>(*tower.eventResultChoice() - base);
            const auto& eventCards = southernHero ? southernHeroCards
                : (swordVillage ? swordVillageCards : (meteor ? meteorCards : ordenCards));
            if (choiceIndex < eventCards.size() && eventCards[choiceIndex])
            {
                const sf::Texture& texture = *eventCards[choiceIndex];
                sf::Sprite card(texture);
                card.setPosition({535.0F, 205.0F});
                const auto size = texture.getSize();
                card.setScale({210.0F / static_cast<float>(size.x),
                    280.0F / static_cast<float>(size.y)});
                target.draw(card);
            }
            else
                drawCard(target, *tower.eventResultChoice(),
                    {535.0F, 205.0F}, {210.0F, 280.0F}, true);
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
    {
        const auto& eventCards = southernHero ? southernHeroCards
            : (swordVillage ? swordVillageCards : (meteor ? meteorCards : ordenCards));
        if (eventCards[index])
        {
            const sf::Texture& texture = *eventCards[index];
            sf::Sprite card(texture);
            card.setPosition({CardX[index], 205.0F});
            const auto size = texture.getSize();
            card.setScale({210.0F / static_cast<float>(size.x),
                280.0F / static_cast<float>(size.y)});
            target.draw(card);
        }
        else
            drawCard(target, choices[index].id,
                {CardX[index], 205.0F}, {210.0F, 280.0F}, false);
    }
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
    const arcane::presentation::PlayerAnimator& playerAnimator,
    const arcane::presentation::ShadeChargeAnimator& shadeChargeAnimator,
    const ArenaTextures& arenaTextures, const StaircaseTextures& staircaseTextures,
    const std::optional<sf::Texture>& meteorNpcTexture,
    const std::optional<sf::Texture>& ordenNpcTexture,
    const std::optional<sf::Texture>& swordVillageNpcTexture,
    const std::optional<sf::Texture>& southernHeroNpcTexture,
    const std::optional<sf::Texture>& merchantNpcTexture)
{
    drawArena(target, tower.arenaLayout(), GroundTop, arenaTextures);
    drawStaircase(target, tower.staircaseBounds(), tower.staircaseUnlocked(),
        tower.arenaLayout().theme, staircaseTextures);
    if (const auto* player = tower.explorationPlayer())
    {
        const auto visual = makePlayerVisualState(*player, tower.run().player().currentHp);
        const sf::Vector2f bottomCenter {
            visual.position.x + arcane::game::PlayerController::Width * 0.5F,
            visual.position.y + arcane::game::PlayerController::Height
        };
        shadeChargeAnimator.drawBack(target, bottomCenter);
        drawPlayer(target, playerAnimator, visual,
            sf::Color {232, 232, 242});
        shadeChargeAnimator.drawFront(target, bottomCenter);
    }
    const auto npc = tower.npcBounds();
    if (tower.currentFloorType() == arcane::game::run::FloorType::Merchant
        && merchantNpcTexture)
    {
        sf::Sprite npcSprite(*merchantNpcTexture);
        const auto size = merchantNpcTexture->getSize();
        npcSprite.setPosition({npc.left
                + (npc.width - static_cast<float>(size.x)) * 0.5F,
            npc.top + npc.height - static_cast<float>(size.y)});
        target.draw(npcSprite);
        return;
    }
    const std::optional<sf::Texture>* eventNpcTexture = nullptr;
    if (tower.eventKind() == arcane::app::EventKind::HalfCenturyMeteorShower)
        eventNpcTexture = &meteorNpcTexture;
    else if (tower.eventKind() == arcane::app::EventKind::AldenBall)
        eventNpcTexture = &ordenNpcTexture;
    else if (tower.eventKind() == arcane::app::EventKind::SwordVillage)
        eventNpcTexture = &swordVillageNpcTexture;
    else if (tower.eventKind() == arcane::app::EventKind::SouthernHero)
        eventNpcTexture = &southernHeroNpcTexture;
    if (tower.currentFloorType() == arcane::game::run::FloorType::Event
        && eventNpcTexture && *eventNpcTexture)
    {
        const sf::Texture& texture = **eventNpcTexture;
        sf::Sprite npcSprite(texture);
        npcSprite.setPosition({npc.left + (npc.width
                - static_cast<float>(texture.getSize().x)) * 0.5F,
            npc.top + npc.height - static_cast<float>(texture.getSize().y)});
        target.draw(npcSprite);
        return;
    }
    sf::RectangleShape npcShape({npc.width, npc.height});
    npcShape.setPosition({npc.left, npc.top});
    npcShape.setFillColor(tower.currentFloorType() == arcane::game::run::FloorType::Merchant
        ? sf::Color {205, 159, 75} : sf::Color {116, 83, 170});
    npcShape.setOutlineColor(sf::Color {235, 225, 190});
    npcShape.setOutlineThickness(3.0F);
    target.draw(npcShape);
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
            if (!detail.masteryDescription.empty())
                drawPixelText(target, wrapPixelText(detail.masteryDescription, 110U),
                    {120.0F, 570.0F}, 0.72F, sf::Color {220, 177, 255});
            drawPixelText(target, "CD " + formatTenths(detail.spell->cooldownSeconds) + " SEC",
                {120.0F, 610.0F}, 0.95F, sf::Color {145, 218, 255});
            drawPixelText(target, spellRangeText(*detail.spell),
                {420.0F, 610.0F}, 0.90F, sf::Color {174, 242, 184});
        }
    }
}

void drawLootDrop(sf::RenderTarget& target, const arcane::game::Aabb bounds,
    const arcane::presentation::LootBookAnimator& animator)
{
    const sf::Vector2f center {
        bounds.left + bounds.width * 0.5F,
        bounds.top - 38.0F
    };
    static_cast<void>(animator.draw(target, center));
    drawPixelText(target, "E  INSPECT", {center.x - 43.0F, bounds.top + bounds.height + 8.0F},
        0.72F, sf::Color {190, 224, 255, 220});
}
}
