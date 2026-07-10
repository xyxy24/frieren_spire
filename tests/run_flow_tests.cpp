#include "app/run/RunController.hpp"
#include "game/economy/MerchantSystem.hpp"
#include "game/events/EventSystem.hpp"
#include "game/floors/FloorScheduler.hpp"
#include "game/run/DeterministicRng.hpp"
#include "presentation/FlowViewState.hpp"

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
            || !expect(run.resolveEncounter(true, 5, spells), "combat victory must resolve once")) return false;
        const auto choice = run.rewardOffer().candidates[0];
        const auto waitingForReward = run.floorResult();
        if (!expect(run.chooseReward(choice), "reward choice must apply")
            || !expect(!run.chooseReward(choice), "reward cannot apply twice")
            || !expect(run.equip(floor % 3U, choice), "learned spell can be equipped")
            || !expect(waitingForReward.floorComplete && !waitingForReward.rewardComplete
                && !waitingForReward.stairsAvailable, "floor result must withhold stairs until reward is complete")
            || !expect(run.finishLoadout(), "loadout can finish")
            || !expect(run.floorResult().stairsAvailable, "completed reward must expose stairs")
            || !expect(run.useStairs(), "stairs complete the floor")
            || !expect(!run.useStairs(), "stairs transaction cannot repeat")) return false;
    }
    return expect(run.context().floorIndex == 5U, "five transitions must advance exactly five floors")
        && expect(run.player().gold == 25, "gold must be granted once per victory");
}

bool healsHalfMissingRoundedUp()
{
    constexpr std::array<arcane::game::run::ContentId, 1> encounters {10U};
    constexpr std::array<arcane::game::run::ContentId, 3> spells {1U, 2U, 3U};
    arcane::app::RunController run(7U);
    run.setCurrentHpForFlow(1);
    static_cast<void>(run.loadFloor(arcane::game::run::FloorType::Combat, encounters));
    if (!run.resolveEncounter(true, 0, spells)) return false;
    if (!run.chooseReward(run.rewardOffer().candidates[0]) || !run.finishLoadout() || !run.useStairs()) return false;
    return expect(run.player().currentHp == 51, "99 missing HP heals 50 with ceiling rounding");
}

bool failureIsTerminal()
{
    constexpr std::array<arcane::game::run::ContentId, 1> encounters {10U};
    arcane::app::RunController run(9U);
    static_cast<void>(run.loadFloor(arcane::game::run::FloorType::Combat, encounters));
    return expect(run.resolveEncounter(false, 0, {}), "defeat result must be accepted")
        && expect(run.phase() == arcane::game::run::RunPhase::Defeat, "defeat must enter terminal state")
        && expect(!run.resolveEncounter(false, 0, {}), "encounter result must settle once");
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
        if (!run.resolveEncounter(true, 0, rewards)) return false;
        if (!run.chooseReward(run.rewardOffer().candidates[0]) || !run.finishLoadout() || !run.useStairs()) return false;
    }
    return expect(run.context().bossesDefeated == 3U, "boss progress must advance once per boss floor")
        && expect(run.phase() == arcane::game::run::RunPhase::Victory, "third boss must end the run in victory")
        && expect(!run.useStairs(), "victory cannot be settled twice");
}

bool schedulerIsDeterministicAndGuaranteed()
{
    using namespace arcane::game;
    floors::FloorScheduler left({4U, 3U});
    floors::FloorScheduler right({4U, 3U});
    constexpr std::array<std::uint32_t, 1> bossFloors {6U};
    run::RunContext context {777U, 0U, 0U, 1U, 0U};
    for (std::uint32_t floor = 0U; floor < 20U; ++floor)
    {
        context.floorIndex = floor;
        context.floorSeed = run::deriveFloorSeed(context.runSeed, floor);
        const auto leftType = left.next(context, bossFloors);
        const auto rightType = right.next(context, bossFloors);
        if (!expect(leftType == rightType, "same seed and schedule state must reproduce")
            || !expect(left.state().floorsSinceMerchant <= 4U, "merchant gap guarantee must hold")
            || !expect(left.state().floorsSinceEvent <= 3U, "event gap guarantee must hold")) return false;
        if (floor == 6U && !expect(leftType == run::FloorType::Boss, "configured boss floor must take priority")) return false;
    }
    return true;
}

