#pragma once

#include "game/combat/Aabb.hpp"
#include "game/math/Vec2.hpp"
#include "game/player/PlayerController.hpp"
#include "game/run/RunTypes.hpp"
#include "common/ui/UiStates.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace arcane::game::floors
{
using ArenaTheme = common::ui::ArenaTheme;

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

using ArenaLayout = common::ui::ArenaState;

[[nodiscard]] run::ContentId selectArenaId(
    const run::RunContext& context, run::FloorType type) noexcept;
[[nodiscard]] const ArenaLayout& arenaLayout(run::ContentId id);
[[nodiscard]] std::span<const ArenaLayout> allArenaLayouts() noexcept;
[[nodiscard]] bool validateArenaLayout(const ArenaLayout& layout) noexcept;
[[nodiscard]] std::vector<ArenaSpawnPoint> enemySpawnPoints(const ArenaLayout& layout);
}
