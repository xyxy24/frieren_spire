#pragma once

#include <cstdint>

namespace arcane::common
{
enum class UiEventKind : std::uint8_t
{
    ScreenChanged,
    FloorChanged,
    SpellAcquired,
    RunEnded
};

struct UiEvent
{
    UiEventKind kind {UiEventKind::ScreenChanged};
    std::uint32_t value {};
};
}
