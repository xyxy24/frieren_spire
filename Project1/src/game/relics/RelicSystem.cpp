#include "game/relics/RelicSystem.hpp"

#include <algorithm>
#include <array>

namespace arcane::game::relics
{
namespace
{
constexpr std::array Definitions {
    RelicDefinition {DarkDragonHornId, "Dark Dragon Horn", "+20% damage; take +20% damage for first 15s of combat."},
    RelicDefinition {MoonGrassFlowerId, "Moon Grass Flower", "Cast Blooming Field when combat starts."}
};
}

const RelicDefinition* findDefinition(const std::uint32_t id) noexcept
{
    const auto found = std::find_if(Definitions.begin(), Definitions.end(), [id](const RelicDefinition& value) {
        return value.id == id;
    });
    return found == Definitions.end() ? nullptr : &*found;
}

RelicRuntime::RelicRuntime(const std::span<const std::uint32_t> relicIds)
{
    darkDragonHorn_ = std::find(relicIds.begin(), relicIds.end(), DarkDragonHornId) != relicIds.end();
    moonGrassFlower_ = std::find(relicIds.begin(), relicIds.end(), MoonGrassFlowerId) != relicIds.end();
    if (darkDragonHorn_) vulnerableRemaining_ = 15.0F;
}

void RelicRuntime::update(const float deltaSeconds) noexcept
{
    if (deltaSeconds > 0.0F) vulnerableRemaining_ = std::max(0.0F, vulnerableRemaining_ - deltaSeconds);
}

float RelicRuntime::outgoingDamageMultiplier() const noexcept { return darkDragonHorn_ ? 1.2F : 1.0F; }
float RelicRuntime::incomingDamageMultiplier() const noexcept
{
    return darkDragonHorn_ && vulnerableRemaining_ > 0.0F ? 1.2F : 1.0F;
}
bool RelicRuntime::castsFlowerFieldOnCombatStart() const noexcept { return moonGrassFlower_; }
float RelicRuntime::vulnerableRemaining() const noexcept { return vulnerableRemaining_; }
}
