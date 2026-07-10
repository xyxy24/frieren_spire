#include "platform/SfmlInputMapper.hpp"

#include <SFML/Window/Keyboard.hpp>

namespace arcane::platform
{
namespace
{
bool pressedOnce(const bool current, bool& previous)
{
    const bool pressed = current && !previous;
    previous = current;
    return pressed;
}
}

game::PlayerIntent SfmlInputMapper::sample()
{
    const bool moveLeft = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)
        || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left);
    const bool moveRight = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)
        || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right);

    const bool jump = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);
    const bool attack = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::J);
    const bool spell0 = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::U);
    const bool spell1 = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::I);
    const bool spell2 = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::O);
    const bool interact = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E);
    const bool toggleLoadout = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Tab);
    const bool pause = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape);
    const bool confirm = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Enter);
    const bool secondary = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R);

    game::PlayerIntent intent;
    intent.moveAxis = static_cast<float>(moveRight) - static_cast<float>(moveLeft);
    intent.jumpPressed = pressedOnce(jump, previousJump_);
    intent.attackPressed = pressedOnce(attack, previousAttack_);
    intent.spellPressed[0] = pressedOnce(spell0, previousSpell0_);
    intent.spellPressed[1] = pressedOnce(spell1, previousSpell1_);
    intent.spellPressed[2] = pressedOnce(spell2, previousSpell2_);
    intent.interactPressed = pressedOnce(interact, previousInteract_);
    intent.toggleLoadoutPressed = pressedOnce(toggleLoadout, previousToggleLoadout_);
    intent.menuPreviousPressed = pressedOnce(moveLeft, previousMenuLeft_);
    intent.menuNextPressed = pressedOnce(moveRight, previousMenuRight_);
    intent.pausePressed = pressedOnce(pause, previousPause_);
    intent.menuConfirmPressed = pressedOnce(confirm, previousConfirm_);
    intent.menuSecondaryPressed = pressedOnce(secondary, previousSecondary_);
    return intent;
}
}
