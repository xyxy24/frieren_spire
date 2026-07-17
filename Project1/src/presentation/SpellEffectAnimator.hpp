#pragma once

#include "common/ui/UiStates.hpp"

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
    [[nodiscard]] bool draw(sf::RenderTarget& target, const common::ui::SpellEffectState& effect,
        float groundTop) const;

private:
    struct Clip
    {
        std::optional<sf::Texture> texture;
    };

    std::array<Clip, ClipCount> clips_;
};
}
