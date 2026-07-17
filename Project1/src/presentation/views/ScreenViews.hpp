#pragma once

#include "common/ui/UiStates.hpp"
#include "presentation/LootBookAnimator.hpp"
#include "presentation/ShadeChargeAnimator.hpp"
#include "presentation/PlayerAnimator.hpp"
#include "presentation/SpellCardArt.hpp"
#include "presentation/views/ArenaView.hpp"
#include "presentation/views/StaircaseView.hpp"

#include <SFML/Graphics.hpp>

#include <array>
#include <optional>

namespace arcane::presentation::views
{
void drawRewardScreen(sf::RenderTarget& target, const common::ui::RewardState& model,
    const SpellCardArt& spellCards);
void drawBreakthroughScreen(sf::RenderTarget& target,
    const common::ui::BreakthroughState& model);
void drawMerchantScreen(sf::RenderTarget& target, const common::ui::MerchantState& model,
    const SpellCardArt& spellCards);
void drawEventScreen(sf::RenderTarget& target, const common::ui::EventPanelState& event,
    const std::array<std::optional<sf::Texture>, 3>& meteorCards,
    const std::array<std::optional<sf::Texture>, 3>& ordenCards,
    const std::array<std::optional<sf::Texture>, 3>& swordVillageCards,
    const std::array<std::optional<sf::Texture>, 3>& southernHeroCards);
void drawPlayer(sf::RenderTarget& target, const PlayerAnimator& animator,
    const PlayerVisualState& state, sf::Color fallbackColor,
    sf::Color spriteTint = sf::Color::White);
void drawSpecialFloor(sf::RenderTarget& target, const common::ui::SpecialFloorState& state,
    const PlayerAnimator& playerAnimator, const ShadeChargeAnimator& shadeChargeAnimator,
    const ArenaTextures& arenaTextures, const StaircaseTextures& staircaseTextures,
    const std::optional<sf::Texture>& meteorNpcTexture,
    const std::optional<sf::Texture>& ordenNpcTexture,
    const std::optional<sf::Texture>& swordVillageNpcTexture,
    const std::optional<sf::Texture>& southernHeroNpcTexture,
    const std::optional<sf::Texture>& merchantNpcTexture);
void drawLoadoutOverlay(sf::RenderTarget& target, const common::ui::LoadoutState& model,
    const SpellCardArt& spellCards);
void drawLootDrop(sf::RenderTarget& target, common::RectF bounds,
    const LootBookAnimator& animator);
}
