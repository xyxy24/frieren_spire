#pragma once

#include "game/run/RunTypes.hpp"

#include <cstdint>
#include <span>

namespace arcane::game::floors
{
struct FloorScheduleConfig
{
    std::uint32_t merchantMaxGap {4U};
    std::uint32_t eventMaxGap {3U};
};

struct FloorScheduleState
{
    std::uint32_t floorsSinceMerchant {};
    std::uint32_t floorsSinceEvent {};
};

class FloorScheduler
{
public:
    explicit FloorScheduler(FloorScheduleConfig config = {});
    [[nodiscard]] run::FloorType next(const run::RunContext& context,
        std::span<const std::uint32_t> bossFloorIndices);
    [[nodiscard]] const FloorScheduleState& state() const noexcept;

private:
    void record(run::FloorType type) noexcept;
    FloorScheduleConfig config_;
    FloorScheduleState state_;
};
}
