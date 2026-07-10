#include "app/run/TowerSession.hpp"

#include <array>
#include <stdexcept>
#include <utility>

namespace arcane::app
{
namespace
{
constexpr std::array<game::run::ContentId, 16> NormalRewardPool {
    1001U, 1002U, 1003U, 1004U, 1005U, 1006U, 1007U, 1008U,
    1009U, 1010U, 1011U, 1012U, 1013U, 1014U, 1015U, 1016U
};
constexpr std::array<game::run::ContentId, 9> BossRewardPool {
    2001U, 2002U, 2003U, 2004U, 2005U, 2006U, 2007U, 2008U, 2009U
};
}

TowerSession::TowerSession(const game::run::Seed seed, TowerSessionConfig config)
    : run_(seed), config_(std::move(config))
{
    if (config_.normalEnemyHealth <= 0 || config_.bossEnemyHealth <= 0
        || config_.normalGoldReward < 0 || config_.bossGoldReward < 0
        || config_.floorsPerBoss == 0U || config_.staircaseBounds.width <= 0.0F
        || config_.staircaseBounds.height <= 0.0F)
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
        if (!combat_) throw std::logic_error("combat phase requires an active combat session");
        combat_->update(intent, deltaSeconds);
        if (combat_->result())
        {
            const game::CombatResult result = *combat_->result();
            const bool resolved = currentFloorType_ == game::run::FloorType::Boss
                ? run_.resolveEncounter(result, BossRewardPool)
                : run_.resolveEncounter(result, NormalRewardPool);
            if (!resolved) throw std::logic_error("combat result was rejected by the run controller");
        }
        break;

    case game::run::RunPhase::Reward:
    {
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
        if (!combat_) throw std::logic_error("completed combat floor must retain its map state");
        combat_->update(intent, deltaSeconds);
        if (intent.interactPressed)
        {
            const game::PlayerStateView player = combat_->playerState();
            const game::Aabb playerBounds {
                player.position.x,
                player.position.y,
                game::PlayerController::Width,
                game::PlayerController::Height
            };
            if (game::intersects(playerBounds, config_.staircaseBounds) && run_.useStairs()
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
    return run_.phase() == game::run::RunPhase::FloorComplete;
}

void TowerSession::startNextFloor()
{
    if (run_.phase() != game::run::RunPhase::FloorLoading)
    {
        throw std::logic_error("a floor can only start from FloorLoading");
    }

    currentFloorType_ = isBossFloor() ? game::run::FloorType::Boss : game::run::FloorType::Combat;
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

bool TowerSession::isBossFloor() const noexcept
{
    return (run_.context().floorIndex + 1U) % config_.floorsPerBoss == 0U;
}
}
