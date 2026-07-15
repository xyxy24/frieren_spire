#include "presentation/ShadeChargeAnimator.hpp"

#include "game/player/PlayerController.hpp"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace arcane::presentation
{
namespace
{
constexpr float Pi = 3.14159265358979323846F;
constexpr std::size_t ParticleCount = 12U;
constexpr float ReadyPulseSeconds = 0.30F;

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
    elapsedSeconds_ += elapsed;
    readyPulseRemaining_ = std::max(0.0F, readyPulseRemaining_ - elapsed);

    const float remaining = std::clamp(player.shadowDashChargeRemaining, 0.0F,
        game::PlayerController::ShadowDashChargeSeconds);
    chargeProgress_ = 1.0F - remaining
        / game::PlayerController::ShadowDashChargeSeconds;
    const bool newlyReady = remaining <= 0.0F && !player.shadowDashing
        && player.currentHealth > 0;
    if (newlyReady && !ready_) readyPulseRemaining_ = ReadyPulseSeconds;
    ready_ = newlyReady;
    visible_ = player.currentHealth > 0 && !player.shadowDashing
        && remaining > 0.0F && chargeProgress_ > 0.02F;
}

void ShadeChargeAnimator::drawBack(
    sf::RenderTarget& target, const sf::Vector2f playerBottomCenter) const
{
    if (!visible_) return;
    const sf::Vector2f focus = playerBottomCenter + sf::Vector2f {0.0F, -38.0F};
    const float intensity = smoothstep(chargeProgress_);

    if (intensity > 0.55F)
    {
        const float auraRadius = 20.0F + intensity * 11.0F
            + std::sin(elapsedSeconds_ * 5.0F) * 2.0F;
        sf::CircleShape aura(auraRadius, 32U);
        aura.setOrigin({auraRadius, auraRadius});
        aura.setPosition(focus);
        aura.setFillColor(sf::Color {4, 2, 10, alphaByte((intensity - 0.55F) * 95.0F)});
        aura.setOutlineColor(sf::Color {72, 45, 108,
            alphaByte((intensity - 0.55F) * 150.0F)});
        aura.setOutlineThickness(2.0F);
        target.draw(aura);
    }

    const std::size_t activeParticles = std::min(ParticleCount,
        static_cast<std::size_t>(2.0F + intensity * static_cast<float>(ParticleCount - 2U)));
    const float cycleSpeed = 0.72F + intensity * 1.45F;
    for (std::size_t index = 0U; index < activeParticles; ++index)
    {
        const auto seed = static_cast<std::uint32_t>(index * 977U + 131U);
        const float phase = random01(seed);
        const float cycle = std::fmod(elapsedSeconds_ * cycleSpeed + phase, 1.0F);
        const float inward = smoothstep(cycle);
        const float baseAngle = random01(seed + 17U) * 2.0F * Pi;
        const float curve = (random01(seed + 31U) - 0.5F) * 1.8F * (1.0F - inward);
        const float angle = baseAngle + curve + elapsedSeconds_ * 0.08F;
        const float startRadius = 48.0F + random01(seed + 47U) * 34.0F;
        const float radius = startRadius * (1.0F - inward) + 4.0F * inward;
        const sf::Vector2f direction {std::cos(angle), std::sin(angle)};
        const sf::Vector2f position = focus + direction * radius;
        const float lifeAlpha = std::sin(cycle * Pi);
        const float alpha = lifeAlpha * (0.35F + intensity * 0.65F);

        const float streakLength = 5.0F + intensity * 9.0F;
        sf::RectangleShape streak({streakLength, 1.5F + intensity});
        streak.setOrigin({streakLength, (1.5F + intensity) * 0.5F});
        streak.setPosition(position);
        streak.setRotation(sf::radians(angle));
        streak.setFillColor(sf::Color {42, 23, 65, alphaByte(alpha * 175.0F)});
        target.draw(streak);

        const float moteRadius = 2.0F + random01(seed + 59U) * 2.6F;
        sf::CircleShape fringe(moteRadius + 1.5F, 5U);
        fringe.setOrigin({moteRadius + 1.5F, moteRadius + 1.5F});
        fringe.setPosition(position);
        fringe.setRotation(sf::radians(-angle + cycle * Pi));
        fringe.setFillColor(sf::Color {72, 45, 108, alphaByte(alpha * 145.0F)});
        target.draw(fringe);
        sf::CircleShape core(moteRadius, 5U);
        core.setOrigin({moteRadius, moteRadius});
        core.setPosition(position);
        core.setRotation(sf::radians(angle + cycle * Pi));
        core.setFillColor(sf::Color {5, 3, 10, alphaByte(alpha * 235.0F)});
        target.draw(core);
    }
}

void ShadeChargeAnimator::drawFront(
    sf::RenderTarget& target, const sf::Vector2f playerBottomCenter) const
{
    if (!visible_ && readyPulseRemaining_ <= 0.0F) return;
    const sf::Vector2f focus = playerBottomCenter + sf::Vector2f {0.0F, -38.0F};
    if (readyPulseRemaining_ > 0.0F)
    {
        const float progress = 1.0F - readyPulseRemaining_ / ReadyPulseSeconds;
        const float radius = 18.0F + progress * 42.0F;
        sf::CircleShape pulse(radius, 40U);
        pulse.setOrigin({radius, radius});
        pulse.setPosition(focus);
        pulse.setFillColor(sf::Color::Transparent);
        pulse.setOutlineColor(sf::Color {24, 12, 38,
            alphaByte((1.0F - progress) * 230.0F)});
        pulse.setOutlineThickness(4.0F - progress * 2.5F);
        target.draw(pulse);
    }
}

void ShadeChargeAnimator::reset() noexcept
{
    elapsedSeconds_ = 0.0F;
    chargeProgress_ = 0.0F;
    readyPulseRemaining_ = 0.0F;
    ready_ = false;
    visible_ = false;
}
}
