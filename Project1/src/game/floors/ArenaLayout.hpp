#pragma once

#include "game/combat/Aabb.hpp"
#include "game/math/Vec2.hpp"
#include "game/player/PlayerController.hpp"
#include "game/run/RunTypes.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace arcane::game::floors
{
enum class ArenaTheme : std::uint8_t
{
    AuraOccupation,
    NorthernFrontier,
    MageExam
};

enum class ArenaSpawnKind : std::uint8_t
{
    GroundEnemy,
    ElevatedEnemy,
    FlyingEnemy
};

struct ArenaSpawnPoint
{
    ArenaSpawnKind kind {ArenaSpawnKind::GroundEnemy};
    Vec2 position;
    WorldBounds movementBounds;
};

struct ArenaLayout
{
    run::ContentId id {};
    ArenaTheme theme {ArenaTheme::AuraOccupation};
    bool safeRoom {};
    std::vector<Aabb> oneWayPlatforms;
};

[[nodiscard]] run::ContentId selectArenaId(
    const run::RunContext& context, run::FloorType type) noexcept;
[[nodiscard]] const ArenaLayout& arenaLayout(run::ContentId id);
[[nodiscard]] std::span<const ArenaLayout> allArenaLayouts() noexcept;
[[nodiscard]] bool validateArenaLayout(const ArenaLayout& layout) noexcept;
[[nodiscard]] std::vector<ArenaSpawnPoint> enemySpawnPoints(const ArenaLayout& layout);
}
