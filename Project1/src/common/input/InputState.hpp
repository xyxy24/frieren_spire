#pragma once

#include <array>
#include <cstdint>

namespace arcane::common
{
struct InputState
{
    float moveAxis {0.0F};
    float verticalMoveAxis {0.0F};
    bool jumpPressed {false};
    bool attackPressed {false};
    bool dashPressed {false};
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
    bool debugSpellAcquisitionPreviewPressed {false};
    std::array<bool, 3> debugBossPreviewPressed {false, false, false};
};

struct FrameCommand
{
    InputState input;
    float deltaSeconds {};
};

enum class UiAction : std::uint8_t
{
    ToggleLoadout,
    SelectPrevious,
    SelectNext,
    PreviousPage,
    NextPage,
    SelectUp,
    SelectDown,
    TogglePause,
    Confirm,
    Secondary,
    PreviewEvent,
    PreviewMerchant,
    PreviewSpellAcquisition,
    PreviewBossOne,
    PreviewBossTwo,
    PreviewBossThree,
};

struct UiCommand
{
    UiAction action {UiAction::Confirm};
};
}
