#pragma once

#include "app/run/TowerSession.hpp"
#include "presentation/PlayerAnimator.hpp"
#include "presentation/SpellEffectAnimator.hpp"
#include "presentation/viewmodel/CombatFeedbackViewModel.hpp"

#include <SFML/Graphics.hpp>

#include <array>
#include <optional>
#include <string>
#include <string_view>

namespace arcane::presentation::views
{
struct EnemyStateTextures
{
    std::optional<sf::Texture> initial;
    std::optional<sf::Texture> idle;
    std::optional<sf::Texture> windup;
    std::optional<sf::Texture> jump;
    std::optional<sf::Texture> attack;
    std::optional<sf::Texture> die;
};

struct DialoguePortraitTextures
{
    std::optional<sf::Texture> frieren;
    std::optional<sf::Texture> auraInitial;
    std::optional<sf::Texture> auraIdle;
    std::optional<sf::Texture> auraWindup;
    std::optional<sf::Texture> auraDie;
};

[[nodiscard]] std::optional<sf::Texture> loadTexture(const std::string& path);
[[nodiscard]] EnemyStateTextures loadEnemyStateTextures(std::string_view base,
    bool loadJump = false, bool loadIntroAndDeath = false, bool loadAttack = true);
[[nodiscard]] PlayerVisualState makePlayerVisualState(const game::PlayerStateView& player);
[[nodiscard]] PlayerVisualState makePlayerVisualState(
    const game::PlayerController& player, int currentHealth);

void drawCombat(sf::RenderTarget& target, const app::TowerSession& tower,
    const EnemyStateTextures& headlessTextures, const EnemyStateTextures& mimicTextures,
    const EnemyStateTextures& birdTextures, const EnemyStateTextures& lugnerTextures,
    const std::array<std::optional<sf::Texture>, 3>& lugnerSkillTextures,
    const EnemyStateTextures& linieTextures,
    const std::array<std::optional<sf::Texture>, 2>& linieSkillTextures,
    const EnemyStateTextures& drahtTextures, const EnemyStateTextures& auraTextures,
    const PlayerAnimator& playerAnimator, const SpellEffectAnimator& spellEffectAnimator,
    const viewmodel::CombatFeedbackSnapshot& feedback);
void drawCombatOverlay(sf::RenderTarget& target, const game::CombatSession& combat,
    const DialoguePortraitTextures& portraits);
}
