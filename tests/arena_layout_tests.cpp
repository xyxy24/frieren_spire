#include "game/floors/ArenaLayout.hpp"
#include "game/run/DeterministicRng.hpp"

#include <iostream>
#include <string_view>

namespace
{
bool expect(const bool condition, const std::string_view message)
{
    if (!condition)
    {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }
    return true;
}

arcane::game::run::RunContext contextForAct(const std::uint32_t act)
{
    constexpr arcane::game::run::Seed RunSeed = 0xA11CE5EEDULL;
    const std::uint32_t floorIndex = (act - 1U) * 5U;
    return {RunSeed, arcane::game::run::deriveFloorSeed(RunSeed, floorIndex),
        floorIndex, act, act - 1U};
}

bool everyCatalogLayoutIsValid()
{
    for (const auto& layout : arcane::game::floors::allArenaLayouts())
    {
        if (!expect(arcane::game::floors::validateArenaLayout(layout),
            "every catalog arena must satisfy the platform bounds contract"))
            return false;
    }
    return true;
}

bool specialFloorsAlwaysUseSafeRooms()
{
    for (std::uint32_t act = 1U; act <= 3U; ++act)
    {
        const auto context = contextForAct(act);
        for (const auto type : {arcane::game::run::FloorType::Merchant,
            arcane::game::run::FloorType::Event})
        {
            const auto id = arcane::game::floors::selectArenaId(context, type);
            const auto& layout = arcane::game::floors::arenaLayout(id);
            if (!expect(id == act * 100U + 10U,
                    "merchant and event floors must select the act SafeRoom")
                || !expect(layout.safeRoom && layout.oneWayPlatforms.empty(),
                    "SafeRoom must remain completely flat"))
                return false;
        }
    }
    return true;
}

bool combatAndBossSelectionsStayInsideTheirAct()
{
    for (std::uint32_t act = 1U; act <= 3U; ++act)
    {
        const auto context = contextForAct(act);
        const auto first = arcane::game::floors::selectArenaId(
            context, arcane::game::run::FloorType::Combat);
        const auto repeated = arcane::game::floors::selectArenaId(
            context, arcane::game::run::FloorType::Combat);
        if (!expect(first == repeated, "arena selection must be deterministic")
            || !expect(first >= act * 100U + 11U && first <= act * 100U + 14U,
                "combat arena must come from the current act pool")
            || !expect(arcane::game::floors::selectArenaId(
                    context, arcane::game::run::FloorType::Boss) == act * 100U + 15U,
                "boss floor must select the act-specific boss room"))
            return false;
    }
    return true;
}

bool auraBossPlatformsRemainClearlyAboveGround()
{
    constexpr float GroundTop = 640.0F;
    constexpr float MinimumBossPlatformRise = 96.0F;
    const auto& boss = arcane::game::floors::arenaLayout(115U);
    if (!expect(boss.oneWayPlatforms.size() == 2U,
        "Aura boss room should retain two symmetric side platforms"))
        return false;
    for (const auto& platform : boss.oneWayPlatforms)
    {
        if (!expect(GroundTop - platform.top >= MinimumBossPlatformRise,
            "Aura boss platforms must remain visually separated from the base ground"))
            return false;
    }
    return true;
}

bool tacticalSpawnPointsFollowPlatformGeometry()
{
    const auto& layout = arcane::game::floors::arenaLayout(214U);
    const auto first = arcane::game::floors::enemySpawnPoints(layout);
    const auto repeated = arcane::game::floors::enemySpawnPoints(layout);
    if (!expect(first.size() == 3U + layout.oneWayPlatforms.size() * 2U,
            "each platform must contribute one elevated and one flying spawn")
        || !expect(first.size() == repeated.size(),
            "tactical spawn generation must be deterministic"))
        return false;

    std::size_t elevatedCount {};
    std::size_t flyingCount {};
    for (std::size_t index = 0U; index < first.size(); ++index)
    {
        const auto& point = first[index];
        const auto& repeatedPoint = repeated[index];
        if (!expect(point.kind == repeatedPoint.kind
                && point.position.x == repeatedPoint.position.x
                && point.position.y == repeatedPoint.position.y,
                "repeated spawn generation must preserve ordering and coordinates"))
            return false;
        if (point.kind == arcane::game::floors::ArenaSpawnKind::ElevatedEnemy)
        {
            ++elevatedCount;
            if (!expect(point.movementBounds.groundTop == point.position.y,
                    "elevated spawn must use its platform top as lane ground"))
                return false;
        }
        else if (point.kind == arcane::game::floors::ArenaSpawnKind::FlyingEnemy)
            ++flyingCount;
    }
    return expect(elevatedCount == layout.oneWayPlatforms.size(),
            "all platforms must expose elevated lanes")
        && expect(flyingCount == layout.oneWayPlatforms.size(),
            "all platforms must expose aerial approach points")
        && expect(arcane::game::floors::enemySpawnPoints(
                arcane::game::floors::arenaLayout(210U)).size() == 3U,
            "safe rooms must expose only base-ground enemy markers");
}
}

int main()
{
    const bool passed = everyCatalogLayoutIsValid()
        && specialFloorsAlwaysUseSafeRooms()
        && combatAndBossSelectionsStayInsideTheirAct()
        && auraBossPlatformsRemainClearlyAboveGround()
        && tacticalSpawnPointsFollowPlatformGeometry();
    if (!passed) return 1;
    std::cout << "All arena layout tests passed.\n";
    return 0;
}
