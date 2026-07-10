#include "app/run/RunController.hpp"
#include "game/economy/MerchantSystem.hpp"
#include "game/events/EventSystem.hpp"
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
        if (!expect(run.chooseReward(choice), "reward choice must apply")
            || !expect(!run.chooseReward(choice), "reward cannot apply twice")
            || !expect(run.equip(floor % 3U, choice), "learned spell can be equipped")
            || !expect(run.finishLoadout(), "loadout can finish")
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
}

int main()
{
    const bool passed = deterministicStreamsAreIsolated() && completesFiveFloorLoops()
        && healsHalfMissingRoundedUp() && failureIsTerminal() && merchantPurchaseIsAtomic()
        && eventChoiceIsAtomic() && thirdBossVictorySettlesOnce();
    if (!passed) return 1;
    std::cout << "All run flow tests passed.\n";
    return 0;
}
