#pragma once

#include "app/run/TowerSession.hpp"
#include "presentation/viewmodel/UiViewModels.hpp"

#include <cstddef>
#include <optional>

namespace arcane::presentation::viewmodel
{
class LoadoutViewModel
{
public:
    // Returns true while the overlay owns the input for this frame.
    [[nodiscard]] bool handleInput(const game::PlayerIntent& intent, app::TowerSession& model);

    void reset() noexcept;
    void selectNewestRegular(const game::run::PlayerProgress& player) noexcept;
    void selectNewestBoss(const game::run::PlayerProgress& player) noexcept;

    [[nodiscard]] bool open() const noexcept;
    [[nodiscard]] LoadoutPage page() const noexcept;
    [[nodiscard]] SpellSection spellSection() const noexcept;
    [[nodiscard]] std::optional<game::run::ContentId> selectedSpell(
        const game::run::PlayerProgress& player) const noexcept;
    [[nodiscard]] std::optional<game::run::ContentId> selectedRelic(
        const game::run::PlayerProgress& player) const noexcept;
    [[nodiscard]] LoadoutSnapshot snapshot(
        const game::run::PlayerProgress& player) const;

private:
    void clampSelections(const game::run::PlayerProgress& player) noexcept;
    void handleSpellPage(const game::PlayerIntent& intent, app::TowerSession& model);
    void handleRelicPage(const game::PlayerIntent& intent,
        const game::run::PlayerProgress& player) noexcept;

    bool open_ {};
    LoadoutPage page_ {LoadoutPage::Spells};
    SpellSection spellSection_ {SpellSection::Regular};
    std::size_t selectedRegularIndex_ {};
    std::size_t selectedBossIndex_ {};
    std::size_t selectedRelicIndex_ {};
};
}
