#include "presentation/views/SpellAcquisitionView.hpp"

#include "presentation/views/UiPrimitives.hpp"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>

namespace arcane::presentation::views
{
namespace
{
constexpr float Pi = 3.14159265358979323846F;
constexpr sf::Color RegistrationColor {202, 255, 243};
constexpr sf::Vector2f RevealCenter {640.0F, 292.0F};

struct PanelSpec
{
    float startSeconds;
    float angle;
    float distance;
    float width;
    float height;
    std::size_t contentIndex;
};

constexpr std::array<PanelSpec, 12U> PanelSpecs {{
    {0.35F, 2.75F, 76.0F, 184.0F, 54.0F, 0U},
    {0.92F, 0.42F, 82.0F, 168.0F, 72.0F, 1U},
    {1.48F, 3.65F, 88.0F, 205.0F, 66.0F, 2U},
    {2.10F, -0.62F, 74.0F, 190.0F, 58.0F, 3U},
    {2.78F, 2.38F, 95.0F, 176.0F, 94.0F, 4U},
    {3.42F, 0.72F, 104.0F, 218.0F, 76.0F, 5U},
    {4.08F, 3.02F, 108.0F, 226.0F, 80.0F, 6U},
    {4.74F, -0.18F, 102.0F, 174.0F, 68.0F, 7U},
    {5.38F, 2.18F, 116.0F, 192.0F, 62.0F, 8U},
    {6.02F, 0.98F, 122.0F, 210.0F, 82.0F, 9U},
    {6.70F, 3.38F, 126.0F, 178.0F, 74.0F, 10U},
    {7.32F, -0.84F, 132.0F, 224.0F, 88.0F, 11U}
}};

constexpr std::array<std::string_view, 12U> PanelHeaders {{
    "ARCANE REGISTER", "PERSONAL INFO", "SYSTEM NOTICE", "DECODE STREAM",
    "CONFORMITY", "AUTHENTICATION", "CONSISTENCY", "SOCKET INFO",
    "SIGNATURE TRACE", "MEMORY MAP", "CHANNEL TEST", "FINAL VALIDATION"
}};

[[nodiscard]] float clamp01(const float value) noexcept
{
    return std::clamp(value, 0.0F, 1.0F);
}

[[nodiscard]] float sineOut(const float value) noexcept
{
    return std::sin(clamp01(value) * Pi * 0.5F);
}

[[nodiscard]] float sineSquared(const float value) noexcept
{
    const float sine = sineOut(value);
    return sine * sine;
}

[[nodiscard]] float lerp(const float from, const float to, const float progress) noexcept
{
    return from + (to - from) * progress;
}

[[nodiscard]] sf::Color withAlpha(sf::Color color, const float alpha) noexcept
{
    color.a = static_cast<std::uint8_t>(std::clamp(alpha, 0.0F, 255.0F));
    return color;
}

[[nodiscard]] sf::Color mix(
    const sf::Color from, const sf::Color to, const float progress) noexcept
{
    const float t = clamp01(progress);
    return {
        static_cast<std::uint8_t>(lerp(static_cast<float>(from.r), static_cast<float>(to.r), t)),
        static_cast<std::uint8_t>(lerp(static_cast<float>(from.g), static_cast<float>(to.g), t)),
        static_cast<std::uint8_t>(lerp(static_cast<float>(from.b), static_cast<float>(to.b), t)),
        static_cast<std::uint8_t>(lerp(static_cast<float>(from.a), static_cast<float>(to.a), t))};
}

[[nodiscard]] std::uint32_t hash(const std::uint32_t value) noexcept
{
    std::uint32_t result = value;
    result ^= result >> 16U;
    result *= 0x7feb352dU;
    result ^= result >> 15U;
    result *= 0x846ca68bU;
    result ^= result >> 16U;
    return result;
}

[[nodiscard]] float random01(const std::uint32_t seed) noexcept
{
    return static_cast<float>(hash(seed) & 0x00FFFFFFU)
        / static_cast<float>(0x01000000U);
}

[[nodiscard]] sf::Color accentFor(
    const game::run::ContentId id, const bool bossSpell) noexcept
{
    if (bossSpell) return {255, 213, 126};
    constexpr std::array<sf::Color, 6U> Palette {{
        {112, 220, 255}, {204, 144, 255}, {112, 244, 191},
        {255, 126, 150}, {255, 181, 92}, {136, 164, 255}
    }};
    return Palette[static_cast<std::size_t>(id % Palette.size())];
}

void drawLine(sf::RenderTarget& target, const sf::Vector2f from,
    const sf::Vector2f to, const sf::Color color)
{
    sf::VertexArray line(sf::PrimitiveType::Lines);
    line.append({from, color});
    line.append({to, color});
    target.draw(line, sf::RenderStates {sf::BlendAdd});
}

void drawThickLine(sf::RenderTarget& target, const sf::Vector2f from,
    const sf::Vector2f to, const float thickness, const sf::Color color)
{
    const sf::Vector2f difference = to - from;
    const float length = std::sqrt(difference.x * difference.x + difference.y * difference.y);
    if (length <= 0.01F) return;
    sf::RectangleShape line({length, thickness});
    line.setOrigin({0.0F, thickness * 0.5F});
    line.setPosition(from);
    line.setRotation(sf::radians(std::atan2(difference.y, difference.x)));
    line.setFillColor(color);
    target.draw(line, sf::RenderStates {sf::BlendAdd});
}

void drawRegularPolygon(sf::RenderTarget& target, const sf::Vector2f center,
    const float radius, const std::size_t sides, const float rotation,
    const sf::Color color)
{
    if (sides < 3U || radius <= 0.0F) return;
    sf::VertexArray line(sf::PrimitiveType::LineStrip);
    for (std::size_t index = 0U; index <= sides; ++index)
    {
        const float angle = rotation + static_cast<float>(index) * 2.0F * Pi
            / static_cast<float>(sides);
        line.append({center + sf::Vector2f {std::cos(angle), std::sin(angle)} * radius,
            color});
    }
    target.draw(line, sf::RenderStates {sf::BlendAdd});
}

void drawCircle(sf::RenderTarget& target, const sf::Vector2f center,
    const float radius, const sf::Color color, const std::size_t segments = 64U)
{
    drawRegularPolygon(target, center, radius, segments, 0.0F, color);
}

void drawFourPointStar(sf::RenderTarget& target, const sf::Vector2f center,
    const float radius, const sf::Color color, const float rotation = 0.0F)
{
    sf::ConvexShape star(8U);
    for (std::size_t index = 0U; index < 8U; ++index)
    {
        const float pointRadius = index % 2U == 0U ? radius : radius * 0.09F;
        const float angle = rotation + static_cast<float>(index) * Pi * 0.25F;
        star.setPoint(index, {std::cos(angle) * pointRadius,
            std::sin(angle) * pointRadius});
    }
    star.setPosition(center);
    star.setFillColor(color);
    target.draw(star, sf::RenderStates {sf::BlendAdd});
}

void drawFocusOrb(sf::RenderTarget& target,
    const viewmodel::SpellAcquisitionSnapshot& model, const SpellCardArt& spellCards,
    const sf::Vector2f focus, const float opacity)
{
    if (opacity <= 0.0F) return;
    const float pulse = 0.5F + 0.5F * std::sin(model.referenceElapsedSeconds * 4.8F);
    const sf::RenderStates additive {sf::BlendAdd};
    for (int layer = 6; layer >= 0; --layer)
    {
        const float ratio = static_cast<float>(layer) / 6.0F;
        const float radius = 34.0F + ratio * 54.0F + pulse * 6.0F;
        sf::CircleShape glow(radius, 48U);
        glow.setOrigin({radius, radius});
        glow.setPosition(focus);
        glow.setFillColor(withAlpha(RegistrationColor,
            opacity * (3.0F + (1.0F - ratio) * 10.0F)));
        target.draw(glow, additive);
    }
    drawCircle(target, focus, 45.0F + pulse * 5.0F,
        withAlpha(sf::Color::White, opacity * 210.0F));
    drawCircle(target, focus, 55.0F + pulse * 3.0F,
        withAlpha(RegistrationColor, opacity * 110.0F));
    drawRegularPolygon(target, focus, 50.0F, 4U,
        model.referenceElapsedSeconds * 0.75F,
        withAlpha(RegistrationColor, opacity * 125.0F));
    drawRegularPolygon(target, focus, 42.0F, 4U,
        -model.referenceElapsedSeconds * 0.52F + Pi * 0.25F,
        withAlpha(sf::Color::White, opacity * 90.0F));

    const float iconSize = 44.0F;
    const sf::Vector2f iconPosition = focus - sf::Vector2f {iconSize * 0.5F, iconSize * 0.5F};
    static_cast<void>(spellCards.draw(target, model.content.summary.id,
        iconPosition, {iconSize, iconSize}, withAlpha(sf::Color::White, opacity * 255.0F)));
}

void drawRotatingSquares(sf::RenderTarget& target,
    const viewmodel::SpellAcquisitionSnapshot& model, const sf::Vector2f focus,
    const float opacity)
{
    for (std::size_t squareIndex = 0U; squareIndex < 4U; ++squareIndex)
    {
        const float localFrames = model.referenceElapsedSeconds * 60.0F
            - static_cast<float>(squareIndex) * 23.0F;
        const float intro = clamp01(localFrames / 40.0F);
        if (intro <= 0.0F) continue;
        const float halfSize = (30.0F + static_cast<float>(squareIndex) * 8.0F)
            / 1.4142135F;
        const float ax = model.referenceElapsedSeconds * 2.0F * Pi
            / (3.7F + squareIndex * 0.42F)
            + random01(static_cast<std::uint32_t>(squareIndex) + 12U) * Pi;
        const float ay = model.referenceElapsedSeconds * 2.0F * Pi
            / (4.6F + squareIndex * 0.37F)
            + random01(static_cast<std::uint32_t>(squareIndex) + 42U) * Pi;
        const float az = model.referenceElapsedSeconds * 2.0F * Pi
            / (5.2F + squareIndex * 0.51F);
        std::array<sf::Vector2f, 4U> points {};
        for (std::size_t corner = 0U; corner < points.size(); ++corner)
        {
            const float x = halfSize * (corner == 0U || corner == 3U ? -1.0F : 1.0F);
            const float y = halfSize * (corner < 2U ? -1.0F : 1.0F);
            const float rotatedX = x * std::cos(az) - y * std::sin(az);
            const float rotatedY = x * std::sin(az) + y * std::cos(az);
            points[corner] = focus + sf::Vector2f {
                rotatedX * std::cos(ax),
                rotatedY * std::cos(ay) + rotatedX * std::sin(ax) * std::sin(ay) * 0.42F}
                * intro;
        }
        for (std::size_t edge = 0U; edge < points.size(); ++edge)
            drawLine(target, points[edge], points[(edge + 1U) % points.size()],
                withAlpha(RegistrationColor, opacity * intro * 120.0F));
    }
}

void drawPanelDecoration(sf::RenderTarget& target, const sf::Vector2f topLeft,
    const sf::Vector2f size, const std::size_t type, const float progress,
    const sf::Color color)
{
    const sf::Vector2f areaTopLeft = topLeft + sf::Vector2f {7.0F, 28.0F};
    const sf::Vector2f areaSize {size.x - 14.0F, size.y - 35.0F};
    if (areaSize.y <= 6.0F) return;
    if (type % 4U == 0U)
    {
        const float filled = areaSize.x * progress;
        sf::RectangleShape meter({filled, 6.0F});
        meter.setPosition(areaTopLeft + sf::Vector2f {0.0F, 5.0F});
        meter.setFillColor(withAlpha(color, 95.0F));
        target.draw(meter, sf::RenderStates {sf::BlendAdd});
        drawPixelText(target, std::to_string(static_cast<int>(progress * 100.0F)) + "%",
            areaTopLeft + sf::Vector2f {areaSize.x - 38.0F, 14.0F}, 0.55F, color);
    }
    else if (type % 4U == 1U)
    {
        const int columns = 8;
        const int rows = 3;
        for (int x = 0; x <= columns; ++x)
            drawLine(target, areaTopLeft + sf::Vector2f {areaSize.x * x / columns, 0.0F},
                areaTopLeft + sf::Vector2f {areaSize.x * x / columns, areaSize.y},
                withAlpha(color, 38.0F));
        for (int y = 0; y <= rows; ++y)
            drawLine(target, areaTopLeft + sf::Vector2f {0.0F, areaSize.y * y / rows},
                areaTopLeft + sf::Vector2f {areaSize.x, areaSize.y * y / rows},
                withAlpha(color, 38.0F));
    }
    else if (type % 4U == 2U)
    {
        const sf::Vector2f center = areaTopLeft
            + sf::Vector2f {areaSize.x * 0.5F, areaSize.y * 0.5F};
        drawCircle(target, center, std::min(areaSize.y * 0.42F, 20.0F) * progress,
            withAlpha(color, 80.0F), 24U);
        drawRegularPolygon(target, center, std::min(areaSize.y * 0.34F, 16.0F) * progress,
            5U, -Pi * 0.5F, withAlpha(color, 130.0F));
    }
    else
    {
        for (std::size_t index = 0U; index < 18U; ++index)
        {
            const float x = areaSize.x * static_cast<float>(index % 9U) / 8.0F;
            const float y = areaSize.y * static_cast<float>(index / 9U) * 0.65F + 4.0F;
            sf::CircleShape bit(index % 3U == 0U ? 2.0F : 1.0F, 6U);
            bit.setOrigin({bit.getRadius(), bit.getRadius()});
            bit.setPosition(areaTopLeft + sf::Vector2f {x, y});
            bit.setFillColor(withAlpha(color,
                index < static_cast<std::size_t>(progress * 18.0F) ? 150.0F : 28.0F));
            target.draw(bit, sf::RenderStates {sf::BlendAdd});
        }
    }
}

void drawDiagnosticPanels(sf::RenderTarget& target,
    const viewmodel::SpellAcquisitionSnapshot& model, const sf::Vector2f focus,
    const float opacity)
{
    for (std::size_t index = 0U; index < PanelSpecs.size(); ++index)
    {
        const PanelSpec& spec = PanelSpecs[index];
        const float age = model.referenceElapsedSeconds - spec.startSeconds;
        constexpr float PanelLifetime = 4.2F;
        if (age < 0.0F || age > PanelLifetime) continue;
        const float fadeIn = clamp01(age / 0.24F);
        const float fadeOut = 1.0F - clamp01((age - 3.45F) / 0.75F);
        const float alpha = opacity * fadeIn * fadeOut;
        const sf::Vector2f direction {std::cos(spec.angle), std::sin(spec.angle)};
        const sf::Vector2f marker = focus + direction * spec.distance;
        const bool placeRight = direction.x >= 0.0F;
        const float desiredX = placeRight ? marker.x + 58.0F : marker.x - spec.width - 58.0F;
        const float desiredY = marker.y + direction.y * 34.0F - spec.height * 0.5F;
        const sf::Vector2f topLeft {
            std::clamp(desiredX, 18.0F, static_cast<float>(WindowWidth) - spec.width - 18.0F),
            std::clamp(desiredY, 62.0F, static_cast<float>(WindowHeight) - spec.height - 22.0F)};
        const sf::Vector2f lineEnd {
            placeRight ? topLeft.x : topLeft.x + spec.width,
            topLeft.y + spec.height * 0.5F};

        drawRegularPolygon(target, marker, 7.0F + std::sin(age * 6.0F) * 1.2F,
            4U, Pi * 0.25F, withAlpha(RegistrationColor, alpha * 190.0F));
        const sf::Vector2f elbow = marker + sf::Vector2f {placeRight ? 24.0F : -24.0F, 0.0F};
        drawLine(target, marker, elbow, withAlpha(RegistrationColor, alpha * 150.0F));
        drawLine(target, elbow, lineEnd, withAlpha(RegistrationColor, alpha * 150.0F));

        sf::RectangleShape panel({spec.width, spec.height});
        panel.setPosition(topLeft);
        panel.setFillColor(withAlpha(sf::Color {4, 12, 18}, alpha * 78.0F));
        panel.setOutlineColor(withAlpha(RegistrationColor, alpha * 92.0F));
        panel.setOutlineThickness(1.0F);
        target.draw(panel);
        sf::RectangleShape headerLine({spec.width * clamp01(age / 0.32F), 2.0F});
        headerLine.setPosition(topLeft + sf::Vector2f {0.0F, 20.0F});
        headerLine.setFillColor(withAlpha(RegistrationColor, alpha * 120.0F));
        target.draw(headerLine, sf::RenderStates {sf::BlendAdd});
        drawPixelText(target, std::string {PanelHeaders[spec.contentIndex]},
            topLeft + sf::Vector2f {6.0F, 5.0F}, 0.58F,
            withAlpha(RegistrationColor, alpha * 205.0F));
        drawPanelDecoration(target, topLeft, {spec.width, spec.height},
            spec.contentIndex, clamp01((age - 0.35F) / 1.6F),
            withAlpha(RegistrationColor, alpha * 180.0F));
    }
}

void drawRaisingCharacters(sf::RenderTarget& target,
    const viewmodel::SpellAcquisitionSnapshot& model, const float opacity)
{
    const std::size_t columnCount = model.circulationProgress > 0.0F ? 34U : 14U;
    for (std::size_t column = 0U; column < columnCount; ++column)
    {
        const std::uint32_t seed = model.content.summary.id * 37U
            + static_cast<std::uint32_t>(column) * 173U;
        const float speed = model.circulationProgress > 0.0F ? 96.0F : 38.0F;
        const float cycle = std::fmod(model.referenceElapsedSeconds * speed
            + random01(seed) * 720.0F, 820.0F);
        const float x = random01(seed + 1U) * static_cast<float>(WindowWidth);
        const float alphaEnvelope = sineOut(std::min(cycle, 820.0F - cycle) / 160.0F);
        for (std::size_t letter = 0U; letter < 7U; ++letter)
        {
            const float y = static_cast<float>(WindowHeight) - cycle
                + static_cast<float>(letter) * 12.0F;
            if (y < 20.0F || y > static_cast<float>(WindowHeight) - 10.0F) continue;
            const char character = static_cast<char>('A'
                + static_cast<int>(hash(seed + static_cast<std::uint32_t>(letter)) % 24U));
            drawPixelText(target, std::string(1U, character), {x, y},
                0.52F + random01(seed + 2U) * 0.26F,
                withAlpha(RegistrationColor, opacity * alphaEnvelope * 72.0F));
        }
    }
}

void drawCirculation(sf::RenderTarget& target,
    const viewmodel::SpellAcquisitionSnapshot& model, const sf::Vector2f focus,
    const float opacity)
{
    const float progress = model.circulationProgress;
    if (progress <= 0.0F || opacity <= 0.0F) return;
    const sf::Color circulationColor = mix(sf::Color::White, sf::Color {255, 98, 108},
        clamp01((progress - 0.62F) / 0.38F));
    const std::size_t rootCount = 2U + static_cast<std::size_t>(progress * 4.0F);
    for (std::size_t root = 0U; root < rootCount; ++root)
    {
        const std::uint32_t seed = model.content.summary.id * 97U
            + static_cast<std::uint32_t>(root) * 997U;
        const std::size_t sides = root % 3U == 0U ? 64U : 3U + hash(seed) % 5U;
        const float maximumRadius = lerp(165.0F, 390.0F, random01(seed + 1U));
        const float radius = maximumRadius * progress;
        const float angle = random01(seed + 2U) * 2.0F * Pi
            + (root % 2U == 0U ? 1.0F : -1.0F)
                * model.referenceElapsedSeconds * 0.055F;
        const sf::Vector2f center = focus + sf::Vector2f {
            (random01(seed + 3U) - 0.5F) * 150.0F * progress,
            (random01(seed + 4U) - 0.5F) * 110.0F * progress};
        drawRegularPolygon(target, center, radius, sides, angle,
            withAlpha(circulationColor, opacity * (105.0F + progress * 80.0F)));

        const std::size_t childCount = 4U + hash(seed + 5U) % 5U;
        for (std::size_t child = 0U; child < childCount; ++child)
        {
            const float childAngle = angle + static_cast<float>(child) * 2.0F * Pi
                / static_cast<float>(childCount);
            const sf::Vector2f childCenter = center
                + sf::Vector2f {std::cos(childAngle), std::sin(childAngle)} * radius;
            const float childRadius = radius * lerp(0.10F, 0.24F,
                random01(seed + static_cast<std::uint32_t>(child) + 20U));
            drawRegularPolygon(target, childCenter, childRadius,
                child % 3U == 0U ? 4U : 6U, -childAngle,
                withAlpha(circulationColor, opacity * 125.0F));
            drawLine(target, center, childCenter,
                withAlpha(circulationColor, opacity * 72.0F));
            if (child % 2U == 0U)
            {
                const std::size_t rayCount = 6U;
                for (std::size_t ray = 0U; ray < rayCount; ++ray)
                {
                    const float rayAngle = childAngle + static_cast<float>(ray) * Pi / 3.0F;
                    drawLine(target, childCenter,
                        childCenter + sf::Vector2f {std::cos(rayAngle), std::sin(rayAngle)}
                            * childRadius * 1.45F,
                        withAlpha(circulationColor, opacity * 74.0F));
                }
            }
        }
    }
}

void drawBurst(sf::RenderTarget& target,
    const viewmodel::SpellAcquisitionSnapshot& model, const sf::Vector2f focus)
{
    const float progress = model.burstProgress;
    if (progress <= 0.0F) return;
    const float initialFlash = 1.0F - clamp01(progress / 0.16F);
    drawFourPointStar(target, focus, 55.0F + initialFlash * 330.0F,
        withAlpha(sf::Color::White, 150.0F + initialFlash * 105.0F), Pi * 0.25F);
    drawFourPointStar(target, focus, 34.0F + initialFlash * 210.0F,
        withAlpha(RegistrationColor, 125.0F + initialFlash * 110.0F));
    drawCircle(target, focus, 45.0F + sineOut(progress) * 520.0F,
        withAlpha(sf::Color::White, (1.0F - progress) * 170.0F), 96U);

    for (std::size_t index = 0U; index < 80U; ++index)
    {
        const std::uint32_t seed = model.content.summary.id * 211U
            + static_cast<std::uint32_t>(index) * 1291U;
        const float delay = random01(seed) * 0.22F;
        const float life = clamp01((progress - delay) / (1.0F - delay));
        if (progress < delay || life >= 1.0F) continue;
        const float angle = random01(seed + 1U) * 2.0F * Pi;
        const sf::Vector2f direction {std::cos(angle), std::sin(angle)};
        const float distance = lerp(200.0F, 500.0F, random01(seed + 2U))
            * (0.8F * sineSquared(life / 0.4F) + 0.2F * sineOut(life));
        const float length = lerp(168.0F, 291.0F, random01(seed + 3U))
            * (1.0F - 0.95F * sineSquared(life / 0.4F));
        const sf::Vector2f head = focus + direction * distance;
        drawThickLine(target, head - direction * length, head,
            1.0F + random01(seed + 4U) * 1.2F,
            withAlpha(sf::Color {245, 211, 241}, (1.0F - life) * 205.0F));
    }

    for (std::size_t index = 0U; index < 37U; ++index)
    {
        const std::uint32_t seed = model.content.summary.id * 67U
            + static_cast<std::uint32_t>(index) * 733U;
        const float delay = static_cast<float>(index) / 37.0F * 0.22F;
        const float life = clamp01((progress - delay) / 0.30F);
        if (progress < delay || life >= 1.0F) continue;
        const float angle = random01(seed) * 2.0F * Pi;
        const float distance = lerp(40.0F, 230.0F, random01(seed + 1U)) * sineOut(life);
        const float radius = lerp(30.0F, 90.0F, random01(seed + 2U))
            * (1.0F - sineSquared(life));
        const sf::Vector2f position = focus
            + sf::Vector2f {std::cos(angle), std::sin(angle)} * distance;
        sf::CircleShape light(radius, 28U);
        light.setOrigin({radius, radius});
        light.setPosition(position);
        light.setFillColor(withAlpha(sf::Color::White, (1.0F - life) * 24.0F));
        target.draw(light, sf::RenderStates {sf::BlendAdd});
    }

    if (initialFlash > 0.0F)
    {
        sf::RectangleShape flash(
            {static_cast<float>(WindowWidth), static_cast<float>(WindowHeight)});
        flash.setFillColor(withAlpha(sf::Color::White, initialFlash * 145.0F));
        target.draw(flash, sf::RenderStates {sf::BlendAdd});
    }
}

void drawReveal(sf::RenderTarget& target,
    const viewmodel::SpellAcquisitionSnapshot& model, const SpellCardArt& spellCards,
    const sf::Color accent)
{
    const float progress = model.revealProgress;
    if (progress <= 0.0F) return;
    const float fade = sineOut(progress / 0.32F);
    sf::RectangleShape shade(
        {static_cast<float>(WindowWidth), static_cast<float>(WindowHeight)});
    shade.setFillColor(withAlpha(sf::Color {2, 3, 9}, fade * 192.0F));
    target.draw(shade);

    constexpr std::array<sf::Vector2f, 8U> Glints {{
        {420.0F, 176.0F}, {860.0F, 176.0F}, {344.0F, 322.0F}, {936.0F, 322.0F},
        {466.0F, 514.0F}, {814.0F, 514.0F}, {560.0F, 110.0F}, {720.0F, 110.0F}
    }};
    for (std::size_t index = 0U; index < Glints.size(); ++index)
    {
        const float delay = static_cast<float>(index) * 0.045F;
        const float starProgress = clamp01((progress - delay) / 0.32F);
        if (starProgress <= 0.0F) continue;
        const float pulse = 0.72F + 0.28F
            * std::sin(model.referenceElapsedSeconds * 4.0F + index);
        drawFourPointStar(target, Glints[index], (12.0F + index % 3U * 7.0F)
            * sineOut(starProgress) * pulse,
            withAlpha(index % 2U == 0U ? sf::Color::White : accent,
                fade * 185.0F), index % 2U == 0U ? 0.0F : Pi * 0.25F);
    }

    const float iconProgress = sineOut(progress / 0.28F);
    const float iconSize = 70.0F * iconProgress;
    for (int layer = 4; layer >= 0; --layer)
    {
        const float ratio = static_cast<float>(layer) / 4.0F;
        const float radius = 42.0F + ratio * 40.0F;
        sf::CircleShape glow(radius, 40U);
        glow.setOrigin({radius, radius});
        glow.setPosition(RevealCenter);
        glow.setFillColor(withAlpha(accent, fade * (3.0F + (1.0F - ratio) * 8.0F)));
        target.draw(glow, sf::RenderStates {sf::BlendAdd});
    }
    drawCircle(target, RevealCenter, 48.0F * iconProgress,
        withAlpha(sf::Color::White, fade * 170.0F));
    static_cast<void>(spellCards.draw(target, model.content.summary.id,
        RevealCenter - sf::Vector2f {iconSize * 0.5F, iconSize * 0.5F},
        {iconSize, iconSize}, withAlpha(sf::Color::White, fade * 255.0F)));

    const float textProgress = clamp01((progress - 0.18F) / 0.42F);
    const std::string name {model.content.summary.name};
    drawPixelText(target, name,
        {640.0F - static_cast<float>(name.size()) * 10.2F, 370.0F},
        1.7F, withAlpha(sf::Color {255, 249, 226}, textProgress * 255.0F));
    sf::RectangleShape separator({360.0F * textProgress, 2.0F});
    separator.setPosition({640.0F - 180.0F * textProgress, 405.0F});
    separator.setFillColor(withAlpha(accent, textProgress * 170.0F));
    target.draw(separator, sf::RenderStates {sf::BlendAdd});
    drawPixelText(target, wrapPixelText(model.content.description, 82U),
        {318.0F, 428.0F}, 0.82F,
        withAlpha(sf::Color {224, 227, 238}, textProgress * 230.0F));
    if (model.content.spell)
    {
        const std::string details = "CD "
            + formatTenths(model.content.spell->cooldownSeconds)
            + " SEC     " + spellRangeText(*model.content.spell);
        drawPixelText(target, details,
            {640.0F - static_cast<float>(details.size()) * 5.7F, 545.0F}, 0.93F,
            withAlpha(accent, textProgress * 235.0F));
    }
    if (model.canDismiss)
    {
        const float pulse = 0.62F + 0.38F
            * std::sin(model.referenceElapsedSeconds * 5.0F);
        drawPixelText(target, "ENTER CONTINUE", {548.0F, 668.0F}, 1.0F,
            withAlpha(sf::Color {230, 232, 243}, 145.0F + pulse * 100.0F));
    }
}
}

void drawSpellAcquisition(sf::RenderTarget& target,
    const viewmodel::SpellAcquisitionSnapshot& model, const SpellCardArt& spellCards,
    sf::Vector2f focusPosition)
{
    focusPosition.x = std::clamp(focusPosition.x, 220.0F, 1060.0F);
    focusPosition.y = std::clamp(focusPosition.y, 160.0F, 500.0F);
    const sf::Color accent = accentFor(model.content.summary.id, model.bossSpell);
    const float registrationOpacity = 1.0F - clamp01(model.burstProgress / 0.24F);
    const float darkness = 55.0F + model.registrationProgress * 50.0F
        + model.circulationProgress * 62.0F;
    sf::RectangleShape shade(
        {static_cast<float>(WindowWidth), static_cast<float>(WindowHeight)});
    shade.setFillColor(withAlpha(sf::Color {3, 5, 16}, darkness * registrationOpacity));
    target.draw(shade);

    drawRaisingCharacters(target, model, registrationOpacity);
    drawRotatingSquares(target, model, focusPosition, registrationOpacity);
    drawDiagnosticPanels(target, model, focusPosition, registrationOpacity);
    drawCirculation(target, model, focusPosition, registrationOpacity);
    drawFocusOrb(target, model, spellCards, focusPosition, registrationOpacity);
    drawBurst(target, model, focusPosition);
    drawReveal(target, model, spellCards, accent);

    if (!model.canDismiss)
    {
        const float opacity = model.canSkip ? 1.0F : 0.35F;
        sf::RectangleShape button({164.0F, 36.0F});
        button.setPosition({1088.0F, 660.0F});
        button.setFillColor(withAlpha(sf::Color {7, 10, 22}, opacity * 205.0F));
        button.setOutlineThickness(2.0F);
        button.setOutlineColor(withAlpha(accent, opacity * 210.0F));
        target.draw(button);
        drawPixelText(target, "SPACE  SKIP", {1106.0F, 671.0F}, 0.82F,
            withAlpha(sf::Color {238, 241, 250}, opacity * 245.0F));
    }
}
}
