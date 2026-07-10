#include "game/events/EventSystem.hpp"
#include <algorithm>
namespace arcane::game::events
{
EventResult EventTransaction::choose(run::PlayerProgress& player, const std::span<const EventChoice> choices,
    const run::ContentId choiceId)
{
    if (resolved_) return EventResult::AlreadyResolved;
    const auto choice = std::find_if(choices.begin(), choices.end(), [choiceId](const EventChoice& value) { return value.id == choiceId; });
    if (choice == choices.end()) return EventResult::ChoiceNotFound;
    const int newHp = player.currentHp + choice->hpDelta;
    const int newGold = player.gold + choice->goldDelta;
    if (newHp <= 0 || newHp > player.maxHp || newGold < 0) return EventResult::InvalidOutcome;
    if (choice->relicId != 0U && std::find(player.relics.begin(), player.relics.end(), choice->relicId) != player.relics.end())
        return EventResult::AlreadyOwned;
    player.currentHp = newHp;
    player.gold = newGold;
    if (choice->relicId != 0U) player.relics.push_back(choice->relicId);
    resolved_ = true;
    return EventResult::Success;
}
bool EventTransaction::resolved() const noexcept { return resolved_; }
}
