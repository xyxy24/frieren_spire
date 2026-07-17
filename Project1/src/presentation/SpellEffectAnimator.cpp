#include "presentation/SpellEffectAnimator.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string_view>

namespace arcane::presentation
{
namespace
{
struct ClipDefinition
{
    std::string_view fileName;
    std::uint32_t frameCount;
    int frameWidth;
    int frameHeight;
    float anchorX;
    float anchorY;
    float framesPerSecond;
    bool alignBottom;
};

enum class VisualLayout : std::uint8_t
{
    Center,
    CasterBottom,
    GroundCenter,
    Endpoint,
    FieldRow,
    Tether,
    Beam,
    TripleBeam,
    HomingVolley,
    Hellfire
};

constexpr std::uint32_t NoLoop = std::numeric_limits<std::uint32_t>::max();

struct SpellBinding
{
    std::size_t clipIndex;
    VisualLayout layout;
    float scale;
    bool directional;
    std::uint32_t loopStart {NoLoop};
    std::uint32_t loopEnd {NoLoop};
    std::uint32_t firstFrame {};
    std::uint32_t lastFrame {NoLoop};
};

constexpr std::array<ClipDefinition, SpellEffectAnimator::ClipCount> ClipDefinitions {{
    {"cast-arcane.png", 6U, 64, 64, 32.0F, 32.0F, 15.0F, false},
    {"zoltraak-impact.png", 8U, 96, 96, 48.0F, 48.0F, 16.0F, false},
    {"ice-lance.png", 6U, 96, 48, 48.0F, 24.0F, 18.0F, false},
    {"stone-shot.png", 6U, 96, 48, 48.0F, 24.0F, 16.0F, false},
    {"homing-bolt.png", 6U, 64, 32, 32.0F, 16.0F, 20.0F, false},
    {"zoltraak-muzzle.png", 6U, 64, 64, 32.0F, 32.0F, 15.0F, false},
    {"flame-burst.png", 8U, 192, 128, 96.0F, 64.0F, 16.0F, false},
    {"wind-pressure.png", 8U, 192, 128, 96.0F, 64.0F, 16.0F, false},
    {"mana-strike.png", 8U, 192, 128, 96.0F, 64.0F, 16.0F, false},
    {"mana-trace-ring.png", 8U, 192, 192, 96.0F, 96.0F, 14.0F, false},
    {"dispel-ring.png", 8U, 192, 192, 96.0F, 96.0F, 16.0F, false},
    {"spatial-shatter.png", 10U, 192, 192, 96.0F, 96.0F, 16.0F, false},
    {"gravity-well.png", 8U, 192, 96, 96.0F, 88.0F, 12.0F, true},
    {"goddess-blessing.png", 8U, 128, 128, 64.0F, 112.0F, 12.0F, true},
    {"flight-aura.png", 8U, 128, 128, 64.0F, 112.0F, 12.0F, true},
    {"lightning-staff.png", 8U, 128, 128, 64.0F, 64.0F, 12.0F, false},
    {"defensive-barrier.png", 8U, 128, 128, 64.0F, 112.0F, 14.0F, true},
    {"magic-thread.png", 8U, 96, 96, 48.0F, 48.0F, 16.0F, false},
    {"golden-binding.png", 8U, 96, 96, 48.0F, 48.0F, 14.0F, false},
    {"sealing-magic.png", 8U, 96, 96, 48.0F, 48.0F, 14.0F, false},
    {"float-slam.png", 8U, 192, 192, 96.0F, 176.0F, 8.0F, true},
    {"goddess-spears.png", 10U, 192, 192, 96.0F, 176.0F, 14.0F, true},
    {"destruction-lightning.png", 10U, 192, 192, 96.0F, 176.0F, 12.5F, true},
    {"earth-domination.png", 10U, 192, 192, 96.0F, 176.0F, 12.0F, true},
    {"phantom.png", 10U, 128, 96, 64.0F, 92.0F, 12.0F, true},
    {"stone-golem.png", 10U, 128, 96, 64.0F, 92.0F, 10.0F, true},
    {"mirror-array.png", 9U, 128, 96, 64.0F, 92.0F, 12.0F, true},
    {"flower-field.png", 9U, 192, 96, 96.0F, 88.0F, 10.0F, true},
    {"blood-magic.png", 8U, 192, 128, 96.0F, 64.0F, 16.0F, false},
    {"severing-magic.png", 10U, 192, 128, 96.0F, 64.0F, 16.0F, false},
    {"hellfire-storm.png", 8U, 128, 256, 64.0F, 248.0F, 12.0F, true},
    {"mimic-magic.png", 10U, 192, 128, 96.0F, 64.0F, 14.0F, false},
}};

[[nodiscard]] std::optional<SpellBinding> bindingFor(const std::uint32_t spellId) noexcept
{
    switch (spellId)
    {
    case 1001U:
        return SpellBinding {27U, VisualLayout::FieldRow, 1.0F, false, 2U, 4U, 0U, 4U};
    case 1002U: return SpellBinding {13U, VisualLayout::CasterBottom, 1.15F, false, 2U, 5U};
    case 1003U: return SpellBinding {28U, VisualLayout::Center, 1.05F, true};
    case 1004U: return SpellBinding {1U, VisualLayout::Beam, 0.72F, true};
    case 1005U: return SpellBinding {2U, VisualLayout::Center, 1.35F, true};
    case 1006U: return SpellBinding {6U, VisualLayout::Center, 1.05F, true};
    case common::ui::BurningFlowerVisualId:
        return SpellBinding {27U, VisualLayout::FieldRow, 1.0F, false, 5U, 7U, 5U, 8U};
    case 1008U: return SpellBinding {17U, VisualLayout::Tether, 0.72F, true};
    case 1009U: return SpellBinding {3U, VisualLayout::Center, 1.35F, true};
    case 1011U:
        return SpellBinding {24U, VisualLayout::CasterBottom, 1.15F, false, 3U, 5U, 0U, 5U};
    case common::ui::PhantomBreakVisualId:
        return SpellBinding {24U, VisualLayout::CasterBottom, 1.15F, false,
            NoLoop, NoLoop, 6U, 9U};
    case 1016U: return SpellBinding {9U, VisualLayout::CasterBottom, 1.2F, false, 2U, 5U};
    case 1017U: return SpellBinding {1U, VisualLayout::TripleBeam, 0.58F, true};
    case 1018U: return SpellBinding {10U, VisualLayout::Center, 1.0F, false};
    case 1019U: return SpellBinding {8U, VisualLayout::Center, 0.8F, true};
    case 1020U: return SpellBinding {18U, VisualLayout::Tether, 0.78F, true};
    case 1021U: return SpellBinding {20U, VisualLayout::GroundCenter, 1.15F, false};
    case 1022U:
        return SpellBinding {25U, VisualLayout::CasterBottom, 1.25F, false, 3U, 5U, 0U, 5U};
    case common::ui::StoneGolemResolveVisualId:
        return SpellBinding {25U, VisualLayout::CasterBottom, 1.25F, false,
            NoLoop, NoLoop, 6U, 9U};
    case 1023U: return SpellBinding {14U, VisualLayout::CasterBottom, 1.15F, false, 2U, 5U};
    case 1024U: return SpellBinding {11U, VisualLayout::Center, 1.15F, false};
    case 1025U: return SpellBinding {19U, VisualLayout::Endpoint, 0.9F, true};
    case 1026U:
        return SpellBinding {15U, VisualLayout::Center, 1.0F, false, 2U, 5U, 0U, 5U};
    case common::ui::LightningStaffDischargeVisualId:
        return SpellBinding {15U, VisualLayout::Center, 1.0F, false,
            NoLoop, NoLoop, 6U, 7U};
    case 1027U: return SpellBinding {4U, VisualLayout::HomingVolley, 0.9F, true};
    case 1028U:
        return SpellBinding {16U, VisualLayout::CasterBottom, 1.15F, false, 2U, 5U, 0U, 5U};
    case common::ui::DefensiveBarrierBreakVisualId:
        return SpellBinding {16U, VisualLayout::CasterBottom, 1.15F, false,
            NoLoop, NoLoop, 6U, 7U};
    case 1029U: return SpellBinding {7U, VisualLayout::Center, 1.05F, true};
    case 1030U: return SpellBinding {12U, VisualLayout::GroundCenter, 1.25F, false, 2U, 5U};
    case 2001U: return SpellBinding {1U, VisualLayout::Beam, 1.0F, true};
    case 2002U: return SpellBinding {21U, VisualLayout::GroundCenter, 1.25F, false};
    case 2003U: return SpellBinding {29U, VisualLayout::Center, 0.9F, true};
    case 2006U: return SpellBinding {31U, VisualLayout::Center, 1.2F, true};
    case 2007U: return SpellBinding {22U, VisualLayout::GroundCenter, 1.35F, false};
    case 2009U: return SpellBinding {30U, VisualLayout::Hellfire, 1.0F, false, 0U, 7U};
    case 2010U: return SpellBinding {1U, VisualLayout::Beam, 1.0F, true, 2U, 4U};
    case 2011U: return SpellBinding {23U, VisualLayout::GroundCenter, 1.35F, false, 4U, 6U};
    case 2012U: return SpellBinding {26U, VisualLayout::CasterBottom, 1.2F, false, 3U, 5U};
    case common::ui::MirrorArrayBreakVisualId:
        return SpellBinding {26U, VisualLayout::CasterBottom, 1.2F, false,
            NoLoop, NoLoop, 6U, 8U};
    case common::ui::ZoltraakMuzzleVisualId:
        return SpellBinding {5U, VisualLayout::Endpoint, 1.0F, true};
    default: return std::nullopt;
    }
}

[[nodiscard]] std::uint32_t frameFor(const ClipDefinition& definition,
    const SpellBinding& binding, const common::ui::SpellEffectState& effect) noexcept
{
    const float elapsed = std::max(0.0F, effect.duration - effect.remaining);
    const std::uint32_t lastFrame = binding.lastFrame == NoLoop
        ? definition.frameCount - 1U
        : std::min(binding.lastFrame, definition.frameCount - 1U);
    const std::uint32_t firstFrame = std::min(binding.firstFrame, lastFrame);
    const auto rawFrame = firstFrame
        + static_cast<std::uint32_t>(elapsed * definition.framesPerSecond);
    if (binding.loopStart == NoLoop || binding.loopEnd < binding.loopStart
        || binding.loopStart < firstFrame || binding.loopEnd > lastFrame)
        return std::min(rawFrame, lastFrame);

    if (rawFrame < binding.loopStart) return rawFrame;
    const std::uint32_t outroCount = lastFrame - binding.loopEnd;
    const float outroDuration = static_cast<float>(outroCount) / definition.framesPerSecond;
    if (outroCount > 0U && effect.remaining <= outroDuration)
    {
        const float outroElapsed = std::max(0.0F, outroDuration - effect.remaining);
        const auto outroFrame = static_cast<std::uint32_t>(outroElapsed
            * definition.framesPerSecond);
        return std::min(binding.loopEnd + 1U + outroFrame, definition.frameCount - 1U);
    }
    const std::uint32_t loopLength = binding.loopEnd - binding.loopStart + 1U;
    return binding.loopStart + (rawFrame - binding.loopStart) % loopLength;
}
}

bool SpellEffectAnimator::loadFromDirectory(const std::filesystem::path& directory)
{
    bool loadedAll = true;
    for (std::size_t index = 0U; index < clips_.size(); ++index)
    {
        Clip& clip = clips_[index];
        sf::Texture texture;
        texture.setSmooth(false);
        if (texture.loadFromFile(directory / ClipDefinitions[index].fileName))
            clip.texture = std::move(texture);
        else
        {
            clip.texture.reset();
            loadedAll = false;
        }
    }
    return loadedAll;
}

bool SpellEffectAnimator::draw(sf::RenderTarget& target,
    const common::ui::SpellEffectState& effect,
    const float groundTop) const
{
    const std::optional<SpellBinding> binding = bindingFor(effect.spellId);
    if (!binding) return false;

    const Clip& clip = clips_[binding->clipIndex];
    if (!clip.texture) return false;
    const ClipDefinition& definition = ClipDefinitions[binding->clipIndex];

    const std::uint32_t frame = frameFor(definition, *binding, effect);
    const float horizontalDirection = binding->directional && effect.facingDirection < 0.0F
        ? -1.0F : 1.0F;

    const auto drawFrame = [&](const sf::Vector2f position, const float scale,
        const std::uint32_t frameOffset) {
        sf::Sprite sprite(*clip.texture);
        const std::uint32_t lastFrame = binding->lastFrame == NoLoop
            ? definition.frameCount - 1U
            : std::min(binding->lastFrame, definition.frameCount - 1U);
        const std::uint32_t firstFrame = std::min(binding->firstFrame, lastFrame);
        const std::uint32_t allowedFrameCount = lastFrame - firstFrame + 1U;
        const std::uint32_t selectedFrame = firstFrame
            + (frame - firstFrame + frameOffset) % allowedFrameCount;
        sprite.setTextureRect(sf::IntRect(
            {static_cast<int>(selectedFrame) * definition.frameWidth, 0},
            {definition.frameWidth, definition.frameHeight}));
        sprite.setOrigin({definition.anchorX, definition.anchorY});
        sprite.setPosition(position);
        sprite.setScale({horizontalDirection * scale, scale});
        target.draw(sprite);
    };

    const float centerX = effect.bounds.left + effect.bounds.width * 0.5F;
    const float centerY = effect.bounds.top + effect.bounds.height * 0.5F;
    const float endpointX = effect.facingDirection < 0.0F
        ? effect.bounds.left : effect.bounds.left + effect.bounds.width;

    if (binding->layout == VisualLayout::Hellfire)
    {
        constexpr std::array<float, 5> HorizontalPositions {
            0.02F, 0.25F, 0.5F, 0.75F, 0.98F
        };
        constexpr std::array<float, 5> Scales {0.82F, 1.0F, 1.12F, 0.94F, 0.86F};
        constexpr std::array<std::uint32_t, 5> FrameOffsets {0U, 5U, 2U, 7U, 4U};
        for (std::size_t index = 0U; index < HorizontalPositions.size(); ++index)
        {
            drawFrame({effect.bounds.left + effect.bounds.width * HorizontalPositions[index],
                groundTop}, Scales[index], FrameOffsets[index]);
        }
        return true;
    }

    if (binding->layout == VisualLayout::Beam
        || binding->layout == VisualLayout::TripleBeam)
    {
        const bool bossBeam = effect.spellId == 2001U || effect.spellId == 2010U;
        const float pulse = 0.9F + 0.1F * std::sin(
            std::max(0.0F, effect.duration - effect.remaining) * 24.0F);
        const float beamThickness = (effect.spellId == 2010U ? 24.0F
            : bossBeam ? 18.0F : binding->layout == VisualLayout::TripleBeam ? 5.0F : 8.0F)
            * pulse;
        const sf::Color core = effect.spellId == 2010U
            ? sf::Color {255, 246, 190, 245} : sf::Color {205, 250, 255, 245};
        const sf::Color glow = effect.spellId == 2010U
            ? sf::Color {255, 211, 104, 90} : sf::Color {88, 208, 255, 90};
        const auto drawBeam = [&](const float offsetY) {
            sf::RectangleShape outer({effect.bounds.width, beamThickness * 2.5F});
            outer.setPosition({effect.bounds.left, centerY + offsetY - beamThickness * 1.25F});
            outer.setFillColor(glow);
            target.draw(outer);
            sf::RectangleShape inner({effect.bounds.width, beamThickness});
            inner.setPosition({effect.bounds.left, centerY + offsetY - beamThickness * 0.5F});
            inner.setFillColor(core);
            target.draw(inner);
        };
        if (binding->layout == VisualLayout::TripleBeam)
        {
            drawBeam(-16.0F);
            drawBeam(0.0F);
            drawBeam(16.0F);
        }
        else drawBeam(0.0F);
        drawFrame({endpointX, centerY}, binding->scale, 0U);
        return true;
    }

    if (binding->layout == VisualLayout::Tether)
    {
        const bool golden = effect.spellId == 1020U;
        sf::RectangleShape glow({effect.bounds.width, golden ? 8.0F : 5.0F});
        glow.setPosition({effect.bounds.left, centerY - glow.getSize().y * 0.5F});
        glow.setFillColor(golden ? sf::Color {255, 214, 89, 100}
            : sf::Color {202, 137, 255, 100});
        target.draw(glow);
        sf::RectangleShape core({effect.bounds.width, golden ? 3.0F : 2.0F});
        core.setPosition({effect.bounds.left, centerY - core.getSize().y * 0.5F});
        core.setFillColor(golden ? sf::Color {255, 244, 174}
            : sf::Color {235, 206, 255});
        target.draw(core);
        drawFrame({endpointX, centerY}, binding->scale, 0U);
        return true;
    }

    if (binding->layout == VisualLayout::FieldRow)
    {
        constexpr std::array<float, 3> Positions {0.18F, 0.5F, 0.82F};
        constexpr std::array<std::uint32_t, 3> Offsets {0U, 2U, 1U};
        for (std::size_t index = 0U; index < Positions.size(); ++index)
            drawFrame({effect.bounds.left + effect.bounds.width * Positions[index], groundTop},
                binding->scale, Offsets[index]);
        return true;
    }

    if (binding->layout == VisualLayout::HomingVolley)
    {
        constexpr std::array<sf::Vector2f, 3> Offsets {
            sf::Vector2f {-44.0F, -24.0F}, sf::Vector2f {0.0F, 10.0F},
            sf::Vector2f {44.0F, -18.0F}
        };
        for (std::size_t index = 0U; index < Offsets.size(); ++index)
            drawFrame({centerX + horizontalDirection * Offsets[index].x,
                centerY + Offsets[index].y}, binding->scale,
                static_cast<std::uint32_t>(index));
        return true;
    }

    sf::Vector2f position {centerX, centerY};
    if (binding->layout == VisualLayout::CasterBottom
        || binding->layout == VisualLayout::GroundCenter)
        position.y = effect.bounds.top + effect.bounds.height;
    else if (binding->layout == VisualLayout::Endpoint)
        position.x = endpointX;
    drawFrame(position, binding->scale, 0U);
    return true;
}
}
