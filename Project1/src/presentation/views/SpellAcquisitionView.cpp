#include "presentation/views/SpellAcquisitionView.hpp"

#include "presentation/views/UiPrimitives.hpp"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>

namespace arcane::presentation::views
{
namespace
{
constexpr float Pi = 3.14159265358979323846F;
constexpr sf::Vector2f Center {640.0F, 326.0F};

[[nodiscard]] float smoothStep(const float value) noexcept
{
    const float t = std::clamp(value, 0.0F, 1.0F);
    return t * t * (3.0F - 2.0F * t);
}

[[nodiscard]] sf::Color accentFor(
    const arcane::game::run::ContentId id, const bool bossSpell) noexcept
{
    if (bossSpell) return {255, 206, 96};
    constexpr std::array<sf::Color, 6> Palette {{
        {124, 220, 255}, {194, 137, 255}, {109, 238, 191},
        {255, 139, 132}, {255, 188, 105}, {141, 164, 255}}};
    return Palette[static_cast<std::size_t>(id % Palette.size())];
}

[[nodiscard]] sf::Color withAlpha(sf::Color color, const float alpha) noexcept
{
    color.a = static_cast<std::uint8_t>(std::clamp(alpha, 0.0F, 255.0F));
    return color;
}

void drawRing(sf::RenderTarget& target, const float radius, const float thickness,
    const sf::Color color)
{
    sf::CircleShape ring(radius, 96U);
    ring.setOrigin({radius, radius});
    ring.setPosition(Center);
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineColor(color);
    ring.setOutlineThickness(thickness);
    target.draw(ring);
}

void drawRadialGlyphs(sf::RenderTarget& target, const float radius, const float rotation,
    const std::size_t count, const sf::Color color)
{
    sf::VertexArray lines(sf::PrimitiveType::Lines);
    for (std::size_t index = 0U; index < count; ++index)
    {
        const float angle = rotation + static_cast<float>(index) * 2.0F * Pi
            / static_cast<float>(count);
        const float alternating = index % 3U == 0U ? 18.0F : 9.0F;
        const sf::Vector2f direction {std::cos(angle), std::sin(angle)};
        lines.append({Center + direction * (radius - alternating), color});
        lines.append({Center + direction * (radius + alternating), color});
    }
    target.draw(lines);
}

void drawFourPointStar(sf::RenderTarget& target, const float radius,
    const sf::Color color, const float rotation)
{
    sf::ConvexShape star(8U);
    for (std::size_t index = 0U; index < 8U; ++index)
    {
        const float pointRadius = index % 2U == 0U ? radius : radius * 0.18F;
        const float angle = rotation + static_cast<float>(index) * Pi * 0.25F;
        star.setPoint(index, {std::cos(angle) * pointRadius, std::sin(angle) * pointRadius});
    }
    star.setPosition(Center);
    star.setFillColor(color);
    target.draw(star);
}

void drawParticles(sf::RenderTarget& target,
    const viewmodel::SpellAcquisitionSnapshot& model, const sf::Color accent)
{
    constexpr std::size_t ParticleCount = 48U;
    const float charge = smoothStep(model.chargeProgress);
    const float unlock = smoothStep(model.unlockProgress);
    for (std::size_t index = 0U; index < ParticleCount; ++index)
    {
        const float seed = static_cast<float>((model.content.summary.id * 37U
            + static_cast<std::uint32_t>(index) * 71U) % 997U) / 997.0F;
        const float baseAngle = static_cast<float>(index) * 2.39996323F
            + seed * 0.8F + model.elapsedSeconds * (index % 2U == 0U ? 0.7F : -0.45F);
        float radius = 50.0F + (250.0F + seed * 120.0F) * (1.0F - charge);
        float alpha = 75.0F + charge * 165.0F;
        if (model.unlockProgress > 0.0F)
        {
            radius = 42.0F + unlock * (190.0F + seed * 170.0F);
            alpha = 230.0F * (1.0F - unlock * 0.72F);
        }
        if (model.revealProgress > 0.0F)
        {
            radius = 185.0F + seed * 125.0F;
            alpha = 70.0F + 75.0F * std::sin(model.elapsedSeconds * 3.0F + seed * 7.0F);
        }
        const sf::Vector2f position = Center
            + sf::Vector2f {std::cos(baseAngle), std::sin(baseAngle)} * radius;
        const float particleRadius = 1.5F + seed * 2.8F + model.flashRatio * 2.0F;
        sf::CircleShape particle(particleRadius, index % 4U == 0U ? 4U : 10U);
        particle.setOrigin({particleRadius, particleRadius});
        particle.setPosition(position);
        particle.setFillColor(withAlpha(accent, alpha));
        target.draw(particle);
    }
}
}

void drawSpellAcquisition(sf::RenderTarget& target,
    const viewmodel::SpellAcquisitionSnapshot& model,
    const arcane::presentation::SpellCardArt& spellCards)
{
    const sf::Color accent = accentFor(model.content.summary.id, model.bossSpell);
    sf::RectangleShape shade({static_cast<float>(WindowWidth), static_cast<float>(WindowHeight)});
    shade.setFillColor(sf::Color {5, 7, 15, 218});
    target.draw(shade);

    const float charge = smoothStep(model.chargeProgress);
    const float unlock = smoothStep(model.unlockProgress);
    const float reveal = smoothStep(model.revealProgress);
    const float geometryAlpha = std::clamp(charge * 210.0F + unlock * 45.0F, 0.0F, 245.0F);
    const float ringExpansion = unlock * 58.0F;
    drawRing(target, 110.0F + ringExpansion, 2.0F,
        withAlpha(accent, geometryAlpha));
    drawRing(target, 168.0F + ringExpansion * 0.65F, model.bossSpell ? 4.0F : 2.0F,
        withAlpha(accent, geometryAlpha * 0.82F));
    drawRing(target, 226.0F + ringExpansion * 0.38F, 1.5F,
        withAlpha(accent, geometryAlpha * 0.55F));
    drawRadialGlyphs(target, 140.0F + ringExpansion,
        model.elapsedSeconds * 0.72F, model.bossSpell ? 36U : 28U,
        withAlpha(accent, geometryAlpha));
    drawRadialGlyphs(target, 212.0F + ringExpansion * 0.4F,
        -model.elapsedSeconds * 0.38F, model.bossSpell ? 52U : 40U,
        withAlpha(accent, geometryAlpha * 0.64F));
    drawParticles(target, model, accent);

    if (model.unlockProgress < 1.0F)
    {
        const float starRadius = 12.0F + charge * 34.0F + model.flashRatio * 165.0F;
        drawFourPointStar(target, starRadius,
            withAlpha(sf::Color::White, 105.0F + charge * 110.0F), Pi * 0.25F);
        drawFourPointStar(target, starRadius * 0.62F,
            withAlpha(accent, 180.0F + model.flashRatio * 75.0F), 0.0F);
    }

    if (model.revealProgress > 0.0F)
    {
        const float cardSize = 48.0F + reveal * (model.bossSpell ? 250.0F : 224.0F);
        const sf::Vector2f cardPosition {Center.x - cardSize * 0.5F,
            Center.y - cardSize * 0.5F};
        sf::RectangleShape aura({cardSize + 24.0F, cardSize + 24.0F});
        aura.setPosition(cardPosition - sf::Vector2f {12.0F, 12.0F});
        aura.setFillColor(withAlpha(accent, 26.0F + reveal * 38.0F));
        aura.setOutlineColor(withAlpha(accent, 125.0F + reveal * 120.0F));
        aura.setOutlineThickness(model.bossSpell ? 6.0F : 3.0F);
        target.draw(aura);
        if (!spellCards.draw(target, model.content.summary.id, cardPosition,
                {cardSize, cardSize}))
            drawCard(target, model.content.summary, cardPosition, {cardSize, cardSize},
                true, &spellCards);

        const float textAlpha = std::clamp((model.revealProgress - 0.25F) * 340.0F, 0.0F, 255.0F);
        drawPixelText(target, model.bossSpell ? "ULTIMATE SPELL ACQUIRED" : "SPELL ACQUIRED",
            {model.bossSpell ? 462.0F : 510.0F, 62.0F}, model.bossSpell ? 1.6F : 1.5F,
            withAlpha(accent, textAlpha));
        const std::string name {model.content.summary.name};
        drawPixelText(target, name,
            {Center.x - static_cast<float>(name.size()) * 7.5F, 502.0F}, 1.6F,
            withAlpha(sf::Color {255, 240, 184}, textAlpha));
        drawPixelText(target, wrapPixelText(model.content.description, 76U),
            {250.0F, 548.0F}, 0.9F, withAlpha(sf::Color {224, 224, 238}, textAlpha));
        if (model.content.spell)
        {
            drawPixelText(target,
                "CD " + formatTenths(model.content.spell->cooldownSeconds)
                    + " SEC   " + spellRangeText(*model.content.spell),
                {390.0F, 614.0F}, 1.0F, withAlpha(sf::Color {156, 224, 255}, textAlpha));
        }
    }

    if (model.canDismiss)
    {
        const float pulse = 0.65F + 0.35F * std::sin(model.elapsedSeconds * 5.0F);
        drawPixelText(target, "ENTER CONTINUE", {548.0F, 674.0F}, 1.15F,
            withAlpha(sf::Color {245, 235, 205}, 150.0F + pulse * 105.0F));
    }

    if (model.flashRatio > 0.0F)
    {
        sf::RectangleShape flash({static_cast<float>(WindowWidth), static_cast<float>(WindowHeight)});
        flash.setFillColor(sf::Color {255, 255, 255,
            static_cast<std::uint8_t>(model.flashRatio * 205.0F)});
        target.draw(flash);
    }
}
}
