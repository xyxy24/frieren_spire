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
constexpr std::array<game::run::ContentId, 20> NormalRewardPool {
    1001U, 1002U, 1003U, 1004U, 1005U, 1006U, 1008U, 1009U, 1011U, 1016U,
    1017U, 1018U, 1019U, 1020U, 1021U, 1022U, 1023U, 1024U, 1025U, 1026U
};
constexpr std::array<game::run::ContentId, 10> DamageRewardPool {
    1003U, 1004U, 1005U, 1006U, 1009U, 1017U, 1019U, 1021U, 1024U, 1026U
};
constexpr std::array<game::run::ContentId, 6> ControlRewardPool {
    1008U, 1011U, 1016U, 1018U, 1020U, 1025U
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
    game::economy::CatalogItem {1026U, game::economy::ItemKind::Spell, 20}
};
constexpr std::array RelicMerchantCatalog {
    game::economy::CatalogItem {4001U, game::economy::ItemKind::Relic, 15},
    game::economy::CatalogItem {4002U, game::economy::ItemKind::Relic, 20}
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

    startNextFloor();
}

void TowerSession::update(const game::PlayerIntent& intent, const float deltaSeconds)
{
    if (intent.toggleLoadoutPressed)
    {
        loadoutOpen_ = !loadoutOpen_;
        if (loadoutOpen_) loadoutPage_ = LoadoutPage::Spells;
        const auto learnedCount = run_.player().learnedSpells.size();
        if (learnedCount > 0U && selectedLearnedSpellIndex_ >= learnedCount)
        {
            selectedLearnedSpellIndex_ = learnedCount - 1U;
        }
        const auto bossSpellCount = run_.player().learnedBossSpells.size();
        if (bossSpellCount > 0U && selectedBossSpellIndex_ >= bossSpellCount)
        {
            selectedBossSpellIndex_ = bossSpellCount - 1U;
        }
        const auto relicCount = run_.player().relics.size();
        if (relicCount > 0U && selectedRelicIndex_ >= relicCount)
        {
            selectedRelicIndex_ = relicCount - 1U;
        }
        return;
    }

    if (loadoutOpen_)
    {
        if (intent.menuPagePreviousPressed || intent.menuPageNextPressed)
        {
            loadoutPage_ = loadoutPage_ == LoadoutPage::Spells
                ? LoadoutPage::Relics
                : LoadoutPage::Spells;
            return;
        }
        if (loadoutPage_ == LoadoutPage::Spells) updateLoadout(intent);
        else updateRelicPage(intent);
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
            if (game::intersects(playerBounds, *lootDropBounds_))
                static_cast<void>(run_.openReward());
        }
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
                if (currentFloorType_ == game::run::FloorType::Boss)
                {
                    selectedBossSpellIndex_ = run_.player().learnedBossSpells.size() - 1U;
                    spellLoadoutSection_ = SpellLoadoutSection::Boss;
                }
                else
                {
                    selectedLearnedSpellIndex_ = run_.player().learnedSpells.size() - 1U;
                    spellLoadoutSection_ = SpellLoadoutSection::Regular;
                }
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

std::optional<game::Aabb> TowerSession::lootDropBounds() const noexcept
{
    if (run_.phase() != game::run::RunPhase::LootPending) return std::nullopt;
    return lootDropBounds_;
}

bool TowerSession::loadoutOpen() const noexcept
{
    return loadoutOpen_;
}

LoadoutPage TowerSession::loadoutPage() const noexcept
{
    return loadoutPage_;
}

SpellLoadoutSection TowerSession::spellLoadoutSection() const noexcept
{
    return spellLoadoutSection_;
}

std::optional<game::run::ContentId> TowerSession::selectedLearnedSpell() const noexcept
{
    if (spellLoadoutSection_ == SpellLoadoutSection::Boss)
    {
        const auto& learnedBossSpells = run_.player().learnedBossSpells;
        if (learnedBossSpells.empty() || selectedBossSpellIndex_ >= learnedBossSpells.size())
            return std::nullopt;
        return learnedBossSpells[selectedBossSpellIndex_];
    }
    const auto& learned = run_.player().learnedSpells;
    if (learned.empty() || selectedLearnedSpellIndex_ >= learned.size()) return std::nullopt;
    return learned[selectedLearnedSpellIndex_];
}

std::optional<game::run::ContentId> TowerSession::selectedRelic() const noexcept
{
    const auto& relics = run_.player().relics;
    if (relics.empty() || selectedRelicIndex_ >= relics.size()) return std::nullopt;
    return relics[selectedRelicIndex_];
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
        static_cast<void>(run_.loadFloor(currentFloorType_, {}));
        const auto merchantSeed = game::run::deriveStreamSeed(
            run_.context().floorSeed, game::run::RandomStream::Merchant);
        const auto spellCount = std::min<std::size_t>(3U, eligibleCount(SpellMerchantCatalog, run_.player()));
        if (spellCount > 0U)
        {
            auto spells = game::economy::generateStock(
                SpellMerchantCatalog, run_.player(), merchantSeed, spellCount);
            merchantStock_.insert(merchantStock_.end(), spells.begin(), spells.end());
        }
        const auto relicCount = std::min<std::size_t>(3U, eligibleCount(RelicMerchantCatalog, run_.player()));
        if (relicCount > 0U)
        {
            auto relics = game::economy::generateStock(RelicMerchantCatalog, run_.player(),
                merchantSeed ^ 0x9e3779b97f4a7c15ULL, relicCount);
            merchantStock_.insert(merchantStock_.end(), relics.begin(), relics.end());
        }
        selectedMerchantIndex_ = 0U;
        explorationPlayer_.emplace(config_.playerSpawn);
        return;
    }
    if (currentFloorType_ == game::run::FloorType::Event)
    {
        static_cast<void>(run_.loadFloor(currentFloorType_, {}));
        eventTransaction_.emplace();
        const auto spellOffer = game::rewards::generateOffer(NormalRewardPool,
            run_.player().learnedSpells,
            game::run::deriveStreamSeed(run_.context().floorSeed, game::run::RandomStream::Event));
        const auto randomSpell = spellOffer.kind == game::rewards::RewardKind::SpellChoice
            ? spellOffer.candidates[0] : 0U;
        game::run::DeterministicRng eventRng(game::run::deriveStreamSeed(
            run_.context().floorSeed, game::run::RandomStream::Event));
        const auto eventIndex = eventRng.index(3U);
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
        : (config_.normalEnemyHealth > 0 ? config_.normalEnemyHealth
            : (run_.context().floorIndex % 2U == 0U ? 60 : 45));
    request.enemyArchetype = currentFloorType_ == game::run::FloorType::Boss
        ? (run_.context().bossesDefeated == 0U && config_.bossEnemyHealth == 225
            ? ((floor.seed & 1ULL) == 0ULL
                ? game::EnemyArchetype::Aura : game::EnemyArchetype::RedMirrorDragon)
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
        std::array<game::EnemyArchetype, 3> encounter;
        const std::uint32_t floorInAct = run_.context().floorIndex % config_.floorsPerBoss;
        if (run_.context().bossesDefeated == 1U && floorInAct == 0U)
            encounter = secondActOpeningGroup;
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
    if (request.enemyArchetype == game::EnemyArchetype::RedMirrorDragon)
        request.enemyMaximumHealth = 300;
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

void TowerSession::updateLoadout(const game::PlayerIntent& intent)
{
    if (intent.menuUpPressed || intent.menuDownPressed)
    {
        spellLoadoutSection_ = spellLoadoutSection_ == SpellLoadoutSection::Regular
            ? SpellLoadoutSection::Boss
            : SpellLoadoutSection::Regular;
        return;
    }

    if (spellLoadoutSection_ == SpellLoadoutSection::Boss)
    {
        const auto& learnedBossSpells = run_.player().learnedBossSpells;
        if (learnedBossSpells.empty()) return;
        if (intent.menuPreviousPressed)
        {
            selectedBossSpellIndex_ = selectedBossSpellIndex_ == 0U
                ? learnedBossSpells.size() - 1U
                : selectedBossSpellIndex_ - 1U;
        }
        if (intent.menuNextPressed)
            selectedBossSpellIndex_ = (selectedBossSpellIndex_ + 1U) % learnedBossSpells.size();
        if (intent.ultimateSpellPressed
            && run_.equipUltimate(learnedBossSpells[selectedBossSpellIndex_]) && combat_)
        {
            static_cast<void>(combat_->equipUltimateSpell(run_.player().equippedUltimateSpell));
        }
        return;
    }

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
            if (run_.equip(slot, learned[selectedLearnedSpellIndex_]) && combat_)
                static_cast<void>(combat_->equipSpell(slot, run_.player().equippedSpells[slot]));
        }
    }
}

void TowerSession::updateRelicPage(const game::PlayerIntent& intent)
{
    const auto& relics = run_.player().relics;
    if (relics.empty()) return;

    if (intent.menuPreviousPressed)
    {
        selectedRelicIndex_ = selectedRelicIndex_ == 0U
            ? relics.size() - 1U
            : selectedRelicIndex_ - 1U;
    }
    if (intent.menuNextPressed)
    {
        selectedRelicIndex_ = (selectedRelicIndex_ + 1U) % relics.size();
    }
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
