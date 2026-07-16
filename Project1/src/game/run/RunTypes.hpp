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
    LootPending,
    Reward,
    Breakthrough,
    FloorComplete,
    Defeat,
    Victory
};

struct SpellMastery
{
    ContentId spellId {};
    std::uint8_t rank {1U};
};

struct PlayerProgress
{
    int currentHp {100};
    int maxHp {100};
    int gold {};
    std::vector<ContentId> learnedSpells;
    std::vector<SpellMastery> spellMasteries;
    std::array<std::optional<ContentId>, 3> equippedSpells;
    std::vector<ContentId> learnedBossSpells;
    std::optional<ContentId> equippedUltimateSpell;
    bool ultimateSpellUnlocked {false};
    std::vector<ContentId> relics;
    std::array<bool, 3> rewardRerollUsed {};
    // Power, Haste and Defense. Only the first two Boss victories offer a choice.
    std::array<std::uint8_t, 3> breakthroughRanks {};
};

struct RunContext
{
    Seed runSeed {};
    Seed floorSeed {};
    std::uint32_t floorIndex {};
    std::uint32_t act {1};
    std::uint32_t bossesDefeated {};
    std::array<bool, 3> triggeredEvents {};
};

struct FloorDescriptor
{
    FloorType type {FloorType::Combat};
    Seed seed {};
    ContentId arenaId {};
    std::vector<ContentId> encounterIds;
};

struct FloorResult
{
    bool floorComplete {};
    bool rewardComplete {};
    bool stairsAvailable {};
};
}
