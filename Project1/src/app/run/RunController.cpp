#include "app/run/RunController.hpp"

#include "game/run/DeterministicRng.hpp"
#include "game/spells/SpellSystem.hpp"
#include "game/relics/RelicSystem.hpp"

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
    const std::span<const game::run::ContentId> rewardPool,
    const std::span<const game::run::ContentId> damageRewardPool,
    const std::span<const game::run::ContentId> controlRewardPool)
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
    const auto& ownedSpells = currentFloorType_ == game::run::FloorType::Boss
        ? player_.learnedBossSpells
        : player_.learnedSpells;
    const game::rewards::RewardOffer offer = damageRewardPool.empty() || controlRewardPool.empty()
        ? game::rewards::generateOffer(rewardPool, ownedSpells, seed)
        : game::rewards::generateCategorizedOffer(damageRewardPool, controlRewardPool,
            rewardPool, ownedSpells, seed);

    floor_.markEncounterComplete();
    if (currentFloorType_ == game::run::FloorType::Boss) ++context_.bossesDefeated;
    player_.currentHp = result.playerHealthRemaining;
    player_.gold += result.goldAwarded;
    reward_ = offer;
    activeEncounterId_.reset();
    rewardApplied_ = false;
    phase_ = game::run::RunPhase::LootPending;
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

bool RunController::openReward()
{
    if (phase_ != game::run::RunPhase::LootPending || !reward_ || rewardApplied_) return false;
    phase_ = game::run::RunPhase::Reward;
    return true;
}

bool RunController::chooseReward(const game::run::ContentId choice)
{
    if (phase_ != game::run::RunPhase::Reward || rewardApplied_) return false;
    const bool bossReward = currentFloorType_ == game::run::FloorType::Boss;
    const auto* definition = game::spells::findDefinition(choice);
    if (definition && bossReward != (definition->tier == game::spells::SpellTier::Boss)) return false;
    const auto rewardType = bossReward
        ? game::rewards::SpellRewardType::Boss
        : game::rewards::SpellRewardType::Regular;
    if (!game::rewards::applySpellChoice(player_, *reward_, choice, rewardType)) return false;
    rewardApplied_ = true;
    phase_ = game::run::RunPhase::FloorComplete;
    return true;
}

bool RunController::claimFallbackReward()
{
    if (phase_ != game::run::RunPhase::Reward || rewardApplied_ || !reward_
        || reward_->kind != game::rewards::RewardKind::GoldFallback
        || reward_->fallbackGold > std::numeric_limits<int>::max() - player_.gold) return false;
    player_.gold += reward_->fallbackGold;
    rewardApplied_ = true;
    phase_ = game::run::RunPhase::FloorComplete;
    return true;
}

bool RunController::rerollRegularReward(const std::span<const game::run::ContentId> damagePool,
    const std::span<const game::run::ContentId> controlPool,
    const std::span<const game::run::ContentId> fullPool)
{
    const std::size_t actIndex = context_.act > 0U ? context_.act - 1U : 0U;
    if (phase_ != game::run::RunPhase::Reward || !reward_ || rewardApplied_
        || currentFloorType_ == game::run::FloorType::Boss
        || actIndex >= player_.rewardRerollUsed.size() || player_.rewardRerollUsed[actIndex]
        || std::find(player_.relics.begin(), player_.relics.end(), game::relics::FirstClassBadgeId)
            == player_.relics.end()) return false;
    const auto seed = game::run::deriveStreamSeed(context_.floorSeed,
        game::run::RandomStream::Reward) ^ 0xd1b54a32d192ed03ULL;
    *reward_ = game::rewards::generateCategorizedOffer(damagePool, controlPool, fullPool,
        player_.learnedSpells, seed);
    player_.rewardRerollUsed[actIndex] = true;
    return true;
}

game::economy::PurchaseResult RunController::purchaseMerchantItem(
    std::vector<game::economy::StockItem>& stock, const game::run::ContentId itemId)
{
    if (phase_ != game::run::RunPhase::InEncounter || currentFloorType_ != game::run::FloorType::Merchant)
        return game::economy::PurchaseResult::NotFound;
    return game::economy::purchase(player_, stock, itemId);
}

game::events::EventResult RunController::chooseEvent(game::events::EventTransaction& transaction,
    const std::span<const game::events::EventChoice> choices, const game::run::ContentId choiceId)
{
    if (phase_ != game::run::RunPhase::InEncounter || currentFloorType_ != game::run::FloorType::Event)
        return game::events::EventResult::ChoiceNotFound;
    return transaction.choose(player_, choices, choiceId);
}

bool RunController::equip(const std::size_t slot, const game::run::ContentId spell)
{
    if (slot >= player_.equippedSpells.size()
        || game::spells::isBossSpell(spell)
        || std::find(player_.learnedSpells.begin(), player_.learnedSpells.end(), spell)
            == player_.learnedSpells.end()) return false;
    for (auto& equipped : player_.equippedSpells) if (equipped == spell) equipped.reset();
    player_.equippedSpells[slot] = spell;
    return true;
}

bool RunController::equipUltimate(const game::run::ContentId spell)
{
    if (!player_.ultimateSpellUnlocked || !game::spells::isBossSpell(spell)
        || std::find(player_.learnedBossSpells.begin(), player_.learnedBossSpells.end(), spell)
            == player_.learnedBossSpells.end())
    {
        return false;
    }
    player_.equippedUltimateSpell = spell;
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

game::run::FloorResult RunController::floorResult() const noexcept
{
    const bool rewardComplete = phase_ == game::run::RunPhase::FloorComplete
        || phase_ == game::run::RunPhase::Victory;
    return floor_.result(rewardComplete);
}
}
