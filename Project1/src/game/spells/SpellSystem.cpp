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
    SpellDefinition {2001U, "Starfall", "Boss-tier direct damage.", SpellEffect::DirectDamage, 70, SpellSystem::UltimateCooldownSeconds, 260.0F, 80.0F, SpellTier::Boss},
    SpellDefinition {2002U, "Sanctuary", "Boss-tier blessing.", SpellEffect::GoddessBlessing, 0, SpellSystem::UltimateCooldownSeconds, 0.0F, 0.0F, SpellTier::Boss},
    SpellDefinition {2003U, "Crimson Verdict", "Boss-tier blood magic.", SpellEffect::BloodMagic, 0, SpellSystem::UltimateCooldownSeconds, 320.0F, 96.0F, SpellTier::Boss}
};
}

const SpellDefinition* findDefinition(const std::uint32_t id) noexcept
{
    const auto found = std::find_if(Definitions.begin(), Definitions.end(), [id](const SpellDefinition& value) {
        return value.id == id;
    });
    return found == Definitions.end() ? nullptr : &*found;
}

bool isBossSpell(const std::uint32_t id) noexcept
{
    const auto* definition = findDefinition(id);
    return definition && definition->tier == SpellTier::Boss;
}

SpellSystem::SpellSystem(const std::array<std::optional<std::uint32_t>, 3>& equippedIds,
    const std::optional<std::uint32_t> equippedUltimateId)
{
    for (std::size_t index = 0U; index < slots_.size(); ++index)
        static_cast<void>(equip(index, equippedIds[index]));
    static_cast<void>(equipUltimate(equippedUltimateId));
}

void SpellSystem::update(const float deltaSeconds) noexcept
{
    if (deltaSeconds <= 0.0F) return;
    for (auto& slot : slots_)
        slot.cooldownRemaining = std::max(0.0F, slot.cooldownRemaining - deltaSeconds);
    ultimate_.cooldownRemaining = std::max(0.0F, ultimate_.cooldownRemaining - deltaSeconds);
}

SpellCastResult SpellSystem::tryCast(const std::size_t slot, const Vec2 casterPosition,
    const float facingDirection, const Aabb& targetBounds) noexcept
{
    if (slot >= slots_.size()) return {};
    return tryCastState(slots_[slot], slots_[slot].definition
        ? slots_[slot].definition->cooldownSeconds : 0.0F,
        casterPosition, facingDirection, targetBounds);
}

SpellCastResult SpellSystem::tryCastUltimate(const Vec2 casterPosition,
    const float facingDirection, const Aabb& targetBounds) noexcept
{
    return tryCastState(ultimate_, UltimateCooldownSeconds,
        casterPosition, facingDirection, targetBounds);
}

SpellCastResult SpellSystem::tryCastState(SlotState& state, const float cooldownSeconds,
    const Vec2 casterPosition, const float facingDirection, const Aabb& targetBounds) noexcept
{
    if (!state.definition || state.cooldownRemaining > 0.0F) return {};
    const auto& spell = *state.definition;
    state.cooldownRemaining = cooldownSeconds;
    ++state.castSequence;
    const float left = facingDirection >= 0.0F
        ? casterPosition.x + PlayerController::Width
        : casterPosition.x - spell.range;
    const Aabb effectBounds {left, casterPosition.y + (PlayerController::Height - spell.height) * 0.5F,
        spell.range, spell.height};
    const bool hit = intersects(effectBounds, targetBounds);
    return {true, hit, hit ? spell.damage : 0, state.castSequence, spell.effect};
}

bool SpellSystem::equip(const std::size_t slot, const std::optional<std::uint32_t> id) noexcept
{
    if (slot >= slots_.size()) return false;
    const SpellDefinition* definition = id ? findDefinition(*id) : nullptr;
    if (id && (!definition || definition->tier != SpellTier::Regular)) return false;
    slots_[slot].definition = definition;
    return true;
}

bool SpellSystem::equipUltimate(const std::optional<std::uint32_t> id) noexcept
{
    const SpellDefinition* definition = id ? findDefinition(*id) : nullptr;
    if (id && (!definition || definition->tier != SpellTier::Boss)) return false;
    ultimate_.definition = definition;
    return true;
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

SpellSlotView SpellSystem::ultimateView() const noexcept
{
    return {ultimate_.definition
            ? std::optional<std::uint32_t> {ultimate_.definition->id}
            : std::nullopt,
        ultimate_.cooldownRemaining,
        ultimate_.definition ? UltimateCooldownSeconds : 0.0F};
}
}
