#include "app/run/RunController.hpp"

#include "game/run/DeterministicRng.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

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
    currentFloorType_ = type;
    activeEncounterId_ = descriptor.encounterIds.empty()
        ? std::nullopt
        : std::optional<game::run::ContentId> {descriptor.encounterIds.front()};
    phase_ = game::run::RunPhase::InEncounter;
    return descriptor;
}

bool RunController::resolveEncounter(const game::CombatResult& result,
    const std::span<const game::run::ContentId> rewardPool)
{
    const bool isCombatFloor = currentFloorType_ == game::run::FloorType::Combat
        || currentFloorType_ == game::run::FloorType::Boss;
    if (phase_ != game::run::RunPhase::InEncounter || !isCombatFloor || !activeEncounterId_
        || result.encounterId != *activeEncounterId_ || result.goldAwarded < 0
        || result.playerHealthRemaining < 0 || result.playerHealthRemaining > player_.maxHp)
    {
        return false;
    }

    if (result.outcome == game::CombatOutcome::Defeat)
    {
        if (result.playerHealthRemaining != 0) return false;
        player_.currentHp = 0;
        activeEncounterId_.reset();
        phase_ = game::run::RunPhase::Defeat;
        return true;
    }

    if (result.playerHealthRemaining == 0
        || result.goldAwarded > std::numeric_limits<int>::max() - player_.gold)
    {
        return false;
    }

    const auto seed = game::run::deriveStreamSeed(context_.floorSeed, game::run::RandomStream::Reward);
    const game::rewards::RewardOffer offer = game::rewards::generateOffer(
        rewardPool, player_.learnedSpells, seed);

    floor_.markEncounterComplete();
    if (currentFloorType_ == game::run::FloorType::Boss) ++context_.bossesDefeated;
    player_.currentHp = result.playerHealthRemaining;
    player_.gold += result.goldAwarded;
    reward_ = offer;
    activeEncounterId_.reset();
    rewardApplied_ = false;
    phase_ = game::run::RunPhase::Reward;
    return true;
}

bool RunController::completeNonCombatFloor()
{
    const bool isNonCombatFloor = currentFloorType_ == game::run::FloorType::Event
        || currentFloorType_ == game::run::FloorType::Merchant;
    if (phase_ != game::run::RunPhase::InEncounter || !isNonCombatFloor) return false;

    floor_.markEncounterComplete();
    phase_ = game::run::RunPhase::FloorComplete;
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
    const bool wasBossFloor = currentFloorType_ == game::run::FloorType::Boss;
    player_.currentHp = recoverHalfMissingHp(player_.currentHp, player_.maxHp);
    static_cast<void>(floor_.unload());
    currentFloorType_.reset();
    activeEncounterId_.reset();
    if (context_.bossesDefeated >= 3U)
    {
        reward_.reset();
        phase_ = game::run::RunPhase::Victory;
        return true;
    }
    if (wasBossFloor) context_.act = context_.bossesDefeated + 1U;
    ++context_.floorIndex;
    context_.floorSeed = game::run::deriveFloorSeed(context_.runSeed, context_.floorIndex);
    reward_.reset();
    rewardApplied_ = false;
    phase_ = game::run::RunPhase::FloorLoading;
    return true;
}

int RunController::recoverHalfMissingHp(const int currentHp, const int maxHp) noexcept
{
    const int missing = maxHp - currentHp;
    return std::min(maxHp, currentHp + ((missing + 1) / 2));
}
}
