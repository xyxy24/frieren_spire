#pragma once

#include "app/run/RunController.hpp"
#include "game/combat/CombatSession.hpp"
#include "game/contracts/PlayerIntent.hpp"
#include "game/economy/MerchantSystem.hpp"
#include "game/events/EventSystem.hpp"
#include "game/floors/ArenaLayout.hpp"
#include "game/floors/FloorScheduler.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace arcane::app
{
struct TowerSessionConfig
{
    game::run::PlayerProgress initialPlayer;
    game::WorldBounds worldBounds {0.0F, 1280.0F, 640.0F};
    game::Vec2 playerSpawn {160.0F, 576.0F};
    game::Vec2 enemySpawn {800.0F, 576.0F};
    game::Aabb staircaseBounds {1150.0F, 520.0F, 90.0F, 120.0F};
    game::Aabb npcBounds {600.0F, 560.0F, 56.0F, 80.0F};
    int normalEnemyHealth {0};
    int bossEnemyHealth {225};
    int normalGoldReward {10};
    int bossGoldReward {30};
    std::uint32_t floorsPerBoss {game::floors::DefaultFloorsPerBoss};
    bool enableSpecialFloors {true};
    std::optional<game::run::FloorType> firstFloorTypeOverride;
};

enum class EventFloorState : std::uint8_t { Untriggered, Choosing, Result };
enum class EventKind : std::uint8_t { AldenBall, HalfCenturyMeteorShower, SwordVillage };
class TowerSession
{
public:
    explicit TowerSession(game::run::Seed seed, TowerSessionConfig config = {});

    void update(const game::PlayerIntent& intent, float deltaSeconds);

    [[nodiscard]] const RunController& run() const noexcept;
    [[nodiscard]] const game::CombatSession* combat() const noexcept;
    [[nodiscard]] game::run::FloorType currentFloorType() const noexcept;
    [[nodiscard]] const game::floors::ArenaLayout& arenaLayout() const;
    [[nodiscard]] std::optional<std::array<game::run::ContentId, 3>> rewardCandidates() const;
    [[nodiscard]] std::optional<game::Aabb> lootDropBounds() const noexcept;
    [[nodiscard]] bool equipRegularSpell(std::size_t slot, game::run::ContentId spell);
    [[nodiscard]] bool equipUltimateSpell(game::run::ContentId spell);
    [[nodiscard]] game::Aabb staircaseBounds() const noexcept;
    [[nodiscard]] bool staircaseUnlocked() const noexcept;
    [[nodiscard]] const std::vector<game::economy::StockItem>& merchantStock() const noexcept;
    [[nodiscard]] std::optional<game::run::ContentId> selectedMerchantItem() const noexcept;
    [[nodiscard]] std::span<const game::events::EventChoice> eventChoices() const noexcept;
    [[nodiscard]] const game::PlayerController* explorationPlayer() const noexcept;
    [[nodiscard]] game::Aabb npcBounds() const noexcept;
    [[nodiscard]] bool specialPanelOpen() const noexcept;
    [[nodiscard]] EventFloorState eventFloorState() const noexcept;
    [[nodiscard]] EventKind eventKind() const noexcept;
    [[nodiscard]] std::optional<game::run::ContentId> eventResultChoice() const noexcept;
    void restartCurrentFloor();

private:
    void startNextFloor();
    void updateSpecialFloor(const game::PlayerIntent& intent, float deltaSeconds);

    RunController run_;
    TowerSessionConfig config_;
    game::floors::FloorScheduler scheduler_;
    std::optional<game::CombatSession> combat_;
    std::optional<game::Aabb> lootDropBounds_;
    std::optional<game::PlayerController> explorationPlayer_;
    game::run::FloorType currentFloorType_ {game::run::FloorType::Combat};
    const game::floors::ArenaLayout* arenaLayout_ {};
    std::vector<game::economy::StockItem> merchantStock_;
    std::size_t selectedMerchantIndex_ {0};
    std::optional<game::events::EventTransaction> eventTransaction_;
    std::array<game::events::EventChoice, 3> eventChoices_;
    EventFloorState eventFloorState_ {EventFloorState::Untriggered};
    EventKind eventKind_ {EventKind::AldenBall};
    std::optional<game::run::ContentId> eventResultChoice_;
    bool specialPanelOpen_ {};
    std::optional<RunController> floorStartRun_;
    std::optional<game::floors::FloorScheduler> floorStartScheduler_;
};
}
