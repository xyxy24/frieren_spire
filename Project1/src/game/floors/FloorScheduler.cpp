#include "game/floors/FloorScheduler.hpp"

#include "game/run/DeterministicRng.hpp"

#include <algorithm>
#include <stdexcept>

namespace arcane::game::floors
{
FloorScheduler::FloorScheduler(const FloorScheduleConfig config) : config_(config)
{
    if (config_.merchantMaxGap == 0U || config_.eventMaxGap == 0U)
        throw std::invalid_argument("floor guarantee gaps must be positive");
}

run::FloorType FloorScheduler::next(const run::RunContext& context,
    const std::span<const std::uint32_t> bossFloorIndices)
{
    run::FloorType type = run::FloorType::Combat;
    if (std::find(bossFloorIndices.begin(), bossFloorIndices.end(), context.floorIndex)
        != bossFloorIndices.end())
    {
        type = run::FloorType::Boss;
    }
    else if (state_.floorsSinceMerchant >= config_.merchantMaxGap)
    {
        type = run::FloorType::Merchant;
    }
    else if (state_.floorsSinceEvent >= config_.eventMaxGap)
    {
        type = run::FloorType::Event;
    }
    else if (state_.floorsSinceMerchant + 1U >= config_.merchantMaxGap
        && state_.floorsSinceEvent + 1U >= config_.eventMaxGap)
    {
        type = state_.floorsSinceMerchant * config_.eventMaxGap
                >= state_.floorsSinceEvent * config_.merchantMaxGap
            ? run::FloorType::Merchant
            : run::FloorType::Event;
    }
    else
    {
        run::DeterministicRng rng(run::deriveStreamSeed(context.floorSeed, run::RandomStream::Encounter));
        const auto roll = rng.index(10U);
        if (roll == 0U) type = run::FloorType::Merchant;
        else if (roll <= 2U) type = run::FloorType::Event;
    }
    record(type);
    return type;
}

const FloorScheduleState& FloorScheduler::state() const noexcept { return state_; }

void FloorScheduler::record(const run::FloorType type) noexcept
{
    if (type == run::FloorType::Boss) return;
    state_.floorsSinceMerchant = type == run::FloorType::Merchant ? 0U : state_.floorsSinceMerchant + 1U;
    state_.floorsSinceEvent = type == run::FloorType::Event ? 0U : state_.floorsSinceEvent + 1U;
}
}
