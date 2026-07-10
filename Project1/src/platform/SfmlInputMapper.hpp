#pragma once

#include "game/contracts/PlayerIntent.hpp"

namespace arcane::platform
{
class SfmlInputMapper
{
public:
    [[nodiscard]] game::PlayerIntent sample();

private:
    bool previousJump_ {false};
    bool previousAttack_ {false};
    bool previousInteract_ {false};
    bool previousSpell0_ {false};
    bool previousSpell1_ {false};
    bool previousSpell2_ {false};
};
}

