#include "presentation/EnemyAnimator.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace arcane::presentation
{
namespace
{
constexpr std::uint32_t Columns = 4U;
constexpr std::uint32_t Rows = 3U;
constexpr std::uint32_t WalkColumns = 8U;
constexpr std::uint32_t DominationColumns = 8U;
constexpr float IdleFramesPerSecond = 4.0F;
constexpr float WalkFramesPerSecond = 10.0F;
constexpr float DominationWindupFramesPerSecond = 7.5F;
constexpr float DominationActiveFramesPerSecond = 10.0F;
constexpr float WindupFramesPerSecond = 9.0F;
constexpr float AttackFramesPerSecond = 30.0F;
constexpr std::array<std::uint32_t, 6> IdleFrameSequence {0U, 1U, 2U, 3U, 2U, 1U};
}

void EnemyAnimator::update(
    const std::span<const common::ui::EnemyState> enemies, const float deltaSeconds)
{
    tracks_.resize(enemies.size());
    const float elapsed = std::max(0.0F, deltaSeconds);
    for (std::size_t index = 0U; index < enemies.size(); ++index)
    {
        Track& track = tracks_[index];
        const common::ui::EnemyState& enemy = enemies[index];
        const Phase desired = phaseFor(enemy);
        const bool sameEnemy = track.initialized && track.archetype == enemy.archetype;
        const bool moving = sameEnemy && desired == Phase::Idle && elapsed > 0.0F
            && std::abs(enemy.position.x - track.lastPositionX) > 0.05F;
        if (!sameEnemy || track.phase != desired || track.moving != moving)
        {
            track.archetype = enemy.archetype;
            track.phase = desired;
            track.seconds = 0.0F;
            track.moving = moving;
            track.initialized = true;
        }
        else
            track.seconds += elapsed;
        track.lastPositionX = enemy.position.x;
    }
}

bool EnemyAnimator::isWalking(const std::size_t enemyIndex) const noexcept
{
    return enemyIndex < tracks_.size() && tracks_[enemyIndex].moving;
}

std::optional<sf::IntRect> EnemyAnimator::walkFrameRect(
    const sf::Texture& atlas, const std::size_t enemyIndex) const noexcept
{
    if (!isWalking(enemyIndex)) return std::nullopt;
    const sf::Vector2u size = atlas.getSize();
    if (size.x == 0U || size.y == 0U || size.x % WalkColumns != 0U)
        return std::nullopt;
    const int frameWidth = static_cast<int>(size.x / WalkColumns);
    const auto frame = static_cast<std::uint32_t>(
        tracks_[enemyIndex].seconds * WalkFramesPerSecond) % WalkColumns;
    return sf::IntRect {{static_cast<int>(frame) * frameWidth, 0},
        {frameWidth, static_cast<int>(size.y)}};
}

std::optional<sf::IntRect> EnemyAnimator::dominationFrameRect(
    const sf::Texture& atlas, const std::size_t enemyIndex) const noexcept
{
    if (enemyIndex >= tracks_.size() || tracks_[enemyIndex].phase == Phase::Idle)
        return std::nullopt;
    const sf::Vector2u size = atlas.getSize();
    if (size.x == 0U || size.y == 0U || size.x % DominationColumns != 0U)
        return std::nullopt;
    const Track& track = tracks_[enemyIndex];
    std::uint32_t frame = 0U;
    if (track.phase == Phase::Windup)
        frame = std::min(static_cast<std::uint32_t>(
            track.seconds * DominationWindupFramesPerSecond), 5U);
    else
        frame = 6U + std::min(static_cast<std::uint32_t>(
            track.seconds * DominationActiveFramesPerSecond), 1U);
    const int frameWidth = static_cast<int>(size.x / DominationColumns);
    return sf::IntRect {{static_cast<int>(frame) * frameWidth, 0},
        {frameWidth, static_cast<int>(size.y)}};
}

void EnemyAnimator::reset() noexcept { tracks_.clear(); }

std::optional<sf::IntRect> EnemyAnimator::frameRect(
    const sf::Texture& atlas, const std::size_t enemyIndex) const noexcept
{
    if (enemyIndex >= tracks_.size()) return std::nullopt;
    const sf::Vector2u size = atlas.getSize();
    if (size.x == 0U || size.y == 0U || size.x % Columns != 0U || size.y % Rows != 0U)
        return std::nullopt;

    const int frameWidth = static_cast<int>(size.x / Columns);
    const int frameHeight = static_cast<int>(size.y / Rows);
    const int column = static_cast<int>(frameFor(tracks_[enemyIndex]));
    const int row = static_cast<int>(tracks_[enemyIndex].phase);
    return sf::IntRect {{column * frameWidth, row * frameHeight},
        {frameWidth, frameHeight}};
}

EnemyAnimator::Phase EnemyAnimator::phaseFor(const common::ui::EnemyState& enemy) noexcept
{
    if (enemy.attackActive) return Phase::Attack;
    if (enemy.windingUp) return Phase::Windup;
    return Phase::Idle;
}

std::uint32_t EnemyAnimator::frameFor(const Track& track) noexcept
{
    float framesPerSecond = IdleFramesPerSecond;
    if (track.phase == Phase::Windup) framesPerSecond = WindupFramesPerSecond;
    else if (track.phase == Phase::Attack) framesPerSecond = AttackFramesPerSecond;

    const auto frame = static_cast<std::uint32_t>(track.seconds * framesPerSecond);
    if (track.phase == Phase::Idle)
        return IdleFrameSequence[frame % IdleFrameSequence.size()];
    return std::min(frame, Columns - 1U);
}
}
