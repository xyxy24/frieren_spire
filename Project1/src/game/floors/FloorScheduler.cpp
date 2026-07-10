#include "game/floors/FloorScheduler.hpp"

#include "game/run/DeterministicRng.hpp"

#include <stdexcept>

namespace arcane::game::floors
{
FloorScheduler::FloorScheduler(const FloorScheduleConfig config) : config_(config)
{
    if (config_.floorsPerBoss == 0U || config_.merchantMaxEligibleGap == 0U
        || config_.eventMaxEligibleGap == 0U)
        throw std::invalid_argument("floor schedule intervals must be positive");
}

run::FloorType FloorScheduler::next(const run::RunContext& context)
{
    if ((context.floorIndex + 1U) % config_.floorsPerBoss == 0U) return run::FloorType::Boss;

    run::FloorType type = run::FloorType::Combat;
    if (eligibleSinceMerchant_ >= config_.merchantMaxEligibleGap) type = run::FloorType::Merchant;
    else if (eligibleSinceEvent_ >= config_.eventMaxEligibleGap) type = run::FloorType::Event;
    else if (eligibleSinceMerchant_ + 1U >= config_.merchantMaxEligibleGap
        && eligibleSinceEvent_ + 1U >= config_.eventMaxEligibleGap)
    {
        type = eligibleSinceMerchant_ * config_.eventMaxEligibleGap
                >= eligibleSinceEvent_ * config_.merchantMaxEligibleGap
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

void FloorScheduler::record(const run::FloorType type) noexcept
{
    eligibleSinceMerchant_ = type == run::FloorType::Merchant ? 0U : eligibleSinceMerchant_ + 1U;
    eligibleSinceEvent_ = type == run::FloorType::Event ? 0U : eligibleSinceEvent_ + 1U;
}
}
