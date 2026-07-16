#pragma once

#include "app/run/TowerSession.hpp"
#include "presentation/LootBookAnimator.hpp"
#include "presentation/ShadeChargeAnimator.hpp"
#include "presentation/PlayerAnimator.hpp"
#include "presentation/SpellCardArt.hpp"
#include "presentation/viewmodel/UiViewModels.hpp"
#include "presentation/views/ArenaView.hpp"
#include "presentation/views/StaircaseView.hpp"

#include <SFML/Graphics.hpp>

#include <array>
#include <optional>

namespace arcane::presentation::views
{
void drawRewardScreen(sf::RenderTarget& target, const viewmodel::RewardViewModel& model,
    const SpellCardArt& spellCards);
void drawBreakthroughScreen(sf::RenderTarget& target,
    const viewmodel::BreakthroughViewModel& model);
void drawMerchantScreen(sf::RenderTarget& target, const viewmodel::MerchantViewModel& model,
    const SpellCardArt& spellCards);
void drawEventScreen(sf::RenderTarget& target, const app::TowerSession& tower,
    const std::array<std::optional<sf::Texture>, 3>& meteorCards);
void drawPlayer(sf::RenderTarget& target, const PlayerAnimator& animator,
    const PlayerVisualState& state, sf::Color fallbackColor,
    sf::Color spriteTint = sf::Color::White);
void drawSpecialFloor(sf::RenderTarget& target, const app::TowerSession& tower,
    const PlayerAnimator& playerAnimator, const ShadeChargeAnimator& shadeChargeAnimator,
    const ArenaTextures& arenaTextures, const StaircaseTextures& staircaseTextures,
    const std::optional<sf::Texture>& meteorNpcTexture);
void drawLoadoutOverlay(sf::RenderTarget& target, const viewmodel::LoadoutSnapshot& model,
    const SpellCardArt& spellCards);
void drawLootDrop(sf::RenderTarget& target, game::Aabb bounds,
    const LootBookAnimator& animator);
}
