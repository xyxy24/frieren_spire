#include "game/spells/SpellSystem.hpp"

#include "game/player/PlayerController.hpp"
#include "game/progression/ProgressionSystem.hpp"

#include <algorithm>
#include <array>

namespace arcane::game::spells
{
namespace
{
constexpr std::array Definitions {
    SpellDefinition {1001U, "Blooming Field", "Create a 4 second field. Enemies move 35 percent slower and the player heals 3 HP each second. Flame turns it into 2 seconds of 8 damage per second.", SpellEffect::FlowerField, SpellShape::SelfArea, 0, 10.0F, 300.0F, 150.0F},
    SpellDefinition {1002U, "Goddess Blessing", "For 5 seconds prevent new control effects and deal 15 percent more damage.", SpellEffect::GoddessBlessing, SpellShape::Self, 0, 15.0F, 0.0F, 0.0F},
    SpellDefinition {1003U, "Blood Magic", "Pay 12 percent current HP then deal 55 percent of the remaining HP as damage in front.", SpellEffect::BloodMagic, SpellShape::ForwardBox, 0, 6.0F, 210.0F, 72.0F},
    SpellDefinition {1004U, "Zoltraak", "Fire a fast piercing line that deals 20 damage to every enemy it crosses.", SpellEffect::DirectDamage, SpellShape::ForwardBox, 20, 1.8F, 180.0F, 48.0F},
    SpellDefinition {1005U, "Frost Lance", "Deal 24 damage, slow by 35 percent for 2 seconds and apply Chill for 5.5 seconds. Frost Lance freezes a Chilled or otherwise slowed target for 0.65 seconds. Flame Burst clears Chill.", SpellEffect::FrostLance, SpellShape::ForwardBox, 24, 4.5F, 220.0F, 54.0F},
    SpellDefinition {1006U, "Flame Burst", "Deal 26 damage plus 12 burning damage over 3 seconds. A slowed or frozen target takes 10 extra thermal damage. Any hit clears Frost Lance slow, Chill and freeze.", SpellEffect::FlameBurst, SpellShape::ForwardBox, 26, 6.5F, 150.0F, 80.0F},
    SpellDefinition {1008U, "Magic Thread", "Deal 8 damage, pull the target up to 110 pixels and bind normal enemies for 0.9 seconds.", SpellEffect::MagicThread, SpellShape::ForwardBox, 8, 6.5F, 280.0F, 12.0F},
    SpellDefinition {1009U, "Stone Shot", "Deal 36 damage with strong knockback. Hitting a world edge deals 14 additional damage.", SpellEffect::StoneShot, SpellShape::ForwardBox, 36, 7.5F, 250.0F, 30.0F},
    SpellDefinition {1010U, "High-speed Movement", "Press K to dash 150 pixels. A fully charged Shade Dash is invulnerable and analyzes crossed enemies for 2 seconds; otherwise a normal dash is used. Normal Dash cooldown is 0.6 seconds and Shade charge time is 1.5 seconds.", SpellEffect::InnateDash, SpellShape::Self, 0, 0.6F, 150.0F, 64.0F, SpellTier::Innate},
    SpellDefinition {1011U, "Phantom", "Create a decoy for 4 seconds. When struck it analyzes the attacker for 3 seconds and reduces the longest cooldown by 1 second.", SpellEffect::Phantom, SpellShape::Summon, 0, 11.0F, 320.0F, 64.0F},
    SpellDefinition {1016U, "Mana Trace", "Scan 500 pixels and analyze the nearest enemy for 4 seconds. It takes 12 percent more damage and reveals attack areas.", SpellEffect::ManaTrace, SpellShape::SelfArea, 0, 9.0F, 500.0F, 300.0F},
    SpellDefinition {1017U, "Multi Zoltraak", "Fire three piercing beams through every enemy in the line. Each beam deals 10 damage; the third deals 10 extra damage to enemies currently marked by Mana Trace, Phantom or Dash.", SpellEffect::MultiZoltraak, SpellShape::ForwardBox, 10, 4.0F, 260.0F, 54.0F},
    SpellDefinition {1018U, "Dispelling Magic", "Cancel one enemy attack that is winding up or currently active inside the area. On success, mark that enemy for 4 seconds and reduce your regular spell with the longest remaining cooldown by 1.5 seconds.", SpellEffect::Dispel, SpellShape::ForwardBox, 0, 9.0F, 180.0F, 120.0F},
    SpellDefinition {1019U, "Mana Strike", "Strike nearby enemies for 34 damage with strong knockback. Cast within 0.4 seconds after Dash for 10 extra damage; knocking the target into a world edge adds another 12 damage.", SpellEffect::ManaStrike, SpellShape::ForwardBox, 34, 5.5F, 90.0F, 80.0F},
    SpellDefinition {1020U, "Golden Binding", "Deal 6 damage and bind normal enemies for 1.4 seconds; bosses are controlled for 0.4 seconds. Stone Shot or Severing Magic consumes the binding to deal 16 extra damage.", SpellEffect::GoldenBinding, SpellShape::ForwardBox, 6, 8.0F, 260.0F, 18.0F},
    SpellDefinition {1021U, "Float and Slam", "Suspend normal enemies for 1 second, then slam every enemy in the target area for 24 damage. A frozen enemy takes 12 additional damage from the slam.", SpellEffect::FloatSlam, SpellShape::TargetArea, 24, 8.0F, 220.0F, 120.0F},
    SpellDefinition {1022U, "Stone Golem", "Summon a golem for 5 seconds that draws enemy attacks. It blocks one attack and shatters for 24 area damage; if it survives until time expires, it attacks one enemy for 20 damage.", SpellEffect::StoneGolem, SpellShape::Summon, 0, 12.0F, 260.0F, 64.0F},
    SpellDefinition {1023U, "Flight Magic", "Fly with W and S for 2.5 seconds, ignore gravity and avoid leaping ground attacks. The first damaging spell cast while flying deals 15 percent more damage; Dash remains available.", SpellEffect::Flight, SpellShape::Self, 0, 12.0F, 0.0F, 0.0F},
    SpellDefinition {1024U, "Spatial Shatter", "Damage every enemy around you for 26. If the blast interrupts at least one active enemy attack, it deals 38 instead and makes you invulnerable for 0.35 seconds.", SpellEffect::SpatialShatter, SpellShape::SelfArea, 26, 9.0F, 115.0F, 96.0F},
    SpellDefinition {1025U, "Sealing Magic", "Deal 16 damage, interrupt the target's current attack and disable special attacks for 4 seconds on normal enemies or 1.5 seconds on bosses. A marked target is sealed 1 second longer.", SpellEffect::Seal, SpellShape::ForwardBox, 16, 10.0F, 200.0F, 72.0F},
    SpellDefinition {1026U, "Lightning Staff", "For 4 seconds, each of your next three basic attacks deals 7 additional lightning damage. The third charged attack also releases a 12 damage blast around you.", SpellEffect::LightningStaff, SpellShape::Self, 0, 9.0F, 0.0F, 0.0F},
    SpellDefinition {1027U, "Homing Volley", "Fire three homing bolts at the nearest enemy within 320 pixels. Each bolt deals 7 damage; the third deals 6 extra damage if the target is flying.", SpellEffect::HomingVolley, SpellShape::TargetArea, 7, 3.5F, 320.0F, 72.0F},
    SpellDefinition {1028U, "Defensive Barrier", "Gain a 24 point shield for 5 seconds. While any shield remains, ordinary enemy contact cannot interrupt your actions; when this barrier breaks it deals 12 damage around you.", SpellEffect::DefensiveBarrier, SpellShape::Self, 0, 10.0F, 0.0F, 0.0F},
    SpellDefinition {1029U, "Wind Pressure", "Blast a wide area in front for 14 damage and strong knockback. Normal enemies that are charging, diving or attacking are interrupted; a wall collision adds 10 damage.", SpellEffect::WindPressure, SpellShape::ForwardBox, 14, 6.0F, 190.0F, 110.0F},
    SpellDefinition {1030U, "Gravity Well", "Create a 2.5 second field at the target. It pulls enemies within 140 pixels toward its center and deals 6 damage each second.", SpellEffect::GravityWell, SpellShape::TargetArea, 0, 9.0F, 260.0F, 280.0F},
    SpellDefinition {2001U, "Demon Killing Zoltraak", "Fire a long beam that pierces every enemy in its path for 70 damage. Demon enemies take 30 percent additional damage; each enemy can be hit only once per cast.", SpellEffect::BossZoltraak, SpellShape::ForwardBox, 70, SpellSystem::UltimateCooldownSeconds, 420.0F, 64.0F, SpellTier::Boss},
    SpellDefinition {2002U, "Goddess Three Spears", "Drop three divine spears into the target area, each dealing 28 damage. If all three hit, restore 12 HP to the player.", SpellEffect::GoddessSpears, SpellShape::TargetArea, 28, SpellSystem::UltimateCooldownSeconds, 340.0F, 96.0F, SpellTier::Boss},
    SpellDefinition {2003U, "Severing Magic", "Slash a short area in front for 78 damage. Targets below 20 percent HP take 30 percent more; hitting Golden Binding consumes it for 16 additional damage.", SpellEffect::SeveringSlash, SpellShape::ForwardBox, 78, SpellSystem::UltimateCooldownSeconds, 105.0F, 96.0F, SpellTier::Boss},
    SpellDefinition {2006U, "Mimic Magic", "Copy the attack template of a nearby living enemy, including its direction and area. The copied attack deals at least 60 damage even when the original attack is weaker.", SpellEffect::Mimic, SpellShape::ForwardBox, 0, SpellSystem::UltimateCooldownSeconds, 260.0F, 96.0F, SpellTier::Boss},
    SpellDefinition {2007U, "Destruction Lightning", "Mark the target area, then strike it after 0.8 seconds for 90 damage. If the selected enemy is marked, the lightning's damage area becomes 40 percent larger.", SpellEffect::DestructionLightning, SpellShape::TargetArea, 90, SpellSystem::UltimateCooldownSeconds, 340.0F, 144.0F, SpellTier::Boss},
    SpellDefinition {2009U, "Hellfire Storm", "Create a forward firestorm for 3 seconds. It continually pulls enemies toward its center and deals 88 total damage to enemies that remain inside.", SpellEffect::HellfireStorm, SpellShape::ForwardBox, 0, SpellSystem::UltimateCooldownSeconds, 360.0F, 140.0F, SpellTier::Boss},
    SpellDefinition {2010U, "Judgment Beam", "Channel a long forward beam for up to 2 seconds. It damages enemies every 0.25 seconds and deals at most 120 total damage to each enemy.", SpellEffect::JudgmentBeam, SpellShape::ForwardBox, 0, SpellSystem::UltimateCooldownSeconds, 420.0F, 72.0F, SpellTier::Boss},
    SpellDefinition {2011U, "Earth Dominion", "Erupt the center of the target area for 45 damage, then raise three stone pillars that deal 25 each. A target near the center is normally hit by the eruption and one pillar for about 70 damage.", SpellEffect::EarthPillars, SpellShape::TargetArea, 45, SpellSystem::UltimateCooldownSeconds, 320.0F, 144.0F, SpellTier::Boss},
    SpellDefinition {2012U, "Mirror Array", "Summon two mirrors. The next regular damaging spell is repeated once by each mirror at 45 percent damage, adding 90 percent copied damage in total; Boss spells are not copied.", SpellEffect::MirrorArray, SpellShape::Summon, 0, SpellSystem::UltimateCooldownSeconds, 0.0F, 64.0F, SpellTier::Boss}
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
    const std::optional<std::uint32_t> equippedUltimateId,
    const std::array<std::uint8_t, 3>& equippedRanks,
    const float regularCooldownMultiplier)
    : regularCooldownMultiplier_(std::max(0.1F, regularCooldownMultiplier))
{
    for (std::size_t index = 0U; index < slots_.size(); ++index)
        static_cast<void>(equip(index, equippedIds[index], equippedRanks[index]));
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
        ? slots_[slot].definition->cooldownSeconds * regularCooldownMultiplier_
            * progression::masteryCooldownMultiplier(slots_[slot].rank) : 0.0F,
        casterPosition, facingDirection, targetBounds);
}

SpellCastResult SpellSystem::tryCastUltimate(const Vec2 casterPosition,
    const float facingDirection, const Aabb& targetBounds) noexcept
{
    return tryCastState(ultimate_, ultimateCooldownSeconds_,
        casterPosition, facingDirection, targetBounds);
}

SpellCastResult SpellSystem::tryCastState(SlotState& state, const float cooldownSeconds,
    const Vec2 casterPosition, const float facingDirection, const Aabb& targetBounds) noexcept
{
    if (!state.definition || state.cooldownRemaining > 0.0F) return {};
    const auto& spell = *state.definition;
    const float areaMultiplier = spell.tier == SpellTier::Regular
        ? progression::masteryAreaMultiplier(state.rank) : 1.0F;
    state.cooldownRemaining = cooldownSeconds;
    ++state.castSequence;
    Aabb effectBounds {casterPosition.x, casterPosition.y,
        PlayerController::Width, PlayerController::Height};
    if (spell.shape == SpellShape::ForwardBox)
    {
        effectBounds.left = facingDirection >= 0.0F
            ? casterPosition.x + PlayerController::Width : casterPosition.x - spell.range * areaMultiplier;
        effectBounds.top = casterPosition.y
            + (PlayerController::Height - spell.height * areaMultiplier) * 0.5F;
        effectBounds.width = spell.range * areaMultiplier;
        effectBounds.height = spell.height * areaMultiplier;
    }
    else if (spell.shape == SpellShape::SelfArea)
    {
        effectBounds = {casterPosition.x + PlayerController::Width * 0.5F
                - spell.range * areaMultiplier,
            casterPosition.y + PlayerController::Height * 0.5F
                - spell.height * areaMultiplier,
            spell.range * areaMultiplier * 2.0F, spell.height * areaMultiplier * 2.0F};
    }
    else if (spell.shape == SpellShape::TargetArea)
    {
        const float targetCenterX = targetBounds.left + targetBounds.width * 0.5F;
        const float casterCenterX = casterPosition.x + PlayerController::Width * 0.5F;
        const float centerX = std::clamp(targetCenterX,
            casterCenterX - spell.range * areaMultiplier,
            casterCenterX + spell.range * areaMultiplier);
        effectBounds = {centerX - spell.height * areaMultiplier * 0.5F,
            targetBounds.top + targetBounds.height * 0.5F
                - spell.height * areaMultiplier * 0.5F,
            spell.height * areaMultiplier, spell.height * areaMultiplier};
    }
    const bool hit = intersects(effectBounds, targetBounds);
    return {true, hit, spell.damage, state.castSequence, spell.effect,
        spell.id, effectBounds, state.rank, spell.tier == SpellTier::Regular
            ? progression::masteryPowerMultiplier(state.rank) : 1.0F};
}

bool SpellSystem::equip(const std::size_t slot, const std::optional<std::uint32_t> id,
    const std::uint8_t rank) noexcept
{
    if (slot >= slots_.size()) return false;
    const SpellDefinition* definition = id ? findDefinition(*id) : nullptr;
    if (id && (!definition || definition->tier != SpellTier::Regular)) return false;
    slots_[slot].definition = definition;
    slots_[slot].rank = std::clamp<std::uint8_t>(rank, 1U, 3U);
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
            slot.cooldownRemaining, slot.definition ? slot.definition->cooldownSeconds
                * regularCooldownMultiplier_
                * progression::masteryCooldownMultiplier(slot.rank) : 0.0F};
    }
    return result;
}

