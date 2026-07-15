#pragma once

#include "game/combat/Aabb.hpp"
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
}
