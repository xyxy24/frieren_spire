#pragma once

#include "game/run/RunTypes.hpp"

#include <optional>
#include <span>

namespace arcane::game::floors
{
class FloorController
{
public:
    [[nodiscard]] const run::FloorDescriptor& load(const run::RunContext& context,
        run::FloorType type, std::span<const run::ContentId> encounterPool);
    void markEncounterComplete();
    [[nodiscard]] bool canUseStairs() const noexcept;
    [[nodiscard]] bool unload() noexcept;
    [[nodiscard]] bool isLoaded() const noexcept;

private:
    std::optional<run::FloorDescriptor> activeFloor_;
    bool encounterComplete_ {};
};
}
