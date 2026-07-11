#pragma once
#include "game/run/RunTypes.hpp"
#include <span>
namespace arcane::game::events
{
struct EventChoice
{
    run::ContentId id {};
    int hpDelta {};
    int goldDelta {};
    run::ContentId relicId {};
    int maxHpDelta {};
    run::ContentId spellId {};
    bool restoreToMaximum {};
};
enum class EventResult : std::uint8_t { Success, ChoiceNotFound, AlreadyResolved, InvalidOutcome, AlreadyOwned };
class EventTransaction
{
public:
    [[nodiscard]] EventResult choose(run::PlayerProgress& player, std::span<const EventChoice> choices,
        run::ContentId choiceId);
    [[nodiscard]] bool resolved() const noexcept;
private:
    bool resolved_ {};
};
}
