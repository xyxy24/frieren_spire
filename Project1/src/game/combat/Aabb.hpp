#pragma once

namespace arcane::game
{
struct Aabb
{
    float left {0.0F};
    float top {0.0F};
    float width {0.0F};
    float height {0.0F};
};

[[nodiscard]] constexpr bool intersects(const Aabb& left, const Aabb& right) noexcept
{
    return left.left < right.left + right.width
        && left.left + left.width > right.left
        && left.top < right.top + right.height
        && left.top + left.height > right.top;
}
}
