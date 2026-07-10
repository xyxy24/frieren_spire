#include "game/combat/Health.hpp"

#include <algorithm>

namespace arcane::game
{
Health::Health(const int maximumHealth, const int currentHealth)
    : maximum_(std::max(1, maximumHealth)),
      current_(std::clamp(currentHealth, 0, maximum_))
{
}

int Health::damage(const int amount) noexcept
{
    if (amount <= 0 || !isAlive())
    {
        return 0;
    }

    const int previous = current_;
    current_ = std::max(0, current_ - amount);
    return previous - current_;
}

int Health::heal(const int amount) noexcept
{
    if (amount <= 0 || !isAlive())
    {
        return 0;
    }

    const int previous = current_;
    current_ = std::min(maximum_, current_ + amount);
    return current_ - previous;
}

int Health::current() const noexcept
{
    return current_;
}

int Health::maximum() const noexcept
{
    return maximum_;
}

bool Health::isAlive() const noexcept
{
    return current_ > 0;
}
}
