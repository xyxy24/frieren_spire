#include "game/floors/ArenaLayout.hpp"

#include "game/run/DeterministicRng.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace arcane::game::floors
{
namespace
{
constexpr float WorldLeft = 0.0F;
constexpr float WorldRight = 1280.0F;
constexpr float GroundTop = 640.0F;
constexpr float HighestPlatformTop = 352.0F;

const std::array<ArenaLayout, 18> Layouts {{
    {110U, ArenaTheme::AuraOccupation, true, {}},
    {111U, ArenaTheme::AuraOccupation, false, {{470.0F, 560.0F, 300.0F, 28.0F}}},
    {112U, ArenaTheme::AuraOccupation, false, {
        {250.0F, 544.0F, 230.0F, 28.0F}, {800.0F, 560.0F, 230.0F, 28.0F}}},
    {113U, ArenaTheme::AuraOccupation, false, {{480.0F, 544.0F, 320.0F, 28.0F}}},
    {114U, ArenaTheme::AuraOccupation, false, {
        {210.0F, 576.0F, 220.0F, 28.0F}, {520.0F, 512.0F, 260.0F, 28.0F},
        {880.0F, 576.0F, 190.0F, 28.0F}}},
    {115U, ArenaTheme::AuraOccupation, false, {
        {280.0F, 536.0F, 220.0F, 28.0F}, {870.0F, 536.0F, 220.0F, 28.0F}}},

    {210U, ArenaTheme::NorthernFrontier, true, {}},
    {211U, ArenaTheme::NorthernFrontier, false, {
        {220.0F, 560.0F, 260.0F, 28.0F}, {800.0F, 544.0F, 250.0F, 28.0F}}},
    {212U, ArenaTheme::NorthernFrontier, false, {
        {240.0F, 576.0F, 200.0F, 28.0F}, {500.0F, 512.0F, 240.0F, 28.0F},
        {780.0F, 416.0F, 260.0F, 28.0F}}},
    {213U, ArenaTheme::NorthernFrontier, false, {{450.0F, 544.0F, 380.0F, 28.0F}}},
    {214U, ArenaTheme::NorthernFrontier, false, {
        {240.0F, 576.0F, 200.0F, 28.0F}, {480.0F, 496.0F, 380.0F, 28.0F},
        {900.0F, 576.0F, 200.0F, 28.0F}}},
    {215U, ArenaTheme::NorthernFrontier, false, {
        {280.0F, 592.0F, 240.0F, 28.0F}, {860.0F, 592.0F, 240.0F, 28.0F}}},

    {310U, ArenaTheme::MageExam, true, {}},
    {311U, ArenaTheme::MageExam, false, {
        {190.0F, 544.0F, 280.0F, 28.0F}, {810.0F, 544.0F, 280.0F, 28.0F}}},
    {312U, ArenaTheme::MageExam, false, {
        {230.0F, 560.0F, 240.0F, 28.0F}, {510.0F, 480.0F, 280.0F, 28.0F},
        {870.0F, 560.0F, 240.0F, 28.0F}}},
    {313U, ArenaTheme::MageExam, false, {
        {450.0F, 560.0F, 380.0F, 28.0F}, {170.0F, 480.0F, 220.0F, 28.0F},
        {890.0F, 480.0F, 220.0F, 28.0F}}},
    {314U, ArenaTheme::MageExam, false, {
        {230.0F, 560.0F, 220.0F, 28.0F}, {490.0F, 480.0F, 370.0F, 28.0F},
        {900.0F, 560.0F, 220.0F, 28.0F}}},
    {315U, ArenaTheme::MageExam, false, {
        {280.0F, 520.0F, 260.0F, 28.0F}, {840.0F, 520.0F, 260.0F, 28.0F}}}
}};

std::uint32_t actNumber(const run::RunContext& context) noexcept
{
    return std::clamp(context.act, 1U, 3U);
}
}

run::ContentId selectArenaId(const run::RunContext& context, const run::FloorType type) noexcept
{
    const std::uint32_t base = actNumber(context) * 100U;
    if (type == run::FloorType::Merchant || type == run::FloorType::Event) return base + 10U;
    if (type == run::FloorType::Boss) return base + 15U;

    run::DeterministicRng layout(
        run::deriveStreamSeed(context.floorSeed, run::RandomStream::Layout));
    return base + 11U + layout.index(4U);
}

const ArenaLayout& arenaLayout(const run::ContentId id)
{
    const auto found = std::find_if(Layouts.begin(), Layouts.end(), [id](const ArenaLayout& layout) {
        return layout.id == id;
    });
    if (found == Layouts.end()) throw std::invalid_argument("unknown arena layout id");
    return *found;
}

std::span<const ArenaLayout> allArenaLayouts() noexcept
{
    return Layouts;
}

bool validateArenaLayout(const ArenaLayout& layout) noexcept
{
    if (layout.id == 0U || (layout.safeRoom && !layout.oneWayPlatforms.empty())) return false;
    for (const Aabb& platform : layout.oneWayPlatforms)
    {
        if (platform.width < 160.0F || platform.height < 24.0F
            || platform.left < WorldLeft || platform.left + platform.width > WorldRight
            || platform.top < HighestPlatformTop || platform.top >= GroundTop
            || platform.top + platform.height > GroundTop)
            return false;
    }
    return true;
}

std::vector<ArenaSpawnPoint> enemySpawnPoints(const ArenaLayout& layout)
{
    std::vector<ArenaSpawnPoint> points;
    points.reserve(3U + layout.oneWayPlatforms.size() * 2U);
    constexpr std::array GroundX {650.0F, 870.0F, 1090.0F};
    for (const float x : GroundX)
        points.push_back({ArenaSpawnKind::GroundEnemy, {x, GroundTop},
            {WorldLeft, WorldRight, GroundTop}});

    for (const Aabb& platform : layout.oneWayPlatforms)
    {
        const float centeredX = platform.left + platform.width * 0.5F - 24.0F;
        points.push_back({ArenaSpawnKind::ElevatedEnemy, {centeredX, platform.top},
            {platform.left, platform.left + platform.width, platform.top}});
        points.push_back({ArenaSpawnKind::FlyingEnemy, {centeredX, platform.top - 72.0F},
            {WorldLeft, WorldRight, GroundTop}});
    }
    return points;
}
}
