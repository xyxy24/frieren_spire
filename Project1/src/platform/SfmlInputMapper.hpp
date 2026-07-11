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
    bool previousToggleLoadout_ {false};
    bool previousMenuLeft_ {false};
    bool previousMenuRight_ {false};
    bool previousMenuPageLeft_ {false};
    bool previousMenuPageRight_ {false};
    bool previousMenuUp_ {false};
    bool previousMenuDown_ {false};
    bool previousPause_ {false};
    bool previousConfirm_ {false};
    bool previousSecondary_ {false};
    bool previousDebugEventPreview_ {false};
    bool previousDebugMerchantPreview_ {false};
};
}
