#pragma once

#include "game/combat/CombatContracts.hpp"
#include "game/economy/MerchantSystem.hpp"
#include "game/events/EventSystem.hpp"
#include "game/floors/FloorController.hpp"
#include "game/rewards/RewardSystem.hpp"

#include <optional>
#include <span>

namespace arcane::app
{
class RunController
{
public:
    explicit RunController(game::run::Seed seed, game::run::PlayerProgress player = {});
    [[nodiscard]] const game::run::RunContext& context() const noexcept;
    [[nodiscard]] const game::run::PlayerProgress& player() const noexcept;
    [[nodiscard]] game::run::RunPhase phase() const noexcept;
    [[nodiscard]] const game::run::FloorDescriptor& loadFloor(game::run::FloorType type,
        std::span<const game::run::ContentId> encounterPool);
    [[nodiscard]] bool resolveEncounter(const game::CombatResult& result,
        std::span<const game::run::ContentId> rewardPool);
    [[nodiscard]] bool completeNonCombatFloor();
    [[nodiscard]] const game::rewards::RewardOffer& rewardOffer() const;
    [[nodiscard]] bool openReward();
    [[nodiscard]] bool chooseReward(game::run::ContentId choice);
    [[nodiscard]] bool claimFallbackReward();
    [[nodiscard]] game::economy::PurchaseResult purchaseMerchantItem(
        std::vector<game::economy::StockItem>& stock, game::run::ContentId itemId);
    [[nodiscard]] game::events::EventResult chooseEvent(game::events::EventTransaction& transaction,
        std::span<const game::events::EventChoice> choices, game::run::ContentId choiceId);
    [[nodiscard]] bool equip(std::size_t slot, game::run::ContentId spell);
    [[nodiscard]] bool useStairs();
    [[nodiscard]] game::run::FloorResult floorResult() const noexcept;

private:
    static int recoverHalfMissingHp(int currentHp, int maxHp) noexcept;
    game::run::RunContext context_;
    game::run::PlayerProgress player_;
    game::run::RunPhase phase_ {game::run::RunPhase::FloorLoading};
    game::floors::FloorController floor_;
    std::optional<game::rewards::RewardOffer> reward_;
    std::optional<game::run::FloorType> currentFloorType_;
    std::optional<game::run::ContentId> activeEncounterId_;
    bool rewardApplied_ {};
};
}
