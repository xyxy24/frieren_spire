#include "presentation/viewmodel/SpellAcquisitionViewModel.hpp"

#include <algorithm>

namespace arcane::presentation::viewmodel
{
namespace
{
[[nodiscard]] float normalizedProgress(
    const float elapsed, const float start, const float duration) noexcept
{
    if (duration <= 0.0F) return 1.0F;
    return std::clamp((elapsed - start) / duration, 0.0F, 1.0F);
}
}

void SpellAcquisitionViewModel::start(const game::run::ContentId spellId) noexcept
{
    const auto content = makeContentSummaryViewModel(spellId);
    if (content.kind != ContentKind::Spell)
    {
        reset();
        return;
    }
    spellId_ = spellId;
    elapsedSeconds_ = 0.0F;
    bossSpell_ = content.bossSpell;
}

void SpellAcquisitionViewModel::reset() noexcept
{
    spellId_.reset();
    elapsedSeconds_ = 0.0F;
    bossSpell_ = false;
}

void SpellAcquisitionViewModel::update(
    const game::PlayerIntent& intent, const float deltaSeconds) noexcept
{
    if (!spellId_) return;
    elapsedSeconds_ += std::max(0.0F, deltaSeconds);
    if (elapsedSeconds_ >= SkipUnlockSeconds
        && elapsedSeconds_ < MinimumDisplaySeconds && intent.jumpPressed)
    {
        elapsedSeconds_ = MinimumDisplaySeconds;
        return;
    }
    if (elapsedSeconds_ >= MinimumDisplaySeconds
        && intent.menuConfirmPressed) reset();
}

bool SpellAcquisitionViewModel::active() const noexcept
{
    return spellId_.has_value();
}

std::optional<SpellAcquisitionSnapshot> SpellAcquisitionViewModel::snapshot() const noexcept
{
    if (!spellId_) return std::nullopt;
    return SpellAcquisitionSnapshot {
        makeContentDetailViewModel(*spellId_),
        elapsedSeconds_,
        elapsedSeconds_ * PlaybackRate,
        normalizedProgress(elapsedSeconds_, 0.0F, RegistrationSeconds),
        normalizedProgress(elapsedSeconds_, CirculationStartSeconds, CirculationSeconds),
        normalizedProgress(elapsedSeconds_, RegistrationSeconds, BurstSeconds),
        normalizedProgress(elapsedSeconds_, RegistrationSeconds + RevealDelaySeconds,
            RevealSeconds),
        elapsedSeconds_ >= SkipUnlockSeconds && elapsedSeconds_ < MinimumDisplaySeconds,
        elapsedSeconds_ >= MinimumDisplaySeconds,
        bossSpell_};
}
}
