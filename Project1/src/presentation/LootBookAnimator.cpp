#include "presentation/LootBookAnimator.hpp"

#include <SFML/Graphics/Sprite.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

namespace arcane::presentation
{
bool LootBookAnimator::load(const std::filesystem::path& path)
{
    sf::Texture texture;
    if (!texture.loadFromFile(path))
    {
        texture_.reset();
        return false;
    }
    const sf::Vector2u size = texture.getSize();
    if (size.x != static_cast<unsigned int>(FrameWidth) * FrameCount
        || size.y != static_cast<unsigned int>(FrameHeight))
    {
        texture_.reset();
        return false;
    }
    texture.setSmooth(false);
    texture_ = std::move(texture);
    return true;
}

void LootBookAnimator::update(const float deltaSeconds) noexcept
{
    animationSeconds_ += std::max(0.0F, deltaSeconds);
}

bool LootBookAnimator::draw(sf::RenderTarget& target, const sf::Vector2f center) const
{
    if (!texture_) return false;
    const auto frame = static_cast<unsigned int>(animationSeconds_ * FramesPerSecond)
        % FrameCount;
    sf::Sprite sprite(*texture_);
    sprite.setTextureRect(sf::IntRect(
        {static_cast<int>(frame) * FrameWidth, 0}, {FrameWidth, FrameHeight}));
    sprite.setOrigin({FrameWidth * 0.5F, FrameHeight * 0.5F});
    sprite.setScale({DisplayScale, DisplayScale});
    sprite.setPosition(center + sf::Vector2f {0.0F,
        std::sin(animationSeconds_ * 2.8F) * 3.0F});
    target.draw(sprite);
    return true;
}

void LootBookAnimator::reset() noexcept
{
    animationSeconds_ = 0.0F;
}
}
