#pragma once

#include "presentation/SpellCardArt.hpp"
#include "presentation/viewmodel/SpellAcquisitionViewModel.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>

namespace arcane::presentation::views
{
void drawSpellAcquisition(sf::RenderTarget& target,
    const viewmodel::SpellAcquisitionSnapshot& model, const SpellCardArt& spellCards,
    sf::Vector2f focusPosition);
}
