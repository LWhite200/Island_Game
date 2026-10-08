// player.h
#pragma once
#include "common.h"
#include "island.h"
#include <stdbool.h>
#include <gccore.h>

typedef struct {
    guVector position;
    float    yaw;
    float    speed;
    float    radius;

    // Vertical physics
    float    yVelocity;
    float    gravity;
} Player;

// Camera-relative yaw offset controlled by the C-stick
// (lives here so camera.c can read it)
extern float g_cameraYawOffset;

void initPlayer   (Player* player);
bool updatePlayer (Player* player, bool upp, bool down, bool left, bool right,
                   IslandManager* islandManager);
// darken: 0 = normal colours, 1 = fully dimmed   (the "switching" cue)
// scale : 1 = full size, 0 = invisible            (shrink out / grow in)
void drawPlayer   (float x, float y, float z, float yaw, float darken, float scale);

// True the moment the player is resting on the ground (island or sea floor) --
// yVelocity gets snapped to exactly 0 on landing, so this doubles as a
// "can I jump right now?" check.
bool playerIsGrounded(const Player* player);

bool playerLandAhead(Player* player, float boatX, float boatZ, float boatYaw,
                     IslandManager* islandManager);
