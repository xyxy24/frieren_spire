#pragma once

#include "game/run/RunTypes.hpp"

#include <cstdint>

namespace arcane::game::run
{
enum class RandomStream : std::uint64_t
{
    Layout = 0x243f6a8885a308d3ULL,
    Encounter = 0x13198a2e03707344ULL,
    Reward = 0xa4093822299f31d0ULL,
    Merchant = 0x082efa98ec4e6c89ULL,
    Event = 0x452821e638d01377ULL
};

[[nodiscard]] Seed deriveFloorSeed(Seed runSeed, std::uint32_t floorIndex) noexcept;
[[nodiscard]] Seed deriveStreamSeed(Seed floorSeed, RandomStream stream) noexcept;

class DeterministicRng
{
public:
    explicit DeterministicRng(Seed seed) noexcept;
    [[nodiscard]] std::uint64_t next() noexcept;
    [[nodiscard]] std::uint32_t index(std::uint32_t upperExclusive);

private:
    Seed state_;
};
}