bool depletedRewardsUseGoldFallback()
{
    using namespace arcane::game;
    constexpr std::array<run::ContentId, 1> encounter {10U};
    constexpr std::array<run::ContentId, 3> pool {1U, 2U, 3U};
    run::PlayerProgress player;
    player.learnedSpells = {1U};
    arcane::app::RunController run(12U, player);
    const auto& floor = run.loadFloor(run::FloorType::Combat, encounter);
    if (!run.resolveEncounter(victoryResult(floor.encounterIds[0], 5), pool)) return false;
    return expect(run.rewardOffer().kind == rewards::RewardKind::GoldFallback,
            "insufficient spell candidates must create fallback")
        && expect(run.claimFallbackReward(), "fallback reward must be claimable")
        && expect(!run.claimFallbackReward(), "fallback reward cannot be claimed twice")
        && expect(run.player().gold == 20, "combat and fallback gold must both apply once");
}

bool merchantStockIsDeterministic()
{
    using namespace arcane::game;
    const std::array catalog {
        economy::CatalogItem {1U, economy::ItemKind::Spell, 10},
        economy::CatalogItem {2U, economy::ItemKind::Spell, 20},
        economy::CatalogItem {3U, economy::ItemKind::Relic, 30},
        economy::CatalogItem {4U, economy::ItemKind::Relic, 40}
    };
    run::PlayerProgress player;
    player.learnedSpells = {1U};
    const auto seed = run::deriveStreamSeed(99U, run::RandomStream::Merchant);
    const auto left = economy::generateStock(catalog, player, seed);
    const auto right = economy::generateStock(catalog, player, seed);
    return expect(left.size() == 3U && right.size() == 3U, "merchant must generate requested stock")
        && expect(left[0].id == right[0].id && left[1].id == right[1].id && left[2].id == right[2].id,
            "merchant stock must reproduce from its stream")
        && expect(left[0].id != 1U && left[1].id != 1U && left[2].id != 1U,
            "merchant must exclude owned content");
}

bool presentationStateIsReadOnlyAndPauseSafe()
{
    using namespace arcane;
    game::run::PlayerProgress player;
    player.gold = 9;
    game::run::RunContext context {1U, 2U, 3U, 2U, 1U};
    game::PlayerStateView combat;
    combat.currentHealth = 45;
    combat.maximumHealth = 100;
    const auto hud = presentation::makeHudState(player, context, &combat);
    presentation::FlowViewController view;
    if (!expect(hud.currentHp == 45 && hud.gold == 9 && hud.floorIndex == 3U,
            "HUD snapshot must combine authoritative combat and run state")
        || !expect(view.togglePause() && view.screen() == presentation::FlowScreen::Pause,
            "gameplay screen must support pause")) return false;
    view.sync(game::run::RunPhase::Reward);
    return expect(view.screen() == presentation::FlowScreen::Reward && !view.gameplayPaused(),
            "modal flow screens must clear gameplay pause")
        && expect(!view.togglePause(), "non-gameplay screens cannot be paused");
}
}

int main()
{
    const bool passed = deterministicStreamsAreIsolated() && completesFiveFloorLoops()
<<<<<<< Updated upstream
        && healsHalfMissingRoundedUp() && failureIsTerminal() && merchantPurchaseIsAtomic()
        && eventChoiceIsAtomic() && thirdBossVictorySettlesOnce();
=======
        && healsHalfMissingRoundedUp() && failureIsTerminal() && nonCombatFloorsSkipCombatRewards()
        && merchantPurchaseIsAtomic()
        && eventChoiceIsAtomic() && thirdBossVictorySettlesOnce()
        && schedulerIsDeterministicAndGuaranteed() && depletedRewardsUseGoldFallback()
        && merchantStockIsDeterministic() && presentationStateIsReadOnlyAndPauseSafe();
>>>>>>> Stashed changes
    if (!passed) return 1;
    std::cout << "All run flow tests passed.\n";
    return 0;
}
