#include "presentation/viewmodel/SpellAcquisitionViewModel.hpp"

#include <algorithm>
#include <cmath>

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
    if (elapsedSeconds_ >= MinimumDisplaySeconds * durationScale()
        && intent.menuConfirmPressed) reset();
}

bool SpellAcquisitionViewModel::active() const noexcept
{
    return spellId_.has_value();
}

std::optional<SpellAcquisitionSnapshot> SpellAcquisitionViewModel::snapshot() const noexcept
{
    if (!spellId_) return std::nullopt;
    const float scale = durationScale();
    const float chargeEnd = ChargeSeconds * scale;
    const float unlockEnd = chargeEnd + UnlockSeconds * scale;
    const float flashCenter = chargeEnd + 0.10F * scale;
    const float flashHalfWidth = 0.16F * scale;
    const float flashRatio = std::clamp(
        1.0F - std::abs(elapsedSeconds_ - flashCenter) / flashHalfWidth, 0.0F, 1.0F);
    return SpellAcquisitionSnapshot {
        makeContentDetailViewModel(*spellId_),
        elapsedSeconds_,
        normalizedProgress(elapsedSeconds_, 0.0F, chargeEnd),
        normalizedProgress(elapsedSeconds_, chargeEnd, UnlockSeconds * scale),
        normalizedProgress(elapsedSeconds_, unlockEnd, RevealSeconds * scale),
        flashRatio,
        elapsedSeconds_ >= MinimumDisplaySeconds * scale,
        bossSpell_};
}

float SpellAcquisitionViewModel::durationScale() const noexcept
{
    return bossSpell_ ? BossDurationScale : 1.0F;
}
}
