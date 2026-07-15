#include "presentation/views/CombatView.hpp"

#include "presentation/views/ArenaView.hpp"
#include "presentation/views/ScreenViews.hpp"
#include "presentation/views/UiPrimitives.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>

namespace arcane::presentation::views
{
std::optional<sf::Texture> loadTexture(const std::string& path)
{
    sf::Texture texture;
    if (!texture.loadFromFile(path)) return std::nullopt;
    texture.setSmooth(false);
    return texture;
}

EnemyStateTextures loadEnemyStateTextures(const std::string_view base, const bool loadJump,
    const bool loadIntroAndDeath, const bool loadAttack, const bool loadPreJump,
    const bool loadWalk)
{
    EnemyStateTextures textures;
    textures.animation = loadTexture(std::string {base} + "animation.png");
    if (loadWalk) textures.walk = loadTexture(std::string {base} + "walk.png");
    if (loadIntroAndDeath) textures.initial = loadTexture(std::string {base} + "initial.png");
    textures.idle = loadTexture(std::string {base} + "idle.png");
    textures.windup = loadTexture(std::string {base} + "windup.png");
    if (loadPreJump) textures.preJump = loadTexture(std::string {base} + "prejump.png");
    if (loadJump) textures.jump = loadTexture(std::string {base} + "jump.png");
    if (loadAttack) textures.attack = loadTexture(std::string {base} + "attack.png");
    if (loadIntroAndDeath) textures.die = loadTexture(std::string {base} + "die.png");
    return textures;
}

arcane::presentation::PlayerVisualState makePlayerVisualState(
    const arcane::game::PlayerStateView& player)
{
    return {player.position, player.velocity, player.currentHealth, player.grounded,
        player.facingDirection, player.attackSequence, player.castSequence, player.hurtSequence,
        player.dashRemaining, player.shadowDashChargeRemaining,
        player.shadowDashing, player.stunned};
}

arcane::presentation::PlayerVisualState makePlayerVisualState(
    const arcane::game::PlayerController& player, const int currentHealth)
{
    return {player.position(), player.velocity(), currentHealth, player.isGrounded(),
        player.facingDirection(), 0U, 0U, 0U, player.dashRemaining(),
        player.shadowDashChargeRemaining(), player.isShadowDashing(), player.isStunned()};
}

namespace
{
void drawSleepParticles(sf::RenderTarget& target, const arcane::game::PlayerStateView& player)
{
    if (player.sleepRemaining <= 0.0F) return;

    constexpr std::array<float, 9> PhaseOffsets {
        0.00F, 0.37F, 0.71F, 0.18F, 0.53F, 0.86F, 0.29F, 0.63F, 0.94F
    };
    constexpr std::array<float, 9> HorizontalOffsets {
        4.0F, 31.0F, 16.0F, 38.0F, 9.0F, 25.0F, 1.0F, 34.0F, 20.0F
    };
    const float elapsed = 5.0F - std::min(player.sleepRemaining, 5.0F);
    for (std::size_t index = 0U; index < PhaseOffsets.size(); ++index)
    {
        const float progress = std::fmod(elapsed * 0.85F + PhaseOffsets[index], 1.0F);
        const float drift = std::sin((progress + PhaseOffsets[index]) * 6.2831853F) * 4.0F;
        const float size = index % 3U == 0U ? 6.0F : 4.0F;
        sf::RectangleShape particle({size, size});
        particle.setPosition({player.position.x + HorizontalOffsets[index] + drift,
            player.position.y + arcane::game::PlayerController::Height
                - 8.0F - progress * 78.0F});
        const float fade = std::sin(progress * 3.1415927F);
        particle.setFillColor(sf::Color {111, 48, 148,
            static_cast<std::uint8_t>(80.0F + fade * 150.0F)});
        particle.setOutlineColor(sf::Color {205, 117, 232,
            static_cast<std::uint8_t>(60.0F + fade * 130.0F)});
        particle.setOutlineThickness(1.0F);
        target.draw(particle);
    }
}
}

void drawCombat(sf::RenderTarget& target, const arcane::app::TowerSession& tower,
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
    const std::optional<sf::Texture>& slashTexture,
    const std::optional<sf::Texture>& largeSlashTexture,
    const EnemyStateTextures& lugnerTextures,
    const std::array<std::optional<sf::Texture>, 3>& lugnerSkillTextures,
    const EnemyStateTextures& linieTextures,
    const std::array<std::optional<sf::Texture>, 2>& linieSkillTextures,
    const EnemyStateTextures& drahtTextures,
    const EnemyStateTextures& auraTextures,
    const EnemyStateTextures& revolteTextures,
    const EnemyStateTextures& denkenTextures,
    const std::array<std::optional<sf::Texture>, 2>& tornadoTextures,
    const ArenaTextures& arenaTextures,
    const arcane::presentation::EnemyAnimator& enemyAnimator,
    const arcane::presentation::PlayerAnimator& playerAnimator,
    const arcane::presentation::ShadeChargeAnimator& shadeChargeAnimator,
    const arcane::presentation::SpellEffectAnimator& spellEffectAnimator,
    const arcane::presentation::viewmodel::CombatFeedbackSnapshot& feedback)
{
    const arcane::game::CombatSession* combat = tower.combat();
    if (!combat) return;

    drawArena(target, tower.arenaLayout(), GroundTop, arenaTextures);

    const arcane::game::PlayerStateView player = combat->playerState();
    const sf::Color playerFallback = player.shadowDashing ? sf::Color {28, 22, 42}
        : player.dashRemaining > 0.0F ? sf::Color {100, 220, 245}
        : player.stunned ? sf::Color {112, 180, 235}
        : player.attackActive ? sf::Color {255, 231, 153} : sf::Color {232, 232, 242};
    sf::Color playerTint = feedback.playerFlashRatio > 0.0F
        ? sf::Color {255, 132, 132} : sf::Color::White;
    if (feedback.playerFlashRatio <= 0.0F && player.hitInvulnerabilityRemaining > 0.0F
        && static_cast<int>(player.hitInvulnerabilityRemaining * 24.0F) % 2 == 0)
        playerTint.a = 105;
    const sf::Vector2f playerBottomCenter {
        player.position.x + arcane::game::PlayerController::Width * 0.5F,
        player.position.y + arcane::game::PlayerController::Height
    };
    shadeChargeAnimator.drawBack(target, playerBottomCenter);
    drawPlayer(target, playerAnimator, makePlayerVisualState(player), playerFallback, playerTint);
    shadeChargeAnimator.drawFront(target, playerBottomCenter);
    drawSleepParticles(target, player);

    for (const arcane::game::SpellEffectView effect : combat->spellEffects())
    {
        if (effect.spellId == 9100U && heimonFogTexture)
        {
            sf::Sprite fog(*heimonFogTexture);
            const auto size = heimonFogTexture->getSize();
            fog.setPosition({effect.bounds.left, effect.bounds.top});
            fog.setScale({effect.bounds.width / static_cast<float>(size.x),
                effect.bounds.height / static_cast<float>(size.y)});
            fog.setColor(sf::Color {255, 255, 255, 205});
            target.draw(fog);
            continue;
        }
        if ((effect.spellId == 9002U || effect.spellId == 9003U)
            && tornadoTextures[effect.spellId == 9002U ? 0U : 1U])
        {
            const sf::Texture& texture = *tornadoTextures[effect.spellId == 9002U ? 0U : 1U];
            sf::Sprite tornado(texture);
            const auto size = texture.getSize();
            tornado.setPosition({effect.bounds.left, effect.bounds.top});
            tornado.setScale({effect.bounds.width / static_cast<float>(size.x),
                effect.bounds.height / static_cast<float>(size.y)});
            target.draw(tornado);
            continue;
        }
        if (effect.spellId == 9200U)
        {
            const std::size_t frame = effect.remaining > effect.duration * 0.5F ? 0U : 1U;
            if (gargoyleSkillTextures[frame])
            {
                const sf::Texture& texture = *gargoyleSkillTextures[frame];
                sf::Sprite laser(texture);
                const auto size = texture.getSize();
                laser.setOrigin({0.0F, static_cast<float>(size.y) * 0.5F});
                laser.setPosition({effect.bounds.left,
                    effect.bounds.top + effect.bounds.height * 0.5F});
                laser.setRotation(sf::degrees(effect.rotationDegrees));
                laser.setScale({effect.bounds.width / static_cast<float>(size.x), 1.0F});
                target.draw(laser);
                continue;
            }
            sf::RectangleShape laser({effect.bounds.width, effect.bounds.height});
            laser.setOrigin({0.0F, effect.bounds.height * 0.5F});
            laser.setPosition({effect.bounds.left, effect.bounds.top + effect.bounds.height * 0.5F});
            laser.setRotation(sf::degrees(effect.rotationDegrees));
            laser.setFillColor(sf::Color {224, 80, 255, 205});
            laser.setOutlineColor(sf::Color {255, 213, 255, 245});
            laser.setOutlineThickness(2.0F);
            target.draw(laser);
            continue;
        }
        const std::optional<sf::Texture>* projectileTexture = nullptr;
        if (effect.spellId == 9101U) projectileTexture = &slashTexture;
        else if (effect.spellId == 9102U) projectileTexture = &largeSlashTexture;
        if (projectileTexture && *projectileTexture)
        {
            sf::Sprite slash(**projectileTexture);
            const auto size = (*projectileTexture)->getSize();
            slash.setOrigin({static_cast<float>(size.x) * 0.5F,
                static_cast<float>(size.y) * 0.5F});
            slash.setPosition({effect.bounds.left + effect.bounds.width * 0.5F,
                effect.bounds.top + effect.bounds.height * 0.5F});
            slash.setScale({(effect.facingDirection > 0.0F ? -1.0F : 1.0F)
                    * effect.bounds.width / static_cast<float>(size.x),
                effect.bounds.height / static_cast<float>(size.y)});
            target.draw(slash);
            continue;
        }
        const float life = effect.duration > 0.0F
            ? std::clamp(effect.remaining / effect.duration, 0.0F, 1.0F) : 0.0F;
        sf::Color color {146, 196, 255};
        switch (effect.spellId)
        {
        case 1001U: color = sf::Color {105, 225, 133}; break;
        case 1002U: color = sf::Color {255, 226, 125}; break;
        case 1003U: color = sf::Color {210, 55, 83}; break;
        case 1004U: color = sf::Color {130, 231, 255}; break;
        case 1005U: color = sf::Color {105, 174, 255}; break;
        case 1006U: color = sf::Color {255, 112, 55}; break;
        case 1008U: color = sf::Color {202, 137, 255}; break;
        case 1009U: color = sf::Color {170, 154, 132}; break;
        case 1010U: color = player.shadowDashing
            ? sf::Color {92, 61, 135} : sf::Color {100, 220, 245}; break;
        case 1011U: color = sf::Color {172, 114, 237}; break;
        case 1016U: color = sf::Color {83, 214, 218}; break;
        case 1017U: color = sf::Color {102, 225, 255}; break;
        case 1018U: color = sf::Color {255, 209, 92}; break;
        case 1019U: color = sf::Color {166, 96, 255}; break;
        case 1020U: color = sf::Color {255, 220, 104}; break;
        case 1021U: color = sf::Color {125, 190, 255}; break;
        case 1022U: color = sf::Color {164, 139, 110}; break;
        case 1023U: color = sf::Color {132, 232, 244}; break;
        case 1024U: color = sf::Color {232, 121, 255}; break;
        case 1025U: color = sf::Color {220, 205, 255}; break;
        case 1026U: color = sf::Color {255, 238, 92}; break;
        case 1027U: color = sf::Color {116, 228, 255}; break;
        case 1028U: color = sf::Color {140, 177, 255}; break;
        case 1029U: color = sf::Color {178, 244, 232}; break;
        case 1030U: color = sf::Color {164, 105, 235}; break;
        case 2001U: color = sf::Color {108, 228, 255}; break;
        case 2002U: color = sf::Color {255, 232, 139}; break;
        case 2003U: color = sf::Color {238, 238, 255}; break;
        case 2006U: color = sf::Color {194, 129, 255}; break;
        case 2007U: color = sf::Color {124, 165, 255}; break;
        case 2009U: color = sf::Color {255, 76, 36}; break;
        case 2010U: color = sf::Color {255, 245, 185}; break;
        case 2011U: color = sf::Color {184, 147, 99}; break;
        case 2012U: color = sf::Color {179, 224, 255}; break;
        case 9100U: color = sf::Color {116, 132, 142}; break;
        case 9101U: color = sf::Color {222, 236, 255}; break;
        case 9102U: color = sf::Color {174, 219, 255}; break;
        default: break;
        }
        sf::RectangleShape rangeShape({effect.bounds.width, effect.bounds.height});
        rangeShape.setPosition({effect.bounds.left, effect.bounds.top});
        color.a = static_cast<std::uint8_t>(35.0F + 70.0F * life);
        rangeShape.setFillColor(color);
        color.a = static_cast<std::uint8_t>(130.0F + 125.0F * life);
        rangeShape.setOutlineColor(color);
        rangeShape.setOutlineThickness(2.0F);
        if (!spellEffectAnimator.draw(target, effect, player.facingDirection, GroundTop))
            target.draw(rangeShape);
    }

    const auto enemies = combat->enemyStates();
    for (std::size_t enemyIndex = 0U; enemyIndex < enemies.size(); ++enemyIndex)
    {
        const arcane::game::EnemyStateView enemy = enemies[enemyIndex];
        const bool hitFlashing = enemyIndex < feedback.enemyFlashRatios.size()
            && feedback.enemyFlashRatios[enemyIndex] > 0.0F;
        const float impactOffset = enemyIndex < feedback.enemyImpactOffsets.size()
            ? feedback.enemyImpactOffsets[enemyIndex] : 0.0F;
        const bool showDefeatedBoss = !enemy.alive
            && (enemy.archetype == arcane::game::EnemyArchetype::Aura
                || enemy.archetype == arcane::game::EnemyArchetype::Revolte)
            && combat->dialogueLine().has_value();
        if (!enemy.alive && !showDefeatedBoss) continue;
        const bool primaryBoss = enemy.archetype == arcane::game::EnemyArchetype::Aura
            || enemy.archetype == arcane::game::EnemyArchetype::Revolte
            || enemy.archetype == arcane::game::EnemyArchetype::RedMirrorDragon
            || enemy.archetype == arcane::game::EnemyArchetype::Boss;
        const EnemyStateTextures* stateTextures = nullptr;
        if (enemy.archetype == arcane::game::EnemyArchetype::HeadlessKnight)
            stateTextures = &headlessTextures;
        else if (enemy.archetype == arcane::game::EnemyArchetype::ChestMimic)
            stateTextures = &mimicTextures;
        else if (enemy.archetype == arcane::game::EnemyArchetype::BirdDemon)
            stateTextures = &birdTextures;
        else if (enemy.archetype == arcane::game::EnemyArchetype::FrostWolf)
            stateTextures = &frostWolfTextures;
        else if (enemy.archetype == arcane::game::EnemyArchetype::ChaosFlower)
            stateTextures = &chaosFlowerTextures;
        else if (enemy.archetype == arcane::game::EnemyArchetype::Heimon)
            stateTextures = &heimonTextures;
        else if (enemy.archetype == arcane::game::EnemyArchetype::DemonWarrior)
            stateTextures = &demonWarriorTextures;
        else if (enemy.archetype == arcane::game::EnemyArchetype::LargeBirdDemon)
            stateTextures = &largeBirdDemonTextures;
        else if (enemy.archetype == arcane::game::EnemyArchetype::Gargoyle)
            stateTextures = &gargoyleTextures;
        else if (enemy.archetype == arcane::game::EnemyArchetype::SwordDemon)
            stateTextures = &swordDemonTextures;
        else if (enemy.archetype == arcane::game::EnemyArchetype::ThreeHeadedDemon)
            stateTextures = &threeHeadedDemonTextures;
        else if (enemy.archetype == arcane::game::EnemyArchetype::Qual)
            stateTextures = &qualTextures;
        else if (enemy.archetype == arcane::game::EnemyArchetype::Lugner)
            stateTextures = &lugnerTextures;
        else if (enemy.archetype == arcane::game::EnemyArchetype::Linie)
            stateTextures = &linieTextures;
        else if (enemy.archetype == arcane::game::EnemyArchetype::Draht)
            stateTextures = &drahtTextures;
        else if (enemy.archetype == arcane::game::EnemyArchetype::Aura)
            stateTextures = &auraTextures;
        else if (enemy.archetype == arcane::game::EnemyArchetype::Revolte)
            stateTextures = &revolteTextures;
        else if (enemy.archetype == arcane::game::EnemyArchetype::Denken)
            stateTextures = &denkenTextures;
        const sf::Texture* texture = nullptr;
        std::optional<sf::IntRect> animationFrame;
        bool usingWalkAnimation = false;
        if (stateTextures)
        {
            const auto dialogue = combat->dialogueLine();
            const bool auraInitial = enemy.archetype == arcane::game::EnemyArchetype::Aura
                && (combat->bossIntro().has_value()
                    || (dialogue && (dialogue->portrait == "aura-initial"
                        || dialogue->portrait == "frieren-pre")));
            if (showDefeatedBoss && stateTextures->die) texture = &*stateTextures->die;
            else if (auraInitial && stateTextures->initial) texture = &*stateTextures->initial;
            else if (enemy.archetype == arcane::game::EnemyArchetype::Heimon
                && enemy.specialAttackActive && stateTextures->summon)
                texture = &*stateTextures->summon;
            else if (enemy.archetype == arcane::game::EnemyArchetype::FrostWolf
                && enemy.specialAttackActive && stateTextures->jump)
                texture = &*stateTextures->jump;
            else if (enemy.archetype == arcane::game::EnemyArchetype::FrostWolf
                && enemy.specialWindingUp && stateTextures->preJump)
                texture = &*stateTextures->preJump;
            else if (enemy.archetype == arcane::game::EnemyArchetype::Linie
                && enemy.attackActive && !enemy.skillEffectBounds
                && stateTextures->jump) texture = &*stateTextures->jump;
            else if (enemy.archetype == arcane::game::EnemyArchetype::LargeBirdDemon
                && enemy.returningToAir && stateTextures->idle) texture = &*stateTextures->idle;
            else if (enemy.archetype == arcane::game::EnemyArchetype::Gargoyle
                && enemy.activated && !enemy.windingUp && !enemy.attackActive
                && stateTextures->jump) texture = &*stateTextures->jump;
            else if (enemy.archetype == arcane::game::EnemyArchetype::SwordDemon
                && enemy.skillVariant == 0 && enemy.attackActive && stateTextures->dash)
                texture = &*stateTextures->dash;
            else if (enemy.archetype == arcane::game::EnemyArchetype::ThreeHeadedDemon)
            {
                const std::size_t form = enemy.currentHealth >= 120 ? 2U
                    : (enemy.currentHealth >= 60 ? 1U : 0U);
                if (enemy.windingUp && stateTextures->skillWindups[form])
                    texture = &*stateTextures->skillWindups[form];
                else if (enemy.attackActive && enemy.skillVariant == 1
                    && stateTextures->skillAttacks[form])
                    texture = &*stateTextures->skillAttacks[form];
                else if (stateTextures->idleVariants[form])
                    texture = &*stateTextures->idleVariants[form];
            }
            else if (enemy.archetype == arcane::game::EnemyArchetype::Revolte
                && enemy.skillVariant == 3 && enemy.attackActive && stateTextures->parry)
                texture = &*stateTextures->parry;
            else if (enemy.archetype == arcane::game::EnemyArchetype::Denken
                && enemy.specialAttackActive && stateTextures->skillWindups[1])
                texture = &*stateTextures->skillWindups[1];
            else if (enemy.archetype == arcane::game::EnemyArchetype::Revolte
                && enemy.skillVariant == 4 && enemy.attackActive && stateTextures->dash)
                texture = &*stateTextures->dash;
            else if (enemy.archetype == arcane::game::EnemyArchetype::Revolte
                && enemy.windingUp && enemy.skillVariant >= 0 && enemy.skillVariant <= 3
                && stateTextures->skillWindups[static_cast<std::size_t>(enemy.skillVariant)])
                texture = &*stateTextures->skillWindups[static_cast<std::size_t>(enemy.skillVariant)];
            else if (enemy.archetype == arcane::game::EnemyArchetype::Revolte
                && enemy.attackActive && enemy.skillVariant >= 0 && enemy.skillVariant <= 2
                && stateTextures->skillAttacks[static_cast<std::size_t>(enemy.skillVariant)])
                texture = &*stateTextures->skillAttacks[static_cast<std::size_t>(enemy.skillVariant)];
            if (!texture && stateTextures->walk && enemyAnimator.isWalking(enemyIndex))
            {
                animationFrame = enemyAnimator.walkFrameRect(*stateTextures->walk, enemyIndex);
                if (animationFrame)
                {
                    texture = &*stateTextures->walk;
                    usingWalkAnimation = true;
                }
            }
            if (!texture && stateTextures->animation)
            {
                animationFrame = enemyAnimator.frameRect(*stateTextures->animation, enemyIndex);
                if (animationFrame) texture = &*stateTextures->animation;
            }
            if (!texture && enemy.attackActive && stateTextures->attack)
                texture = &*stateTextures->attack;
            else if (!texture && enemy.windingUp && stateTextures->windup)
                texture = &*stateTextures->windup;
            else if (!texture && stateTextures->idle)
                texture = &*stateTextures->idle;
        }
        if (texture)
        {
            sf::Sprite sprite(*texture);
            sf::Vector2f frameSize {static_cast<float>(texture->getSize().x),
                static_cast<float>(texture->getSize().y)};
            if (animationFrame)
            {
                sprite.setTextureRect(*animationFrame);
                frameSize = {static_cast<float>(animationFrame->size.x),
                    static_cast<float>(animationFrame->size.y)};
            }
            sprite.setOrigin({frameSize.x * 0.5F, frameSize.y});
            sprite.setPosition({enemy.position.x + enemy.width * 0.5F + impactOffset,
                enemy.position.y + enemy.height});
            if (enemy.archetype == arcane::game::EnemyArchetype::Revolte
                && enemy.skillVariant == 4 && (enemy.windingUp || enemy.attackActive))
                sprite.move({0.0F, 10.0F});
            float horizontalScale = enemy.facingDirection > 0.0F ? -1.0F : 1.0F;
            if (enemy.archetype == arcane::game::EnemyArchetype::FrostWolf
                || (enemy.archetype == arcane::game::EnemyArchetype::HeadlessKnight
                    && usingWalkAnimation))
                horizontalScale = -horizontalScale;
            sprite.setScale({horizontalScale, 1.0F});
auto alpha = static_cast<std::uint8_t>(255.0F * (1.0F - std::clamp(enemy.concealmentProgress, 0.0F, 1.0F)));
if (hitFlashing) sprite.setColor(sf::Color{255, 166, 166, alpha});
else sprite.setColor(sf::Color{255, 255, 255, alpha});
            target.draw(sprite);
        }
        else
        {
            sf::RectangleShape enemyShape({enemy.width, enemy.height});
            enemyShape.setPosition({enemy.position.x + impactOffset, enemy.position.y});
        sf::Color enemyColor = primaryBoss
            ? sf::Color {129, 68, 172} : sf::Color {176, 70, 78};
        if (enemy.windingUp) enemyColor = sf::Color {242, 154, 69};
        if (enemy.attackActive) enemyColor = sf::Color {248, 222, 105};
auto alpha = static_cast<std::uint8_t>(255.0F * (1.0F - std::clamp(enemy.concealmentProgress, 0.0F, 1.0F)));
if (hitFlashing) enemyColor = sf::Color{255, 238, 225, alpha};
else enemyColor.a = alpha;
        enemyShape.setFillColor(enemyColor);
        sf::Color outline {245, 176, 129};
        outline.a = enemyColor.a;
        enemyShape.setOutlineColor(outline);
        enemyShape.setOutlineThickness(primaryBoss ? 6.0F : 3.0F);
        target.draw(enemyShape);
        }

        if (enemy.skillEffectBounds)
        {
            const auto area = *enemy.skillEffectBounds;
            const bool lugnerBlood = enemy.archetype == arcane::game::EnemyArchetype::Lugner
                && enemy.attackActive;
            const bool linieCleave = enemy.archetype == arcane::game::EnemyArchetype::Linie
                && enemy.attackActive;
            const bool qualKillingMagic = enemy.archetype == arcane::game::EnemyArchetype::Qual
                && enemy.attackActive;
            const bool heimonAttack = enemy.archetype == arcane::game::EnemyArchetype::Heimon
                && enemy.attackActive && !enemy.specialAttackActive;
            const bool auraDomination = enemy.archetype == arcane::game::EnemyArchetype::Aura
                && (enemy.windingUp || enemy.attackActive);
            std::optional<sf::IntRect> dominationFrame;
            const sf::Texture* dominationTexture = nullptr;
            if (auraDomination && stateTextures && stateTextures->domination)
            {
                dominationFrame = enemyAnimator.dominationFrameRect(
                    *stateTextures->domination, enemyIndex);
                if (dominationFrame) dominationTexture = &*stateTextures->domination;
            }
            const sf::Texture* bloodTexture = nullptr;
            if (lugnerBlood)
            {
                const std::size_t frame = std::min<std::size_t>(2U,
                    static_cast<std::size_t>(enemy.skillEffectProgress * 3.0F));
                if (lugnerSkillTextures[frame]) bloodTexture = &*lugnerSkillTextures[frame];
            }
            const sf::Texture* cleaveTexture = nullptr;
            std::size_t cleaveFrame = 0U;
            if (linieCleave)
            {
                constexpr float LandingStartProgress = 0.9F / 1.9F;
                const float landingProgress = std::clamp(
                    (enemy.skillEffectProgress - LandingStartProgress)
                        / (1.0F - LandingStartProgress), 0.0F, 1.0F);
                cleaveFrame = landingProgress < 0.5F ? 0U : 1U;
                if (linieSkillTextures[cleaveFrame])
                    cleaveTexture = &*linieSkillTextures[cleaveFrame];
            }
            const sf::Texture* killingMagicTexture = nullptr;
            if (qualKillingMagic)
            {
                const std::size_t frame = std::min<std::size_t>(2U,
                    static_cast<std::size_t>(enemy.skillEffectProgress * 3.0F));
                if (qualSkillTextures[frame])
                    killingMagicTexture = &*qualSkillTextures[frame];
            }
            const sf::Texture* fogAttackTexture = nullptr;
            if (heimonAttack)
            {
                const std::size_t frame = std::min<std::size_t>(1U,
                    static_cast<std::size_t>(enemy.skillEffectProgress * 2.0F));
                if (heimonSkillTextures[frame])
                    fogAttackTexture = &*heimonSkillTextures[frame];
            }
            if (dominationTexture && dominationFrame)
            {
                sf::Sprite effect(*dominationTexture, *dominationFrame);
                const sf::Vector2f size {
                    static_cast<float>(dominationFrame->size.x),
                    static_cast<float>(dominationFrame->size.y)};
                effect.setOrigin({size.x * 0.5F, size.y * 0.5F});
                effect.setPosition({area.left + area.width * 0.5F,
                    area.top + area.height * 0.5F});
                effect.setScale({area.width / size.x, area.height / size.y});
                effect.setColor(enemy.attackActive
                    ? sf::Color::White : sf::Color {255, 255, 255, 205});
                target.draw(effect);
            }
            else if (fogAttackTexture)
            {
                sf::Sprite effect(*fogAttackTexture);
                const auto size = fogAttackTexture->getSize();
                effect.setOrigin({static_cast<float>(size.x) * 0.5F,
                    static_cast<float>(size.y) * 0.5F});
                effect.setPosition({area.left + area.width * 0.5F,
                    area.top + area.height * 0.5F});
                effect.setScale({(enemy.facingDirection > 0.0F ? 1.0F : -1.0F)
                        * area.width / static_cast<float>(size.x),
                    area.height / static_cast<float>(size.y)});
                target.draw(effect);
            }
            else if (killingMagicTexture)
            {
                sf::Sprite effect(*killingMagicTexture);
                const auto size = killingMagicTexture->getSize();
                effect.setOrigin({static_cast<float>(size.x) * 0.5F,
                    static_cast<float>(size.y) * 0.5F});
                effect.setPosition({area.left + area.width * 0.5F,
                    area.top + area.height * 0.5F});
                effect.setScale({(enemy.facingDirection > 0.0F ? 1.0F : -1.0F)
                        * area.width / static_cast<float>(size.x),
                    area.height / static_cast<float>(size.y)});
                target.draw(effect);
            }
            else if (cleaveTexture)
            {
                sf::Sprite effect(*cleaveTexture);
                const auto size = cleaveTexture->getSize();
                effect.setOrigin({static_cast<float>(size.x) * 0.5F,
                    static_cast<float>(size.y)});
                effect.setPosition({area.left + area.width * 0.5F,
                    area.top + area.height + 8.0F + (cleaveFrame == 0U ? 10.0F : 0.0F)});
                effect.setScale({area.width / static_cast<float>(size.x), 1.0F});
                target.draw(effect);
            }
            else if (bloodTexture)
            {
                sf::Sprite effect(*bloodTexture);
                const auto size = bloodTexture->getSize();
                effect.setOrigin({static_cast<float>(size.x) * 0.5F,
                    static_cast<float>(size.y) * 0.5F});
                effect.setPosition({area.left + area.width * 0.5F,
                    area.top + area.height * 0.5F});
                effect.setScale({(enemy.facingDirection > 0.0F ? -1.0F : 1.0F)
                        * area.width / static_cast<float>(size.x),
                    area.height / static_cast<float>(size.y)});
                target.draw(effect);
            }
            else
            {
            sf::RectangleShape skillShape({area.width, area.height});
            skillShape.setPosition({area.left, area.top});
            skillShape.setFillColor(enemy.attackActive
                ? sf::Color {242, 92, 92, 105} : sf::Color {242, 174, 72, 65});
            skillShape.setOutlineColor(enemy.attackActive
                ? sf::Color {255, 120, 120} : sf::Color {255, 201, 105});
            skillShape.setOutlineThickness(2.0F);
            target.draw(skillShape);
            }
        }

        if (enemy.concealmentProgress < 1.0F)
        {
            if (primaryBoss)
                drawHealthBar(target, {static_cast<float>(WindowWidth) - 332.0F, 28.0F},
                    {300.0F, 22.0F}, enemy.currentHealth, enemy.maximumHealth,
                    sf::Color {218, 92, 103});
            else
                drawHealthBar(target, {enemy.position.x - 4.0F, enemy.position.y - 14.0F},
                    {enemy.width + 8.0F, 7.0F}, enemy.currentHealth, enemy.maximumHealth,
                    sf::Color {218, 92, 103});
        }
    }

    if (player.attackActive)
    {
        const arcane::game::Aabb bounds = combat->attackBounds();
        sf::RectangleShape attackShape({bounds.width, bounds.height});
        attackShape.setPosition({bounds.left, bounds.top});
        attackShape.setFillColor(sf::Color {255, 196, 92, 105});
        attackShape.setOutlineColor(sf::Color {255, 218, 145});
        attackShape.setOutlineThickness(2.0F);
        target.draw(attackShape);
    }
    for (const auto& burst : feedback.impactBursts)
    {
        const float ratio = burst.lifetime > 0.0F
            ? std::clamp(burst.remaining / burst.lifetime, 0.0F, 1.0F) : 0.0F;
        const float progress = 1.0F - ratio;
        const float radius = (burst.lethal ? 12.0F : 7.0F)
            + (burst.lethal ? 34.0F : 20.0F) * progress;
        sf::CircleShape ring(radius, 16U);
        ring.setOrigin({radius, radius});
        ring.setPosition({burst.position.x, burst.position.y});
        ring.setFillColor(sf::Color::Transparent);
        sf::Color color = burst.playerTarget
            ? sf::Color {255, 90, 110} : sf::Color {255, 225, 145};
        color.a = static_cast<std::uint8_t>(230.0F * ratio);
        ring.setOutlineColor(color);
        ring.setOutlineThickness(burst.lethal ? 5.0F : 3.0F);
        target.draw(ring);
    }
    for (const auto& number : feedback.damageNumbers)
    {
        const float ratio = number.lifetime > 0.0F
            ? std::clamp(number.remaining / number.lifetime, 0.0F, 1.0F) : 0.0F;
        sf::Color color = number.playerTarget
            ? sf::Color {255, 104, 112} : sf::Color {255, 224, 112};
        color.a = static_cast<std::uint8_t>(255.0F * ratio);
        drawPixelText(target, "-" + std::to_string(number.amount),
            {number.position.x, number.position.y}, number.playerTarget ? 1.25F : 1.05F, color);
    }
    if (const auto dialogue = combat->dialogueLine(); false && dialogue)
    {
        sf::RectangleShape shade({static_cast<float>(WindowWidth), static_cast<float>(WindowHeight)});
        shade.setFillColor(sf::Color {8, 8, 14, 48});
        target.draw(shade);
        const bool auraSpeaking = dialogue->speaker == "AURA";
        sf::RectangleShape panel({1232.0F, 166.0F});
        panel.setPosition({24.0F, 530.0F});
        panel.setFillColor(sf::Color {30, 27, 39, 218});
        panel.setOutlineColor(dialogue->speaker == "AURA"
            ? sf::Color {201, 132, 235} : sf::Color {172, 213, 255});
        panel.setOutlineThickness(4.0F);
        target.draw(panel);

        sf::RectangleShape portraitFrame({126.0F, 126.0F});
        portraitFrame.setPosition({44.0F, 550.0F});
        portraitFrame.setFillColor(auraSpeaking
            ? sf::Color {73, 47, 88, 235} : sf::Color {46, 66, 82, 235});
        portraitFrame.setOutlineColor(auraSpeaking
            ? sf::Color {224, 164, 246} : sf::Color {188, 226, 255});
        portraitFrame.setOutlineThickness(3.0F);
        target.draw(portraitFrame);

        sf::CircleShape hair(45.0F, 32U);
        hair.setPosition({62.0F, 560.0F});
        hair.setFillColor(auraSpeaking
            ? sf::Color {198, 116, 211} : sf::Color {226, 225, 220});
        target.draw(hair);
        sf::CircleShape face(34.0F, 32U);
        face.setPosition({73.0F, 578.0F});
        face.setFillColor(sf::Color {241, 207, 181});
        target.draw(face);
        for (const float eyeX : std::array {87.0F, 115.0F})
        {
            sf::CircleShape eye(3.5F, 12U);
            eye.setPosition({eyeX, 608.0F});
            eye.setFillColor(auraSpeaking ? sf::Color {125, 45, 155} : sf::Color {58, 120, 151});
            target.draw(eye);
        }
        sf::RectangleShape mouth({auraSpeaking ? 22.0F : 16.0F, 3.0F});
        mouth.setPosition({auraSpeaking ? 91.0F : 94.0F, 638.0F});
        mouth.setFillColor(auraSpeaking ? sf::Color {126, 53, 92} : sf::Color {145, 86, 82});
        target.draw(mouth);
        if (auraSpeaking)
        {
            for (const float hornX : std::array {63.0F, 133.0F})
            {
                sf::ConvexShape horn(3U);
                horn.setPoint(0U, {hornX, 582.0F});
                horn.setPoint(1U, {hornX + 18.0F, 558.0F});
                horn.setPoint(2U, {hornX + 13.0F, 591.0F});
                horn.setFillColor(sf::Color {105, 69, 122});
                target.draw(horn);
            }
        }

        drawPixelText(target, dialogue->speaker, {194.0F, 550.0F}, 1.55F,
            auraSpeaking ? sf::Color {225, 165, 250}
                : sf::Color {188, 225, 255});
        drawPixelText(target, dialogue->text, {194.0F, 597.0F}, 1.18F,
            sf::Color {240, 236, 224});
        drawPixelText(target, "ENTER", {1146.0F, 663.0F}, 0.9F,
            sf::Color {190, 184, 205});
    }
}

void drawCombatOverlay(sf::RenderTarget& target, const arcane::game::CombatSession& combat,
    const DialoguePortraitTextures& portraits)
{
    if (const auto intro = combat.bossIntro())
    {
        const float progress = 1.0F - intro->remaining / intro->duration;
        const float alphaFactor = std::min(1.0F, std::min(progress * 4.0F,
            (1.0F - progress) * 4.0F));
        sf::RectangleShape shade({static_cast<float>(WindowWidth), static_cast<float>(WindowHeight)});
        shade.setFillColor(sf::Color {8, 6, 12,
            static_cast<std::uint8_t>(125.0F * std::max(0.0F, alphaFactor))});
        target.draw(shade);
        sf::RectangleShape line({760.0F, 3.0F});
        line.setPosition({260.0F, 402.0F});
        line.setFillColor(sf::Color {198, 126, 224,
            static_cast<std::uint8_t>(220.0F * std::max(0.0F, alphaFactor))});
        target.draw(line);
        drawPixelText(target, intro->name, {430.0F, 330.0F}, 3.0F,
            sf::Color {235, 193, 250,
                static_cast<std::uint8_t>(255.0F * std::max(0.0F, alphaFactor))});
        drawPixelText(target, "BOSS ENCOUNTER", {520.0F, 430.0F}, 1.2F,
            sf::Color {205, 195, 216,
                static_cast<std::uint8_t>(230.0F * std::max(0.0F, alphaFactor))});
        return;
    }
    const auto dialogue = combat.dialogueLine();
    if (!dialogue) return;

    sf::RectangleShape shade({static_cast<float>(WindowWidth), static_cast<float>(WindowHeight)});
    shade.setFillColor(sf::Color {8, 8, 14, 48});
    target.draw(shade);
    const bool auraSpeaking = dialogue->speaker == "AURA";
    sf::RectangleShape panel({1232.0F, 166.0F});
    panel.setPosition({24.0F, 530.0F});
    panel.setFillColor(sf::Color {30, 27, 39, 218});
    panel.setOutlineColor(auraSpeaking ? sf::Color {201, 132, 235} : sf::Color {172, 213, 255});
    panel.setOutlineThickness(4.0F);
    target.draw(panel);

    sf::RectangleShape portraitFrame({126.0F, 126.0F});
    portraitFrame.setPosition({44.0F, 550.0F});
    portraitFrame.setFillColor(auraSpeaking ? sf::Color {73, 47, 88, 235} : sf::Color {46, 66, 82, 235});
    portraitFrame.setOutlineColor(auraSpeaking ? sf::Color {224, 164, 246} : sf::Color {188, 226, 255});
    portraitFrame.setOutlineThickness(3.0F);
    target.draw(portraitFrame);

    const sf::Texture* portrait = nullptr;
    if (dialogue->portrait.starts_with("frieren") && portraits.frieren)
        portrait = &*portraits.frieren;
    else if (dialogue->portrait == "aura-initial" && portraits.auraInitial)
        portrait = &*portraits.auraInitial;
    else if (dialogue->portrait == "aura-idle" && portraits.auraIdle)
        portrait = &*portraits.auraIdle;
    else if (dialogue->portrait == "aura-windup" && portraits.auraWindup)
        portrait = &*portraits.auraWindup;
    else if (dialogue->portrait == "aura-die" && portraits.auraDie)
        portrait = &*portraits.auraDie;
    else if (dialogue->portrait.starts_with("revolte-"))
    {
        const std::size_t index = static_cast<std::size_t>(dialogue->portrait.back() - '1');
        if (index < portraits.revolte.size() && portraits.revolte[index])
            portrait = &*portraits.revolte[index];
    }
    if (portrait)
    {
        sf::Sprite sprite(*portrait);
        const auto size = portrait->getSize();
        sprite.setPosition({44.0F, 550.0F});
        sprite.setScale({126.0F / static_cast<float>(size.x),
            126.0F / static_cast<float>(size.y)});
        target.draw(sprite);
    }

    drawPixelText(target, dialogue->speaker, {194.0F, 550.0F}, 1.55F,
        auraSpeaking ? sf::Color {225, 165, 250} : sf::Color {188, 225, 255});
    drawPixelText(target, dialogue->text, {194.0F, 597.0F}, 1.18F, sf::Color {240, 236, 224});
    drawPixelText(target, "ENTER", {1146.0F, 663.0F}, 0.9F, sf::Color {190, 184, 205});
}
}
