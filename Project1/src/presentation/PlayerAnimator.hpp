#pragma once

#include "common/ui/UiStates.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>

namespace arcane::presentation
{
enum class PlayerAnimation : std::uint8_t
{
    Idle,
    Run,
    Jump,
    Attack,
    Cast,
    Dash,
    ShadowDash,
    Hit,
    Defeat,
    Pickup,
    Interact,
    Count
};

using PlayerVisualState = common::ui::PlayerVisualState;

class PlayerAnimator
{
public:
    [[nodiscard]] bool loadFromDirectory(const std::filesystem::path& directory);
    void update(const PlayerVisualState& player, float deltaSeconds,
        std::optional<PlayerAnimation> presentationOverride = std::nullopt) noexcept;
    [[nodiscard]] bool draw(sf::RenderTarget& target, sf::Vector2f bottomCenter,
        float facingDirection, sf::Color tint = sf::Color::White) const;
    void reset() noexcept;

    [[nodiscard]] PlayerAnimation animation() const noexcept;
    [[nodiscard]] std::uint32_t frameIndex() const noexcept;

private:
    struct Clip
    {
        std::optional<sf::Texture> texture;
        std::uint32_t frameCount {1U};
        float framesPerSecond {1.0F};
        bool loops {true};
    };

    [[nodiscard]] static std::size_t indexOf(PlayerAnimation animation) noexcept;
    [[nodiscard]] float duration(PlayerAnimation animation) const noexcept;
    void select(PlayerAnimation animation) noexcept;

    std::array<Clip, static_cast<std::size_t>(PlayerAnimation::Count)> clips_;
    PlayerAnimation animation_ {PlayerAnimation::Idle};
    float animationSeconds_ {};
    std::optional<PlayerAnimation> actionAnimation_;
    float actionSeconds_ {};
    std::uint64_t observedAttackSequence_ {};
    std::uint64_t observedCastSequence_ {};
    std::uint64_t observedHurtSequence_ {};
};
}
