#include "game/combat/CombatSession.hpp"

#include <algorithm>
#include <array>

namespace arcane::game
{
namespace
{
constexpr std::array AuraPreBattleDialogue {
    CombatDialogueLineView {"AURA", "LUGNER'S PRESENCE IS GONE. IT SEEMS HE WAS KILLED.", "aura-initial"},
    CombatDialogueLineView {"FRIEREN", "NOT ONLY LUGNER. I DEFEATED ALL OF YOUR EXECUTIONERS.", "frieren-pre"},
    CombatDialogueLineView {"AURA", "IF I DEFEAT YOU HERE, THE RESULT WILL STILL BE ENOUGH.", "aura-idle"}
};
constexpr std::array AuraFirstDominationDialogue {
    CombatDialogueLineView {"AURA", "DOMINATION MAGIC!", "aura-windup"},
    CombatDialogueLineView {"AURA", "YOUR MANA HAS BARELY GROWN SINCE EIGHTY YEARS AGO.", "aura-windup"},
    CombatDialogueLineView {"FRIEREN", "...", "frieren"}
};
constexpr std::array AuraDefeatDialogue {
    CombatDialogueLineView {"FRIEREN", "KILL YOURSELF, AURA.", "frieren"},
    CombatDialogueLineView {"AURA", "IMPOSSIBLE... HOW COULD I...", "aura-die"}
};
constexpr std::array RevoltePreBattleDialogue {
    CombatDialogueLineView {"REVOLTE", "AN ELF MAGE. WHO ARE YOU?", "revolte-1"},
    CombatDialogueLineView {"FRIEREN", "FOUR BLADES... EVEN DEFENSIVE MAGIC COULD HARDLY TAKE THEM HEAD-ON.", "frieren"},
    CombatDialogueLineView {"REVOLTE", "I ASKED WHO YOU ARE.", "revolte-1"},
    CombatDialogueLineView {"FRIEREN", "DO YOU GIVE YOUR NAME EVERY TIME YOU EXTERMINATE VERMIN?", "frieren"}
};
constexpr std::array RevolteSecondPhaseDialogue {
    CombatDialogueLineView {"REVOLTE", "INTERESTING. IT SEEMS I MUST FIGHT YOU SERIOUSLY.", "revolte-2"},
    CombatDialogueLineView {"FRIEREN", "...", "frieren"}
};
constexpr std::array RevolteDefeatDialogue {
    CombatDialogueLineView {"REVOLTE", "YOU WIN, ELF.", "revolte-3"}
};
constexpr std::array WaterMirrorPreBattleDialogue {
    CombatDialogueLineView {"FRIEREN", "SO THIS IS THE TOP. THAT WAS EASIER THAN I EXPECTED.", "frieren"},
    CombatDialogueLineView {"WATER MIRROR DEMON", "...", "water-mirror"},
    CombatDialogueLineView {"FRIEREN", "COPIES THAT REPRODUCE EVEN INDIVIDUAL ABILITIES... INTERESTING.", "frieren"},
    CombatDialogueLineView {"WATER MIRROR DEMON", "...", "water-mirror"}
};
constexpr std::array WaterMirrorSecondPhaseDialogue {
    CombatDialogueLineView {"FRIEREN", "IS THAT... A COPY OF ME?", "frieren"},
    CombatDialogueLineView {"WATER MIRROR DEMON", "...", "frieren-copy"},
    CombatDialogueLineView {"FRIEREN", "THEN LET US SEE WHAT IT CAN DO.", "frieren"}
};
constexpr std::array WaterMirrorDefeatDialogue {
    CombatDialogueLineView {"WATER MIRROR DEMON", "...", "water-mirror-die"},
    CombatDialogueLineView {"FRIEREN", "WHEW. THAT WAS NOT EASY.", "frieren"}
};
}

bool CombatSession::updateBossPhaseRules()
{
    const auto defeatedAura = std::find_if(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
        return enemy.archetype == EnemyArchetype::Aura && !enemy.health.isAlive();
    });
    if (defeatedAura != enemies_.end())
    {
        for (auto& enemy : enemies_)
        {
            if (&enemy == &*defeatedAura || !enemy.health.isAlive()) continue;
            static_cast<void>(enemy.health.damage(enemy.health.current()));
            enemy.controller.markDead();
        }
    }

