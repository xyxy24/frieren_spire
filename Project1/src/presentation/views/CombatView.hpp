#pragma once

#include "app/run/TowerSession.hpp"
#include "presentation/EnemyAnimator.hpp"
#include "presentation/PlayerAnimator.hpp"
#include "presentation/ShadeChargeAnimator.hpp"
#include "presentation/SpellEffectAnimator.hpp"
#include "presentation/viewmodel/CombatFeedbackViewModel.hpp"
#include "presentation/views/ArenaView.hpp"
#include "presentation/views/StaircaseView.hpp"

#include <SFML/Graphics.hpp>

#include <array>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace arcane::presentation::views
{
struct EnemyStateTextures
{
    std::optional<sf::Texture> animation;
    std::optional<sf::Texture> walk;
    std::optional<sf::Texture> domination;
    std::optional<sf::Texture> guillotineFrame;
    std::optional<sf::Texture> guillotineBlade;
    std::optional<sf::Texture> initial;
    std::optional<sf::Texture> idle;
    std::optional<sf::Texture> windup;
    std::optional<sf::Texture> preJump;
    std::optional<sf::Texture> jump;
    std::optional<sf::Texture> attack;
    std::optional<sf::Texture> summon;
    std::optional<sf::Texture> die;
    std::array<std::optional<sf::Texture>, 4> skillWindups;
    std::array<std::optional<sf::Texture>, 3> skillAttacks;
    std::optional<sf::Texture> parry;
    std::optional<sf::Texture> dash;
    std::array<std::optional<sf::Texture>, 3> idleVariants;
};

struct DialoguePortraitTextures
{
    std::optional<sf::Texture> frieren;
    std::optional<sf::Texture> frierenCopy;
    std::optional<sf::Texture> waterMirror;
    std::optional<sf::Texture> waterMirrorDie;
    std::optional<sf::Texture> auraInitial;
    std::optional<sf::Texture> auraIdle;
    std::optional<sf::Texture> auraWindup;
    std::optional<sf::Texture> auraDie;
    std::array<std::optional<sf::Texture>, 3> revolte;
};

[[nodiscard]] std::optional<sf::Texture> loadTexture(const std::string& path);
[[nodiscard]] EnemyStateTextures loadEnemyStateTextures(std::string_view base,
    bool loadJump = false, bool loadIntroAndDeath = false, bool loadAttack = true,
    bool loadPreJump = false, bool loadWalk = false);
[[nodiscard]] PlayerVisualState makePlayerVisualState(const game::PlayerStateView& player);
[[nodiscard]] PlayerVisualState makePlayerVisualState(
    const game::PlayerController& player, int currentHealth);

void drawCombat(sf::RenderTarget& target, const app::TowerSession& tower,
    std::span<const game::EnemyStateView> enemyStates,
    std::span<const game::SpellEffectView> spellEffects,
    const EnemyStateTextures& headlessTextures, const EnemyStateTextures& mimicTextures,
    const EnemyStateTextures& birdTextures, const EnemyStateTextures& frostWolfTextures,
    const EnemyStateTextures& chaosFlowerTextures, const EnemyStateTextures& qualTextures,
    const std::array<std::optional<sf::Texture>, 3>& qualSkillTextures,
    const EnemyStateTextures& heimonTextures,
    const std::array<std::optional<sf::Texture>, 2>& heimonSkillTextures,
    const std::optional<sf::Texture>& heimonFogTexture,
    const EnemyStateTextures& demonWarriorTextures,
    const EnemyStateTextures& largeBirdDemonTextures,
    const EnemyStateTextures& gargoyleTextures,
    const std::array<std::optional<sf::Texture>, 2>& gargoyleSkillTextures,
    const EnemyStateTextures& swordDemonTextures,
    const EnemyStateTextures& threeHeadedDemonTextures,
    const EnemyStateTextures& richterTextures,
    const std::array<std::optional<sf::Texture>, 3>& pillarTextures,
    const EnemyStateTextures& laufenTextures,
    const EnemyStateTextures& starkCopyTextures,
    const std::optional<sf::Texture>& starkSlashTexture,
    const EnemyStateTextures& frierenCopyTextures,
    const EnemyStateTextures& fernCopyTextures,
    const EnemyStateTextures& waterMirrorTextures,
    const std::array<std::optional<sf::Texture>, 2>& frierenBeamTextures,
    const std::array<std::optional<sf::Texture>, 2>& frierenLightningTextures,
    const std::optional<sf::Texture>& frierenFireTexture,
    const std::optional<sf::Texture>& slashTexture,
    const std::optional<sf::Texture>& largeSlashTexture,
    const EnemyStateTextures& lugnerTextures,
    const std::array<std::optional<sf::Texture>, 3>& lugnerSkillTextures,
    const EnemyStateTextures& linieTextures,
    const std::array<std::optional<sf::Texture>, 2>& linieSkillTextures,
    const EnemyStateTextures& drahtTextures, const EnemyStateTextures& auraTextures,
    const EnemyStateTextures& revolteTextures,
    const EnemyStateTextures& denkenTextures,
    const std::array<std::optional<sf::Texture>, 2>& tornadoTextures,
    const ArenaTextures& arenaTextures,
    const StaircaseTextures& staircaseTextures,
    const EnemyAnimator& enemyAnimator, const PlayerAnimator& playerAnimator,
    const ShadeChargeAnimator& shadeChargeAnimator,
    const SpellEffectAnimator& spellEffectAnimator,
    const viewmodel::CombatFeedbackSnapshot& feedback);
void drawCombatOverlay(sf::RenderTarget& target, const game::CombatSession& combat,
    const DialoguePortraitTextures& portraits);
}
