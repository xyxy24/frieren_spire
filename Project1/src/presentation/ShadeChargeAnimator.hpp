#pragma once

#include "presentation/PlayerAnimator.hpp"

#include <SFML/Graphics/RenderTarget.hpp>

namespace arcane::presentation
{
class ShadeChargeAnimator
{
public:
    void update(const PlayerVisualState& player, float deltaSeconds) noexcept;
    void drawBack(sf::RenderTarget& target, sf::Vector2f playerBottomCenter) const;
    void drawFront(sf::RenderTarget& target, sf::Vector2f playerBottomCenter) const;
    void reset() noexcept;

private:
    float completionCueRemaining_ {};
    bool initialized_ {};
    bool wasCharging_ {};
};
}
