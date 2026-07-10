#pragma once

#include <cstdint>
#include <span>

namespace arcane::game::relics
{
inline constexpr std::uint32_t DarkDragonHornId = 4001U;
inline constexpr std::uint32_t MoonGrassFlowerId = 4002U;

struct RelicDefinition
{
    std::uint32_t id {};
    const char* name {};
    const char* description {};
};

[[nodiscard]] const RelicDefinition* findDefinition(std::uint32_t id) noexcept;

class RelicRuntime
{
public:
    explicit RelicRuntime(std::span<const std::uint32_t> relicIds = {});
    void update(float deltaSeconds) noexcept;
    [[nodiscard]] float outgoingDamageMultiplier() const noexcept;
    [[nodiscard]] float incomingDamageMultiplier() const noexcept;
    [[nodiscard]] bool castsFlowerFieldOnCombatStart() const noexcept;
    [[nodiscard]] float vulnerableRemaining() const noexcept;

private:
    bool darkDragonHorn_ {};
    bool moonGrassFlower_ {};
    float vulnerableRemaining_ {};
};
}