    auto waterMirror = std::find_if(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
        return enemy.archetype == EnemyArchetype::WaterMirrorDemon;
    });
    if (waterMirror != enemies_.end() && waterMirror->health.isAlive())
    {
        const bool starkAlive = std::any_of(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
            return enemy.archetype == EnemyArchetype::StarkCopy && enemy.health.isAlive();
        });
        const bool fernAlive = std::any_of(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
            return enemy.archetype == EnemyArchetype::FernCopy && enemy.health.isAlive();
        });
        const bool frierenPresent = std::any_of(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
            return enemy.archetype == EnemyArchetype::FrierenCopy;
        });
        if (!waterMirrorSecondPhase_ && !starkAlive && !fernAlive)
        {
            waterMirrorSecondPhase_ = true;
            static_cast<void>(waterMirror->health.damage(waterMirror->health.current() / 2));
            const auto config = enemyConfigFor(EnemyArchetype::FrierenCopy);
            Vec2 position {820.0F,
                request_.worldBounds.groundTop - FrierenCopyHoverHeight - config.height};
            const int copyHealth = waterMirror->health.maximum() == 200
                ? 350 : waterMirror->health.maximum();
            enemies_.push_back({EnemyArchetype::FrierenCopy,
                ai::EnemyController(position, config), Health(copyHealth, copyHealth)});
            enemies_.back().revolteCooldowns = {4.0F, 4.0F, 1.5F, 0.0F, 0.0F};
            beginDialogue(DialogueScript::WaterMirrorSecondPhase);
            return false;
        }
        const bool frierenAlive = std::any_of(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
            return enemy.archetype == EnemyArchetype::FrierenCopy && enemy.health.isAlive();
        });
        if (waterMirrorSecondPhase_ && frierenPresent && !frierenAlive)
        {
            static_cast<void>(waterMirror->health.damage(waterMirror->health.current()));
            waterMirror->controller.markDead();
        }
    }

    return true;
}

void CombatSession::resolveCombatEnd()
{
    if (std::none_of(enemies_.begin(), enemies_.end(), [](const auto& enemy) { return enemy.health.isAlive(); }))
    {
        clearCombatTransientEffects();
        const bool auraEncounter = std::any_of(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
            return enemy.archetype == EnemyArchetype::Aura;
        });
        const bool revolteEncounter = std::any_of(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
            return enemy.archetype == EnemyArchetype::Revolte;
        });
        const bool waterMirrorEncounter = std::any_of(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
            return enemy.archetype == EnemyArchetype::WaterMirrorDemon;
        });
        if (auraEncounter && !auraDefeatDialogueShown_)
        {
            auraDefeatDialogueShown_ = true;
            outcomeAfterDialogue_ = CombatOutcome::Victory;
            beginDialogue(DialogueScript::AuraDefeat);
        }
        else if (revolteEncounter && !revolteDefeatDialogueShown_)
        {
            revolteDefeatDialogueShown_ = true;
            outcomeAfterDialogue_ = CombatOutcome::Victory;
            beginDialogue(DialogueScript::RevolteDefeat);
        }
        else if (waterMirrorEncounter && !waterMirrorDefeatDialogueShown_)
        {
            waterMirrorDefeatDialogueShown_ = true;
            outcomeAfterDialogue_ = CombatOutcome::Victory;
            beginDialogue(DialogueScript::WaterMirrorDefeat);
        }
        else finish(CombatOutcome::Victory);
    }
    else if (!playerHealth_.isAlive()) finish(CombatOutcome::Defeat);
}

