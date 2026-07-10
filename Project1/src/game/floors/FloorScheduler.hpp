#pragma once

#include "game/run/RunTypes.hpp"

#include <cstdint>

namespace arcane::game::floors
{
struct FloorScheduleConfig
{
    std::uint32_t floorsPerBoss {3U};
    std::uint32_t merchantMaxEligibleGap {4U};
    std::uint32_t eventMaxEligibleGap {3U};
};

class FloorScheduler
{
public:
    explicit FloorScheduler(FloorScheduleConfig config = {});
    [[nodiscard]] run::FloorType next(const run::RunContext& context);

private:
    void record(run::FloorType type) noexcept;
    FloorScheduleConfig config_;
    std::uint32_t eligibleSinceMerchant_ {};
    std::uint32_t eligibleSinceEvent_ {};
};
}
