// boat.h
#pragma once
#include "common.h"
#include "island.h"
#include <stdbool.h>
#include <gccore.h>

typedef struct {
    guVector position;   // XZ = stored world position; Y is set by wave formula at draw time
    float    yaw;
    float    speed;
    float    radius;     // Collision sphere radius
} Boat;

void initBoat   (Boat* boat);
bool updateBoat (Boat* boat, bool upp, bool down, bool left, bool right,
                 float time, IslandManager* islandManager);
// darken: 0 = normal colours, 1 = fully dimmed   (the "switching" cue)
// scale : 1 = full size, 0 = invisible            (shrink out / grow in)
void drawBoat   (float x, float y, float z, float yaw, float darken, float scale);

void boatLaunch (Boat* boat, float x, float z, float yaw,
                 IslandManager* islandManager);