SpellSlotView SpellSystem::ultimateView() const noexcept
{
    return {ultimate_.definition
            ? std::optional<std::uint32_t> {ultimate_.definition->id}
            : std::nullopt,
        ultimate_.cooldownRemaining,
        ultimate_.definition ? ultimateCooldownSeconds_ : 0.0F};
}

void SpellSystem::reduceLongestRegularCooldown(const float seconds) noexcept
{
    if (seconds <= 0.0F) return;
    auto found = std::max_element(slots_.begin(), slots_.end(), [](const auto& left, const auto& right) {
        return left.cooldownRemaining < right.cooldownRemaining;
    });
    if (found != slots_.end())
        found->cooldownRemaining = std::max(0.0F, found->cooldownRemaining - seconds);
}

void SpellSystem::reduceRegularCooldown(const std::size_t slot, const float seconds) noexcept
{
    if (slot < slots_.size()) slots_[slot].cooldownRemaining =
        std::max(0.0F, slots_[slot].cooldownRemaining - std::max(0.0F, seconds));
}

void SpellSystem::addRegularCooldown(const std::size_t slot, const float seconds) noexcept
{
    if (slot < slots_.size()) slots_[slot].cooldownRemaining += std::max(0.0F, seconds);
}

void SpellSystem::reduceUltimateCooldown(const float seconds) noexcept
{ ultimate_.cooldownRemaining = std::max(0.0F, ultimate_.cooldownRemaining - std::max(0.0F, seconds)); }
void SpellSystem::setUltimateCooldown(const float seconds) noexcept
{ ultimateCooldownSeconds_ = std::max(0.0F, seconds); }
}
