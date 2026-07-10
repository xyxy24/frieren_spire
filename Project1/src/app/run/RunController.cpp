#include "app/run/RunController.hpp"

#include "game/run/DeterministicRng.hpp"

#include <algorithm>
#include <stdexcept>

namespace arcane::app
{
RunController::RunController(const game::run::Seed seed, game::run::PlayerProgress player)
    : context_ {seed, game::run::deriveFloorSeed(seed, 0U), 0U, 1U, 0U}, player_(std::move(player))
{
    if (player_.maxHp <= 0 || player_.currentHp < 0 || player_.currentHp > player_.maxHp || player_.gold < 0)
        throw std::invalid_argument("invalid initial player progress");
}

const game::run::RunContext& RunController::context() const noexcept { return context_; }
const game::run::PlayerProgress& RunController::player() const noexcept { return player_; }
game::run::RunPhase RunController::phase() const noexcept { return phase_; }

const game::run::FloorDescriptor& RunController::loadFloor(const game::run::FloorType type,
    const std::span<const game::run::ContentId> encounterPool)
{
    if (phase_ != game::run::RunPhase::FloorLoading) throw std::logic_error("run is not loading a floor");
    const auto& descriptor = floor_.load(context_, type, encounterPool);
    currentFloorWasBoss_ = type == game::run::FloorType::Boss;
    phase_ = game::run::RunPhase::InEncounter;
    return descriptor;
}

bool RunController::resolveEncounter(const bool victory, const int goldAward,
    const std::span<const game::run::ContentId> rewardPool)
{
    if (phase_ != game::run::RunPhase::InEncounter || goldAward < 0) return false;
    if (!victory)
    {
        player_.currentHp = 0;
        phase_ = game::run::RunPhase::Defeat;
        return true;
    }

    floor_.markEncounterComplete();
    if (currentFloorWasBoss_) ++context_.bossesDefeated;
    player_.gold += goldAward;
    const auto seed = game::run::deriveStreamSeed(context_.floorSeed, game::run::RandomStream::Reward);
    reward_ = game::rewards::generateOffer(rewardPool, player_.learnedSpells, seed);
    rewardApplied_ = false;
    phase_ = game::run::RunPhase::Reward;
    return true;
}

const game::rewards::RewardOffer& RunController::rewardOffer() const
{
    if (!reward_) throw std::logic_error("no reward is pending");
    return *reward_;
}

bool RunController::chooseReward(const game::run::ContentId choice)
{
    if (phase_ != game::run::RunPhase::Reward || rewardApplied_) return false;
    if (!game::rewards::applySpellChoice(player_, *reward_, choice)) return false;
    rewardApplied_ = true;
    phase_ = game::run::RunPhase::Loadout;
    return true;
}

bool RunController::claimFallbackReward()
{
    if (phase_ != game::run::RunPhase::Reward || rewardApplied_ || !reward_
        || reward_->kind != game::rewards::RewardKind::GoldFallback
        || reward_->fallbackGold > std::numeric_limits<int>::max() - player_.gold) return false;
    player_.gold += reward_->fallbackGold;
    rewardApplied_ = true;
    phase_ = game::run::RunPhase::Loadout;
    return true;
}

bool RunController::equip(const std::size_t slot, const game::run::ContentId spell)
{
    if (phase_ != game::run::RunPhase::Loadout || slot >= player_.equippedSpells.size()
        || std::find(player_.learnedSpells.begin(), player_.learnedSpells.end(), spell)
            == player_.learnedSpells.end()) return false;
    for (auto& equipped : player_.equippedSpells) if (equipped == spell) equipped.reset();
    player_.equippedSpells[slot] = spell;
    return true;
}

bool RunController::finishLoadout()
{
    if (phase_ != game::run::RunPhase::Loadout) return false;
    phase_ = game::run::RunPhase::FloorComplete;
    return true;
}

bool RunController::useStairs()
{
    if (phase_ != game::run::RunPhase::FloorComplete || !floor_.canUseStairs()) return false;
    player_.currentHp = recoverHalfMissingHp(player_.currentHp, player_.maxHp);
    static_cast<void>(floor_.unload());
    if (context_.bossesDefeated >= 3U)
    {
        reward_.reset();
        phase_ = game::run::RunPhase::Victory;
        return true;
    }
    if (currentFloorWasBoss_) context_.act = context_.bossesDefeated + 1U;
    ++context_.floorIndex;
    context_.floorSeed = game::run::deriveFloorSeed(context_.runSeed, context_.floorIndex);
    reward_.reset();
    currentFloorWasBoss_ = false;
    phase_ = game::run::RunPhase::FloorLoading;
    return true;
}

<<<<<<< Updated upstream
void RunController::setCurrentHpForFlow(const int hp)
{
    if (hp < 0 || hp > player_.maxHp) throw std::out_of_range("HP is outside player bounds");
    player_.currentHp = hp;
=======
game::run::FloorResult RunController::floorResult() const noexcept
{
    const bool rewardComplete = phase_ == game::run::RunPhase::FloorComplete
        || phase_ == game::run::RunPhase::Victory;
    return floor_.result(rewardComplete);
>>>>>>> Stashed changes
}

int RunController::recoverHalfMissingHp(const int currentHp, const int maxHp) noexcept
{
    const int missing = maxHp - currentHp;
    return std::min(maxHp, currentHp + ((missing + 1) / 2));
}
}
