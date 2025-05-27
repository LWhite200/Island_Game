#ifndef BOAT_H
#define BOAT_H

#include "manager.h"
#include "common.h"

// Boat structure to hold boat-related data
typedef struct {
    guVector position;
    float yaw;
    float speed;
    float radius;
} Boat;

void initBoat(Boat* boat);
void updateBoat(Boat* boat, bool upp, bool down, bool left, bool right, float time, IslandManager* islandManager);
void drawBoat(float x, float y, float z, float yaw);

#endif