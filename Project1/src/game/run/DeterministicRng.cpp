#include "game/run/DeterministicRng.hpp"

#include <stdexcept>

namespace arcane::game::run
{
namespace
{
std::uint64_t mix(std::uint64_t value) noexcept
{
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31U);
}
}

Seed deriveFloorSeed(const Seed runSeed, const std::uint32_t floorIndex) noexcept
{
    return mix(runSeed ^ (static_cast<Seed>(floorIndex) * 0x9e3779b97f4a7c15ULL));
}

Seed deriveStreamSeed(const Seed floorSeed, const RandomStream stream) noexcept
{
    return mix(floorSeed ^ static_cast<Seed>(stream));
}

DeterministicRng::DeterministicRng(const Seed seed) noexcept : state_(seed) {}

std::uint64_t DeterministicRng::next() noexcept
{
    state_ = mix(state_);
    return state_;
}

std::uint32_t DeterministicRng::index(const std::uint32_t upperExclusive)
{
    if (upperExclusive == 0U)
    {
        throw std::invalid_argument("random index requires a non-empty range");
    }
    return static_cast<std::uint32_t>(next() % upperExclusive);
}
}
