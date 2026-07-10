#include "game/spells/SpellSystem.hpp"

#include "game/player/PlayerController.hpp"

#include <algorithm>
#include <array>

namespace arcane::game::spells
{
namespace
{
constexpr std::array Definitions {
    SpellDefinition {1001U, "Blooming Field", "Large field: enemies -50% speed; heal 5 HP/s for 4s.", SpellEffect::FlowerField, 0, 7.0F, 360.0F, 180.0F},
    SpellDefinition {1002U, "Goddess Blessing", "8s control immunity and +20% damage.", SpellEffect::GoddessBlessing, 0, 10.0F, 0.0F, 0.0F},
    SpellDefinition {1003U, "Blood Magic", "Lose 10% current HP; deal 50% current HP ahead.", SpellEffect::BloodMagic, 0, 4.0F, 210.0F, 72.0F},
    SpellDefinition {1004U, "Arc Bolt", "Fast medium-range direct damage.", SpellEffect::DirectDamage, 20, 0.8F, 150.0F, 48.0F},
    SpellDefinition {1005U, "Frost Lance", "Long-range direct damage.", SpellEffect::DirectDamage, 45, 2.8F, 220.0F, 54.0F},
    SpellDefinition {1006U, "Flame Burst", "Heavy short-range direct damage.", SpellEffect::DirectDamage, 60, 4.5F, 130.0F, 80.0F},
    SpellDefinition {2001U, "Starfall", "Boss-tier direct damage.", SpellEffect::DirectDamage, 70, 5.0F, 260.0F, 80.0F},
    SpellDefinition {2002U, "Sanctuary", "Boss-tier blessing.", SpellEffect::GoddessBlessing, 0, 10.0F, 0.0F, 0.0F},
    SpellDefinition {2003U, "Crimson Verdict", "Boss-tier blood magic.", SpellEffect::BloodMagic, 0, 8.0F, 320.0F, 96.0F}
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
    return {true, hit, hit ? spell.damage : 0, state.castSequence, spell.effect};
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
