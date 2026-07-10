#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace arcane::game::run
{
using ContentId = std::uint32_t;
using Seed = std::uint64_t;

enum class FloorType : std::uint8_t
{
    Combat,
    Event,
    Merchant,
    Boss
};

enum class RunPhase : std::uint8_t
{
    FloorLoading,
    InEncounter,
    Reward,
    FloorComplete,
    Defeat,
    Victory
};

struct PlayerProgress
{
    int currentHp {100};
    int maxHp {100};
    int gold {};
    std::vector<ContentId> learnedSpells;
    std::array<std::optional<ContentId>, 3> equippedSpells;
    std::vector<ContentId> relics;
};

struct RunContext
{
    Seed runSeed {};
    Seed floorSeed {};
    std::uint32_t floorIndex {};
    std::uint32_t act {1};
    std::uint32_t bossesDefeated {};
};

struct FloorDescriptor
{
    FloorType type {FloorType::Combat};
    Seed seed {};
    ContentId arenaId {};
    std::vector<ContentId> encounterIds;
};
}
