#include "presentation/viewmodel/UiViewModels.hpp"

#include "game/relics/RelicSystem.hpp"
#include "game/spells/SpellSystem.hpp"

#include <algorithm>

namespace arcane::presentation::viewmodel
{
namespace
{
SpellRangeKind toRangeKind(const game::spells::SpellShape shape) noexcept
{
    using game::spells::SpellShape;
    switch (shape)
    {
    case SpellShape::Self: return SpellRangeKind::Self;
    case SpellShape::ForwardBox: return SpellRangeKind::ForwardBox;
    case SpellShape::SelfArea: return SpellRangeKind::SelfArea;
    case SpellShape::TargetArea: return SpellRangeKind::TargetArea;
    case SpellShape::Summon: return SpellRangeKind::Summon;
    }
    return SpellRangeKind::Self;
}
}

ContentSummaryViewModel makeContentSummaryViewModel(
    const game::run::ContentId id, const bool selected) noexcept
{
    if (const auto* spell = game::spells::findDefinition(id))
    {
        return {id, ContentKind::Spell, spell->name,
            spell->tier == game::spells::SpellTier::Boss, selected};
    }
    if (const auto* relic = game::relics::findDefinition(id))
        return {id, ContentKind::Relic, relic->name, false, selected};
    return {id, ContentKind::Unknown, "UNKNOWN CONTENT", false, selected};
}

ContentDetailViewModel makeContentDetailViewModel(
    const game::run::ContentId id, const bool selected) noexcept
{
    ContentDetailViewModel model;
    model.summary = makeContentSummaryViewModel(id, selected);
    if (const auto* spell = game::spells::findDefinition(id))
    {
        model.description = spell->description;
        model.spell = SpellDetailViewModel {
            spell->cooldownSeconds, toRangeKind(spell->shape), spell->range, spell->height};
    }
    else if (const auto* relic = game::relics::findDefinition(id))
        model.description = relic->description;
    return model;
}

EquippedSlotsViewModel makeEquippedSlotsViewModel(
    const game::run::PlayerProgress& player) noexcept
{
    EquippedSlotsViewModel model;
    for (std::size_t index = 0U; index < player.equippedSpells.size(); ++index)
        if (player.equippedSpells[index])
            model.regular[index] = makeContentSummaryViewModel(*player.equippedSpells[index]);
    if (player.equippedUltimateSpell)
        model.ultimate = makeContentSummaryViewModel(*player.equippedUltimateSpell);
    model.ultimateUnlocked = player.ultimateSpellUnlocked;
    return model;
}

RewardViewModel makeRewardViewModel(
    const std::array<game::run::ContentId, 3>& candidates,
    const game::run::PlayerProgress& player) noexcept
{
    RewardViewModel model;
    for (std::size_t index = 0U; index < candidates.size(); ++index)
        model.cards[index] = makeContentDetailViewModel(candidates[index]);
    model.showRerollHint = std::find(player.relics.begin(), player.relics.end(),
        game::relics::FirstClassBadgeId) != player.relics.end();
    return model;
}

MerchantViewModel makeMerchantViewModel(
    const std::span<const game::economy::StockItem> stock,
    const std::optional<game::run::ContentId> selectedId)
{
    MerchantViewModel model;
    model.items.reserve(stock.size());
    for (const auto& item : stock)
    {
        const bool selected = selectedId == item.id;
        model.items.push_back({makeContentSummaryViewModel(item.id, selected), item.price});
        if (selected) model.selectedDetail = makeContentDetailViewModel(item.id, true);
    }
    return model;
}

LoadoutSnapshot makeLoadoutSnapshot(
    const game::run::PlayerProgress& player, const LoadoutPage page,
    const SpellSection section, const std::optional<game::run::ContentId> selectedSpell,
    const std::optional<game::run::ContentId> selectedRelic)
{
    LoadoutSnapshot model;
    model.page = page;
    model.spellSection = section;
    model.regularSpells.reserve(player.learnedSpells.size());
    for (const auto id : player.learnedSpells)
        model.regularSpells.push_back(makeContentSummaryViewModel(
            id, section == SpellSection::Regular && selectedSpell == id));
    model.bossSpells.reserve(player.learnedBossSpells.size());
    for (const auto id : player.learnedBossSpells)
        model.bossSpells.push_back(makeContentSummaryViewModel(
            id, section == SpellSection::Boss && selectedSpell == id));
    model.relics.reserve(player.relics.size());
    for (const auto id : player.relics)
        model.relics.push_back(makeContentSummaryViewModel(id, selectedRelic == id));

    const auto detailId = page == LoadoutPage::Relics ? selectedRelic : selectedSpell;
    if (detailId) model.selectedDetail = makeContentDetailViewModel(*detailId, true);
    model.equipped = makeEquippedSlotsViewModel(player);
    return model;
}
}
