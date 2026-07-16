#include "game/economy/MerchantSystem.hpp"
#include "game/progression/ProgressionSystem.hpp"
#include "game/run/DeterministicRng.hpp"

#include <algorithm>
#include <stdexcept>

namespace arcane::game::economy
{
PurchaseResult purchase(run::PlayerProgress& player, std::vector<StockItem>& stock, const run::ContentId itemId)
{
    const auto item = std::find_if(stock.begin(), stock.end(), [itemId](const StockItem& value) {
        return value.id == itemId;
    });
    if (item == stock.end()) return PurchaseResult::NotFound;
    if (item->sold) return PurchaseResult::AlreadySold;
    if (item->price < 0) return PurchaseResult::InvalidPrice;
    if (player.gold < item->price) return PurchaseResult::InsufficientGold;

    auto& inventory = item->kind == ItemKind::Spell ? player.learnedSpells : player.relics;
    if (std::find(inventory.begin(), inventory.end(), item->id) != inventory.end())
    {
        return PurchaseResult::AlreadyOwned;
    }
    player.gold -= item->price;
    inventory.push_back(item->id);
    if (item->kind == ItemKind::Spell)
        progression::registerLearnedSpell(player, item->id);
    item->sold = true;
    return PurchaseResult::Success;
}

std::vector<StockItem> generateStock(const std::span<const CatalogItem> catalog,
    const run::PlayerProgress& player, const run::Seed seed, const std::size_t itemCount)
{
    std::vector<CatalogItem> eligible;
    for (const auto& item : catalog)
    {
        if (item.id == 0U || item.price < 0) throw std::invalid_argument("invalid merchant catalog item");
        const auto& owned = item.kind == ItemKind::Spell ? player.learnedSpells : player.relics;
        if (std::find(owned.begin(), owned.end(), item.id) == owned.end()
            && std::find_if(eligible.begin(), eligible.end(), [&item](const CatalogItem& candidate) {
                return candidate.id == item.id;
            }) == eligible.end()) eligible.push_back(item);
    }
    if (eligible.size() < itemCount) throw std::invalid_argument("insufficient eligible merchant stock");
    run::DeterministicRng rng(seed);
    for (std::size_t index = eligible.size() - 1U; index > 0U; --index)
    {
        const auto swapIndex = rng.index(static_cast<std::uint32_t>(index + 1U));
        std::swap(eligible[index], eligible[swapIndex]);
    }
    std::vector<StockItem> stock;
    stock.reserve(itemCount);
    for (std::size_t index = 0U; index < itemCount; ++index)
        stock.push_back({eligible[index].id, eligible[index].kind, eligible[index].price, false});
    return stock;
}
}
