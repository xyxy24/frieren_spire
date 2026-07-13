#include "presentation/SpellEffectAnimator.hpp"

#include <SFML/Graphics/Sprite.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
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

struct SpellBinding
{
    std::size_t clipIndex;
    bool directional;
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
    {"float-slam.png", 8U, 192, 192, 96.0F, 176.0F, 14.0F, true},
    {"goddess-spears.png", 10U, 192, 192, 96.0F, 176.0F, 14.0F, true},
    {"destruction-lightning.png", 10U, 192, 192, 96.0F, 176.0F, 16.0F, true},
    {"earth-domination.png", 10U, 192, 192, 96.0F, 176.0F, 12.0F, true},
    {"phantom.png", 10U, 128, 96, 64.0F, 92.0F, 12.0F, true},
    {"stone-golem.png", 10U, 128, 96, 64.0F, 92.0F, 10.0F, true},
    {"mirror-array.png", 9U, 128, 96, 64.0F, 92.0F, 12.0F, true},
    {"flower-field.png", 9U, 192, 96, 96.0F, 88.0F, 10.0F, true},
    {"blood-magic.png", 8U, 192, 128, 96.0F, 64.0F, 16.0F, false},
    {"severing-magic.png", 10U, 192, 128, 96.0F, 64.0F, 16.0F, false},
    {"hellfire-storm.png", 10U, 192, 192, 96.0F, 176.0F, 12.0F, true},
    {"mimic-magic.png", 10U, 192, 128, 96.0F, 64.0F, 14.0F, false},
}};

[[nodiscard]] std::optional<SpellBinding> bindingFor(const std::uint32_t spellId) noexcept
{
    switch (spellId)
    {
    case 1001U: return SpellBinding {27U, false};
    case 1002U: return SpellBinding {13U, false};
    case 1003U: return SpellBinding {28U, true};
    case 1004U: return SpellBinding {1U, true};
    case 1005U: return SpellBinding {2U, true};
    case 1006U: return SpellBinding {6U, true};
    case 1008U: return SpellBinding {17U, true};
    case 1009U: return SpellBinding {3U, true};
    case 1011U: return SpellBinding {24U, false};
    case 1016U: return SpellBinding {9U, false};
    case 1017U: return SpellBinding {1U, true};
    case 1018U: return SpellBinding {10U, false};
    case 1019U: return SpellBinding {8U, true};
    case 1020U: return SpellBinding {18U, false};
    case 1021U: return SpellBinding {20U, false};
    case 1022U: return SpellBinding {25U, false};
    case 1023U: return SpellBinding {14U, false};
    case 1024U: return SpellBinding {11U, false};
    case 1025U: return SpellBinding {19U, false};
    case 1026U: return SpellBinding {15U, true};
    case 1027U: return SpellBinding {4U, true};
    case 1028U: return SpellBinding {16U, false};
    case 1029U: return SpellBinding {7U, true};
    case 1030U: return SpellBinding {12U, false};
    case 2001U: return SpellBinding {1U, true};
    case 2002U: return SpellBinding {21U, false};
    case 2003U: return SpellBinding {29U, true};
    case 2006U: return SpellBinding {31U, true};
    case 2007U: return SpellBinding {22U, false};
    case 2009U: return SpellBinding {30U, false};
    case 2010U: return SpellBinding {1U, true};
    case 2011U: return SpellBinding {23U, false};
    case 2012U: return SpellBinding {26U, false};
    default: return std::nullopt;
    }
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

bool SpellEffectAnimator::draw(sf::RenderTarget& target, const game::SpellEffectView& effect,
    const float playerFacingDirection) const
{
    const std::optional<SpellBinding> binding = bindingFor(effect.spellId);
    if (!binding) return false;

    const Clip& clip = clips_[binding->clipIndex];
    if (!clip.texture) return false;
    const ClipDefinition& definition = ClipDefinitions[binding->clipIndex];

    const float elapsed = std::max(0.0F, effect.duration - effect.remaining);
    const auto rawFrame = static_cast<std::uint32_t>(elapsed * definition.framesPerSecond);
    const std::uint32_t frame = std::min(rawFrame, definition.frameCount - 1U);

    sf::Sprite sprite(*clip.texture);
    sprite.setTextureRect(sf::IntRect(
        {static_cast<int>(frame) * definition.frameWidth, 0},
        {definition.frameWidth, definition.frameHeight}));
    sprite.setOrigin({definition.anchorX, definition.anchorY});

    const float positionY = definition.alignBottom
        ? effect.bounds.top + effect.bounds.height
        : effect.bounds.top + effect.bounds.height * 0.5F;
    sprite.setPosition({effect.bounds.left + effect.bounds.width * 0.5F, positionY});

    const float widthScale = effect.bounds.width / static_cast<float>(definition.frameWidth);
    const float heightScale = effect.bounds.height / static_cast<float>(definition.frameHeight);
    const float horizontalDirection = binding->directional && playerFacingDirection < 0.0F ? -1.0F : 1.0F;
    sprite.setScale({horizontalDirection * std::max(0.01F, widthScale),
        std::max(0.01F, heightScale)});
    target.draw(sprite);
    return true;
}
}
