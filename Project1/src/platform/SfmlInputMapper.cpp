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
    const bool dash = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::K);
    const bool guard = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::L);
    const bool spell0 = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::U);
    const bool spell1 = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::I);
    const bool spell2 = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::O);
    const bool interact = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E);
    const bool toggleLoadout = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Tab);
    const bool menuPageLeft = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q);
    const bool menuPageRight = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E);
    const bool menuUp = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)
        || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up);
    const bool menuDown = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)
        || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down);
    const bool pause = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape);
    const bool confirm = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Enter);
    const bool secondary = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R);
    const bool debugEventPreview = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F2);
    const bool debugMerchantPreview = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F3);

    game::PlayerIntent intent;
    intent.moveAxis = static_cast<float>(moveRight) - static_cast<float>(moveLeft);
    intent.verticalMoveAxis = static_cast<float>(menuDown) - static_cast<float>(menuUp);
    intent.jumpPressed = pressedOnce(jump, previousJump_);
    intent.attackPressed = pressedOnce(attack, previousAttack_);
    intent.dashPressed = pressedOnce(dash, previousDash_);
    intent.guardPressed = pressedOnce(guard, previousGuard_);
    intent.spellPressed[0] = pressedOnce(spell0, previousSpell0_);
    intent.spellPressed[1] = pressedOnce(spell1, previousSpell1_);
    intent.spellPressed[2] = pressedOnce(spell2, previousSpell2_);
    intent.interactPressed = pressedOnce(interact, previousInteract_);
    intent.toggleLoadoutPressed = pressedOnce(toggleLoadout, previousToggleLoadout_);
    intent.menuPreviousPressed = pressedOnce(moveLeft, previousMenuLeft_);
    intent.menuNextPressed = pressedOnce(moveRight, previousMenuRight_);
    intent.menuPagePreviousPressed = pressedOnce(menuPageLeft, previousMenuPageLeft_);
    intent.menuPageNextPressed = pressedOnce(menuPageRight, previousMenuPageRight_);
    intent.menuUpPressed = pressedOnce(menuUp, previousMenuUp_);
    intent.menuDownPressed = pressedOnce(menuDown, previousMenuDown_);
    intent.pausePressed = pressedOnce(pause, previousPause_);
    intent.menuConfirmPressed = pressedOnce(confirm, previousConfirm_);
    const bool secondaryPressed = pressedOnce(secondary, previousSecondary_);
    intent.ultimateSpellPressed = secondaryPressed;
    intent.menuSecondaryPressed = secondaryPressed;
    intent.debugEventPreviewPressed = pressedOnce(debugEventPreview, previousDebugEventPreview_);
    intent.debugMerchantPreviewPressed = pressedOnce(debugMerchantPreview, previousDebugMerchantPreview_);
    return intent;
}
}
