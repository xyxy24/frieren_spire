#include "presentation/FlowViewState.hpp"

namespace arcane::presentation
{
HudState makeHudState(const game::run::PlayerProgress& progress, const game::run::RunContext& context,
    const game::PlayerStateView* const combatView) noexcept
{
    return {
        combatView ? combatView->currentHealth : progress.currentHp,
        combatView ? combatView->maximumHealth : progress.maxHp,
        progress.gold,
        progress.equippedSpells,
        context.floorIndex,
        context.act,
        context.bossesDefeated
    };
}

void FlowViewController::sync(const game::run::RunPhase phase) noexcept
{
    switch (phase)
    {
    case game::run::RunPhase::Reward: baseScreen_ = FlowScreen::Reward; break;
    case game::run::RunPhase::Loadout: baseScreen_ = FlowScreen::Loadout; break;
    case game::run::RunPhase::FloorComplete: baseScreen_ = FlowScreen::FloorComplete; break;
    case game::run::RunPhase::Defeat: baseScreen_ = FlowScreen::Defeat; break;
    case game::run::RunPhase::Victory: baseScreen_ = FlowScreen::Victory; break;
    default: baseScreen_ = FlowScreen::Gameplay; break;
    }
    if (baseScreen_ != FlowScreen::Gameplay) paused_ = false;
}

bool FlowViewController::togglePause() noexcept
{
    if (baseScreen_ != FlowScreen::Gameplay) return false;
    paused_ = !paused_;
    return true;
}

FlowScreen FlowViewController::screen() const noexcept { return paused_ ? FlowScreen::Pause : baseScreen_; }
bool FlowViewController::gameplayPaused() const noexcept { return paused_; }
}
