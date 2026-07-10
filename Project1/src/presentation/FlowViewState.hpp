#pragma once

#include "game/combat/CombatContracts.hpp"
#include "game/run/RunTypes.hpp"

#include <array>
#include <optional>

namespace arcane::presentation
{
enum class FlowScreen : std::uint8_t
{
    Gameplay,
    Reward,
    Loadout,
    FloorComplete,
    Pause,
    Defeat,
    Victory
};

struct HudState
{
    int currentHp {};
    int maxHp {};
    int gold {};
    std::array<std::optional<game::run::ContentId>, 3> equippedSpells;
    std::uint32_t floorIndex {};
    std::uint32_t act {};
    std::uint32_t bossesDefeated {};
};

[[nodiscard]] HudState makeHudState(const game::run::PlayerProgress& progress,
    const game::run::RunContext& context, const game::PlayerStateView* combatView = nullptr) noexcept;

class FlowViewController
{
public:
    void sync(game::run::RunPhase phase) noexcept;
    [[nodiscard]] bool togglePause() noexcept;
    [[nodiscard]] FlowScreen screen() const noexcept;
    [[nodiscard]] bool gameplayPaused() const noexcept;

private:
    FlowScreen baseScreen_ {FlowScreen::Gameplay};
    bool paused_ {};
};
}
