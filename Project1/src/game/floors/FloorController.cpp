#include "game/floors/FloorController.hpp"

#include "game/run/DeterministicRng.hpp"

#include <stdexcept>

namespace arcane::game::floors
{
const run::FloorDescriptor& FloorController::load(const run::RunContext& context,
    const run::FloorType type, const std::span<const run::ContentId> encounterPool)
{
    if (activeFloor_)
    {
        throw std::logic_error("an active floor must be unloaded before loading another");
    }
    if ((type == run::FloorType::Combat || type == run::FloorType::Boss) && encounterPool.empty())
    {
        throw std::invalid_argument("combat floors require an encounter pool");
    }

    run::DeterministicRng layout(run::deriveStreamSeed(context.floorSeed, run::RandomStream::Layout));
    run::DeterministicRng encounters(run::deriveStreamSeed(context.floorSeed, run::RandomStream::Encounter));
    run::FloorDescriptor descriptor {type, context.floorSeed, 100U + layout.index(4U), {}};
    if (!encounterPool.empty())
    {
        descriptor.encounterIds.push_back(encounterPool[encounters.index(
            static_cast<std::uint32_t>(encounterPool.size()))]);
    }
    activeFloor_ = std::move(descriptor);
    encounterComplete_ = false;
    return *activeFloor_;
}

void FloorController::markEncounterComplete()
{
    if (!activeFloor_)
    {
        throw std::logic_error("cannot complete an unloaded floor");
    }
    encounterComplete_ = true;
}

bool FloorController::canUseStairs() const noexcept { return activeFloor_.has_value() && encounterComplete_; }

bool FloorController::unload() noexcept
{
    if (!activeFloor_) return false;
    activeFloor_.reset();
    encounterComplete_ = false;
    return true;
}

bool FloorController::isLoaded() const noexcept { return activeFloor_.has_value(); }
}
