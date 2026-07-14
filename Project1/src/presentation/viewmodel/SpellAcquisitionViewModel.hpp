#pragma once

#include "game/contracts/PlayerIntent.hpp"
#include "presentation/viewmodel/UiViewModels.hpp"

#include <optional>

namespace arcane::presentation::viewmodel
{
struct SpellAcquisitionSnapshot
{
    ContentDetailViewModel content;
    float elapsedSeconds {};
    float chargeProgress {};
    float unlockProgress {};
    float revealProgress {};
    float flashRatio {};
    bool canDismiss {};
    bool bossSpell {};
};

class SpellAcquisitionViewModel
{
public:
    static constexpr float ChargeSeconds = 0.72F;
    static constexpr float UnlockSeconds = 0.58F;
    static constexpr float RevealSeconds = 0.48F;
    static constexpr float MinimumDisplaySeconds = 1.85F;
    static constexpr float BossDurationScale = 1.18F;

    void start(game::run::ContentId spellId) noexcept;
    void reset() noexcept;
    void update(const game::PlayerIntent& intent, float deltaSeconds) noexcept;

    [[nodiscard]] bool active() const noexcept;
    [[nodiscard]] std::optional<SpellAcquisitionSnapshot> snapshot() const noexcept;

private:
    [[nodiscard]] float durationScale() const noexcept;

    std::optional<game::run::ContentId> spellId_;
    float elapsedSeconds_ {};
    bool bossSpell_ {};
};
}
