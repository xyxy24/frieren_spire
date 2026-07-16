#pragma once

#include "game/economy/MerchantSystem.hpp"
#include "game/run/RunTypes.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace arcane::presentation::viewmodel
{
enum class ContentKind : std::uint8_t { Unknown, Spell, Relic };
enum class SpellRangeKind : std::uint8_t { Self, ForwardBox, SelfArea, TargetArea, Summon };
enum class LoadoutPage : std::uint8_t { Spells, Relics };
enum class SpellSection : std::uint8_t { Regular, Boss };

struct SpellDetailViewModel
{
    float cooldownSeconds {};
    SpellRangeKind rangeKind {SpellRangeKind::Self};
    float range {};
    float height {};
};

struct ContentSummaryViewModel
{
    game::run::ContentId id {};
    ContentKind kind {ContentKind::Unknown};
    std::string_view name {"UNKNOWN CONTENT"};
    bool bossSpell {};
    bool selected {};
    std::uint8_t rank {};
};

struct ContentDetailViewModel
{
    struct SynergyHint
    {
        game::run::ContentId ownedSpellId {};
        std::string_view ownedSpellName;
        std::string_view description;
    };

    ContentSummaryViewModel summary;
    std::string_view description {"NO DESCRIPTION AVAILABLE"};
    std::optional<SpellDetailViewModel> spell;
    std::string masteryDescription;
    std::vector<SynergyHint> synergies;
    bool upgradeReward {};
};

struct EquippedSlotsViewModel
{
    std::array<std::optional<ContentSummaryViewModel>, 3> regular;
    std::optional<ContentSummaryViewModel> ultimate;
    bool ultimateUnlocked {};
};

struct RewardViewModel
{
    std::array<ContentDetailViewModel, 3> cards;
    bool showRerollHint {};
};

struct BreakthroughCardViewModel
{
    std::string_view name;
    std::string_view description;
    std::uint8_t currentRank {};
};

struct BreakthroughViewModel
{
    std::array<BreakthroughCardViewModel, 3> cards;
};

struct MerchantItemViewModel
{
    ContentSummaryViewModel content;
    int price {};
};

struct MerchantViewModel
{
    std::vector<MerchantItemViewModel> items;
    std::optional<ContentDetailViewModel> selectedDetail;
};

struct LoadoutSnapshot
{
    LoadoutPage page {LoadoutPage::Spells};
    SpellSection spellSection {SpellSection::Regular};
    std::vector<ContentSummaryViewModel> regularSpells;
    std::vector<ContentSummaryViewModel> bossSpells;
    std::vector<ContentSummaryViewModel> relics;
    std::optional<ContentDetailViewModel> selectedDetail;
    EquippedSlotsViewModel equipped;
};

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
