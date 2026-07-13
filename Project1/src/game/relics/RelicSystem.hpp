#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace arcane::game::relics
{
inline constexpr std::uint32_t DarkDragonHornId = 4001U;
inline constexpr std::uint32_t MoonGrassFlowerId = 4002U;
inline constexpr std::uint32_t FlammeNotesId = 4003U;
inline constexpr std::uint32_t ObedienceScaleId = 4004U;
inline constexpr std::uint32_t HolyStaffId = 4005U;
inline constexpr std::uint32_t KraftBracerId = 4006U;
inline constexpr std::uint32_t MirrorLotusRingId = 4007U;
inline constexpr std::uint32_t PriestScriptureId = 4008U;
inline constexpr std::uint32_t ElderSealFragmentId = 4009U;
inline constexpr std::uint32_t HeroFlowerCrownId = 4010U;
inline constexpr std::uint32_t PurpleGrapeHairpinId = 4011U;
inline constexpr std::uint32_t FirstClassBadgeId = 4012U;
inline constexpr std::uint32_t OldSpellBookmarkId = 4013U;
inline constexpr std::uint32_t ManaLensId = 4014U;
inline constexpr std::uint32_t BrokenBarrierId = 4015U;
inline constexpr std::uint32_t MimicTongueId = 4016U;
inline constexpr std::uint32_t ManaSuppressionEarringId = 4017U;
inline constexpr std::uint32_t SeriePageId = 4018U;
inline constexpr std::uint32_t NorthernCloakId = 4019U;
inline constexpr std::uint32_t SteelPetalBookmarkId = 4020U;
inline constexpr std::uint32_t WarriorAxeFragmentId = 4021U;
inline constexpr std::uint32_t GoddessTabletId = 4022U;
inline constexpr std::uint32_t DemonCoinId = 4023U;
inline constexpr std::uint32_t OldCopperCoinId = 4024U;
inline constexpr std::uint32_t HeroSwordId = 4101U;
inline constexpr std::uint32_t TrueHeroSwordId = 4102U;

struct RelicDefinition
{
    std::uint32_t id {};
    const char* name {};
    const char* description {};
};

[[nodiscard]] const RelicDefinition* findDefinition(std::uint32_t id) noexcept;

class RelicRuntime
{
public:
    explicit RelicRuntime(std::span<const std::uint32_t> relicIds = {});
    void update(float deltaSeconds) noexcept;
    [[nodiscard]] float outgoingDamageMultiplier() const noexcept;
    [[nodiscard]] float incomingDamageMultiplier() const noexcept;
    [[nodiscard]] bool castsFlowerFieldOnCombatStart() const noexcept;
    [[nodiscard]] float vulnerableRemaining() const noexcept;
    [[nodiscard]] int collisionFlatReduction() const noexcept;
    [[nodiscard]] bool retaliatesOnCollision() const noexcept;
    [[nodiscard]] bool has(std::uint32_t id) const noexcept;

private:
    bool darkDragonHorn_ {};
    bool moonGrassFlower_ {};
    bool heroSword_ {};
    bool trueHeroSword_ {};
    float vulnerableRemaining_ {};
    std::vector<std::uint32_t> ids_;
};
}
