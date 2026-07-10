#include "app/run/TowerSession.hpp"
#include "game/run/DeterministicRng.hpp"

#include <array>
#include <stdexcept>
#include <utility>

namespace arcane::app
{
namespace
{
constexpr std::array<game::run::ContentId, 6> NormalRewardPool {
    1001U, 1002U, 1003U, 1004U, 1005U, 1006U
};
constexpr std::array<game::run::ContentId, 3> BossRewardPool {
    2001U, 2002U, 2003U
};
constexpr std::array MerchantCatalog {
    game::economy::CatalogItem {3001U, game::economy::ItemKind::Spell, 10},
    game::economy::CatalogItem {3002U, game::economy::ItemKind::Spell, 15},
    game::economy::CatalogItem {3003U, game::economy::ItemKind::Spell, 20},
    game::economy::CatalogItem {4001U, game::economy::ItemKind::Relic, 15},
    game::economy::CatalogItem {4002U, game::economy::ItemKind::Relic, 20},
    game::economy::CatalogItem {4003U, game::economy::ItemKind::Relic, 25}
};
constexpr std::array EventChoices {
    game::events::EventChoice {5001U, 0, 10, 0U},
    game::events::EventChoice {5002U, -20, 0, 4101U}
};
}

TowerSession::TowerSession(const game::run::Seed seed, TowerSessionConfig config)
    : run_(seed, config.initialPlayer), config_(std::move(config)), scheduler_({config_.floorsPerBoss, 4U, 3U})
{
    if (config_.normalEnemyHealth <= 0 || config_.bossEnemyHealth <= 0
        || config_.normalGoldReward < 0 || config_.bossGoldReward < 0
        || config_.floorsPerBoss == 0U || config_.staircaseBounds.width <= 0.0F
        || config_.staircaseBounds.height <= 0.0F || config_.npcBounds.width <= 0.0F
        || config_.npcBounds.height <= 0.0F)
    {
        throw std::invalid_argument("invalid tower session configuration");
    }

    startNextFloor();
}

void TowerSession::update(const game::PlayerIntent& intent, const float deltaSeconds)
{
    if (intent.toggleLoadoutPressed)
    {
        loadoutOpen_ = !loadoutOpen_;
        const auto learnedCount = run_.player().learnedSpells.size();
        if (learnedCount > 0U && selectedLearnedSpellIndex_ >= learnedCount)
        {
            selectedLearnedSpellIndex_ = learnedCount - 1U;
        }
        return;
    }

    if (loadoutOpen_)
    {
        updateLoadout(intent);
        return;
    }

    switch (run_.phase())
    {
    case game::run::RunPhase::InEncounter:
        if (currentFloorType_ == game::run::FloorType::Combat
            || currentFloorType_ == game::run::FloorType::Boss)
        {
            if (!combat_) throw std::logic_error("combat floor requires an active combat session");
            combat_->update(intent, deltaSeconds);
            if (combat_->result())
            {
                const game::CombatResult result = *combat_->result();
                const bool resolved = currentFloorType_ == game::run::FloorType::Boss
                    ? run_.resolveEncounter(result, BossRewardPool)
                    : run_.resolveEncounter(result, NormalRewardPool);
                if (!resolved) throw std::logic_error("combat result was rejected by the run controller");
            }
        }
        else
            updateSpecialFloor(intent, deltaSeconds);
        break;

    case game::run::RunPhase::Reward:
    {
        if (run_.rewardOffer().kind == game::rewards::RewardKind::GoldFallback)
        {
            static_cast<void>(run_.claimFallbackReward());
            break;
        }
        const auto candidates = run_.rewardOffer().candidates;
        for (std::size_t index = 0; index < intent.spellPressed.size(); ++index)
        {
            if (intent.spellPressed[index] && run_.chooseReward(candidates[index]))
            {
                selectedLearnedSpellIndex_ = run_.player().learnedSpells.size() - 1U;
                break;
            }
        }
        break;
    }

    case game::run::RunPhase::FloorComplete:
        if (combat_) combat_->update(intent, deltaSeconds);
        if (intent.interactPressed)
        {
            bool canClimb = !combat_;
            if (combat_)
            {
                const game::PlayerStateView player = combat_->playerState();
                const game::Aabb playerBounds {player.position.x, player.position.y,
                    game::PlayerController::Width, game::PlayerController::Height};
                canClimb = game::intersects(playerBounds, config_.staircaseBounds);
            }
            if (canClimb && run_.useStairs()
                && run_.phase() == game::run::RunPhase::FloorLoading)
            {
                startNextFloor();
            }
        }
        break;

    case game::run::RunPhase::FloorLoading:
        startNextFloor();
        break;

    case game::run::RunPhase::Defeat:
    case game::run::RunPhase::Victory:
        break;
    }
}

const RunController& TowerSession::run() const noexcept
{
    return run_;
}

const game::CombatSession* TowerSession::combat() const noexcept
{
    return combat_ ? &*combat_ : nullptr;
}

game::run::FloorType TowerSession::currentFloorType() const noexcept
{
    return currentFloorType_;
}

std::optional<std::array<game::run::ContentId, 3>> TowerSession::rewardCandidates() const
{
    if (run_.phase() != game::run::RunPhase::Reward) return std::nullopt;
    return run_.rewardOffer().candidates;
}

bool TowerSession::loadoutOpen() const noexcept
{
    return loadoutOpen_;
}

std::optional<game::run::ContentId> TowerSession::selectedLearnedSpell() const noexcept
{
    const auto& learned = run_.player().learnedSpells;
    if (learned.empty() || selectedLearnedSpellIndex_ >= learned.size()) return std::nullopt;
    return learned[selectedLearnedSpellIndex_];
}

game::Aabb TowerSession::staircaseBounds() const noexcept
{
    return config_.staircaseBounds;
}

bool TowerSession::staircaseUnlocked() const noexcept
{
    if (currentFloorType_ == game::run::FloorType::Merchant) return true;
    if (currentFloorType_ == game::run::FloorType::Event)
        return eventFloorState_ == EventFloorState::Result;
    return run_.phase() == game::run::RunPhase::FloorComplete;
}

const std::vector<game::economy::StockItem>& TowerSession::merchantStock() const noexcept
{
    return merchantStock_;
}

std::span<const game::events::EventChoice> TowerSession::eventChoices() const noexcept
{
    return EventChoices;
}

const game::PlayerController* TowerSession::explorationPlayer() const noexcept
{
    return explorationPlayer_ ? &*explorationPlayer_ : nullptr;
}

game::Aabb TowerSession::npcBounds() const noexcept { return config_.npcBounds; }
bool TowerSession::specialPanelOpen() const noexcept { return specialPanelOpen_; }
EventFloorState TowerSession::eventFloorState() const noexcept { return eventFloorState_; }
std::optional<game::run::ContentId> TowerSession::eventResultChoice() const noexcept
{
    return eventResultChoice_;
}

void TowerSession::restartCurrentFloor()
{
    if (!floorStartRun_ || !floorStartScheduler_)
        throw std::logic_error("current floor has no restart snapshot");
    run_ = *floorStartRun_;
    scheduler_ = *floorStartScheduler_;
    startNextFloor();
}

void TowerSession::startNextFloor()
{
    if (run_.phase() != game::run::RunPhase::FloorLoading)
    {
        throw std::logic_error("a floor can only start from FloorLoading");
    }

    floorStartRun_ = run_;
    floorStartScheduler_ = scheduler_;
    currentFloorType_ = config_.enableSpecialFloors
        ? scheduler_.next(run_.context())
        : ((run_.context().floorIndex + 1U) % config_.floorsPerBoss == 0U
            ? game::run::FloorType::Boss : game::run::FloorType::Combat);
    merchantStock_.clear();
    eventTransaction_.reset();
    combat_.reset();
    explorationPlayer_.reset();
    specialPanelOpen_ = false;
    eventFloorState_ = EventFloorState::Untriggered;
    eventResultChoice_.reset();
    if (currentFloorType_ == game::run::FloorType::Merchant)
    {
        static_cast<void>(run_.loadFloor(currentFloorType_, {}));
        merchantStock_ = game::economy::generateStock(MerchantCatalog, run_.player(),
            game::run::deriveStreamSeed(run_.context().floorSeed, game::run::RandomStream::Merchant));
        explorationPlayer_.emplace(config_.playerSpawn);
        return;
    }
    if (currentFloorType_ == game::run::FloorType::Event)
    {
        static_cast<void>(run_.loadFloor(currentFloorType_, {}));
        eventTransaction_.emplace();
        explorationPlayer_.emplace(config_.playerSpawn);
        return;
    }
    const game::run::ContentId encounterId = currentFloorType_ == game::run::FloorType::Boss
        ? 2000U + run_.context().bossesDefeated
        : 1000U + run_.context().floorIndex;
    const std::array encounterPool {encounterId};
    const game::run::FloorDescriptor& floor = run_.loadFloor(currentFloorType_, encounterPool);

    game::CombatRequest request;
    request.encounterId = floor.encounterIds.front();
    request.seed = floor.seed;
    request.playerSpawn = config_.playerSpawn;
    request.enemySpawn = config_.enemySpawn;
    request.worldBounds = config_.worldBounds;
    request.playerMaximumHealth = run_.player().maxHp;
    request.playerCurrentHealth = run_.player().currentHp;
    request.enemyMaximumHealth = currentFloorType_ == game::run::FloorType::Boss
        ? config_.bossEnemyHealth
        : config_.normalEnemyHealth;
    request.enemyContactDamage = currentFloorType_ == game::run::FloorType::Boss ? 20 : 10;
    request.goldReward = currentFloorType_ == game::run::FloorType::Boss
        ? config_.bossGoldReward
        : config_.normalGoldReward;
    request.equippedSpellIds = run_.player().equippedSpells;

    combat_.emplace(request);
}

void TowerSession::updateLoadout(const game::PlayerIntent& intent)
{
    const auto& learned = run_.player().learnedSpells;
    if (learned.empty()) return;

    if (intent.menuPreviousPressed)
    {
        selectedLearnedSpellIndex_ = selectedLearnedSpellIndex_ == 0U
            ? learned.size() - 1U
            : selectedLearnedSpellIndex_ - 1U;
    }
    if (intent.menuNextPressed)
    {
        selectedLearnedSpellIndex_ = (selectedLearnedSpellIndex_ + 1U) % learned.size();
    }

    for (std::size_t slot = 0; slot < intent.spellPressed.size(); ++slot)
    {
        if (intent.spellPressed[slot])
        {
            static_cast<void>(run_.equip(slot, learned[selectedLearnedSpellIndex_]));
        }
    }
}

void TowerSession::updateSpecialFloor(const game::PlayerIntent& intent, const float deltaSeconds)
{
    if (!explorationPlayer_) throw std::logic_error("special floor requires an exploration player");

    if (specialPanelOpen_)
    {
        if (currentFloorType_ == game::run::FloorType::Merchant)
        {
            for (std::size_t index = 0U; index < intent.spellPressed.size() && index < merchantStock_.size(); ++index)
            {
                if (!intent.spellPressed[index]) continue;
                const auto result = run_.purchaseMerchantItem(merchantStock_, merchantStock_[index].id);
                if (result == game::economy::PurchaseResult::Success)
                    merchantStock_.erase(merchantStock_.begin() + static_cast<std::ptrdiff_t>(index));
                break;
            }
        }
        else if (eventFloorState_ == EventFloorState::Choosing && eventTransaction_)
        {
            for (std::size_t index = 0U; index < 2U; ++index)
            {
                if (intent.spellPressed[index]
                    && run_.chooseEvent(*eventTransaction_, EventChoices, EventChoices[index].id)
                        == game::events::EventResult::Success)
                {
                    eventFloorState_ = EventFloorState::Result;
                    eventResultChoice_ = EventChoices[index].id;
                    break;
                }
            }
        }
        if (intent.interactPressed) specialPanelOpen_ = false;
        return;
    }

    explorationPlayer_->update(intent, deltaSeconds, config_.worldBounds);
    if (!intent.interactPressed) return;
    const auto position = explorationPlayer_->position();
    const game::Aabb playerBounds {position.x, position.y,
        game::PlayerController::Width, game::PlayerController::Height};
    if (game::intersects(playerBounds, config_.npcBounds))
    {
        specialPanelOpen_ = true;
        if (currentFloorType_ == game::run::FloorType::Event
            && eventFloorState_ == EventFloorState::Untriggered)
            eventFloorState_ = EventFloorState::Choosing;
        return;
    }

    const bool eventResolved = currentFloorType_ != game::run::FloorType::Event
        || eventFloorState_ == EventFloorState::Result;
    if (eventResolved && game::intersects(playerBounds, config_.staircaseBounds))
    {
        if (!run_.completeNonCombatFloor()) throw std::logic_error("special floor completion was rejected");
        if (run_.useStairs() && run_.phase() == game::run::RunPhase::FloorLoading) startNextFloor();
    }
}

}
