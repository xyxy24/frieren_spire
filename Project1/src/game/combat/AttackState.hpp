#pragma once

#include <cstdint>

namespace arcane::game
{
class AttackState
{
public:
    void update(float deltaSeconds) noexcept;
    bool tryStart() noexcept;

    [[nodiscard]] bool isActive() const noexcept;
    [[nodiscard]] float cooldownRemaining() const noexcept;
    [[nodiscard]] std::uint64_t sequence() const noexcept;

private:
    static constexpr float CooldownSeconds = 0.35F;
    static constexpr float ActiveSeconds = 0.10F;

    float cooldownRemaining_ {0.0F};
    float activeRemaining_ {0.0F};
    std::uint64_t sequence_ {0};
};
}
