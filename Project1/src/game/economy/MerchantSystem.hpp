#pragma once

#include "game/run/RunTypes.hpp"

#include <span>
#include <vector>

namespace arcane::game::economy
{
enum class ItemKind : std::uint8_t { Spell, Relic };

struct StockItem
{
    run::ContentId id {};
    ItemKind kind {ItemKind::Spell};
    int price {};
    bool sold {};
};

struct CatalogItem
{
    run::ContentId id {};
    ItemKind kind {ItemKind::Spell};
    int price {};
};

enum class PurchaseResult : std::uint8_t { Success, NotFound, AlreadySold, InvalidPrice, InsufficientGold, AlreadyOwned };

[[nodiscard]] PurchaseResult purchase(run::PlayerProgress& player, std::vector<StockItem>& stock,
    run::ContentId itemId);
[[nodiscard]] std::vector<StockItem> generateStock(std::span<const CatalogItem> catalog,
    const run::PlayerProgress& player, run::Seed seed, std::size_t itemCount = 3U);
}
