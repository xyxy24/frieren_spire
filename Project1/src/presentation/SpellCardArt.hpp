#pragma once

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>

namespace arcane::presentation
{
class SpellCardArt
{
public:
    static constexpr std::size_t CardCount = 33U;

    [[nodiscard]] bool loadFromDirectory(const std::filesystem::path& directory);
    [[nodiscard]] bool draw(sf::RenderTarget& target, std::uint32_t spellId,
        sf::Vector2f position, sf::Vector2f size,
        sf::Color tint = sf::Color::White) const;

private:
    std::array<std::optional<sf::Texture>, CardCount> textures_;
};
}
