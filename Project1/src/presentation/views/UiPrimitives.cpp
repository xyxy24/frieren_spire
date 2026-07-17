#include "presentation/views/UiPrimitives.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <string>

namespace arcane::presentation::views
{
namespace ui = common::ui;

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

sf::Color colorForContent(const ui::ContentId id)
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
    case '.': return {0, 0, 0, 0, 0, 4, 4}; case ',': return {0, 0, 0, 0, 4, 4, 8};
    default: return {};
    }
}

void drawPixelText(sf::RenderTarget& target, const std::string_view text, const sf::Vector2f origin,
    const float scale, const sf::Color color)
{
    sf::VertexArray vertices(sf::PrimitiveType::Triangles);
    sf::Vector2f cursor = origin;
    for (const char character : text)
    {
        if (character == '\n') { cursor.x = origin.x; cursor.y += 9.0F * scale; continue; }
        const auto rows = glyphRows(character);
        for (std::size_t row = 0U; row < rows.size(); ++row)
            for (std::size_t column = 0U; column < 5U; ++column)
                if ((rows[row] & (1U << (4U - column))) != 0U)
                {
                    const float left = cursor.x + static_cast<float>(column) * scale;
                    const float top = cursor.y + static_cast<float>(row) * scale;
                    const float right = left + scale;
                    const float bottom = top + scale;
                    vertices.append({{left, top}, color});
                    vertices.append({{right, top}, color});
                    vertices.append({{right, bottom}, color});
                    vertices.append({{left, top}, color});
                    vertices.append({{right, bottom}, color});
                    vertices.append({{left, bottom}, color});
                }
        cursor.x += 6.0F * scale;
    }
    target.draw(vertices);
}

std::string wrapPixelText(const std::string_view text, const std::size_t maximumCharacters)
{
    std::string wrapped;
    std::size_t lineLength = 0U;
    std::size_t cursor = 0U;
    while (cursor < text.size())
    {
        while (cursor < text.size() && text[cursor] == ' ') ++cursor;
        const std::size_t wordStart = cursor;
        while (cursor < text.size() && text[cursor] != ' ') ++cursor;
        if (wordStart == cursor) break;
        const std::string_view word = text.substr(wordStart, cursor - wordStart);
        if (lineLength > 0U && lineLength + 1U + word.size() > maximumCharacters)
        {
            wrapped.push_back('\n');
            lineLength = 0U;
        }
        if (lineLength > 0U)
        {
            wrapped.push_back(' ');
            ++lineLength;
        }
        wrapped.append(word);
        lineLength += word.size();
    }
    return wrapped;
}

std::string formatTenths(const float value)
{
    const int tenths = static_cast<int>(std::lround(value * 10.0F));
    if (tenths % 10 == 0) return std::to_string(tenths / 10);
    return std::to_string(tenths / 10) + "." + std::to_string(std::abs(tenths % 10));
}

std::string spellRangeText(const ui::SpellDetailState& spell)
{
    using ui::SpellRangeKind;
    switch (spell.rangeKind)
    {
    case SpellRangeKind::Self: return "RANGE SELF";
    case SpellRangeKind::ForwardBox:
        return "RANGE " + formatTenths(spell.range) + " X " + formatTenths(spell.height);
    case SpellRangeKind::SelfArea:
        return "RADIUS " + formatTenths(spell.range) + " X " + formatTenths(spell.height);
    case SpellRangeKind::TargetArea:
        return "CAST " + formatTenths(spell.range) + " AREA " + formatTenths(spell.height);
    case SpellRangeKind::Summon:
        return spell.range > 0.0F
            ? "SUMMON SELF AGGRO " + formatTenths(spell.range) : "SUMMON BESIDE SELF";
    }
    return "RANGE UNKNOWN";
}

void drawCard(
    sf::RenderTarget& target,
    const ui::ContentSummaryState& content,
    const sf::Vector2f position,
    const sf::Vector2f size,
    const bool selected,
    const arcane::presentation::SpellCardArt* spellCards)
{
    const auto id = content.id;
    const bool spellCard = content.kind == ui::ContentKind::Spell && spellCards != nullptr;
    const bool bossSpell = content.bossSpell;
    const sf::Color accent = bossSpell ? sf::Color {255, 205, 92} : colorForContent(id);
    sf::RectangleShape card(size);
    card.setPosition(position);
    card.setFillColor(spellCard
        ? sf::Color {static_cast<std::uint8_t>(accent.r / 4U),
            static_cast<std::uint8_t>(accent.g / 4U),
            static_cast<std::uint8_t>(accent.b / 4U)}
        : colorForContent(id));
    card.setOutlineColor(selected ? sf::Color {255, 241, 178}
        : (bossSpell ? sf::Color {255, 205, 92} : sf::Color {210, 210, 225}));
    card.setOutlineThickness(selected ? 6.0F : 3.0F);
    target.draw(card);

    if (spellCard)
    {
        const float inset = std::max(5.0F, std::min(size.x, size.y) * 0.055F);
        const float artHeight = size.y > size.x * 1.25F
            ? std::min(size.y * 0.49F, size.x - inset * 2.0F)
            : size.y - inset * 2.0F;
        const sf::Vector2f artPosition {position.x + inset, position.y + inset};
        const sf::Vector2f artSize {size.x - inset * 2.0F, artHeight};
        sf::RectangleShape artPanel(artSize);
        artPanel.setPosition(artPosition);
        artPanel.setFillColor(sf::Color {10, 13, 24, 225});
        artPanel.setOutlineColor(sf::Color {accent.r, accent.g, accent.b, 170});
        artPanel.setOutlineThickness(std::max(1.0F, inset * 0.18F));
        target.draw(artPanel);
        if (spellCards->draw(target, id, artPosition, artSize))
        {
            if (content.rank > 0U)
                drawPixelText(target, "R" + std::to_string(content.rank),
                    {position.x + size.x - 34.0F, position.y + 8.0F}, 0.75F,
                    sf::Color {255, 241, 178});
            return;
        }
    }

    sf::CircleShape sigil(size.x * 0.18F, 6U);
    sigil.setOrigin({sigil.getRadius(), sigil.getRadius()});
    sigil.setPosition({position.x + size.x * 0.5F, position.y + size.y * 0.42F});
    sigil.setFillColor(sf::Color {245, 245, 255, 190});
    target.draw(sigil);
    if (content.rank > 0U)
        drawPixelText(target, "R" + std::to_string(content.rank),
            {position.x + size.x - 34.0F, position.y + 8.0F}, 0.75F,
            sf::Color {255, 241, 178});
}

