#pragma once

namespace arcane::game
{
class Health
{
public:
    Health(int maximumHealth, int currentHealth);

    int damage(int amount) noexcept;
    int heal(int amount) noexcept;

    [[nodiscard]] int current() const noexcept;
    [[nodiscard]] int maximum() const noexcept;
    [[nodiscard]] bool isAlive() const noexcept;

private:
    int maximum_ {1};
    int current_ {1};
};
}
