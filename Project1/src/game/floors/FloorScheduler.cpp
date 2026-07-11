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
    const std::uint32_t floorInAct = context.floorIndex % config_.floorsPerBoss + 1U;
    if (floorInAct == config_.floorsPerBoss) return run::FloorType::Boss;

    if (config_.floorsPerBoss == DefaultFloorsPerBoss)
    {
        run::DeterministicRng rng(run::deriveStreamSeed(context.floorSeed, run::RandomStream::Encounter));
        if (floorInAct == 1U || floorInAct == 4U)
        {
            record(run::FloorType::Combat);
            return run::FloorType::Combat;
        }
        if (floorInAct == 2U)
        {
            const auto type = rng.index(10U) <= 2U
                ? run::FloorType::Event
                : run::FloorType::Combat;
            record(type);
            return type;
        }

        run::FloorType special = run::FloorType::Event;
        if (eligibleSinceEvent_ == 0U || eligibleSinceMerchant_ >= config_.merchantMaxEligibleGap)
            special = run::FloorType::Merchant;
        else if (eligibleSinceEvent_ >= config_.eventMaxEligibleGap)
            special = run::FloorType::Event;
        else
            special = rng.index(2U) == 0U ? run::FloorType::Merchant : run::FloorType::Event;
        record(special);
        return special;
    }

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