void drawCard(
    sf::RenderTarget& target,
    const ui::ContentId id,
    const sf::Vector2f position,
    const sf::Vector2f size,
    const bool selected,
    const arcane::presentation::SpellCardArt* spellCards)
{
    const ui::ContentSummaryState content {
        id, ui::ContentKind::Unknown, "EVENT CHOICE", false, selected, 0U};
    drawCard(target, content, position, size, selected, spellCards);
}

void drawEquippedSlots(sf::RenderTarget& target, const ui::EquippedSlotsState& model,
    const arcane::presentation::SpellCardArt& spellCards,
    const ui::PlayerCombatState* combatView)
{
    constexpr float SlotSize = 66.0F;
    constexpr float Gap = 18.0F;
    constexpr float UltimateGap = 34.0F;
    const float totalWidth = SlotSize * 4.0F + Gap * 2.0F + UltimateGap;
    const float startX = (static_cast<float>(WindowWidth) - totalWidth) * 0.5F;

    for (std::size_t index = 0; index < model.regular.size(); ++index)
    {
        sf::RectangleShape slot({SlotSize, SlotSize});
        slot.setPosition({startX + static_cast<float>(index) * (SlotSize + Gap), 630.0F});
        slot.setFillColor(model.regular[index]
            ? colorForContent(model.regular[index]->id)
            : sf::Color {38, 41, 52});
        slot.setOutlineColor(sf::Color {175, 164, 214});
        slot.setOutlineThickness(3.0F);
        target.draw(slot);
        if (model.regular[index])
        {
            static_cast<void>(spellCards.draw(target, model.regular[index]->id,
                {startX + static_cast<float>(index) * (SlotSize + Gap) + 4.0F, 634.0F},
                {SlotSize - 8.0F, SlotSize - 8.0F}));
            drawPixelText(target, "R" + std::to_string(model.regular[index]->rank),
                {startX + static_cast<float>(index) * (SlotSize + Gap) + 42.0F, 638.0F},
                0.58F, sf::Color {255, 241, 178});
        }
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
    ultimateSlot.setFillColor(model.ultimate
        ? colorForContent(model.ultimate->id)
        : sf::Color {38, 41, 52});
    ultimateSlot.setOutlineColor(model.ultimateUnlocked
        ? sf::Color {255, 205, 92}
        : sf::Color {85, 85, 98});
    ultimateSlot.setOutlineThickness(4.0F);
    target.draw(ultimateSlot);
    if (model.ultimate)
        static_cast<void>(spellCards.draw(target, model.ultimate->id,
            {ultimateX + 4.0F, 634.0F}, {SlotSize - 8.0F, SlotSize - 8.0F}));
    if (combatView && combatView->ultimateSpellSlot.cooldownDuration > 0.0F)
    {
        const float ratio = std::clamp(combatView->ultimateSpellSlot.cooldownRemaining
            / combatView->ultimateSpellSlot.cooldownDuration, 0.0F, 1.0F);
        sf::RectangleShape cooldown({SlotSize, SlotSize * ratio});
        cooldown.setPosition({ultimateX, 630.0F});
        cooldown.setFillColor(sf::Color {8, 10, 18, 205});
        target.draw(cooldown);
    }
    drawPixelText(target, model.ultimateUnlocked ? "R" : "LOCK",
        {ultimateX + (model.ultimateUnlocked ? 27.0F : 10.0F), 607.0F}, 1.0F,
        model.ultimateUnlocked ? sf::Color {255, 221, 130} : sf::Color {105, 105, 118});

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
    drawPixelText(target, "F4 SPELL REGISTER", {495.0F, 510.0F}, 1.5F,
        sf::Color {148, 231, 224});
    drawPixelText(target, "F5 AURA  F6 REVOLTE  F7 MIRROR", {405.0F, 560.0F}, 1.5F,
        sf::Color {241, 151, 185});
}

void drawPauseMenu(sf::RenderTarget& target, const ui::PauseMenuItem selection)
{
    drawPixelText(target, "PAUSED", {555.0F, 150.0F}, 2.5F, sf::Color {232, 218, 255});
    drawMenuPanel(target, 250.0F, sf::Color {74, 66, 105}, "REPLAY CURRENT FLOOR",
        selection == ui::PauseMenuItem::ReplayCurrentFloor);
    drawMenuPanel(target, 370.0F, sf::Color {74, 66, 105}, "SAVE AND EXIT",
        selection == ui::PauseMenuItem::SaveAndExit);
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
