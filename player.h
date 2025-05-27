#ifndef PLAYER_H
#define PLAYER_H

#include "manager.h"
#include "common.h"

// Player structure to hold player-related data
typedef struct {
    guVector position;
    float yaw;
    float speed;
    float radius;
    float gravity;
    float upPush;
} Player;

void initPlayer(Player* player);
void updatePlayer(Player* player, bool upp, bool down, bool left, bool right, float time, IslandManager* islandManager);
void drawPlayer(float x, float y, float z, float yaw);

#endif