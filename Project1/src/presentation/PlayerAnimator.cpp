#include "presentation/PlayerAnimator.hpp"

#include <SFML/Graphics/Sprite.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <string_view>

namespace arcane::presentation
{
namespace
{
constexpr int FrameWidth = 128;
constexpr int FrameHeight = 96;
constexpr float AnchorX = 64.0F;
constexpr float BaselineY = 92.0F;

struct ClipDefinition
{
    std::string_view fileName;
    std::uint32_t frameCount;
    float framesPerSecond;
    bool loops;
};

constexpr std::array<ClipDefinition, static_cast<std::size_t>(PlayerAnimation::Count)>
    ClipDefinitions {{
        {"frieren-idle.png", 4U, 6.0F, true},
        {"frieren-run.png", 6U, 10.0F, true},
        {"frieren-jump.png", 8U, 10.0F, false},
        {"frieren-attack.png", 6U, 12.0F, false},
        {"frieren-cast.png", 6U, 10.0F, false},
        {"frieren-dash.png", 4U, 22.0F, false},
        {"frieren-shade-dash.png", 6U, 33.0F, false},
        {"frieren-hit.png", 4U, 10.0F, false},
        {"frieren-defeat.png", 6U, 8.0F, false},
        {"frieren-pickup.png", 4U, 8.0F, false},
        {"frieren-interact.png", 4U, 8.0F, false},
    }};
}

bool PlayerAnimator::loadFromDirectory(const std::filesystem::path& directory)
{
    bool loadedAll = true;
    for (std::size_t index = 0U; index < clips_.size(); ++index)
    {
        const ClipDefinition& definition = ClipDefinitions[index];
        Clip& clip = clips_[index];
        clip.frameCount = definition.frameCount;
        clip.framesPerSecond = definition.framesPerSecond;
        clip.loops = definition.loops;

        sf::Texture texture;
        if (texture.loadFromFile(directory / definition.fileName))
        {
            texture.setSmooth(false);
            clip.texture = std::move(texture);
        }
        else
        {
            clip.texture.reset();
            loadedAll = false;
        }
    }
    return loadedAll;
}

void PlayerAnimator::update(const PlayerVisualState& player, const float deltaSeconds) noexcept
{
    const float elapsed = std::max(0.0F, deltaSeconds);

    const bool attackStarted = player.attackSequence > observedAttackSequence_;
    const bool castStarted = player.castSequence > observedCastSequence_;
    observedAttackSequence_ = player.attackSequence;
    observedCastSequence_ = player.castSequence;

    if (attackStarted)
    {
        actionAnimation_ = PlayerAnimation::Attack;
        actionSeconds_ = 0.0F;
    }
    if (castStarted)
    {
        actionAnimation_ = PlayerAnimation::Cast;
        actionSeconds_ = 0.0F;
    }

    if (actionAnimation_)
    {
        actionSeconds_ += elapsed;
        if (actionSeconds_ >= duration(*actionAnimation_)) actionAnimation_.reset();
    }

    PlayerAnimation desired = PlayerAnimation::Idle;
    if (player.currentHealth <= 0)
        desired = PlayerAnimation::Defeat;
    else if (player.shadowDashing)
        desired = PlayerAnimation::ShadowDash;
    else if (player.dashRemaining > 0.0F)
        desired = PlayerAnimation::Dash;
    else if (player.stunned)
        desired = PlayerAnimation::Hit;
    else if (actionAnimation_)
        desired = *actionAnimation_;
    else if (!player.grounded)
        desired = PlayerAnimation::Jump;
    else if (std::abs(player.velocity.x) > 1.0F)
        desired = PlayerAnimation::Run;

    select(desired);
    animationSeconds_ += elapsed;
}

bool PlayerAnimator::draw(sf::RenderTarget& target, const sf::Vector2f bottomCenter,
    const float facingDirection) const
{
    const Clip& clip = clips_[indexOf(animation_)];
    if (!clip.texture) return false;

    sf::Sprite sprite(*clip.texture);
    sprite.setTextureRect(sf::IntRect(
        {static_cast<int>(frameIndex()) * FrameWidth, 0}, {FrameWidth, FrameHeight}));
    sprite.setOrigin({AnchorX, BaselineY});
    sprite.setPosition(bottomCenter);
    sprite.setScale({facingDirection < 0.0F ? -1.0F : 1.0F, 1.0F});
    target.draw(sprite);
    return true;
}

void PlayerAnimator::reset() noexcept
{
    animation_ = PlayerAnimation::Idle;
    animationSeconds_ = 0.0F;
    actionAnimation_.reset();
    actionSeconds_ = 0.0F;
    observedAttackSequence_ = 0U;
    observedCastSequence_ = 0U;
}

PlayerAnimation PlayerAnimator::animation() const noexcept { return animation_; }

std::uint32_t PlayerAnimator::frameIndex() const noexcept
{
    const Clip& clip = clips_[indexOf(animation_)];
    if (clip.frameCount <= 1U) return 0U;
    const auto rawFrame = static_cast<std::uint32_t>(animationSeconds_ * clip.framesPerSecond);
    return clip.loops ? rawFrame % clip.frameCount : std::min(rawFrame, clip.frameCount - 1U);
}

std::size_t PlayerAnimator::indexOf(const PlayerAnimation animation) noexcept
{
    return static_cast<std::size_t>(animation);
}

float PlayerAnimator::duration(const PlayerAnimation animation) const noexcept
{
    const Clip& clip = clips_[indexOf(animation)];
    return clip.framesPerSecond > 0.0F
        ? static_cast<float>(clip.frameCount) / clip.framesPerSecond : 0.0F;
}

void PlayerAnimator::select(const PlayerAnimation animation) noexcept
{
    if (animation_ == animation) return;
    animation_ = animation;
    animationSeconds_ = 0.0F;
}
}
