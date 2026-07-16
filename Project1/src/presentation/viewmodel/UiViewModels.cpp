#include "presentation/viewmodel/UiViewModels.hpp"

#include "game/progression/ProgressionSystem.hpp"
#include "game/relics/RelicSystem.hpp"
#include "game/rewards/RewardSystem.hpp"
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
    const game::run::ContentId id, const bool selected, const std::uint8_t rank) noexcept
{
    if (const auto* spell = game::spells::findDefinition(id))
    {
        return {id, ContentKind::Spell, spell->name,
            spell->tier == game::spells::SpellTier::Boss, selected, rank};
    }
    if (const auto* relic = game::relics::findDefinition(id))
        return {id, ContentKind::Relic, relic->name, false, selected, 0U};
    return {id, ContentKind::Unknown, "UNKNOWN CONTENT", false, selected, 0U};
}

ContentDetailViewModel makeContentDetailViewModel(
    const game::run::ContentId id, const bool selected, const std::uint8_t rank)
{
    ContentDetailViewModel model;
    model.summary = makeContentSummaryViewModel(id, selected, rank);
    if (const auto* spell = game::spells::findDefinition(id))
    {
        model.description = spell->description;
        const float cooldownMultiplier = spell->tier == game::spells::SpellTier::Regular
            ? game::progression::masteryCooldownMultiplier(rank) : 1.0F;
        const float areaMultiplier = spell->tier == game::spells::SpellTier::Regular
            ? game::progression::masteryAreaMultiplier(rank) : 1.0F;
        model.spell = SpellDetailViewModel {
            spell->cooldownSeconds * cooldownMultiplier, toRangeKind(spell->shape),
            spell->range * areaMultiplier, spell->height * areaMultiplier};
        if (spell->tier == game::spells::SpellTier::Regular && rank > 0U)
        {
            if (rank == 1U)
                model.masteryDescription = "RANK I - BASE FORMULA";
            else
            {
                model.masteryDescription = rank >= 3U
                    ? "RANK III: +30% POWER, +16% AREA, -15% COOLDOWN. "
                    : "RANK II: +15% POWER, +8% AREA, -8% COOLDOWN.";
                if (rank >= 3U)
                    model.masteryDescription += game::progression::rankThreeSignature(id);
            }
        }
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
            model.regular[index] = makeContentSummaryViewModel(*player.equippedSpells[index],
                false, game::progression::spellRank(player, *player.equippedSpells[index]));
    if (player.equippedUltimateSpell)
        model.ultimate = makeContentSummaryViewModel(*player.equippedUltimateSpell);
    model.ultimateUnlocked = player.ultimateSpellUnlocked;
    return model;
}

RewardViewModel makeRewardViewModel(
    const std::array<game::run::ContentId, 3>& candidates,
    const game::run::PlayerProgress& player)
{
    RewardViewModel model;
    for (std::size_t index = 0U; index < candidates.size(); ++index)
    {
        const auto currentRank = game::progression::spellRank(player, candidates[index]);
        const std::uint8_t displayRank = currentRank > 0U
            ? static_cast<std::uint8_t>(std::min<int>(3, currentRank + 1))
            : std::uint8_t {1U};
        model.cards[index] = makeContentDetailViewModel(candidates[index], false, displayRank);
        if (model.cards[index].spell && !model.cards[index].summary.bossSpell)
            model.cards[index].spell->cooldownSeconds *=
                game::progression::regularCooldownMultiplier(player);
        for (const auto& synergy : game::rewards::spellSynergyHints(player, candidates[index]))
            if (const auto* owned = game::spells::findDefinition(synergy.ownedSpellId))
                model.cards[index].synergies.push_back(
                    {synergy.ownedSpellId, owned->name, synergy.description});
        model.cards[index].upgradeReward = currentRank > 0U;
    }
    model.showRerollHint = std::find(player.relics.begin(), player.relics.end(),
        game::relics::FirstClassBadgeId) != player.relics.end();
    return model;
}

BreakthroughViewModel makeBreakthroughViewModel(
    const game::run::PlayerProgress& player) noexcept
{
    BreakthroughViewModel model;
    const auto& definitions = game::progression::breakthroughDefinitions();
    for (std::size_t index = 0U; index < definitions.size(); ++index)
    {
        const auto rank = player.breakthroughRanks[index];
        model.cards[index] = {definitions[index].name,
            rank == 0U ? definitions[index].firstDescription
                       : definitions[index].secondDescription,
            rank};
    }
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
            id, section == SpellSection::Regular && selectedSpell == id,
            game::progression::spellRank(player, id)));
    model.bossSpells.reserve(player.learnedBossSpells.size());
    for (const auto id : player.learnedBossSpells)
        model.bossSpells.push_back(makeContentSummaryViewModel(
            id, section == SpellSection::Boss && selectedSpell == id));
    model.relics.reserve(player.relics.size());
    for (const auto id : player.relics)
        model.relics.push_back(makeContentSummaryViewModel(id, selectedRelic == id));

    const auto detailId = page == LoadoutPage::Relics ? selectedRelic : selectedSpell;
    if (detailId)
    {
        model.selectedDetail = makeContentDetailViewModel(*detailId, true,
            game::progression::spellRank(player, *detailId));
        if (model.selectedDetail->spell && !model.selectedDetail->summary.bossSpell)
            model.selectedDetail->spell->cooldownSeconds *=
                game::progression::regularCooldownMultiplier(player);
    }
    model.equipped = makeEquippedSlotsViewModel(player);
    return model;
}
}
