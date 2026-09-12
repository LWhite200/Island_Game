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
void updatePlayer (Player* player, bool upp, bool down, bool left, bool right,
                   IslandManager* islandManager);
void drawPlayer   (float x, float y, float z, float yaw);

// True the moment the player is resting on the ground (island or sea floor) --
// yVelocity gets snapped to exactly 0 on landing, so this doubles as a
// "can I jump right now?" check.
bool playerIsGrounded(const Player* player);
