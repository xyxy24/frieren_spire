#include "game/player/PlayerController.hpp"

#include <cmath>
#include <iostream>
#include <string_view>

namespace
{
constexpr arcane::game::WorldBounds TestBounds {0.0F, 1280.0F, 640.0F};
constexpr float Tolerance = 0.001F;

bool nearlyEqual(const float left, const float right)
{
    return std::fabs(left - right) <= Tolerance;
}

bool expect(const bool condition, const std::string_view message)
{
    if (!condition)
    {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }

    return true;
}

bool movesHorizontally()
{
    arcane::game::PlayerController player;
    arcane::game::PlayerIntent intent;
    intent.moveAxis = 1.0F;

    player.update(intent, 0.5F, TestBounds);
    return expect(nearlyEqual(player.position().x, 290.0F), "player should move at the configured speed");
}

bool clampsToWorldBounds()
{
    arcane::game::PlayerController player({1230.0F, 576.0F});
    arcane::game::PlayerIntent intent;
    intent.moveAxis = 1.0F;

    player.update(intent, 1.0F, TestBounds);
    return expect(
        nearlyEqual(player.position().x, TestBounds.right - arcane::game::PlayerController::Width),
        "player should remain inside the right world boundary");
}

bool remembersLastMovementDirection()
{
    arcane::game::PlayerController player;
    arcane::game::PlayerIntent moveLeft;
    moveLeft.moveAxis = -1.0F;
    player.update(moveLeft, 0.01F, TestBounds);
    player.update(arcane::game::PlayerIntent {}, 0.01F, TestBounds);

    return expect(nearlyEqual(player.facingDirection(), -1.0F), "player should keep facing the last movement direction");
}

bool jumpsFromGround()
{
    arcane::game::PlayerController player;
    arcane::game::PlayerIntent intent;
    intent.jumpPressed = true;

    player.update(intent, 0.1F, TestBounds);
    return expect(player.position().y < 576.0F, "jump should move the player above the ground")
        && expect(player.velocity().y < 0.0F, "jump should produce upward velocity")
        && expect(!player.isGrounded(), "jumping player should not be grounded");
}

bool landsOnGround()
{
    arcane::game::PlayerController player({160.0F, 100.0F});
    const arcane::game::PlayerIntent intent;

    for (int step = 0; step < 200; ++step)
    {
        player.update(intent, 0.01F, TestBounds);
    }

    return expect(nearlyEqual(player.position().y, 576.0F), "falling player should land on the ground")
        && expect(nearlyEqual(player.velocity().y, 0.0F), "vertical velocity should reset after landing")
        && expect(player.isGrounded(), "landed player should be grounded");
}
}

int main()
{
    const bool passed = movesHorizontally()
        && clampsToWorldBounds()
        && remembersLastMovementDirection()
        && jumpsFromGround()
        && landsOnGround();

    if (!passed)
    {
        return 1;
    }

    std::cout << "All player controller tests passed.\n";
    return 0;
}
