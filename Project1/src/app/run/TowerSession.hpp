#pragma once

#include "app/run/RunController.hpp"
#include "game/combat/CombatSession.hpp"
#include "game/contracts/PlayerIntent.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace arcane::app
{
struct TowerSessionConfig
{
    game::WorldBounds worldBounds {0.0F, 1280.0F, 640.0F};
    game::Vec2 playerSpawn {160.0F, 576.0F};
    game::Vec2 enemySpawn {800.0F, 576.0F};
    game::Aabb staircaseBounds {1150.0F, 520.0F, 90.0F, 120.0F};
    int normalEnemyHealth {100};
    int bossEnemyHealth {200};
    int normalGoldReward {10};
    int bossGoldReward {30};
    std::uint32_t floorsPerBoss {3};
};

class TowerSession
{
public:
    explicit TowerSession(game::run::Seed seed, TowerSessionConfig config = {});

    void update(const game::PlayerIntent& intent, float deltaSeconds);

    [[nodiscard]] const RunController& run() const noexcept;
    [[nodiscard]] const game::CombatSession* combat() const noexcept;
    [[nodiscard]] game::run::FloorType currentFloorType() const noexcept;
    [[nodiscard]] std::optional<std::array<game::run::ContentId, 3>> rewardCandidates() const;
    [[nodiscard]] bool loadoutOpen() const noexcept;
    [[nodiscard]] std::optional<game::run::ContentId> selectedLearnedSpell() const noexcept;
    [[nodiscard]] game::Aabb staircaseBounds() const noexcept;
    [[nodiscard]] bool staircaseUnlocked() const noexcept;

private:
    void startNextFloor();
    void updateLoadout(const game::PlayerIntent& intent);
    [[nodiscard]] bool isBossFloor() const noexcept;

    RunController run_;
    TowerSessionConfig config_;
    std::optional<game::CombatSession> combat_;
    game::run::FloorType currentFloorType_ {game::run::FloorType::Combat};
    std::size_t selectedLearnedSpellIndex_ {0};
    bool loadoutOpen_ {false};
};
}