std::optional<CombatDialogueLineView> CombatSession::dialogueLine() const noexcept
{
    switch (dialogueScript_)
    {
    case DialogueScript::AuraPreBattle: return AuraPreBattleDialogue[dialogueLineIndex_];
    case DialogueScript::AuraFirstDomination: return AuraFirstDominationDialogue[dialogueLineIndex_];
    case DialogueScript::AuraDefeat: return AuraDefeatDialogue[dialogueLineIndex_];
    case DialogueScript::RevoltePreBattle: return RevoltePreBattleDialogue[dialogueLineIndex_];
    case DialogueScript::RevolteSecondPhase: return RevolteSecondPhaseDialogue[dialogueLineIndex_];
    case DialogueScript::RevolteDefeat: return RevolteDefeatDialogue[dialogueLineIndex_];
    case DialogueScript::WaterMirrorPreBattle: return WaterMirrorPreBattleDialogue[dialogueLineIndex_];
    case DialogueScript::WaterMirrorSecondPhase: return WaterMirrorSecondPhaseDialogue[dialogueLineIndex_];
    case DialogueScript::WaterMirrorDefeat: return WaterMirrorDefeatDialogue[dialogueLineIndex_];
    case DialogueScript::None: return std::nullopt;
    }
    return std::nullopt;
}
std::optional<BossIntroView> CombatSession::bossIntro() const noexcept
{
    if (bossIntroRemaining_ <= 0.0F) return std::nullopt;
    const bool revolte = std::any_of(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
        return enemy.archetype == EnemyArchetype::Revolte;
    });
    const bool waterMirror = std::any_of(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
        return enemy.archetype == EnemyArchetype::WaterMirrorDemon;
    });
    return BossIntroView {waterMirror ? "WATER MIRROR DEMON"
            : (revolte ? "REVOLTE" : "GUILLOTINE AURA"),
        bossIntroRemaining_, 2.4F};
}
void CombatSession::beginDialogue(const DialogueScript script) noexcept
{
    dialogueScript_ = script;
    dialogueLineIndex_ = 0U;
}
void CombatSession::advanceDialogue() noexcept
{
    std::size_t lineCount = 0U;
    switch (dialogueScript_)
    {
    case DialogueScript::AuraPreBattle: lineCount = AuraPreBattleDialogue.size(); break;
    case DialogueScript::AuraFirstDomination: lineCount = AuraFirstDominationDialogue.size(); break;
    case DialogueScript::AuraDefeat: lineCount = AuraDefeatDialogue.size(); break;
    case DialogueScript::RevoltePreBattle: lineCount = RevoltePreBattleDialogue.size(); break;
    case DialogueScript::RevolteSecondPhase: lineCount = RevolteSecondPhaseDialogue.size(); break;
    case DialogueScript::RevolteDefeat: lineCount = RevolteDefeatDialogue.size(); break;
    case DialogueScript::WaterMirrorPreBattle: lineCount = WaterMirrorPreBattleDialogue.size(); break;
    case DialogueScript::WaterMirrorSecondPhase: lineCount = WaterMirrorSecondPhaseDialogue.size(); break;
    case DialogueScript::WaterMirrorDefeat: lineCount = WaterMirrorDefeatDialogue.size(); break;
    case DialogueScript::None: return;
    }
    ++dialogueLineIndex_;
    if (dialogueLineIndex_ < lineCount) return;
    const DialogueScript completedScript = dialogueScript_;
    dialogueScript_ = DialogueScript::None;
    dialogueLineIndex_ = 0U;
    if (completedScript == DialogueScript::RevolteSecondPhase)
    {
        const auto revolte = std::find_if(enemies_.begin(), enemies_.end(), [](const auto& enemy) {
            return enemy.archetype == EnemyArchetype::Revolte && enemy.health.isAlive();
        });
        if (revolte != enemies_.end())
        {
            static_cast<void>(revolte->health.heal(
                revolte->health.maximum() - revolte->health.current()));
            revolte->revolteSecondPhase = true;
            revolte->revolteTransitionPending = false;
            revolte->revolteCooldowns = {6.0F, 6.0F, 7.5F, 8.0F, 2.2F, 3.5F, 5.0F};
        }
    }
    if (outcomeAfterDialogue_)
    {
        const CombatOutcome outcome = *outcomeAfterDialogue_;
        outcomeAfterDialogue_.reset();
        finish(outcome);
    }
}
}
