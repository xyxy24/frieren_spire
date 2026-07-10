#include "game/economy/MerchantSystem.hpp"

#include <algorithm>

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
    item->sold = true;
    return PurchaseResult::Success;
}
}
