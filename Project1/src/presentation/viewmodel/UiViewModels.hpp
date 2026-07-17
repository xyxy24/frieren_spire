#pragma once

#include "game/economy/MerchantSystem.hpp"
#include "game/run/RunTypes.hpp"
#include "common/ui/UiStates.hpp"

#include <span>

namespace arcane::presentation::viewmodel
{
using ContentKind = common::ui::ContentKind;
using SpellRangeKind = common::ui::SpellRangeKind;
using LoadoutPage = common::ui::LoadoutPage;
using SpellSection = common::ui::SpellSection;
using SpellDetailViewModel = common::ui::SpellDetailState;
using ContentSummaryViewModel = common::ui::ContentSummaryState;
using ContentDetailViewModel = common::ui::ContentDetailState;
using EquippedSlotsViewModel = common::ui::EquippedSlotsState;
using RewardViewModel = common::ui::RewardState;
using BreakthroughCardViewModel = common::ui::BreakthroughCardState;
using BreakthroughViewModel = common::ui::BreakthroughState;
using MerchantItemViewModel = common::ui::MerchantItemState;
using MerchantViewModel = common::ui::MerchantState;
using LoadoutSnapshot = common::ui::LoadoutState;

[[nodiscard]] ContentSummaryViewModel makeContentSummaryViewModel(
    game::run::ContentId id, bool selected = false, std::uint8_t rank = 0U) noexcept;
[[nodiscard]] ContentDetailViewModel makeContentDetailViewModel(
    game::run::ContentId id, bool selected = false, std::uint8_t rank = 0U);
[[nodiscard]] EquippedSlotsViewModel makeEquippedSlotsViewModel(
    const game::run::PlayerProgress& player) noexcept;
[[nodiscard]] RewardViewModel makeRewardViewModel(
    const std::array<game::run::ContentId, 3>& candidates,
    const game::run::PlayerProgress& player);
[[nodiscard]] BreakthroughViewModel makeBreakthroughViewModel(
    const game::run::PlayerProgress& player) noexcept;
[[nodiscard]] MerchantViewModel makeMerchantViewModel(
    std::span<const game::economy::StockItem> stock,
    std::optional<game::run::ContentId> selectedId);
[[nodiscard]] LoadoutSnapshot makeLoadoutSnapshot(
    const game::run::PlayerProgress& player, LoadoutPage page, SpellSection section,
    std::optional<game::run::ContentId> selectedSpell,
    std::optional<game::run::ContentId> selectedRelic);
}
