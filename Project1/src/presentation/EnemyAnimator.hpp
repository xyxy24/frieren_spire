#pragma once

#include "common/ui/UiStates.hpp"

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace arcane::presentation
{
class EnemyAnimator
{
public:
    void update(std::span<const common::ui::EnemyState> enemies, float deltaSeconds);
    void reset() noexcept;
    [[nodiscard]] std::optional<sf::IntRect> frameRect(
        const sf::Texture& atlas, std::size_t enemyIndex) const noexcept;
    [[nodiscard]] bool isWalking(std::size_t enemyIndex) const noexcept;
    [[nodiscard]] std::optional<sf::IntRect> walkFrameRect(
        const sf::Texture& atlas, std::size_t enemyIndex) const noexcept;
    [[nodiscard]] std::optional<sf::IntRect> dominationFrameRect(
        const sf::Texture& atlas, std::size_t enemyIndex) const noexcept;

private:
    enum class Phase : std::uint8_t { Idle, Windup, Attack };

    struct Track
    {
        common::ui::EnemyArchetype archetype {common::ui::EnemyArchetype::HeadlessKnight};
        Phase phase {Phase::Idle};
        float seconds {};
        float lastPositionX {};
        bool moving {};
        bool initialized {false};
    };

    [[nodiscard]] static Phase phaseFor(const common::ui::EnemyState& enemy) noexcept;
    [[nodiscard]] static std::uint32_t frameFor(const Track& track) noexcept;

    std::vector<Track> tracks_;
};
}
