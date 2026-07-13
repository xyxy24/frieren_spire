#pragma once

#include "game/combat/Aabb.hpp"
#include "game/math/Vec2.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace arcane::game::spells
{
enum class SpellEffect : std::uint8_t {
    DirectDamage, FlowerField, GoddessBlessing, BloodMagic, FrostLance, FlameBurst,
    InnateGuard, MagicThread, StoneShot, InnateDash, Phantom, SourGrape,
    Cleaning, HotTea, BirdCapture, ManaTrace, BossZoltraak, GoddessSpears,
    SeveringSlash, Mimic, DestructionLightning, HellfireStorm, JudgmentBeam,
    EarthPillars, MirrorArray, MultiZoltraak, Dispel, ManaStrike, GoldenBinding,
    FloatSlam, StoneGolem, Flight, SpatialShatter, Seal, LightningStaff,
    HomingVolley, DefensiveBarrier, WindPressure, GravityWell
};
enum class SpellShape : std::uint8_t { Self, ForwardBox, SelfArea, TargetArea, Summon };
enum class SpellTier : std::uint8_t { Regular, Innate, Boss };

struct SpellDefinition
{
    std::uint32_t id {};
    const char* name {};
    const char* description {};
    SpellEffect effect {SpellEffect::DirectDamage};
    SpellShape shape {SpellShape::ForwardBox};
    // Base damage before combat modifiers.
    int damage {};
    float cooldownSeconds {};
    float range {};
    float height {48.0F};
    SpellTier tier {SpellTier::Regular};
};

struct SpellSlotView
{
    std::optional<std::uint32_t> id;
    float cooldownRemaining {};
    float cooldownDuration {};
};

struct SpellCastResult
{
    bool cast {};
    bool hit {};
    // Successful casts retain base damage even when the preview target is missed.
    int damage {};
    std::uint64_t sequence {};
    SpellEffect effect {SpellEffect::DirectDamage};
    std::uint32_t spellId {};
    Aabb effectBounds;
};

[[nodiscard]] const SpellDefinition* findDefinition(std::uint32_t id) noexcept;
[[nodiscard]] bool isBossSpell(std::uint32_t id) noexcept;

class SpellSystem
{
public:
    static constexpr float UltimateCooldownSeconds = 18.0F;

    explicit SpellSystem(const std::array<std::optional<std::uint32_t>, 3>& equippedIds = {},
        std::optional<std::uint32_t> equippedUltimateId = std::nullopt);
    void update(float deltaSeconds) noexcept;
    [[nodiscard]] SpellCastResult tryCast(std::size_t slot, Vec2 casterPosition,
        float facingDirection, const Aabb& targetBounds) noexcept;
    [[nodiscard]] SpellCastResult tryCastUltimate(Vec2 casterPosition,
        float facingDirection, const Aabb& targetBounds) noexcept;
    [[nodiscard]] std::array<SpellSlotView, 3> view() const noexcept;
    [[nodiscard]] SpellSlotView ultimateView() const noexcept;
    [[nodiscard]] bool equip(std::size_t slot, std::optional<std::uint32_t> id) noexcept;
    [[nodiscard]] bool equipUltimate(std::optional<std::uint32_t> id) noexcept;
    void reduceLongestRegularCooldown(float seconds) noexcept;
    void reduceRegularCooldown(std::size_t slot, float seconds) noexcept;
    void addRegularCooldown(std::size_t slot, float seconds) noexcept;
    void reduceUltimateCooldown(float seconds) noexcept;
    void setUltimateCooldown(float seconds) noexcept;

private:
    struct SlotState
    {
        const SpellDefinition* definition {};
        float cooldownRemaining {};
        std::uint64_t castSequence {};
    };
    [[nodiscard]] static SpellCastResult tryCastState(SlotState& state, float cooldownSeconds,
        Vec2 casterPosition, float facingDirection, const Aabb& targetBounds) noexcept;
    std::array<SlotState, 3> slots_;
    SlotState ultimate_;
    float ultimateCooldownSeconds_ {UltimateCooldownSeconds};
};
}
