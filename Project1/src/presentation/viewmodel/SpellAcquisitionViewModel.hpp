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
    float referenceElapsedSeconds {};
    float registrationProgress {};
    float circulationProgress {};
    float burstProgress {};
    float revealProgress {};
    bool canSkip {};
    bool canDismiss {};
    bool bossSpell {};
};

class SpellAcquisitionViewModel
{
public:
    // Preserve every reference stage while compressing the roughly 17-second
    // classroom-study sequence to a pace suitable for a repeated roguelike reward.
    static constexpr float PlaybackRate = 3.0F;
    static constexpr float RegistrationSeconds = (713.0F / 60.0F) / PlaybackRate;
    static constexpr float CirculationStartSeconds = (384.0F / 60.0F) / PlaybackRate;
    static constexpr float CirculationSeconds = (329.0F / 60.0F) / PlaybackRate;
    static constexpr float BurstSeconds = (190.0F / 60.0F) / PlaybackRate;
    static constexpr float RevealDelaySeconds = (125.0F / 60.0F) / PlaybackRate;
    static constexpr float RevealSeconds = 3.0F / PlaybackRate;
    static constexpr float MinimumDisplaySeconds = RegistrationSeconds
        + RevealDelaySeconds + RevealSeconds;
    static constexpr float SkipUnlockSeconds = 0.35F;

    void start(game::run::ContentId spellId, std::uint8_t rank = 1U) noexcept;
    void reset() noexcept;
    void update(const game::PlayerIntent& intent, float deltaSeconds) noexcept;

    [[nodiscard]] bool active() const noexcept;
    [[nodiscard]] std::optional<SpellAcquisitionSnapshot> snapshot() const noexcept;

private:
    std::optional<game::run::ContentId> spellId_;
    float elapsedSeconds_ {};
    bool bossSpell_ {};
    std::uint8_t rank_ {1U};
};
}
