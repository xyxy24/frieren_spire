#include "app/run/RunController.hpp"
#include "app/run/TowerSession.hpp"
#include "game/economy/MerchantSystem.hpp"
#include "game/events/EventSystem.hpp"
#include "game/floors/FloorScheduler.hpp"
#include "game/progression/ProgressionSystem.hpp"
#include "game/rewards/RewardSystem.hpp"
#include "game/run/DeterministicRng.hpp"
#include "game/relics/RelicSystem.hpp"
#include "game/spells/SpellSystem.hpp"
#include "presentation/viewmodel/LoadoutViewModel.hpp"

#include <array>
#include <algorithm>
#include <cmath>
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

bool progressionRewardsAndPowerGrowthAreDeterministic()
{
    using namespace arcane::game;
    constexpr std::array<run::ContentId, 8> actTwo {
        1001U, 1011U, 1016U, 1017U, 1019U, 1020U, 1021U, 1030U};
    constexpr std::array<run::ContentId, 16> unlocked {
        1004U, 1005U, 1006U, 1008U, 1009U, 1027U, 1028U, 1029U,
        1001U, 1011U, 1016U, 1017U, 1019U, 1020U, 1021U, 1030U};
    run::PlayerProgress player;
    player.learnedSpells = {1004U, 1005U, 1006U};
    player.spellMasteries = {{1004U, 1U}, {1005U, 1U}, {1006U, 1U}};
    player.equippedSpells = {1004U, 1005U, 1006U};

    const auto left = rewards::generateProgressionOffer(actTwo, unlocked, player, 2U, 991U);
    const auto right = rewards::generateProgressionOffer(actTwo, unlocked, player, 2U, 991U);
    const bool containsUpgrade = std::any_of(left.candidates.begin(), left.candidates.end(),
        [&](const auto id) { return std::find(player.equippedSpells.begin(),
            player.equippedSpells.end(), id) != player.equippedSpells.end(); });
    const bool containsActTwoSpell = std::any_of(left.candidates.begin(), left.candidates.end(),
        [&](const auto id) { return std::find(actTwo.begin(), actTwo.end(), id) != actTwo.end(); });
    if (!expect(left.candidates == right.candidates,
            "progression rewards must reproduce for the same seed")
        || !expect(containsUpgrade,
            "Act II rewards must include an upgrade for an equipped spell")
        || !expect(containsActTwoSpell,
            "Act II rewards must include newly unlocked Act II magic")
        || !expect(!progression::upgradeSpell(player, 1004U, 1U),
            "Act I must keep regular spells at rank one")
        || !expect(progression::upgradeSpell(player, 1004U, 2U)
                && progression::spellRank(player, 1004U) == 2U,
            "Act II must unlock rank-two mastery")
        || !expect(!progression::upgradeSpell(player, 1004U, 2U),
            "Act II must not grant rank three early")
        || !expect(progression::upgradeSpell(player, 1004U, 3U)
                && progression::spellRank(player, 1004U) == 3U,
            "Act III must unlock rank-three mastery")) return false;

    const std::array<std::optional<std::uint32_t>, 3> ids {1004U, std::nullopt, std::nullopt};
    spells::SpellSystem base(ids, std::nullopt, {1U, 1U, 1U});
    spells::SpellSystem mastered(ids, std::nullopt, {3U, 1U, 1U});
    const Aabb target {100.0F, 0.0F, 40.0F, 64.0F};
    const auto baseCast = base.tryCast(0U, {0.0F, 0.0F}, 1.0F, target);
    const auto masteredCast = mastered.tryCast(0U, {0.0F, 0.0F}, 1.0F, target);
    if (!expect(masteredCast.masteryPowerMultiplier > baseCast.masteryPowerMultiplier
            && masteredCast.effectBounds.width > baseCast.effectBounds.width
            && mastered.view()[0].cooldownDuration < base.view()[0].cooldownDuration,
        "mastery must increase power and area while reducing cooldown")) return false;

    run::PlayerProgress power;
    run::PlayerProgress haste;
    run::PlayerProgress defense;
    return expect(progression::applyBreakthrough(power,
            progression::BreakthroughType::Power)
            && progression::applyBreakthrough(power, progression::BreakthroughType::Power)
            && std::abs(progression::regularDamageMultiplier(power) - 1.17F) < 0.001F,
            "two power breakthroughs must use the documented diminishing total")
        && expect(progression::applyBreakthrough(haste,
            progression::BreakthroughType::Haste)
            && progression::regularCooldownMultiplier(haste) < 1.0F,
            "haste breakthrough must shorten regular cooldowns")
        && expect(progression::applyBreakthrough(defense,
            progression::BreakthroughType::Defense)
            && progression::applyBreakthrough(defense,
                progression::BreakthroughType::Defense)
            && defense.maxHp == 125 && defense.currentHp == 125
            && progression::startingShield(defense) == 14,
            "defense breakthroughs must grant persistent HP and combat shield growth");
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

bool bossRewardUnlocksOnlyTheUltimateCollection()
{
    constexpr std::array<arcane::game::run::ContentId, 1> encounters {77U};
    constexpr std::array<arcane::game::run::ContentId, 3> bossSpells {2001U, 2002U, 2003U};
    arcane::app::RunController run(88U);
    const auto& descriptor = run.loadFloor(arcane::game::run::FloorType::Boss, encounters);
    if (!expect(run.resolveEncounter(victoryResult(descriptor.encounterIds[0], 0), bossSpells),
            "boss victory must create a boss reward")
        || !expect(run.openReward(), "boss loot must open its reward")
        || !expect(run.chooseReward(run.rewardOffer().candidates[0]),
            "boss spell choice must apply")) return false;
    if (!expect(run.phase() == arcane::game::run::RunPhase::Breakthrough,
            "the first Boss spell must be followed by a mana breakthrough")
        || !expect(run.chooseBreakthrough(
            arcane::game::progression::BreakthroughType::Power),
            "a mana breakthrough choice must complete Boss progression")) return false;

    const auto selected = run.player().learnedBossSpells[0];
    return expect(run.player().ultimateSpellUnlocked,
            "first boss spell must unlock the ultimate slot")
        && expect(run.player().learnedSpells.empty(),
            "boss rewards must not enter the regular learned list")
        && expect(run.player().learnedBossSpells.size() == 1U,
            "boss reward must enter the dedicated boss spell list")
        && expect(!run.player().equippedUltimateSpell,
            "boss reward must not force automatic equipment")
        && expect(!run.equip(0U, selected),
            "boss spell must not equip into a regular slot")
        && expect(run.equipUltimate(selected)
            && run.player().equippedUltimateSpell == selected,
            "learned boss spell must equip into the unlocked ultimate slot");
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

bool meteorRestChoiceRestoresToMaximum()
{
    using namespace arcane::game;
    run::PlayerProgress player;
    player.currentHp = 23;
    player.maxHp = 100;
    events::EventTransaction event;
    const std::array choices {events::EventChoice {5102U, 0, 0, 0U, 0, 0U, true}};
    return expect(event.choose(player, choices, 5102U) == events::EventResult::Success,
            "meteor rest choice must resolve")
        && expect(player.currentHp == player.maxHp,
            "meteor rest choice must restore current HP to maximum");
}

bool southernHeroChoicesApplyPersistentProgression()
{
    using namespace arcane::game;
    run::PlayerProgress boosted;
    events::EventTransaction weakness;
    const std::array weaknessChoice {events::EventChoice {
        5301U, 0, 0, 0U, 0, 0U, false, true, false}};
    if (!expect(weakness.choose(boosted, weaknessChoice, 5301U, 2U)
            == events::EventResult::Success && boosted.southernHeroBossDamageBoost,
            "Southern Hero weakness answer must persist the final-boss damage boost"))
        return false;

    run::PlayerProgress spells;
    spells.learnedSpells = {1001U, 1002U};
    progression::registerLearnedSpell(spells, 1001U);
    progression::registerLearnedSpell(spells, 1002U);
    events::EventTransaction mastery;
    const std::array masteryChoice {events::EventChoice {
        5302U, 0, 0, 0U, 0, 0U, false, false, true}};
    return expect(mastery.choose(spells, masteryChoice, 5302U, 2U)
            == events::EventResult::Success,
            "Southern Hero magic answer must resolve")
        && expect(progression::spellRank(spells, 1001U) == 2U
                && progression::spellRank(spells, 1002U) == 2U,
            "Southern Hero magic answer must upgrade every learned spell once");
}

bool runTracksTriggeredEventsWithoutDuplicates()
{
    arcane::app::RunController run(808U);
    if (!expect(!run.eventTriggered(1U),
        "a new run must begin with every event untriggered")) return false;
    run.markEventTriggered(1U);
    run.markEventTriggered(1U);
    return expect(run.eventTriggered(1U),
            "triggered event state must persist for the run")
        && expect(!run.eventTriggered(0U) && !run.eventTriggered(2U),
            "triggering one event must not consume other event definitions");
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
        if (!run.openReward() || !run.chooseReward(run.rewardOffer().candidates[0])) return false;
        if (defeated < 2 && !run.chooseBreakthrough(
                arcane::game::progression::BreakthroughType::Power)) return false;
        if (!run.useStairs()) return false;
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

bool firstClassBadgeRerollsOncePerAct()
{
    constexpr std::array<arcane::game::run::ContentId, 1> encounters {10U};
    constexpr std::array<arcane::game::run::ContentId, 4> damage {1004U, 1005U, 1006U, 1009U};
    constexpr std::array<arcane::game::run::ContentId, 4> control {1008U, 1011U, 1018U, 1020U};
    constexpr std::array<arcane::game::run::ContentId, 8> full {
        1004U, 1005U, 1006U, 1009U, 1008U, 1011U, 1018U, 1020U};
    arcane::game::run::PlayerProgress player;
    player.relics = {arcane::game::relics::FirstClassBadgeId};
    arcane::app::RunController run(2468U, player);
    const auto& floor = run.loadFloor(arcane::game::run::FloorType::Combat, encounters);
    if (!run.resolveEncounter(victoryResult(floor.encounterIds[0], 0), full, damage, control)
        || !run.openReward()) return false;
    return expect(run.rerollRegularReward(damage, control, full),
            "First-Class Badge must reroll a regular reward")
        && expect(run.player().rewardRerollUsed[0],
            "reroll must be recorded for the current act")
        && expect(!run.rerollRegularReward(damage, control, full),
            "First-Class Badge must not reroll twice in one act");
}

bool categorizedRewardsGuaranteeCombatRoles()
{
    constexpr std::array<arcane::game::run::ContentId, 3> damage {1003U, 1004U, 1005U};
    constexpr std::array<arcane::game::run::ContentId, 3> control {1008U, 1016U, 1018U};
    constexpr std::array<arcane::game::run::ContentId, 7> full {
        1001U, 1003U, 1004U, 1005U, 1008U, 1016U, 1018U
    };
    const auto offer = arcane::game::rewards::generateCategorizedOffer(
        damage, control, full, {}, 42U);
    return expect(std::find(damage.begin(), damage.end(), offer.candidates[0]) != damage.end(),
            "categorized rewards must put a damage spell first")
        && expect(std::find(control.begin(), control.end(), offer.candidates[1]) != control.end(),
            "categorized rewards must put a control or counter spell second")
        && expect(offer.candidates[0] != offer.candidates[1]
                && offer.candidates[0] != offer.candidates[2]
                && offer.candidates[1] != offer.candidates[2],
            "categorized rewards must not contain duplicates");
}

bool defaultScheduleBuildsThreeFiveFloorActs()
{
    using namespace arcane::game;
    floors::FloorScheduler scheduler;
    run::RunContext context {31415U, 0U, 0U, 1U, 0U};
    std::uint32_t bossCount = 0U;
    std::uint32_t combatCount = 0U;
    std::uint32_t specialCount = 0U;

    for (std::uint32_t floor = 0U; floor < 15U; ++floor)
    {
        context.floorIndex = floor;
        context.floorSeed = run::deriveFloorSeed(context.runSeed, floor);
        const auto type = scheduler.next(context);
        const bool expectedBoss = floor == 4U || floor == 9U || floor == 14U;
        if (!expect((type == run::FloorType::Boss) == expectedBoss,
                "default schedule must place bosses only on floors five, ten and fifteen")) return false;

        if (type == run::FloorType::Boss)
        {
            ++bossCount;
            if (!expect(combatCount >= 2U,
                    "each act must provide at least two normal combats before its boss")
                || !expect(specialCount >= 1U,
                    "each act must provide at least one merchant or event before its boss")) return false;
            combatCount = 0U;
            specialCount = 0U;
        }
        else if (type == run::FloorType::Combat)
            ++combatCount;
        else
            ++specialCount;
    }

    return expect(bossCount == 3U, "the fifteen-floor default run must contain exactly three bosses")
        && expect(arcane::app::TowerSessionConfig {}.floorsPerBoss == 5U,
            "tower sessions must use the five-floor act by default");
}

bool towerSessionKeepsLoadoutOptionalAndRequiresSpatialStairsInteraction()
{
    namespace ui = arcane::presentation::viewmodel;
    arcane::app::TowerSessionConfig config;
    config.enemySpawn = {210.0F, 576.0F};
    config.normalEnemyHealth = 15;
    config.bossEnemyHealth = 15;
    config.enableSpecialFloors = false;
    config.staircaseBounds = {300.0F, 560.0F, 90.0F, 80.0F};
    arcane::app::TowerSession tower(123U, config);
    ui::LoadoutViewModel loadout;

    arcane::game::PlayerIntent toggleLoadout;
    toggleLoadout.toggleLoadoutPressed = true;
    static_cast<void>(loadout.handleInput(toggleLoadout, tower));
    if (!expect(loadout.open(), "loadout should be available during combat")) return false;
    static_cast<void>(loadout.handleInput(toggleLoadout, tower));
    if (!expect(!loadout.open(), "loadout should close without changing the run phase")
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

    static_cast<void>(loadout.handleInput(toggleLoadout, tower));
    if (!expect(loadout.open(), "loadout should open on the completed floor")
        || !expect(loadout.selectedSpell(tower.run().player()) == (*candidates)[0],
            "newly learned reward should be selectable in loadout")) return false;

    arcane::game::PlayerIntent equip;
    equip.spellPressed[1] = true;
    static_cast<void>(loadout.handleInput(equip, tower));
    if (!expect(tower.run().player().equippedSpells[1] == (*candidates)[0],
        "chosen spell should equip into the requested slot")) return false;

    static_cast<void>(loadout.handleInput(toggleLoadout, tower));
    if (!expect(!loadout.open(), "closing loadout should preserve the equipped spell")) return false;

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
    const bool passed = deterministicStreamsAreIsolated()
        && progressionRewardsAndPowerGrowthAreDeterministic()
        && firstClassBadgeRerollsOncePerAct()
        && categorizedRewardsGuaranteeCombatRoles()
        && completesFiveFloorLoops()
        && healsHalfMissingRoundedUp() && bossRewardUnlocksOnlyTheUltimateCollection()
        && failureIsTerminal() && nonCombatFloorsSkipCombatRewards()
        && merchantPurchaseIsAtomic()
        && eventChoiceIsAtomic() && meteorRestChoiceRestoresToMaximum()
        && southernHeroChoicesApplyPersistentProgression()
        && runTracksTriggeredEventsWithoutDuplicates() && thirdBossVictorySettlesOnce()
        && depletedRewardUsesGoldFallback() && merchantStockAndFloorScheduleReproduce()
        && defaultScheduleBuildsThreeFiveFloorActs()
        && towerSessionKeepsLoadoutOptionalAndRequiresSpatialStairsInteraction();
    if (!passed) return 1;
    std::cout << "All run flow tests passed.\n";
    return 0;
}
