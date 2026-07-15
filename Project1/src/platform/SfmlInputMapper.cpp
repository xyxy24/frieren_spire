#include "platform/SfmlInputMapper.hpp"

#include <SFML/Window/Keyboard.hpp>

namespace arcane::platform
{
void SfmlInputMapper::handleEvent(const sf::Event& event)
{
    const auto* keyPressed = event.getIf<sf::Event::KeyPressed>();
    if (keyPressed == nullptr) return;

    switch (keyPressed->code)
    {
    case sf::Keyboard::Key::Space: pendingIntent_.jumpPressed = true; break;
    case sf::Keyboard::Key::J: pendingIntent_.attackPressed = true; break;
    case sf::Keyboard::Key::K: pendingIntent_.dashPressed = true; break;
    case sf::Keyboard::Key::U: pendingIntent_.spellPressed[0] = true; break;
    case sf::Keyboard::Key::I: pendingIntent_.spellPressed[1] = true; break;
    case sf::Keyboard::Key::O: pendingIntent_.spellPressed[2] = true; break;
    case sf::Keyboard::Key::E:
        pendingIntent_.interactPressed = true;
        pendingIntent_.menuPageNextPressed = true;
        break;
    case sf::Keyboard::Key::Tab: pendingIntent_.toggleLoadoutPressed = true; break;
    case sf::Keyboard::Key::Q: pendingIntent_.menuPagePreviousPressed = true; break;
    case sf::Keyboard::Key::A:
    case sf::Keyboard::Key::Left: pendingIntent_.menuPreviousPressed = true; break;
    case sf::Keyboard::Key::D:
    case sf::Keyboard::Key::Right: pendingIntent_.menuNextPressed = true; break;
    case sf::Keyboard::Key::W:
    case sf::Keyboard::Key::Up: pendingIntent_.menuUpPressed = true; break;
    case sf::Keyboard::Key::S:
    case sf::Keyboard::Key::Down: pendingIntent_.menuDownPressed = true; break;
    case sf::Keyboard::Key::Escape: pendingIntent_.pausePressed = true; break;
    case sf::Keyboard::Key::Enter: pendingIntent_.menuConfirmPressed = true; break;
    case sf::Keyboard::Key::R:
        pendingIntent_.ultimateSpellPressed = true;
        pendingIntent_.menuSecondaryPressed = true;
        break;
    case sf::Keyboard::Key::F2: pendingIntent_.debugEventPreviewPressed = true; break;
    case sf::Keyboard::Key::F3: pendingIntent_.debugMerchantPreviewPressed = true; break;
    case sf::Keyboard::Key::F4:
        pendingIntent_.debugSpellAcquisitionPreviewPressed = true;
        break;
    default: break;
    }
}

game::PlayerIntent SfmlInputMapper::sample()
{
    const bool moveLeft = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)
        || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left);
    const bool moveRight = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)
        || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right);

    const bool menuUp = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)
        || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up);
    const bool menuDown = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)
        || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down);

    game::PlayerIntent intent = pendingIntent_;
    pendingIntent_ = {};
    intent.moveAxis = static_cast<float>(moveRight) - static_cast<float>(moveLeft);
    intent.verticalMoveAxis = static_cast<float>(menuDown) - static_cast<float>(menuUp);
    return intent;
}
}
