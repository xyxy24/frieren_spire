#include "game/player/PlayerController.hpp"

#include <cmath>
#include <iostream>
#include <string_view>
#include <vector>

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

bool passesThroughAndLandsOnOneWayPlatform()
{
    arcane::game::PlayerController player({300.0F, 576.0F});
    const std::vector platforms {arcane::game::Aabb {250.0F, 536.0F, 300.0F, 28.0F}};
    arcane::game::PlayerIntent jump;
    jump.jumpPressed = true;
    player.update(jump, 0.01F, TestBounds, platforms);

    bool roseAbovePlatform = false;
    for (int step = 0; step < 200; ++step)
    {
        player.update({}, 0.01F, TestBounds, platforms);
        if (player.position().y + arcane::game::PlayerController::Height < platforms.front().top)
            roseAbovePlatform = true;
        if (roseAbovePlatform && player.isGrounded()) break;
    }

    return expect(roseAbovePlatform, "player should pass upward through a one-way platform")
        && expect(nearlyEqual(player.position().y,
            platforms.front().top - arcane::game::PlayerController::Height),
            "falling player should land on the platform top")
        && expect(player.isGrounded(), "player standing on a platform should be grounded");
}

bool walksOffOneWayPlatform()
{
    const std::vector platforms {arcane::game::Aabb {250.0F, 500.0F, 300.0F, 28.0F}};
    arcane::game::PlayerController player({300.0F,
        platforms.front().top - arcane::game::PlayerController::Height});
    player.update({}, 0.01F, TestBounds, platforms);

    arcane::game::PlayerIntent moveLeft;
    moveLeft.moveAxis = -1.0F;
    player.update(moveLeft, 0.5F, TestBounds, platforms);
    if (!expect(!player.isGrounded(), "leaving a platform edge should clear grounded state"))
        return false;
    const float platformHeight = player.position().y;
    player.update({}, 0.1F, TestBounds, platforms);
    return expect(player.position().y > platformHeight,
        "player should begin falling after walking off a platform");
}

bool dropsThroughOneWayPlatformWithDownAndJump()
{
    const std::vector platforms {arcane::game::Aabb {250.0F, 500.0F, 300.0F, 28.0F}};
    const float standingY = platforms.front().top - arcane::game::PlayerController::Height;
    arcane::game::PlayerController player({300.0F, standingY});
    player.update({}, 0.01F, TestBounds, platforms);

    arcane::game::PlayerIntent drop;
    drop.verticalMoveAxis = 1.0F;
    drop.jumpPressed = true;
    player.update(drop, 0.01F, TestBounds, platforms);
    if (!expect(!player.isGrounded(), "down plus jump should leave a one-way platform")
        || !expect(player.position().y > standingY,
            "dropping through should move the player below the platform top")
        || !expect(player.velocity().y > 0.0F,
            "dropping through should start with downward velocity"))
        return false;

    for (int step = 0; step < 200 && !player.isGrounded(); ++step)
        player.update({}, 0.01F, TestBounds, platforms);
    return expect(nearlyEqual(player.position().y, 576.0F),
            "a player dropping through a platform should land on the base ground")
        && expect(player.isGrounded(), "base ground should stop a platform drop");
}

bool downAndJumpStillJumpsFromBaseGround()
{
    arcane::game::PlayerController player;
    arcane::game::PlayerIntent intent;
    intent.verticalMoveAxis = 1.0F;
    intent.jumpPressed = true;
    player.update(intent, 0.01F, TestBounds);
    return expect(player.velocity().y < 0.0F,
            "down plus jump on base ground should remain a normal jump")
        && expect(player.position().y < 576.0F,
            "base ground must never behave like a drop-through platform");
}

bool dashUsesNormalCooldownAndIndependentShadeCharge()
{
    arcane::game::PlayerController player;
    arcane::game::PlayerIntent dash;
    dash.dashPressed = true;
    player.update(dash, 0.01F, TestBounds);
    if (!expect(player.position().x > 160.0F
            && player.position().x < 160.0F + arcane::game::PlayerController::DashDistance,
            "dash must travel over time instead of teleporting")
        || !expect(player.isDashing() && player.isShadowDashing(),
            "the initially charged dash must start as a Shade Dash")
        || !expect(nearlyEqual(player.dashCooldownRemaining(),
            arcane::game::PlayerController::DashCooldownSeconds),
            "every dash must start the short normal-dash cooldown")
        || !expect(nearlyEqual(player.shadowDashChargeRemaining(),
            arcane::game::PlayerController::ShadowDashChargeSeconds),
            "Shade Dash must start its independent charge timer")) return false;

    arcane::game::PlayerIntent steerAgainstDash;
    steerAgainstDash.moveAxis = -1.0F;
    player.update(steerAgainstDash, 0.17F, TestBounds);
    const float completedDashX = player.position().x;
    player.update({}, 0.43F, TestBounds);
    player.update(dash, 0.01F, TestBounds);
    return expect(nearlyEqual(completedDashX,
            160.0F + arcane::game::PlayerController::DashDistance),
            "dash must cover exactly 150 pixels over 0.18 seconds")
        && expect(nearlyEqual(player.facingDirection(), 1.0F),
            "dash direction must remain locked until the dash ends")
        && expect(player.position().x > completedDashX && player.isDashing(),
            "normal dash must be usable once its short cooldown ends even while Shade is charging")
        && expect(!player.isShadowDashing(),
            "a dash used before Shade finishes charging must be a normal, non-invulnerable dash")
        && expect(player.shadowDashChargeRemaining() > 0.0F,
            "normal dash must not reset or consume the independent Shade charge");
}
}

int main()
{
    const bool passed = movesHorizontally()
        && clampsToWorldBounds()
        && remembersLastMovementDirection()
        && jumpsFromGround()
        && landsOnGround()
        && passesThroughAndLandsOnOneWayPlatform()
        && walksOffOneWayPlatform()
        && dropsThroughOneWayPlatformWithDownAndJump()
        && downAndJumpStillJumpsFromBaseGround()
        && dashUsesNormalCooldownAndIndependentShadeCharge();

    if (!passed)
    {
        return 1;
    }

    std::cout << "All player controller tests passed.\n";
    return 0;
}
