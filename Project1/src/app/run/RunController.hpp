#pragma once

#include "game/combat/CombatContracts.hpp"
#include "game/economy/MerchantSystem.hpp"
#include "game/events/EventSystem.hpp"
#include "game/floors/FloorController.hpp"
#include "game/progression/ProgressionSystem.hpp"
#include "game/rewards/RewardSystem.hpp"

#include <optional>
#include <span>

namespace arcane::app
{
struct RunStartPosition
{
    std::uint32_t floorIndex {};
    std::uint32_t act {1U};
    std::uint32_t bossesDefeated {};
};

class RunController
{
public:
    explicit RunController(game::run::Seed seed, game::run::PlayerProgress player = {},
        RunStartPosition start = {});
    [[nodiscard]] const game::run::RunContext& context() const noexcept;
    [[nodiscard]] const game::run::PlayerProgress& player() const noexcept;
    [[nodiscard]] game::run::RunPhase phase() const noexcept;
    [[nodiscard]] const game::run::FloorDescriptor& loadFloor(game::run::FloorType type,
        std::span<const game::run::ContentId> encounterPool);
    [[nodiscard]] bool resolveEncounter(const game::CombatResult& result,
        std::span<const game::run::ContentId> rewardPool,
        std::span<const game::run::ContentId> damageRewardPool = {},
        std::span<const game::run::ContentId> controlRewardPool = {},
        std::span<const game::run::ContentId> actRewardPool = {});
    [[nodiscard]] bool completeNonCombatFloor();
    [[nodiscard]] const game::rewards::RewardOffer& rewardOffer() const;
    [[nodiscard]] bool openReward();
    [[nodiscard]] bool chooseReward(game::run::ContentId choice);
    [[nodiscard]] bool chooseBreakthrough(game::progression::BreakthroughType choice);
    [[nodiscard]] bool claimFallbackReward();
    [[nodiscard]] bool rerollRegularReward(std::span<const game::run::ContentId> damagePool,
        std::span<const game::run::ContentId> controlPool,
        std::span<const game::run::ContentId> fullPool,
        std::span<const game::run::ContentId> actPool = {});
    [[nodiscard]] game::economy::PurchaseResult purchaseMerchantItem(
        std::vector<game::economy::StockItem>& stock, game::run::ContentId itemId);
    [[nodiscard]] game::events::EventResult chooseEvent(game::events::EventTransaction& transaction,
        std::span<const game::events::EventChoice> choices, game::run::ContentId choiceId);
    [[nodiscard]] bool equip(std::size_t slot, game::run::ContentId spell);
    [[nodiscard]] bool equipUltimate(game::run::ContentId spell);
    [[nodiscard]] bool useStairs();
    [[nodiscard]] game::run::FloorResult floorResult() const noexcept;
    [[nodiscard]] bool eventTriggered(std::size_t eventIndex) const noexcept;
    void markEventTriggered(std::size_t eventIndex);

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
