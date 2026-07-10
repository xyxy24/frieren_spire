#include "app/run/RunController.hpp"
#include "app/run/TowerSession.hpp"
#include "game/economy/MerchantSystem.hpp"
#include "game/events/EventSystem.hpp"
#include "game/floors/FloorScheduler.hpp"
#include "game/run/DeterministicRng.hpp"

#include <array>
#include <iostream>
#include <string_view>
#include <vector>

namespace
{
bool expect(const bool condition, const std::string_view message)
{
    if (!condition) std::cerr << "FAILED: " << message << '\n';
    return condition;
}

arcane::game::CombatResult victoryResult(
    const arcane::game::run::ContentId encounterId,
    const int goldAwarded,
    const int playerHealthRemaining = 100)
{
    return {
        arcane::game::CombatOutcome::Victory,
        encounterId,
        1,
        goldAwarded,
        playerHealthRemaining
    };
}

arcane::game::CombatResult defeatResult(const arcane::game::run::ContentId encounterId)
{
    return {arcane::game::CombatOutcome::Defeat, encounterId, 0, 0, 0};
}

bool deterministicStreamsAreIsolated()
{
    using namespace arcane::game::run;
    const auto floorA = deriveFloorSeed(42U, 3U);
    const auto floorB = deriveFloorSeed(42U, 3U);
    return expect(floorA == floorB, "same run and floor seeds must reproduce")
        && expect(deriveStreamSeed(floorA, RandomStream::Reward)
            != deriveStreamSeed(floorA, RandomStream::Layout), "random channels must be isolated");
}

bool completesFiveFloorLoops()
{
    constexpr std::array<arcane::game::run::ContentId, 1> encounters {10U};
    constexpr std::array<arcane::game::run::ContentId, 8> spells {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U};
    arcane::app::RunController run(12345U);

    for (std::uint32_t floor = 0U; floor < 5U; ++floor)
    {
        const auto expectedSeed = arcane::game::run::deriveFloorSeed(12345U, floor);
        const auto& descriptor = run.loadFloor(arcane::game::run::FloorType::Combat, encounters);
        if (!expect(descriptor.seed == expectedSeed, "floor seed must match its index")
            || !expect(run.resolveEncounter(victoryResult(descriptor.encounterIds[0], 5), spells),
                "combat victory must resolve once")) return false;
        const auto beforeReward = run.floorResult();
        if (!expect(run.phase() == arcane::game::run::RunPhase::LootPending,
                "combat victory must wait for loot interaction")
            || !expect(run.openReward(), "loot interaction must open the reward")
            || !expect(!run.openReward(), "reward cannot be opened twice")) return false;
        const auto choice = run.rewardOffer().candidates[0];
        if (!expect(run.chooseReward(choice), "reward choice must apply")
            || !expect(!run.chooseReward(choice), "reward cannot apply twice")
            || !expect(beforeReward.floorComplete && !beforeReward.rewardComplete
                && !beforeReward.stairsAvailable, "floor result must withhold stairs before reward")
            || !expect(run.floorResult().stairsAvailable, "completed reward must expose stairs")
            || !expect(run.useStairs(), "stairs complete the floor")
            || !expect(!run.useStairs(), "stairs transaction cannot repeat")) return false;
    }
    return expect(run.context().floorIndex == 5U, "five transitions must advance exactly five floors")
        && expect(run.player().gold == 25, "gold must be granted once per victory")
        && expect(run.player().learnedSpells.size() == 5U, "each reward should join the learned spell list")
        && expect(!run.player().equippedSpells[0] && !run.player().equippedSpells[1]
            && !run.player().equippedSpells[2], "reward choice must not force a loadout replacement");
}

bool healsHalfMissingRoundedUp()
{
    constexpr std::array<arcane::game::run::ContentId, 1> encounters {10U};
    constexpr std::array<arcane::game::run::ContentId, 3> spells {1U, 2U, 3U};
    arcane::app::RunController run(7U);
    const auto& descriptor = run.loadFloor(arcane::game::run::FloorType::Combat, encounters);
    if (!run.resolveEncounter(victoryResult(descriptor.encounterIds[0], 0, 1), spells)) return false;
    if (!run.openReward() || !run.chooseReward(run.rewardOffer().candidates[0]) || !run.useStairs()) return false;
    return expect(run.player().currentHp == 51, "99 missing HP heals 50 with ceiling rounding");
}

bool failureIsTerminal()
{
    constexpr std::array<arcane::game::run::ContentId, 1> encounters {10U};
    arcane::app::RunController run(9U);
    const auto& descriptor = run.loadFloor(arcane::game::run::FloorType::Combat, encounters);
    if (!expect(!run.resolveEncounter(victoryResult(999U, 0), {}),
        "a stale encounter result must be rejected")) return false;
    return expect(run.resolveEncounter(defeatResult(descriptor.encounterIds[0]), {}),
            "defeat result must be accepted")
        && expect(run.phase() == arcane::game::run::RunPhase::Defeat, "defeat must enter terminal state")
        && expect(!run.resolveEncounter(defeatResult(descriptor.encounterIds[0]), {}),
            "encounter result must settle once");
}

bool nonCombatFloorsSkipCombatRewards()
{
    using namespace arcane::game::run;
    arcane::app::RunController run(55U);

    const auto& eventFloor = run.loadFloor(FloorType::Event, {});
    if (!expect(eventFloor.encounterIds.empty(), "event floors should not create combat encounters")
        || !expect(!run.resolveEncounter(victoryResult(1U, 0), {}),
            "event floors must reject combat results")
        || !expect(run.completeNonCombatFloor(), "event floors should complete without combat rewards")
        || !expect(!run.completeNonCombatFloor(), "non-combat completion cannot repeat")
        || !expect(run.useStairs(), "completed event floors should enable stairs")) return false;

    const auto& merchantFloor = run.loadFloor(FloorType::Merchant, {});
    return expect(merchantFloor.encounterIds.empty(), "merchant floors should not create combat encounters")
        && expect(run.completeNonCombatFloor(), "merchant floors should complete without combat rewards")
        && expect(run.useStairs(), "completed merchant floors should enable stairs")
        && expect(run.context().floorIndex == 2U, "both non-combat floors should advance the run");
}

bool merchantPurchaseIsAtomic()
{
    using namespace arcane::game;
    run::PlayerProgress player;
    player.gold = 20;
    std::vector<economy::StockItem> stock {{55U, economy::ItemKind::Relic, 30, false}};
    const auto failed = economy::purchase(player, stock, 55U);
    if (!expect(failed == economy::PurchaseResult::InsufficientGold, "unaffordable purchase must fail")
        || !expect(player.gold == 20 && player.relics.empty() && !stock[0].sold,
            "failed purchase must not mutate either side")) return false;
    stock[0].price = 15;
    return expect(economy::purchase(player, stock, 55U) == economy::PurchaseResult::Success,
            "affordable purchase must succeed")
        && expect(player.gold == 5 && player.relics == std::vector<run::ContentId> {55U} && stock[0].sold,
            "successful purchase must commit all state");
}

bool eventChoiceIsAtomic()
{
    using namespace arcane::game;
    run::PlayerProgress player;
    player.currentHp = 40;
    player.gold = 5;
    events::EventTransaction event;
    const std::array choices {events::EventChoice {1U, -50, 20, 0U}, events::EventChoice {2U, -10, -5, 77U}};
    if (!expect(event.choose(player, choices, 1U) == events::EventResult::InvalidOutcome,
            "lethal event choice must be rejected")
        || !expect(player.currentHp == 40 && player.gold == 5 && !event.resolved(),
            "rejected event must not partially mutate state")) return false;
    return expect(event.choose(player, choices, 2U) == events::EventResult::Success, "valid event choice must apply")
        && expect(player.currentHp == 30 && player.gold == 0 && player.relics[0] == 77U,
            "event effects must commit together")
        && expect(event.choose(player, choices, 2U) == events::EventResult::AlreadyResolved,
            "event can resolve only once");
}

bool thirdBossVictorySettlesOnce()
{
    constexpr std::array<arcane::game::run::ContentId, 1> boss {99U};
    constexpr std::array<arcane::game::run::ContentId, 5> rewards {1U, 2U, 3U, 4U, 5U};
    arcane::app::RunController run(88U);
    for (int defeated = 0; defeated < 3; ++defeated)
    {
        static_cast<void>(run.loadFloor(arcane::game::run::FloorType::Boss, boss));
        if (!run.resolveEncounter(victoryResult(99U, 0), rewards)) return false;
        if (!run.openReward() || !run.chooseReward(run.rewardOffer().candidates[0]) || !run.useStairs()) return false;
    }
    return expect(run.context().bossesDefeated == 3U, "boss progress must advance once per boss floor")
        && expect(run.phase() == arcane::game::run::RunPhase::Victory, "third boss must end the run in victory")
        && expect(!run.useStairs(), "victory cannot be settled twice");
}

bool depletedRewardUsesGoldFallback()
{
    using namespace arcane::game;
    constexpr std::array<run::ContentId, 1> encounters {10U};
    constexpr std::array<run::ContentId, 3> spells {1U, 2U, 3U};
    run::PlayerProgress player;
    player.learnedSpells = {1U};
    arcane::app::RunController run(42U, player);
    const auto& floor = run.loadFloor(run::FloorType::Combat, encounters);
    if (!run.resolveEncounter(victoryResult(floor.encounterIds[0], 5), spells)) return false;
    if (!run.openReward()) return false;
    return expect(run.rewardOffer().kind == rewards::RewardKind::GoldFallback,
            "depleted reward pool must use fallback")
        && expect(run.claimFallbackReward(), "fallback must be claimable")
        && expect(!run.claimFallbackReward(), "fallback cannot be claimed twice")
        && expect(run.player().gold == 20, "fallback must add configured gold once");
}

bool merchantStockAndFloorScheduleReproduce()
{
    using namespace arcane::game;
    const std::array catalog {economy::CatalogItem {1U, economy::ItemKind::Spell, 10},
        economy::CatalogItem {2U, economy::ItemKind::Spell, 15},
        economy::CatalogItem {3U, economy::ItemKind::Relic, 20},
        economy::CatalogItem {4U, economy::ItemKind::Relic, 25}};
    run::PlayerProgress player;
    player.learnedSpells = {1U};
    const auto leftStock = economy::generateStock(catalog, player, 99U);
    const auto rightStock = economy::generateStock(catalog, player, 99U);
    if (!expect(leftStock.size() == 3U && leftStock[0].id == rightStock[0].id
            && leftStock[1].id == rightStock[1].id && leftStock[2].id == rightStock[2].id,
        "merchant stock must reproduce and exclude owned content")) return false;

    floors::FloorScheduler left({3U, 4U, 3U});
    floors::FloorScheduler right({3U, 4U, 3U});
    run::RunContext context {77U, 0U, 0U, 1U, 0U};
    for (std::uint32_t floor = 0U; floor < 18U; ++floor)
    {
        context.floorIndex = floor;
        context.floorSeed = run::deriveFloorSeed(context.runSeed, floor);
        const auto leftType = left.next(context);
        const auto rightType = right.next(context);
        if (!expect(leftType == rightType, "floor schedule must reproduce")) return false;
        if ((floor + 1U) % 3U == 0U && !expect(leftType == run::FloorType::Boss,
            "configured boss floors must take priority")) return false;
    }
    return true;
}

bool towerSessionKeepsLoadoutOptionalAndRequiresSpatialStairsInteraction()
{
    arcane::app::TowerSessionConfig config;
    config.enemySpawn = {210.0F, 576.0F};
    config.normalEnemyHealth = 15;
    config.bossEnemyHealth = 15;
    config.enableSpecialFloors = false;
    config.staircaseBounds = {300.0F, 560.0F, 90.0F, 80.0F};
    arcane::app::TowerSession tower(123U, config);

    arcane::game::PlayerIntent toggleLoadout;
    toggleLoadout.toggleLoadoutPressed = true;
    tower.update(toggleLoadout, 0.01F);
    if (!expect(tower.loadoutOpen(), "loadout should be available during combat")) return false;
    tower.update(toggleLoadout, 0.01F);
    if (!expect(!tower.loadoutOpen(), "loadout should close without changing the run phase")
        || !expect(tower.run().phase() == arcane::game::run::RunPhase::InEncounter,
            "closing loadout should resume the underlying combat phase")) return false;

    arcane::game::PlayerIntent attack;
    attack.attackPressed = true;
    tower.update(attack, 0.01F);
    if (!expect(tower.run().phase() == arcane::game::run::RunPhase::LootPending,
            "combat victory should create a pending loot drop")
        || !expect(tower.lootDropBounds().has_value(), "enemy defeat should expose a loot drop")) return false;

    arcane::game::PlayerIntent prematureRewardInput;
    prematureRewardInput.spellPressed[0] = true;
    tower.update(prematureRewardInput, 0.01F);
    if (!expect(tower.run().phase() == arcane::game::run::RunPhase::LootPending,
            "spell input must not choose a reward before loot interaction")
        || !expect(tower.run().player().learnedSpells.empty(),
            "premature reward input must not teach a spell")) return false;

    arcane::game::PlayerIntent moveToLoot;
    moveToLoot.moveAxis = 1.0F;
    tower.update(moveToLoot, 0.10F);
    arcane::game::PlayerIntent interact;
    interact.interactPressed = true;
    tower.update(interact, 0.01F);
    if (!expect(tower.run().phase() == arcane::game::run::RunPhase::Reward,
        "interacting with the loot drop should open the reward phase")) return false;

    const auto candidates = tower.rewardCandidates();
    if (!expect(candidates.has_value(), "reward phase should expose three candidates")) return false;

    arcane::game::PlayerIntent choose;
    choose.spellPressed[0] = true;
    tower.update(choose, 0.01F);
    if (!expect(tower.run().phase() == arcane::game::run::RunPhase::FloorComplete,
            "choosing a reward should return to the completed floor")
        || !expect(tower.run().player().learnedSpells.back() == (*candidates)[0],
            "chosen reward should join learned spells")
        || !expect(!tower.run().player().equippedSpells[0]
            && !tower.run().player().equippedSpells[1]
            && !tower.run().player().equippedSpells[2],
            "chosen reward should not automatically replace an equipped spell")) return false;

    tower.update(toggleLoadout, 0.01F);
    if (!expect(tower.loadoutOpen(), "loadout should open on the completed floor")
        || !expect(tower.selectedLearnedSpell() == (*candidates)[0],
            "newly learned reward should be selectable in loadout")) return false;

    arcane::game::PlayerIntent equip;
    equip.spellPressed[1] = true;
    tower.update(equip, 0.01F);
    if (!expect(tower.run().player().equippedSpells[1] == (*candidates)[0],
        "chosen spell should equip into the requested slot")) return false;

    tower.update(toggleLoadout, 0.01F);
    if (!expect(!tower.loadoutOpen(), "closing loadout should preserve the equipped spell")) return false;

    tower.update(interact, 0.01F);
    if (!expect(tower.run().phase() == arcane::game::run::RunPhase::FloorComplete,
        "interacting away from the staircase must not change floors")) return false;

    arcane::game::PlayerIntent moveToStairs;
    moveToStairs.moveAxis = 1.0F;
    tower.update(moveToStairs, 0.5F);
    tower.update(interact, 0.01F);
    return expect(tower.run().phase() == arcane::game::run::RunPhase::InEncounter,
            "interacting inside the staircase should start the next floor")
        && expect(tower.run().context().floorIndex == 1U, "stairs should advance exactly one floor")
        && expect(tower.combat() != nullptr, "the next floor should own a fresh combat session");
}
}

int main()
{
    const bool passed = deterministicStreamsAreIsolated() && completesFiveFloorLoops()
        && healsHalfMissingRoundedUp() && failureIsTerminal() && nonCombatFloorsSkipCombatRewards()
        && merchantPurchaseIsAtomic()
        && eventChoiceIsAtomic() && thirdBossVictorySettlesOnce()
        && depletedRewardUsesGoldFallback() && merchantStockAndFloorScheduleReproduce()
        && towerSessionKeepsLoadoutOptionalAndRequiresSpatialStairsInteraction();
    if (!passed) return 1;
    std::cout << "All run flow tests passed.\n";
    return 0;
}
