#include "app/run/TowerSession.hpp"
#include "game/run/DeterministicRng.hpp"
#include "game/spells/SpellSystem.hpp"

#include <array>
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace arcane::app
{
namespace
{
constexpr std::array<game::run::ContentId, 24> NormalRewardPool {
    1001U, 1002U, 1003U, 1004U, 1005U, 1006U, 1008U, 1009U, 1011U, 1016U,
    1017U, 1018U, 1019U, 1020U, 1021U, 1022U, 1023U, 1024U, 1025U, 1026U,
    1027U, 1028U, 1029U, 1030U
};
constexpr std::array<game::run::ContentId, 12> DamageRewardPool {
    1003U, 1004U, 1005U, 1006U, 1009U, 1017U, 1019U, 1021U, 1024U, 1026U,
    1027U, 1029U
};
constexpr std::array<game::run::ContentId, 7> ControlRewardPool {
    1008U, 1011U, 1016U, 1018U, 1020U, 1025U, 1030U
};
constexpr std::array<game::run::ContentId, 9> BossRewardPool {
    2001U, 2002U, 2003U, 2006U, 2007U, 2009U, 2010U, 2011U, 2012U
};
constexpr std::array SpellMerchantCatalog {
    game::economy::CatalogItem {1001U, game::economy::ItemKind::Spell, 10},
    game::economy::CatalogItem {1002U, game::economy::ItemKind::Spell, 15},
    game::economy::CatalogItem {1003U, game::economy::ItemKind::Spell, 20},
    game::economy::CatalogItem {1004U, game::economy::ItemKind::Spell, 10},
    game::economy::CatalogItem {1005U, game::economy::ItemKind::Spell, 15},
    game::economy::CatalogItem {1006U, game::economy::ItemKind::Spell, 20},
    game::economy::CatalogItem {1008U, game::economy::ItemKind::Spell, 15},
    game::economy::CatalogItem {1009U, game::economy::ItemKind::Spell, 20},
    game::economy::CatalogItem {1011U, game::economy::ItemKind::Spell, 15},
    game::economy::CatalogItem {1016U, game::economy::ItemKind::Spell, 15},
    game::economy::CatalogItem {1017U, game::economy::ItemKind::Spell, 15},
    game::economy::CatalogItem {1018U, game::economy::ItemKind::Spell, 15},
    game::economy::CatalogItem {1019U, game::economy::ItemKind::Spell, 20},
    game::economy::CatalogItem {1020U, game::economy::ItemKind::Spell, 15},
    game::economy::CatalogItem {1021U, game::economy::ItemKind::Spell, 20},
    game::economy::CatalogItem {1022U, game::economy::ItemKind::Spell, 20},
    game::economy::CatalogItem {1023U, game::economy::ItemKind::Spell, 20},
    game::economy::CatalogItem {1024U, game::economy::ItemKind::Spell, 20},
    game::economy::CatalogItem {1025U, game::economy::ItemKind::Spell, 15},
    game::economy::CatalogItem {1026U, game::economy::ItemKind::Spell, 20},
    game::economy::CatalogItem {1027U, game::economy::ItemKind::Spell, 15},
    game::economy::CatalogItem {1028U, game::economy::ItemKind::Spell, 20},
    game::economy::CatalogItem {1029U, game::economy::ItemKind::Spell, 20},
    game::economy::CatalogItem {1030U, game::economy::ItemKind::Spell, 20}
};
constexpr std::array RelicMerchantCatalog {
    game::economy::CatalogItem {4001U, game::economy::ItemKind::Relic, 45},
    game::economy::CatalogItem {4002U, game::economy::ItemKind::Relic, 40},
    game::economy::CatalogItem {4003U, game::economy::ItemKind::Relic, 40},
    game::economy::CatalogItem {4004U, game::economy::ItemKind::Relic, 45},
    game::economy::CatalogItem {4005U, game::economy::ItemKind::Relic, 45},
    game::economy::CatalogItem {4006U, game::economy::ItemKind::Relic, 40},
    game::economy::CatalogItem {4007U, game::economy::ItemKind::Relic, 55},
    game::economy::CatalogItem {4008U, game::economy::ItemKind::Relic, 40},
    game::economy::CatalogItem {4009U, game::economy::ItemKind::Relic, 45},
    game::economy::CatalogItem {4010U, game::economy::ItemKind::Relic, 40},
    game::economy::CatalogItem {4011U, game::economy::ItemKind::Relic, 50},
    game::economy::CatalogItem {4012U, game::economy::ItemKind::Relic, 45},
    game::economy::CatalogItem {4013U, game::economy::ItemKind::Relic, 45},
    game::economy::CatalogItem {4014U, game::economy::ItemKind::Relic, 45},
    game::economy::CatalogItem {4015U, game::economy::ItemKind::Relic, 45},
    game::economy::CatalogItem {4016U, game::economy::ItemKind::Relic, 40},
    game::economy::CatalogItem {4017U, game::economy::ItemKind::Relic, 50},
    game::economy::CatalogItem {4018U, game::economy::ItemKind::Relic, 55},
    game::economy::CatalogItem {4019U, game::economy::ItemKind::Relic, 40},
    game::economy::CatalogItem {4020U, game::economy::ItemKind::Relic, 45},
    game::economy::CatalogItem {4021U, game::economy::ItemKind::Relic, 40},
    game::economy::CatalogItem {4022U, game::economy::ItemKind::Relic, 45},
    game::economy::CatalogItem {4023U, game::economy::ItemKind::Relic, 45},
    game::economy::CatalogItem {4024U, game::economy::ItemKind::Relic, 40}
};

std::size_t eligibleCount(const std::span<const game::economy::CatalogItem> catalog,
    const game::run::PlayerProgress& player)
{
    return static_cast<std::size_t>(std::count_if(catalog.begin(), catalog.end(), [&player](const auto& item) {
        const auto& owned = item.kind == game::economy::ItemKind::Spell
            ? player.learnedSpells : player.relics;
        return std::find(owned.begin(), owned.end(), item.id) == owned.end();
    }));
}
}

TowerSession::TowerSession(const game::run::Seed seed, TowerSessionConfig config)
    : run_(seed, config.initialPlayer), config_(std::move(config)), scheduler_({config_.floorsPerBoss, 4U, 3U})
{
    if (config_.normalEnemyHealth < 0 || config_.bossEnemyHealth <= 0
        || config_.normalGoldReward < 0 || config_.bossGoldReward < 0
        || config_.floorsPerBoss == 0U || config_.staircaseBounds.width <= 0.0F
        || config_.staircaseBounds.height <= 0.0F || config_.npcBounds.width <= 0.0F
        || config_.npcBounds.height <= 0.0F)
    {
        throw std::invalid_argument("invalid tower session configuration");
    }

    for (const auto& item : SpellMerchantCatalog)
    {
        if (item.kind == game::economy::ItemKind::Spell
            && game::spells::findDefinition(item.id) == nullptr)
        {
            throw std::logic_error("merchant spell has no combat definition");
        }
    }
    for (const auto& item : RelicMerchantCatalog)
        if (game::relics::findDefinition(item.id) == nullptr)
            throw std::logic_error("merchant relic has no definition");

    startNextFloor();
}

void TowerSession::update(const game::PlayerIntent& intent, const float deltaSeconds)
{
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
                if (result.outcome == game::CombatOutcome::Victory)
                {
                    constexpr float DropSize = 32.0F;
                    const game::EnemyStateView enemy = combat_->enemyState();
                    lootDropBounds_ = game::Aabb {
                        enemy.position.x + (game::CombatSession::EnemyWidth - DropSize) * 0.5F,
                        enemy.position.y + game::CombatSession::EnemyHeight - DropSize,
                        DropSize,
                        DropSize
                    };
                }
                const bool resolved = currentFloorType_ == game::run::FloorType::Boss
                    ? run_.resolveEncounter(result, BossRewardPool)
                    : run_.resolveEncounter(result, NormalRewardPool,
                        DamageRewardPool, ControlRewardPool);
                if (!resolved) throw std::logic_error("combat result was rejected by the run controller");
            }
        }
        else
            updateSpecialFloor(intent, deltaSeconds);
        break;

    case game::run::RunPhase::LootPending:
        if (!combat_ || !lootDropBounds_)
            throw std::logic_error("loot phase requires a completed combat map and drop");
        combat_->update(intent, deltaSeconds);
        if (intent.interactPressed)
        {
            const game::PlayerStateView player = combat_->playerState();
            const game::Aabb playerBounds {player.position.x, player.position.y,
                game::PlayerController::Width, game::PlayerController::Height};
            if (game::intersects(playerBounds, *lootDropBounds_) && run_.openReward())
                combat_->settlePlayerForReward();
        }
        break;

    case game::run::RunPhase::Reward:
    {
        if (run_.rewardOffer().kind == game::rewards::RewardKind::GoldFallback)
        {
            static_cast<void>(run_.claimFallbackReward());
            break;
        }
        if (intent.menuSecondaryPressed && currentFloorType_ != game::run::FloorType::Boss)
        {
            static_cast<void>(run_.rerollRegularReward(DamageRewardPool, ControlRewardPool,
                NormalRewardPool));
            break;
        }
        const auto candidates = run_.rewardOffer().candidates;
        for (std::size_t index = 0; index < intent.spellPressed.size(); ++index)
        {
            if (intent.spellPressed[index] && run_.chooseReward(candidates[index])) break;
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

const game::floors::ArenaLayout& TowerSession::arenaLayout() const
{
    if (!arenaLayout_) throw std::logic_error("tower session has no active arena layout");
    return *arenaLayout_;
}

std::optional<std::array<game::run::ContentId, 3>> TowerSession::rewardCandidates() const
{
    if (run_.phase() != game::run::RunPhase::Reward) return std::nullopt;
    return run_.rewardOffer().candidates;
}

std::optional<game::Aabb> TowerSession::lootDropBounds() const noexcept
{
    if (run_.phase() != game::run::RunPhase::LootPending) return std::nullopt;
    return lootDropBounds_;
}

bool TowerSession::equipRegularSpell(const std::size_t slot, const game::run::ContentId spell)
{
    if (!run_.equip(slot, spell)) return false;
    if (combat_)
        return combat_->equipSpell(slot, run_.player().equippedSpells[slot]);
    return true;
}

bool TowerSession::equipUltimateSpell(const game::run::ContentId spell)
{
    if (!run_.equipUltimate(spell)) return false;
    if (combat_)
        return combat_->equipUltimateSpell(run_.player().equippedUltimateSpell);
    return true;
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

std::optional<game::run::ContentId> TowerSession::selectedMerchantItem() const noexcept
{
    if (merchantStock_.empty() || selectedMerchantIndex_ >= merchantStock_.size()) return std::nullopt;
    return merchantStock_[selectedMerchantIndex_].id;
}

std::span<const game::events::EventChoice> TowerSession::eventChoices() const noexcept
{
    return eventChoices_;
}

const game::PlayerController* TowerSession::explorationPlayer() const noexcept
{
    return explorationPlayer_ ? &*explorationPlayer_ : nullptr;
}

game::Aabb TowerSession::npcBounds() const noexcept { return config_.npcBounds; }
bool TowerSession::specialPanelOpen() const noexcept { return specialPanelOpen_; }
EventFloorState TowerSession::eventFloorState() const noexcept { return eventFloorState_; }
EventKind TowerSession::eventKind() const noexcept { return eventKind_; }
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
    if (run_.context().floorIndex == 0U && config_.firstFloorTypeOverride)
        currentFloorType_ = *config_.firstFloorTypeOverride;
    else
        currentFloorType_ = config_.enableSpecialFloors
            ? scheduler_.next(run_.context())
            : ((run_.context().floorIndex + 1U) % config_.floorsPerBoss == 0U
                ? game::run::FloorType::Boss : game::run::FloorType::Combat);
    if (currentFloorType_ == game::run::FloorType::Event
        && std::all_of(run_.context().triggeredEvents.begin(),
            run_.context().triggeredEvents.end(), [](const bool triggered) { return triggered; }))
        currentFloorType_ = game::run::FloorType::Combat;
    merchantStock_.clear();
    eventTransaction_.reset();
    combat_.reset();
    lootDropBounds_.reset();
    explorationPlayer_.reset();
    specialPanelOpen_ = false;
    eventFloorState_ = EventFloorState::Untriggered;
    eventResultChoice_.reset();
    if (currentFloorType_ == game::run::FloorType::Merchant)
    {
        const auto& floor = run_.loadFloor(currentFloorType_, {});
        arenaLayout_ = &game::floors::arenaLayout(floor.arenaId);
        const auto merchantSeed = game::run::deriveStreamSeed(
            run_.context().floorSeed, game::run::RandomStream::Merchant);
        const auto spellCount = std::min<std::size_t>(3U, eligibleCount(SpellMerchantCatalog, run_.player()));
        if (spellCount > 0U)
        {
            auto spells = game::economy::generateStock(
                SpellMerchantCatalog, run_.player(), merchantSeed, spellCount);
            merchantStock_.insert(merchantStock_.end(), spells.begin(), spells.end());
        }
        const std::size_t relicCatalogCount = run_.context().act >= 3U ? 24U
            : (run_.context().act >= 2U ? 20U : 12U);
        const std::span relicCatalog {RelicMerchantCatalog.data(), relicCatalogCount};
        const bool extraRelic = std::find(run_.player().relics.begin(), run_.player().relics.end(),
            game::relics::MimicTongueId) != run_.player().relics.end();
        const auto relicCount = std::min<std::size_t>(extraRelic ? 3U : 2U,
            eligibleCount(relicCatalog, run_.player()));
        if (relicCount > 0U)
        {
            auto relics = game::economy::generateStock(relicCatalog, run_.player(),
                merchantSeed ^ 0x9e3779b97f4a7c15ULL, relicCount);
            if (extraRelic)
                for (auto& relic : relics) relic.price = (relic.price * 11 + 9) / 10;
            merchantStock_.insert(merchantStock_.end(), relics.begin(), relics.end());
        }
        selectedMerchantIndex_ = 0U;
        explorationPlayer_.emplace(config_.playerSpawn);
        return;
    }
    if (currentFloorType_ == game::run::FloorType::Event)
    {
        const auto& floor = run_.loadFloor(currentFloorType_, {});
        arenaLayout_ = &game::floors::arenaLayout(floor.arenaId);
        eventTransaction_.emplace();
        const auto spellOffer = game::rewards::generateOffer(NormalRewardPool,
            run_.player().learnedSpells,
            game::run::deriveStreamSeed(run_.context().floorSeed, game::run::RandomStream::Event));
        const auto randomSpell = spellOffer.kind == game::rewards::RewardKind::SpellChoice
            ? spellOffer.candidates[0] : 0U;
        game::run::DeterministicRng eventRng(game::run::deriveStreamSeed(
            run_.context().floorSeed, game::run::RandomStream::Event));
        std::array<std::size_t, 3> availableEvents {};
        std::size_t availableCount = 0U;
        for (std::size_t index = 0U; index < availableEvents.size(); ++index)
            if (!run_.eventTriggered(index)) availableEvents[availableCount++] = index;
        if (availableCount == 0U)
            throw std::logic_error("event floor has no untriggered event");
        const auto eventIndex = availableEvents[eventRng.index(
            static_cast<std::uint32_t>(availableCount))];
        run_.markEventTriggered(eventIndex);
        eventKind_ = eventIndex == 0U ? EventKind::AldenBall
            : (eventIndex == 1U ? EventKind::HalfCenturyMeteorShower : EventKind::SwordVillage);
        if (eventKind_ == EventKind::AldenBall)
            eventChoices_ = {{
                game::events::EventChoice {5001U, 0, 0, 0U, 30, 0U, false},
                game::events::EventChoice {5002U, 0, randomSpell == 0U ? 15 : 0, 0U, 0, randomSpell, false},
                game::events::EventChoice {5003U, 0, 50, 0U, 0, 0U, false}
            }};
        else if (eventKind_ == EventKind::HalfCenturyMeteorShower)
            eventChoices_ = {{
                game::events::EventChoice {5101U, 0, randomSpell == 0U ? 15 : 0, 0U, 0, randomSpell, false},
                game::events::EventChoice {5102U, 0, 0, 0U, 0, 0U, true},
                game::events::EventChoice {5103U, 0, eventRng.index(2U) == 0U ? 99 : 0, 0U, 0, 0U, false}
            }};
        else
            eventChoices_ = {{
                game::events::EventChoice {5201U, 0, 0, game::relics::HeroSwordId, 0, 0U, false},
                game::events::EventChoice {5202U, 0, 0, game::relics::TrueHeroSwordId, 0, 0U, false},
                game::events::EventChoice {5203U, 0, 50, 0U, 0, 0U, false}
            }};
        explorationPlayer_.emplace(config_.playerSpawn);
        return;
    }
    const game::run::ContentId encounterId = currentFloorType_ == game::run::FloorType::Boss
        ? 2000U + run_.context().bossesDefeated
        : 1000U + run_.context().floorIndex;
    const std::array encounterPool {encounterId};
    const game::run::FloorDescriptor& floor = run_.loadFloor(currentFloorType_, encounterPool);
    arenaLayout_ = &game::floors::arenaLayout(floor.arenaId);

    game::CombatRequest request;
    request.encounterId = floor.encounterIds.front();
    request.seed = floor.seed;
    request.playerSpawn = config_.playerSpawn;
    request.enemySpawn = config_.enemySpawn;
    request.worldBounds = config_.worldBounds;
    request.oneWayPlatforms = arenaLayout_->oneWayPlatforms;
    request.playerMaximumHealth = run_.player().maxHp;
    request.playerCurrentHealth = run_.player().currentHp;
    request.enemyMaximumHealth = currentFloorType_ == game::run::FloorType::Boss
        ? config_.bossEnemyHealth
        : (config_.normalEnemyHealth > 0 ? config_.normalEnemyHealth
            : (run_.context().floorIndex % 2U == 0U ? 60 : 45));
    if (currentFloorType_ == game::run::FloorType::Boss
        && run_.context().bossesDefeated == 1U && config_.bossEnemyHealth == 225)
        request.enemyMaximumHealth = 300;
    request.enemyArchetype = currentFloorType_ == game::run::FloorType::Boss
        ? (config_.bossEnemyHealth == 225
            ? (run_.context().bossesDefeated == 0U ? game::EnemyArchetype::Aura
                : (run_.context().bossesDefeated == 1U ? game::EnemyArchetype::Revolte
                    : game::EnemyArchetype::Boss))
            : game::EnemyArchetype::Boss)
        : (config_.normalEnemyHealth > 0 ? game::EnemyArchetype::HeadlessKnight
            : (run_.context().floorIndex % 2U == 0U
                ? game::EnemyArchetype::ChestMimic : game::EnemyArchetype::HeadlessKnight));
    if (currentFloorType_ == game::run::FloorType::Combat && config_.normalEnemyHealth == 0)
    {
        constexpr std::array firstGroup {
            game::EnemyArchetype::ChestMimic, game::EnemyArchetype::HeadlessKnight,
            game::EnemyArchetype::BirdDemon
        };
        constexpr std::array fourthGroup {
            game::EnemyArchetype::Lugner, game::EnemyArchetype::Linie, game::EnemyArchetype::Draht
        };
        constexpr std::array secondActOpeningGroup {
            game::EnemyArchetype::ChaosFlower, game::EnemyArchetype::FrostWolf,
            game::EnemyArchetype::Qual
        };
        constexpr std::array secondActLateGroup {
            game::EnemyArchetype::Heimon, game::EnemyArchetype::DemonWarrior,
            game::EnemyArchetype::LargeBirdDemon
        };
        constexpr std::array thirdActLateGroup {
            game::EnemyArchetype::Laufen, game::EnemyArchetype::Richter,
            game::EnemyArchetype::Denken
        };
        constexpr std::array thirdActOpeningGroup {
            game::EnemyArchetype::Gargoyle, game::EnemyArchetype::ThreeHeadedDemon,
            game::EnemyArchetype::SwordDemon
        };
        std::array<game::EnemyArchetype, 3> encounter;
        const std::uint32_t floorInAct = run_.context().floorIndex % config_.floorsPerBoss;
        if (run_.context().bossesDefeated == 1U && floorInAct == 0U)
            encounter = secondActOpeningGroup;
        else if (run_.context().bossesDefeated == 1U && floorInAct == 3U)
            encounter = secondActLateGroup;
        else if (run_.context().bossesDefeated == 2U && floorInAct == 3U)
            encounter = thirdActLateGroup;
        else if (run_.context().bossesDefeated == 2U && floorInAct == 0U)
            encounter = thirdActOpeningGroup;
        else if (run_.context().bossesDefeated == 1U
            && (floorInAct == 1U || floorInAct == 2U))
        {
            game::run::DeterministicRng rng(game::run::deriveStreamSeed(
                floor.seed, game::run::RandomStream::Encounter));
            const std::size_t first = rng.index(
                static_cast<std::uint32_t>(secondActOpeningGroup.size()));
            const std::size_t second = (first + 1U + rng.index(2U))
                % secondActOpeningGroup.size();
            encounter = {secondActOpeningGroup[first], secondActOpeningGroup[second],
                secondActLateGroup[rng.index(
                    static_cast<std::uint32_t>(secondActLateGroup.size()))]};
        }
        else if (run_.context().bossesDefeated == 2U
            && (floorInAct == 1U || floorInAct == 2U))
        {
            game::run::DeterministicRng rng(game::run::deriveStreamSeed(
                floor.seed, game::run::RandomStream::Encounter));
            const std::size_t first = rng.index(
                static_cast<std::uint32_t>(thirdActOpeningGroup.size()));
            const std::size_t second = (first + 1U + rng.index(2U))
                % thirdActOpeningGroup.size();
            encounter = {thirdActOpeningGroup[first], thirdActOpeningGroup[second],
                thirdActLateGroup[rng.index(
                    static_cast<std::uint32_t>(thirdActLateGroup.size()))]};
        }
        else if (floorInAct == 0U)
            encounter = firstGroup;
        else if (floorInAct == 3U)
            encounter = fourthGroup;
        else
        {
            game::run::DeterministicRng rng(game::run::deriveStreamSeed(
                floor.seed, game::run::RandomStream::Encounter));
            const std::size_t first = rng.index(3U);
            const std::size_t second = (first + 1U + rng.index(2U)) % firstGroup.size();
            encounter = {firstGroup[first], firstGroup[second], fourthGroup[rng.index(3U)]};
        }
        for (std::size_t index = 0U; index < encounter.size(); ++index)
            request.enemies.push_back({encounter[index],
                {650.0F + static_cast<float>(index) * 220.0F, 576.0F}});
    }
    request.enemyContactDamage = currentFloorType_ == game::run::FloorType::Boss ? 20 : 15;
    request.enemyAttackDamage = request.enemyArchetype == game::EnemyArchetype::HeadlessKnight ? 20 : 15;
    request.enemyControlSeconds = request.enemyArchetype == game::EnemyArchetype::ChestMimic ? 1.0F : 0.28F;
    request.goldReward = currentFloorType_ == game::run::FloorType::Boss
        ? config_.bossGoldReward
        : config_.normalGoldReward;
    request.equippedSpellIds = run_.player().equippedSpells;
    request.equippedUltimateSpellId = run_.player().ultimateSpellUnlocked
        ? run_.player().equippedUltimateSpell
        : std::nullopt;
    request.relicIds = run_.player().relics;

    combat_.emplace(request);
}

void TowerSession::updateSpecialFloor(const game::PlayerIntent& intent, const float deltaSeconds)
{
    if (!explorationPlayer_) throw std::logic_error("special floor requires an exploration player");

    if (specialPanelOpen_)
    {
        if (currentFloorType_ == game::run::FloorType::Merchant)
        {
            if (!merchantStock_.empty())
            {
                const auto selectedKind = merchantStock_[selectedMerchantIndex_].kind;
                std::vector<std::size_t> sameRow;
                std::vector<std::size_t> otherRow;
                for (std::size_t index = 0U; index < merchantStock_.size(); ++index)
                    (merchantStock_[index].kind == selectedKind ? sameRow : otherRow).push_back(index);
                const auto current = std::find(sameRow.begin(), sameRow.end(), selectedMerchantIndex_);
                const auto column = static_cast<std::size_t>(std::distance(sameRow.begin(), current));
                if (intent.menuPreviousPressed)
                    selectedMerchantIndex_ = sameRow[(column + sameRow.size() - 1U) % sameRow.size()];
                if (intent.menuNextPressed)
                    selectedMerchantIndex_ = sameRow[(column + 1U) % sameRow.size()];
                if ((intent.menuUpPressed || intent.menuDownPressed) && !otherRow.empty())
                    selectedMerchantIndex_ = otherRow[std::min(column, otherRow.size() - 1U)];
                if (intent.menuConfirmPressed)
                {
                    const auto result = run_.purchaseMerchantItem(
                        merchantStock_, merchantStock_[selectedMerchantIndex_].id);
                    if (result == game::economy::PurchaseResult::Success)
                    {
                        merchantStock_.erase(merchantStock_.begin()
                            + static_cast<std::ptrdiff_t>(selectedMerchantIndex_));
                        selectedMerchantIndex_ = merchantStock_.empty() ? 0U
                            : std::min(selectedMerchantIndex_, merchantStock_.size() - 1U);
                    }
                }
            }
        }
        else if (eventFloorState_ == EventFloorState::Choosing && eventTransaction_)
        {
            for (std::size_t index = 0U; index < eventChoices_.size(); ++index)
            {
                if (intent.spellPressed[index]
                    && run_.chooseEvent(*eventTransaction_, eventChoices_, eventChoices_[index].id)
                        == game::events::EventResult::Success)
                {
                    eventFloorState_ = EventFloorState::Result;
                    eventResultChoice_ = eventChoices_[index].id;
                    break;
                }
            }
        }
        if (intent.interactPressed) specialPanelOpen_ = false;
        return;
    }

    explorationPlayer_->update(intent, deltaSeconds, config_.worldBounds,
        arenaLayout().oneWayPlatforms);
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
