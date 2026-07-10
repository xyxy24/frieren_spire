#pragma once

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
    [[nodiscard]] bool resolveEncounter(bool victory, int goldAward,
        std::span<const game::run::ContentId> rewardPool);
    [[nodiscard]] const game::rewards::RewardOffer& rewardOffer() const;
    [[nodiscard]] bool chooseReward(game::run::ContentId choice);
    [[nodiscard]] bool equip(std::size_t slot, game::run::ContentId spell);
    [[nodiscard]] bool finishLoadout();
    [[nodiscard]] bool useStairs();
    void setCurrentHpForFlow(int hp);

private:
    static int recoverHalfMissingHp(int currentHp, int maxHp) noexcept;
    game::run::RunContext context_;
    game::run::PlayerProgress player_;
    game::run::RunPhase phase_ {game::run::RunPhase::FloorLoading};
    game::floors::FloorController floor_;
    std::optional<game::rewards::RewardOffer> reward_;
    bool rewardApplied_ {};
    bool currentFloorWasBoss_ {};
};
}
