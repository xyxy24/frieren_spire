#include "presentation/viewmodel/LoadoutViewModel.hpp"

namespace arcane::presentation::viewmodel
{
namespace
{
void movePrevious(std::size_t& index, const std::size_t count) noexcept
{
    if (count == 0U) return;
    index = index == 0U ? count - 1U : index - 1U;
}

void moveNext(std::size_t& index, const std::size_t count) noexcept
{
    if (count != 0U) index = (index + 1U) % count;
}
}

bool LoadoutViewModel::handleInput(
    const game::PlayerIntent& intent, app::TowerSession& model)
{
    const auto& player = model.run().player();
    if (intent.toggleLoadoutPressed)
    {
        open_ = !open_;
        if (open_) page_ = LoadoutPage::Spells;
        clampSelections(player);
        return true;
    }
    if (!open_) return false;

    if (intent.menuPagePreviousPressed || intent.menuPageNextPressed)
    {
        page_ = page_ == LoadoutPage::Spells ? LoadoutPage::Relics : LoadoutPage::Spells;
        return true;
    }
    if (page_ == LoadoutPage::Spells) handleSpellPage(intent, model);
    else handleRelicPage(intent, player);
    return true;
}

void LoadoutViewModel::reset() noexcept
{
    *this = LoadoutViewModel {};
}

void LoadoutViewModel::selectNewestRegular(
    const game::run::PlayerProgress& player) noexcept
{
    if (!player.learnedSpells.empty()) selectedRegularIndex_ = player.learnedSpells.size() - 1U;
    spellSection_ = SpellSection::Regular;
}

void LoadoutViewModel::selectNewestBoss(
    const game::run::PlayerProgress& player) noexcept
{
    if (!player.learnedBossSpells.empty()) selectedBossIndex_ = player.learnedBossSpells.size() - 1U;
    spellSection_ = SpellSection::Boss;
}

bool LoadoutViewModel::open() const noexcept { return open_; }
LoadoutPage LoadoutViewModel::page() const noexcept { return page_; }
SpellSection LoadoutViewModel::spellSection() const noexcept { return spellSection_; }

std::optional<game::run::ContentId> LoadoutViewModel::selectedSpell(
    const game::run::PlayerProgress& player) const noexcept
{
    if (spellSection_ == SpellSection::Boss)
    {
        if (player.learnedBossSpells.empty() || selectedBossIndex_ >= player.learnedBossSpells.size())
            return std::nullopt;
        return player.learnedBossSpells[selectedBossIndex_];
    }
    if (player.learnedSpells.empty() || selectedRegularIndex_ >= player.learnedSpells.size())
        return std::nullopt;
    return player.learnedSpells[selectedRegularIndex_];
}

std::optional<game::run::ContentId> LoadoutViewModel::selectedRelic(
    const game::run::PlayerProgress& player) const noexcept
{
    if (player.relics.empty() || selectedRelicIndex_ >= player.relics.size())
        return std::nullopt;
    return player.relics[selectedRelicIndex_];
}

LoadoutSnapshot LoadoutViewModel::snapshot(const game::run::PlayerProgress& player) const
{
    return makeLoadoutSnapshot(
        player, page_, spellSection_, selectedSpell(player), selectedRelic(player));
}

void LoadoutViewModel::clampSelections(const game::run::PlayerProgress& player) noexcept
{
    if (!player.learnedSpells.empty() && selectedRegularIndex_ >= player.learnedSpells.size())
        selectedRegularIndex_ = player.learnedSpells.size() - 1U;
    if (!player.learnedBossSpells.empty() && selectedBossIndex_ >= player.learnedBossSpells.size())
        selectedBossIndex_ = player.learnedBossSpells.size() - 1U;
    if (!player.relics.empty() && selectedRelicIndex_ >= player.relics.size())
        selectedRelicIndex_ = player.relics.size() - 1U;
}

void LoadoutViewModel::handleSpellPage(
    const game::PlayerIntent& intent, app::TowerSession& model)
{
    if (intent.menuUpPressed || intent.menuDownPressed)
    {
        spellSection_ = spellSection_ == SpellSection::Regular
            ? SpellSection::Boss : SpellSection::Regular;
        return;
    }

    const auto& player = model.run().player();
    if (spellSection_ == SpellSection::Boss)
    {
        const auto count = player.learnedBossSpells.size();
        if (intent.menuPreviousPressed) movePrevious(selectedBossIndex_, count);
        if (intent.menuNextPressed) moveNext(selectedBossIndex_, count);
        if (intent.ultimateSpellPressed && count != 0U)
            static_cast<void>(model.equipUltimateSpell(player.learnedBossSpells[selectedBossIndex_]));
        return;
    }

    const auto count = player.learnedSpells.size();
    if (intent.menuPreviousPressed) movePrevious(selectedRegularIndex_, count);
    if (intent.menuNextPressed) moveNext(selectedRegularIndex_, count);
    if (count == 0U) return;
    for (std::size_t slot = 0U; slot < intent.spellPressed.size(); ++slot)
        if (intent.spellPressed[slot])
            static_cast<void>(model.equipRegularSpell(slot, player.learnedSpells[selectedRegularIndex_]));
}

void LoadoutViewModel::handleRelicPage(
    const game::PlayerIntent& intent, const game::run::PlayerProgress& player) noexcept
{
    if (intent.menuPreviousPressed) movePrevious(selectedRelicIndex_, player.relics.size());
    if (intent.menuNextPressed) moveNext(selectedRelicIndex_, player.relics.size());
}
}
