#pragma once

#include "app/run/TowerSession.hpp"
#include "presentation/LootBookAnimator.hpp"
#include "presentation/ShadeChargeAnimator.hpp"
#include "presentation/PlayerAnimator.hpp"
#include "presentation/SpellCardArt.hpp"
#include "presentation/viewmodel/UiViewModels.hpp"

#include <SFML/Graphics.hpp>

namespace arcane::presentation::views
{
void drawRewardScreen(sf::RenderTarget& target, const viewmodel::RewardViewModel& model,
    const SpellCardArt& spellCards);
void drawMerchantScreen(sf::RenderTarget& target, const viewmodel::MerchantViewModel& model,
    const SpellCardArt& spellCards);
void drawEventScreen(sf::RenderTarget& target, const app::TowerSession& tower);
void drawPlayer(sf::RenderTarget& target, const PlayerAnimator& animator,
    const PlayerVisualState& state, sf::Color fallbackColor,
    sf::Color spriteTint = sf::Color::White);
void drawSpecialFloor(sf::RenderTarget& target, const app::TowerSession& tower,
    const PlayerAnimator& playerAnimator, const ShadeChargeAnimator& shadeChargeAnimator);
void drawLoadoutOverlay(sf::RenderTarget& target, const viewmodel::LoadoutSnapshot& model,
    const SpellCardArt& spellCards);
void drawStaircase(sf::RenderTarget& target, game::Aabb bounds, bool unlocked);
void drawLootDrop(sf::RenderTarget& target, game::Aabb bounds,
    const LootBookAnimator& animator);
}
