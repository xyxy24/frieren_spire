#include "game/spells/SpellSystem.hpp"

#include "game/player/PlayerController.hpp"

#include <algorithm>
#include <array>

namespace arcane::game::spells
{
namespace
{
constexpr std::array Definitions {
    SpellDefinition {1001U, 25, 1.20F, 190.0F, 42.0F},
    SpellDefinition {1002U, 35, 2.00F, 270.0F, 34.0F},
    SpellDefinition {1003U, 50, 3.50F, 110.0F, 72.0F},
    SpellDefinition {1004U, 20, 0.80F, 150.0F, 48.0F},
    SpellDefinition {1005U, 45, 2.80F, 220.0F, 54.0F},
    SpellDefinition {1006U, 60, 4.50F, 130.0F, 80.0F},
    SpellDefinition {2001U, 70, 5.00F, 260.0F, 80.0F},
    SpellDefinition {2002U, 80, 6.00F, 180.0F, 110.0F},
    SpellDefinition {2003U, 100, 8.00F, 320.0F, 96.0F}
};
}

const SpellDefinition* findDefinition(const std::uint32_t id) noexcept
{
    const auto found = std::find_if(Definitions.begin(), Definitions.end(), [id](const SpellDefinition& value) {
        return value.id == id;
    });
    return found == Definitions.end() ? nullptr : &*found;
}

SpellSystem::SpellSystem(const std::array<std::optional<std::uint32_t>, 3>& equippedIds)
{
    for (std::size_t index = 0U; index < slots_.size(); ++index)
        if (equippedIds[index]) slots_[index].definition = findDefinition(*equippedIds[index]);
}

void SpellSystem::update(const float deltaSeconds) noexcept
{
    if (deltaSeconds <= 0.0F) return;
    for (auto& slot : slots_)
        slot.cooldownRemaining = std::max(0.0F, slot.cooldownRemaining - deltaSeconds);
}

SpellCastResult SpellSystem::tryCast(const std::size_t slot, const Vec2 casterPosition,
    const float facingDirection, const Aabb& targetBounds) noexcept
{
    if (slot >= slots_.size()) return {};
    auto& state = slots_[slot];
    if (!state.definition || state.cooldownRemaining > 0.0F) return {};
    const auto& spell = *state.definition;
    state.cooldownRemaining = spell.cooldownSeconds;
    ++state.castSequence;
    const float left = facingDirection >= 0.0F
        ? casterPosition.x + PlayerController::Width
        : casterPosition.x - spell.range;
    const Aabb effectBounds {left, casterPosition.y + (PlayerController::Height - spell.height) * 0.5F,
        spell.range, spell.height};
    const bool hit = intersects(effectBounds, targetBounds);
    return {true, hit, hit ? spell.damage : 0, state.castSequence};
}

std::array<SpellSlotView, 3> SpellSystem::view() const noexcept
{
    std::array<SpellSlotView, 3> result;
    for (std::size_t index = 0U; index < slots_.size(); ++index)
    {
        const auto& slot = slots_[index];
        result[index] = {slot.definition ? std::optional<std::uint32_t> {slot.definition->id} : std::nullopt,
            slot.cooldownRemaining, slot.definition ? slot.definition->cooldownSeconds : 0.0F};
    }
    return result;
}
}
