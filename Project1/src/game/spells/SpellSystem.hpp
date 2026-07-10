#pragma once

#include "game/combat/Aabb.hpp"
#include "game/math/Vec2.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace arcane::game::spells
{
struct SpellDefinition
{
    std::uint32_t id {};
    int damage {};
    float cooldownSeconds {};
    float range {};
    float height {48.0F};
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
    int damage {};
};

[[nodiscard]] const SpellDefinition* findDefinition(std::uint32_t id) noexcept;

class SpellSystem
{
public:
    explicit SpellSystem(const std::array<std::optional<std::uint32_t>, 3>& equippedIds = {});
    void update(float deltaSeconds) noexcept;
    [[nodiscard]] SpellCastResult tryCast(std::size_t slot, Vec2 casterPosition,
        float facingDirection, const Aabb& targetBounds) noexcept;
    [[nodiscard]] std::array<SpellSlotView, 3> view() const noexcept;

private:
    struct SlotState
    {
        const SpellDefinition* definition {};
        float cooldownRemaining {};
    };
    std::array<SlotState, 3> slots_;
};
}
