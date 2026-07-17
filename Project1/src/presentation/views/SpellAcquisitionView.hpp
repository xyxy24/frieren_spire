#pragma once

#include "presentation/SpellCardArt.hpp"
#include "common/ui/UiStates.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>

namespace arcane::presentation::views
{
void drawSpellAcquisition(sf::RenderTarget& target,
    const common::ui::SpellAcquisitionState& model, const SpellCardArt& spellCards,
    sf::Vector2f focusPosition);
}
