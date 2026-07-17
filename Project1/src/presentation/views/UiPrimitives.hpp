#pragma once

#include "common/ui/UiStates.hpp"
#include "presentation/SpellCardArt.hpp"

#include <SFML/Graphics.hpp>

#include <cstddef>
#include <string>
#include <string_view>

namespace arcane::presentation::views
{
inline constexpr unsigned int WindowWidth = 1280U;
inline constexpr unsigned int WindowHeight = 720U;
inline constexpr float GroundTop = 640.0F;

void drawHealthBar(sf::RenderTarget& target, sf::Vector2f position, sf::Vector2f size,
    int current, int maximum, sf::Color fillColor);
void drawPixelText(sf::RenderTarget& target, std::string_view text, sf::Vector2f origin,
    float scale = 2.0F, sf::Color color = sf::Color {235, 235, 245});
[[nodiscard]] std::string wrapPixelText(std::string_view text, std::size_t maximumCharacters);
[[nodiscard]] std::string formatTenths(float value);
[[nodiscard]] std::string spellRangeText(
    const common::ui::SpellDetailState& spell);

void drawCard(sf::RenderTarget& target, const common::ui::ContentSummaryState& content,
    sf::Vector2f position, sf::Vector2f size, bool selected,
    const SpellCardArt* spellCards = nullptr);
void drawCard(sf::RenderTarget& target, common::ui::ContentId id,
    sf::Vector2f position, sf::Vector2f size, bool selected,
    const SpellCardArt* spellCards = nullptr);
void drawEquippedSlots(sf::RenderTarget& target,
    const common::ui::EquippedSlotsState& model, const SpellCardArt& spellCards,
    const common::ui::PlayerCombatState* combatView = nullptr);

void drawStartMenu(sf::RenderTarget& target, bool canContinue);
void drawPauseMenu(sf::RenderTarget& target, common::ui::PauseMenuItem selection);
void drawResultMenu(sf::RenderTarget& target, bool victory);
void drawGold(sf::RenderTarget& target, int gold);
}
