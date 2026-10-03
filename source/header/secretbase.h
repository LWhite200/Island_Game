// secretbase.h
//
// The interior of a secret base. There is only ever ONE room -- every
// island's secret base leads to the same physical space, just like the
// entrance markers on the islands are the "front door" into it. This
// keeps the feature simple: no per-island room data, no extra memory,
// no loading -- entering just teleports the player into this shared room,
// and leaving teleports them back to whichever island entrance they used.
//
// The room lives at a fixed coordinate offset (SECRET_BASE_ROOM_ORIGIN_*)
// far away from the sailable world, so it never spatially overlaps real
// islands or the world boundary, and none of the outdoor collision code
// (island collision, world boundary, wave height) needs to know it exists.

#pragma once
#include "common.h"
#include <stdbool.h>

// ---- Room geometry ----
#define SECRET_BASE_ROOM_HALF_X   6.0f
#define SECRET_BASE_ROOM_HALF_Z   6.0f
#define SECRET_BASE_ROOM_HEIGHT   4.0f
#define SECRET_BASE_EXIT_RADIUS   1.2f

// Fixed spot in world-space where the room sits. Chosen far outside the
// sailable world (WORLD_RADIUS is 80) so it can never intersect an island.
#define SECRET_BASE_ROOM_ORIGIN_X 1000.0f
#define SECRET_BASE_ROOM_ORIGIN_Y 0.0f
#define SECRET_BASE_ROOM_ORIGIN_Z 0.0f

typedef struct {
    bool  active;         // True while the player is inside the room
    Vec3  exitWorldPos;   // Where to put the player back outside on leaving
    float exitWorldYaw;   // Yaw to restore on leaving
} SecretBase;

void initSecretBase(SecretBase* sb);

// Call when the player walks into an island's cave-mouth entrance.
// exitWorldPos/exitWorldYaw are remembered so leaving puts the player back
// where they entered. Writes the player's new (room-space) spawn position
// and yaw into *playerPos / *playerYaw.
void enterSecretBase(SecretBase* sb, Vec3 exitWorldPos, float exitWorldYaw,
                     Vec3* playerPos, float* playerYaw);

// Call once per frame while sb->active. If the player has reached the
// room's exit doorway, deactivates the room and writes the outside
// position/yaw to return to into *outExitPos / *outExitYaw, returning true.
bool updateSecretBaseExit(SecretBase* sb, Vec3 playerPos,
                          Vec3* outExitPos, float* outExitYaw);

// Keeps a position inside the room's four walls (flat floor, no gravity).
void clampToSecretBaseRoom(Vec3* pos, float radius);

void drawSecretBaseRoom(void);
