#include "presentation/ShadeChargeAnimator.hpp"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace arcane::presentation
{
namespace
{
constexpr float Pi = 3.14159265358979323846F;
constexpr std::size_t ParticleCount = 8U;
constexpr float CompletionCueSeconds = 0.35F;

[[nodiscard]] float clamp01(const float value) noexcept
{
    return std::clamp(value, 0.0F, 1.0F);
}

[[nodiscard]] float smoothstep(const float value) noexcept
{
    const float t = clamp01(value);
    return t * t * (3.0F - 2.0F * t);
}

[[nodiscard]] float random01(std::uint32_t value) noexcept
{
    value ^= value >> 16U;
    value *= 0x7feb352dU;
    value ^= value >> 15U;
    value *= 0x846ca68bU;
    value ^= value >> 16U;
    return static_cast<float>(value & 0x00FFFFFFU) / 16777215.0F;
}

[[nodiscard]] std::uint8_t alphaByte(const float value) noexcept
{
    return static_cast<std::uint8_t>(std::clamp(value, 0.0F, 255.0F));
}
}

void ShadeChargeAnimator::update(
    const PlayerVisualState& player, const float deltaSeconds) noexcept
{
    const float elapsed = std::max(0.0F, deltaSeconds);
    completionCueRemaining_ = std::max(0.0F, completionCueRemaining_ - elapsed);

    const float remaining = std::max(0.0F, player.shadowDashChargeRemaining);
    const bool charging = remaining > 0.0F;
    if (!initialized_)
    {
        initialized_ = true;
        wasCharging_ = charging;
        return;
    }
    if (wasCharging_ && !charging && !player.shadowDashing && player.currentHealth > 0)
        completionCueRemaining_ = CompletionCueSeconds;
    if (player.currentHealth <= 0) completionCueRemaining_ = 0.0F;
    wasCharging_ = charging;
}

void ShadeChargeAnimator::drawBack(
    sf::RenderTarget& target, const sf::Vector2f playerBottomCenter) const
{
    if (completionCueRemaining_ <= 0.0F) return;
    const sf::Vector2f focus = playerBottomCenter + sf::Vector2f {0.0F, -38.0F};
    const float progress = 1.0F - completionCueRemaining_ / CompletionCueSeconds;
    const float inward = smoothstep(progress);
    const float lifeAlpha = std::sin(progress * Pi);
    for (std::size_t index = 0U; index < ParticleCount; ++index)
    {
        const auto seed = static_cast<std::uint32_t>(index * 977U + 131U);
        const float baseAngle = random01(seed + 17U) * 2.0F * Pi;
        const float curve = (random01(seed + 31U) - 0.5F) * 1.4F * (1.0F - inward);
        const float angle = baseAngle + curve;
        const float startRadius = 42.0F + random01(seed + 47U) * 30.0F;
        const float radius = startRadius * (1.0F - inward) + 3.0F * inward;
        const sf::Vector2f direction {std::cos(angle), std::sin(angle)};
        const sf::Vector2f position = focus + direction * radius;
        const float alpha = lifeAlpha * (0.65F + inward * 0.35F);

        const float streakLength = 7.0F + inward * 8.0F;
        sf::RectangleShape streak({streakLength, 1.5F + inward});
        streak.setOrigin({streakLength, (1.5F + inward) * 0.5F});
        streak.setPosition(position);
        streak.setRotation(sf::radians(angle));
        streak.setFillColor(sf::Color {42, 23, 65, alphaByte(alpha * 175.0F)});
        target.draw(streak);

        const float moteRadius = 1.5F + random01(seed + 59U) * 2.0F;
        sf::CircleShape fringe(moteRadius + 1.5F, 5U);
        fringe.setOrigin({moteRadius + 1.5F, moteRadius + 1.5F});
        fringe.setPosition(position);
        fringe.setRotation(sf::radians(-angle + progress * Pi));
        fringe.setFillColor(sf::Color {72, 45, 108, alphaByte(alpha * 145.0F)});
        target.draw(fringe);
        sf::CircleShape core(moteRadius, 5U);
        core.setOrigin({moteRadius, moteRadius});
        core.setPosition(position);
        core.setRotation(sf::radians(angle + progress * Pi));
        core.setFillColor(sf::Color {5, 3, 10, alphaByte(alpha * 235.0F)});
        target.draw(core);
    }
}

void ShadeChargeAnimator::drawFront(
    sf::RenderTarget& target, const sf::Vector2f playerBottomCenter) const
{
    if (completionCueRemaining_ <= 0.0F) return;
    const sf::Vector2f focus = playerBottomCenter + sf::Vector2f {0.0F, -38.0F};
    const float progress = 1.0F - completionCueRemaining_ / CompletionCueSeconds;
    if (progress > 0.62F)
    {
        const float flashProgress = (progress - 0.62F) / 0.38F;
        const float radius = 8.0F + flashProgress * 13.0F;
        sf::CircleShape pulse(radius, 40U);
        pulse.setOrigin({radius, radius});
        pulse.setPosition(focus);
        pulse.setFillColor(sf::Color::Transparent);
        pulse.setOutlineColor(sf::Color {24, 12, 38,
            alphaByte((1.0F - flashProgress) * 220.0F)});
        pulse.setOutlineThickness(3.0F - flashProgress * 1.5F);
        target.draw(pulse);
    }
}

void ShadeChargeAnimator::reset() noexcept
{
    completionCueRemaining_ = 0.0F;
    initialized_ = false;
    wasCharging_ = false;
}
}
