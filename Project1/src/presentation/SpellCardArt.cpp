#include "presentation/SpellCardArt.hpp"

#include <SFML/Graphics/Sprite.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace arcane::presentation
{
namespace
{
constexpr std::array<std::uint32_t, SpellCardArt::CardCount> SpellIds {{
    1001U, 1002U, 1003U, 1004U, 1005U, 1006U, 1008U, 1009U, 1011U,
    1016U, 1017U, 1018U, 1019U, 1020U, 1021U, 1022U, 1023U, 1024U,
    1025U, 1026U, 1027U, 1028U, 1029U, 1030U,
    2001U, 2002U, 2003U, 2006U, 2007U, 2009U, 2010U, 2011U, 2012U
}};

[[nodiscard]] std::optional<std::size_t> indexFor(const std::uint32_t spellId) noexcept
{
    const auto found = std::find(SpellIds.begin(), SpellIds.end(), spellId);
    if (found == SpellIds.end()) return std::nullopt;
    return static_cast<std::size_t>(std::distance(SpellIds.begin(), found));
}
}

bool SpellCardArt::loadFromDirectory(const std::filesystem::path& directory)
{
    bool loadedAll = true;
    for (std::size_t index = 0U; index < SpellIds.size(); ++index)
    {
        sf::Texture texture;
        const std::string fileName = std::to_string(SpellIds[index]) + ".png";
        const bool loadedPixelCard = texture.loadFromFile(directory / "v2" / fileName);
        if (loadedPixelCard || texture.loadFromFile(directory / fileName))
        {
            texture.setSmooth(!loadedPixelCard);
            textures_[index] = std::move(texture);
        }
        else
        {
            textures_[index].reset();
            loadedAll = false;
        }
    }
    return loadedAll;
}

bool SpellCardArt::draw(sf::RenderTarget& target, const std::uint32_t spellId,
    const sf::Vector2f position, const sf::Vector2f size, const sf::Color tint) const
{
    const auto index = indexFor(spellId);
    if (!index || !textures_[*index] || size.x <= 0.0F || size.y <= 0.0F) return false;

    const sf::Texture& texture = *textures_[*index];
    const sf::Vector2u textureSize = texture.getSize();
    if (textureSize.x == 0U || textureSize.y == 0U) return false;

    const float sourceAspect = static_cast<float>(textureSize.x)
        / static_cast<float>(textureSize.y);
    const float targetAspect = size.x / size.y;
    sf::IntRect source({0, 0},
        {static_cast<int>(textureSize.x), static_cast<int>(textureSize.y)});
    if (sourceAspect > targetAspect)
    {
        source.size.x = std::max(1, static_cast<int>(std::lround(
            static_cast<float>(textureSize.y) * targetAspect)));
        source.position.x = (static_cast<int>(textureSize.x) - source.size.x) / 2;
    }
    else if (sourceAspect < targetAspect)
    {
        source.size.y = std::max(1, static_cast<int>(std::lround(
            static_cast<float>(textureSize.x) / targetAspect)));
        source.position.y = (static_cast<int>(textureSize.y) - source.size.y) / 2;
    }

    sf::Sprite sprite(texture, source);
    sprite.setPosition(position);
    sprite.setScale({size.x / static_cast<float>(source.size.x),
        size.y / static_cast<float>(source.size.y)});
    sprite.setColor(tint);
    target.draw(sprite);
    return true;
}
}
