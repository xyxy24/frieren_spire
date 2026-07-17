#pragma once

#include "common/geometry/Geometry.hpp"

namespace arcane::game
{
using Aabb = common::RectF;

[[nodiscard]] constexpr bool intersects(const Aabb& left, const Aabb& right) noexcept
{
    return left.left < right.left + right.width
        && left.left + left.width > right.left
        && left.top < right.top + right.height
        && left.top + left.height > right.top;
}
}
