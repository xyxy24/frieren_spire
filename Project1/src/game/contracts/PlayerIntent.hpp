#pragma once

#include <array>

namespace arcane::game
{
struct PlayerIntent
{
    float moveAxis {0.0F};
    bool jumpPressed {false};
    bool attackPressed {false};
    std::array<bool, 3> spellPressed {false, false, false};
    bool interactPressed {false};
    bool toggleLoadoutPressed {false};
    bool menuPreviousPressed {false};
    bool menuNextPressed {false};
};
}
