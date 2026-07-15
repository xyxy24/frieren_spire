#pragma once

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <filesystem>
#include <optional>

namespace arcane::presentation
{
class LootBookAnimator
{
public:
    [[nodiscard]] bool load(const std::filesystem::path& path);
    void update(float deltaSeconds) noexcept;
    [[nodiscard]] bool draw(sf::RenderTarget& target, sf::Vector2f center) const;
    void reset() noexcept;

private:
    static constexpr int FrameWidth = 128;
    static constexpr int FrameHeight = 96;
    static constexpr unsigned int FrameCount = 8U;
    static constexpr float FramesPerSecond = 8.0F;
    static constexpr float DisplayScale = 0.52F;

    std::optional<sf::Texture> texture_;
    float animationSeconds_ {};
};
}
