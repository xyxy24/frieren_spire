#include "game/spells/SpellSystem.hpp"

#include "game/player/PlayerController.hpp"

#include <algorithm>
#include <array>

namespace arcane::game::spells
{
namespace
{
constexpr std::array Definitions {
    SpellDefinition {1001U, "Blooming Field", "Large field: enemies -50% speed; heal 5 HP/s for 4s.", SpellEffect::FlowerField, SpellShape::SelfArea, 0, 7.0F, 360.0F, 180.0F},
    SpellDefinition {1002U, "Goddess Blessing", "6s control immunity and +20% damage.", SpellEffect::GoddessBlessing, SpellShape::Self, 0, 14.0F, 0.0F, 0.0F},
    SpellDefinition {1003U, "Blood Magic", "Lose 10% current HP; deal 50% current HP ahead.", SpellEffect::BloodMagic, SpellShape::ForwardBox, 0, 4.0F, 210.0F, 72.0F},
    SpellDefinition {1004U, "Zoltraak", "Fast medium-range piercing bolt.", SpellEffect::DirectDamage, SpellShape::ForwardBox, 22, 1.5F, 150.0F, 48.0F},
    SpellDefinition {1005U, "Frost Lance", "28 damage and 40% slow for 2.5s.", SpellEffect::FrostLance, SpellShape::ForwardBox, 28, 4.0F, 220.0F, 54.0F},
    SpellDefinition {1006U, "Flame Burst", "32 damage and 12 burning damage over 3s.", SpellEffect::FlameBurst, SpellShape::ForwardBox, 32, 6.0F, 130.0F, 80.0F},
    SpellDefinition {1007U, "Defensive Magic", "Innate guard action bound to L.", SpellEffect::InnateGuard, SpellShape::Self, 0, 2.0F, 0.0F, 0.0F, SpellTier::Innate},
    SpellDefinition {1008U, "Magic Thread", "Line hit: 10 damage, pull and bind.", SpellEffect::MagicThread, SpellShape::ForwardBox, 10, 6.0F, 280.0F, 12.0F},
    SpellDefinition {1009U, "Stone Shot", "Heavy projectile with strong knockback.", SpellEffect::StoneShot, SpellShape::ForwardBox, 42, 7.0F, 250.0F, 30.0F},
    SpellDefinition {1010U, "High-speed Movement", "Innate shadow dash action bound to K.", SpellEffect::InnateDash, SpellShape::Self, 0, 1.2F, 150.0F, 64.0F, SpellTier::Innate},
    SpellDefinition {1011U, "Phantom", "Summon a phantom decoy for 3s.", SpellEffect::Phantom, SpellShape::Summon, 0, 9.0F, 340.0F, 64.0F},
    SpellDefinition {1012U, "Sour Grape", "Create a 5s acid field that exposes enemies.", SpellEffect::SourGrape, SpellShape::TargetArea, 0, 8.0F, 210.0F, 168.0F},
    SpellDefinition {1013U, "Cleaning Magic", "Heal 10 HP and gain brief protection.", SpellEffect::Cleaning, SpellShape::Self, 0, 8.0F, 0.0F, 0.0F},
    SpellDefinition {1014U, "Hot Tea", "Channel for 1s, then heal 18 HP.", SpellEffect::HotTea, SpellShape::Self, 0, 12.0F, 0.0F, 0.0F},
    SpellDefinition {1015U, "Bird Capture", "Lock a target: 8 damage and bind.", SpellEffect::BirdCapture, SpellShape::SelfArea, 8, 9.0F, 340.0F, 180.0F},
    SpellDefinition {1016U, "Mana Trace", "Scan 500px and mark the nearest enemy for 6s.", SpellEffect::ManaTrace, SpellShape::SelfArea, 0, 7.0F, 500.0F, 300.0F},
    SpellDefinition {2001U, "Demon Killing Zoltraak", "Pierce all targets for 70 damage. Demons take 30 percent more.", SpellEffect::BossZoltraak, SpellShape::ForwardBox, 70, SpellSystem::UltimateCooldownSeconds, 420.0F, 64.0F, SpellTier::Boss},
    SpellDefinition {2002U, "Goddess Three Spears", "Three spears deal 28 damage each. Full hit heals 12 HP.", SpellEffect::GoddessSpears, SpellShape::TargetArea, 28, SpellSystem::UltimateCooldownSeconds, 340.0F, 96.0F, SpellTier::Boss},
    SpellDefinition {2003U, "Severing Magic", "Short slash deals 78 damage and 30 percent more below 20 percent HP.", SpellEffect::SeveringSlash, SpellShape::ForwardBox, 78, SpellSystem::UltimateCooldownSeconds, 105.0F, 96.0F, SpellTier::Boss},
    SpellDefinition {2006U, "Mimic Magic", "Copy a nearby enemy attack at 120 percent power.", SpellEffect::Mimic, SpellShape::ForwardBox, 0, SpellSystem::UltimateCooldownSeconds, 260.0F, 96.0F, SpellTier::Boss},
    SpellDefinition {2007U, "Destruction Lightning", "After 0.8 seconds strike for 90 damage. Marked targets enlarge the area.", SpellEffect::DestructionLightning, SpellShape::TargetArea, 90, SpellSystem::UltimateCooldownSeconds, 340.0F, 144.0F, SpellTier::Boss},
    SpellDefinition {2009U, "Hellfire Storm", "Pull enemies and deal 88 damage over 3 seconds.", SpellEffect::HellfireStorm, SpellShape::ForwardBox, 0, SpellSystem::UltimateCooldownSeconds, 360.0F, 140.0F, SpellTier::Boss},
    SpellDefinition {2010U, "Judgment Beam", "Channel a beam for 2 seconds dealing up to 120 damage.", SpellEffect::JudgmentBeam, SpellShape::ForwardBox, 0, SpellSystem::UltimateCooldownSeconds, 420.0F, 72.0F, SpellTier::Boss},
    SpellDefinition {2011U, "Earth Dominion", "Raise three pillars for 35 damage each. Repeat hits deal less.", SpellEffect::EarthPillars, SpellShape::TargetArea, 35, SpellSystem::UltimateCooldownSeconds, 320.0F, 144.0F, SpellTier::Boss},
    SpellDefinition {2012U, "Mirror Array", "Two mirrors repeat the next regular damage spell at 35 percent each.", SpellEffect::MirrorArray, SpellShape::Summon, 0, SpellSystem::UltimateCooldownSeconds, 0.0F, 64.0F, SpellTier::Boss}
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
    Aabb effectBounds {casterPosition.x, casterPosition.y,
        PlayerController::Width, PlayerController::Height};
    if (spell.shape == SpellShape::ForwardBox)
    {
        effectBounds.left = facingDirection >= 0.0F
            ? casterPosition.x + PlayerController::Width : casterPosition.x - spell.range;
        effectBounds.top = casterPosition.y + (PlayerController::Height - spell.height) * 0.5F;
        effectBounds.width = spell.range;
        effectBounds.height = spell.height;
    }
    else if (spell.shape == SpellShape::SelfArea)
    {
        effectBounds = {casterPosition.x + PlayerController::Width * 0.5F - spell.range,
            casterPosition.y + PlayerController::Height * 0.5F - spell.height,
            spell.range * 2.0F, spell.height * 2.0F};
    }
    else if (spell.shape == SpellShape::TargetArea)
    {
        const float targetCenterX = targetBounds.left + targetBounds.width * 0.5F;
        const float casterCenterX = casterPosition.x + PlayerController::Width * 0.5F;
        const float centerX = std::clamp(targetCenterX,
            casterCenterX - spell.range, casterCenterX + spell.range);
        effectBounds = {centerX - spell.height * 0.5F,
            targetBounds.top + targetBounds.height * 0.5F - spell.height * 0.5F,
            spell.height, spell.height};
    }
    const bool hit = intersects(effectBounds, targetBounds);
    return {true, hit, spell.damage, state.castSequence, spell.effect,
        spell.id, effectBounds};
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
