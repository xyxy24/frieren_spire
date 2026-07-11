#pragma once

#include <array>

namespace arcane::game
{
struct PlayerIntent
{
    float moveAxis {0.0F};
    bool jumpPressed {false};
    bool attackPressed {false};
    bool dashPressed {false};
    bool guardPressed {false};
    std::array<bool, 3> spellPressed {false, false, false};
    bool ultimateSpellPressed {false};
    bool interactPressed {false};
    bool toggleLoadoutPressed {false};
    bool menuPreviousPressed {false};
    bool menuNextPressed {false};
    bool menuPagePreviousPressed {false};
    bool menuPageNextPressed {false};
    bool menuUpPressed {false};
    bool menuDownPressed {false};
    bool pausePressed {false};
    bool menuConfirmPressed {false};
    bool menuSecondaryPressed {false};
    bool debugEventPreviewPressed {false};
    bool debugMerchantPreviewPressed {false};
};
}
