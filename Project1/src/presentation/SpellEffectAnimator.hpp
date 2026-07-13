#pragma once

#include "game/combat/CombatContracts.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <array>
#include <cstddef>
#include <filesystem>
#include <optional>

namespace arcane::presentation
{
class SpellEffectAnimator
{
public:
    static constexpr std::size_t ClipCount = 32U;

    [[nodiscard]] bool loadFromDirectory(const std::filesystem::path& directory);
    [[nodiscard]] bool draw(sf::RenderTarget& target, const game::SpellEffectView& effect,
        float playerFacingDirection, float groundTop) const;

private:
    struct Clip
    {
        std::optional<sf::Texture> texture;
    };

    std::array<Clip, ClipCount> clips_;
};
}
