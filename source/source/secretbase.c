// secretbase.c
#include <gccore.h>
#include <math.h>
#include "secretbase.h"

// Player spawns just inside the "back" of the room (near side, close to
// where they walked in) and the exit doorway is at the far wall -- so
// walking forward through the room takes you back outside, which reads
// naturally without needing a button prompt.
static const Vec3 ROOM_SPAWN = {
    SECRET_BASE_ROOM_ORIGIN_X, SECRET_BASE_ROOM_ORIGIN_Y, SECRET_BASE_ROOM_ORIGIN_Z - 4.0f
};
static const Vec3 ROOM_EXIT = {
    SECRET_BASE_ROOM_ORIGIN_X, SECRET_BASE_ROOM_ORIGIN_Y, SECRET_BASE_ROOM_ORIGIN_Z + 4.0f
};

void initSecretBase(SecretBase* sb) {
    sb->active        = false;
    sb->exitWorldPos  = (Vec3){ 0.0f, 0.0f, 0.0f };
    sb->exitWorldYaw  = 0.0f;
}

void enterSecretBase(SecretBase* sb, Vec3 exitWorldPos, float exitWorldYaw,
                     Vec3* playerPos, float* playerYaw)
{
    sb->active       = true;
    sb->exitWorldPos = exitWorldPos;
    sb->exitWorldYaw = exitWorldYaw;

    *playerPos = ROOM_SPAWN;
    *playerYaw = 0.0f; // face into the room
}

bool updateSecretBaseExit(SecretBase* sb, Vec3 playerPos,
                          Vec3* outExitPos, float* outExitYaw)
{
    if (!sb->active) return false;

    float dx = playerPos.x - ROOM_EXIT.x;
    float dz = playerPos.z - ROOM_EXIT.z;
    if (dx*dx + dz*dz <= SECRET_BASE_EXIT_RADIUS * SECRET_BASE_EXIT_RADIUS) {
        sb->active   = false;
        *outExitPos  = sb->exitWorldPos;
        *outExitYaw  = sb->exitWorldYaw;
        return true;
    }
    return false;
}

void clampToSecretBaseRoom(Vec3* pos, float radius) {
    float hx = SECRET_BASE_ROOM_HALF_X - radius;
    float hz = SECRET_BASE_ROOM_HALF_Z - radius;

    float localX = pos->x - SECRET_BASE_ROOM_ORIGIN_X;
    float localZ = pos->z - SECRET_BASE_ROOM_ORIGIN_Z;

    if (localX < -hx) localX = -hx;
    if (localX >  hx) localX =  hx;
    if (localZ < -hz) localZ = -hz;
    if (localZ >  hz) localZ =  hz;

    pos->x = SECRET_BASE_ROOM_ORIGIN_X + localX;
    pos->z = SECRET_BASE_ROOM_ORIGIN_Z + localZ;
    pos->y = SECRET_BASE_ROOM_ORIGIN_Y; // flat floor, no vertical physics inside
}

// ============================================================
// Rendering
// ============================================================

static void quad(Vec3 a, Vec3 b, Vec3 c, Vec3 d, float r, float g, float bl) {
    GX_Position3f32(a.x, a.y, a.z); GX_Color3f32(r, g, bl);
    GX_Position3f32(b.x, b.y, b.z); GX_Color3f32(r, g, bl);
    GX_Position3f32(c.x, c.y, c.z); GX_Color3f32(r, g, bl);
    GX_Position3f32(d.x, d.y, d.z); GX_Color3f32(r, g, bl);
}

void drawSecretBaseRoom(void) {
    const float ox = SECRET_BASE_ROOM_ORIGIN_X;
    const float oy = SECRET_BASE_ROOM_ORIGIN_Y;
    const float oz = SECRET_BASE_ROOM_ORIGIN_Z;

    const float hx = SECRET_BASE_ROOM_HALF_X;
    const float hz = SECRET_BASE_ROOM_HALF_Z;
    const float ht = SECRET_BASE_ROOM_HEIGHT;
    const float gap = 2.0f; // half-width of the open doorway in the far wall

    // 7 quads total: floor, ceiling, back wall, left wall, right wall,
    // and two far-wall panels flanking the exit gap = 28 vertices.
    GX_Begin(GX_QUADS, GX_VTXFMT0, 28);

    // Floor
    quad((Vec3){ox-hx, oy, oz-hz}, (Vec3){ox+hx, oy, oz-hz},
         (Vec3){ox+hx, oy, oz+hz}, (Vec3){ox-hx, oy, oz+hz},
         0.35f, 0.28f, 0.20f);

    // Ceiling
    quad((Vec3){ox-hx, oy+ht, oz+hz}, (Vec3){ox+hx, oy+ht, oz+hz},
         (Vec3){ox+hx, oy+ht, oz-hz}, (Vec3){ox-hx, oy+ht, oz-hz},
         0.15f, 0.13f, 0.12f);

    // Back wall (the side the player spawns near, at -Z)
    quad((Vec3){ox-hx, oy, oz-hz}, (Vec3){ox-hx, oy+ht, oz-hz},
         (Vec3){ox+hx, oy+ht, oz-hz}, (Vec3){ox+hx, oy, oz-hz},
         0.30f, 0.22f, 0.15f);

    // Left wall
    quad((Vec3){ox-hx, oy, oz-hz}, (Vec3){ox-hx, oy, oz+hz},
         (Vec3){ox-hx, oy+ht, oz+hz}, (Vec3){ox-hx, oy+ht, oz-hz},
         0.28f, 0.20f, 0.14f);

    // Right wall
    quad((Vec3){ox+hx, oy, oz+hz}, (Vec3){ox+hx, oy, oz-hz},
         (Vec3){ox+hx, oy+ht, oz-hz}, (Vec3){ox+hx, oy+ht, oz+hz},
         0.28f, 0.20f, 0.14f);

    // Far wall (+Z), split into two panels with a gap in the middle -- the
    // open doorway back outside.
    quad((Vec3){ox-hx, oy, oz+hz}, (Vec3){ox-gap, oy, oz+hz},
         (Vec3){ox-gap, oy+ht, oz+hz}, (Vec3){ox-hx, oy+ht, oz+hz},
         0.30f, 0.22f, 0.15f);
    quad((Vec3){ox+gap, oy, oz+hz}, (Vec3){ox+hx, oy, oz+hz},
         (Vec3){ox+hx, oy+ht, oz+hz}, (Vec3){ox+gap, oy+ht, oz+hz},
         0.30f, 0.22f, 0.15f);

    GX_End();

    // A small glowing floor patch marking the exit, so it reads clearly
    // even without a UI prompt.
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
    GX_Position3f32(ox-0.8f, oy+0.01f, oz+hz-1.4f); GX_Color3f32(1.0f, 0.85f, 0.4f);
    GX_Position3f32(ox+0.8f, oy+0.01f, oz+hz-1.4f); GX_Color3f32(1.0f, 0.85f, 0.4f);
    GX_Position3f32(ox+0.8f, oy+0.01f, oz+hz-0.2f); GX_Color3f32(1.0f, 0.85f, 0.4f);
    GX_Position3f32(ox-0.8f, oy+0.01f, oz+hz-0.2f); GX_Color3f32(1.0f, 0.85f, 0.4f);
    GX_End();
}
