// camera.c
// Third-person follow camera with C-stick orbit control.
//
// The player can push the C-stick left/right to orbit the camera around the
// active entity, and up/down to raise/lower the viewing angle. The orbital
// offset is stored in the global g_cameraYawOffset so player.c can use it
// to make movement feel camera-relative.
//
// Occlusion zoom: when an island is between the camera and the player the
// camera zooms in until the view is clear, then smoothly returns to the
// default distance.

#include "camera.h"
#include "island.h"
#include "player.h"
#include <math.h>

// ============================================================
// Initialisation
// ============================================================

void initCamera(Camera* camera) {
    camera->position       = (guVector){ 0.0f, 2.0f,  5.0f };
    camera->up             = (guVector){ 0.0f, 1.0f,  0.0f };
    camera->look           = (guVector){ 0.0f, 0.0f,  0.0f };
    camera->followDistance = 6.0f;
    camera->heightOffset   = 1.5f;
    camera->zoomLevel      = 0.0f;
    camera->minZoom        = -4.0f;
    camera->maxZoom        =  3.0f;
    camera->zoomSpeed      = 0.025f;
    camera->smoothingSpeed = 0.12f;
    camera->pitchOffset    = 0.0f;
    camera->pitchTarget    = 0.0f;
    camera->lookLag        = (guVector){ 0.0f, 0.0f, 0.0f };
}

// ============================================================
// Helpers
// ============================================================

static float lerp(float a, float b, float t) { return a + (b - a) * t; }

// Wrap angle to [-pi, pi]
static float wrapAngle(float a) {
    while (a >  M_PI) a -= 2.0f * M_PI;
    while (a < -M_PI) a += 2.0f * M_PI;
    return a;
}

// ============================================================
// Entity switch (player <-> boat)
// ============================================================

void cameraRetarget(Camera* camera, float oldCamYaw,
                    const guVector* oldPos, const guVector* newPos,
                    float newEntityYaw)
{
    // The camera heading is (entity yaw + g_cameraYawOffset).  The new entity
    // has a different yaw, so re-derive the offset that keeps the heading
    // exactly where it was -- otherwise the camera swings round to the new yaw.
    g_cameraYawOffset = wrapAngle(oldCamYaw - newEntityYaw);

    // Start the look target on the old entity and let it glide to the new one.
    camera->lookLag.x = oldPos->x - newPos->x;
    camera->lookLag.y = oldPos->y - newPos->y;
    camera->lookLag.z = oldPos->z - newPos->z;
}

// ============================================================
// Per-frame update
// ============================================================
// cYaw   -- C-stick X axis (orbit left/right), in [-1, 1] range
// cPitch -- C-stick Y axis (look up/down),     in [-1, 1] range

void updateCamera(Camera* camera,
                  const Boat*   boat,
                  const Player* player,
                  bool          isPlayerActive,
                  IslandManager* manager,
                  float cYaw, float cPitch)
{
    // ---- Update C-stick orbital offset ----
    g_cameraYawOffset = wrapAngle(g_cameraYawOffset + cYaw * 0.04f);

    // Pitch target: raise camera when C-stick pushed up
    camera->pitchTarget += cPitch * 0.04f;
    if (camera->pitchTarget >  1.2f) camera->pitchTarget =  1.2f;
    if (camera->pitchTarget < -0.3f) camera->pitchTarget = -0.3f;
    camera->pitchOffset = lerp(camera->pitchOffset, camera->pitchTarget, 0.15f);

    // ---- Select tracked entity ----
    const guVector* entityPos = isPlayerActive ? &player->position : &boat->position;
    float           entityYaw = isPlayerActive ?  player->yaw      :  boat->yaw;

    // Effective camera heading = entity yaw + orbital offset
    float camYaw = entityYaw + g_cameraYawOffset;

    // ---- Occlusion zoom ----
    Vec3 camV = { camera->position.x, camera->position.y, camera->position.z };
    Vec3 entV = { entityPos->x,       entityPos->y,       entityPos->z       };

    if (checkCameraPlayerCovered(camV, entV, manager)) {
        camera->zoomLevel -= camera->zoomSpeed;
        if (camera->zoomLevel < camera->minZoom) camera->zoomLevel = camera->minZoom;
    } else {
        if (camera->zoomLevel < 0.0f) {
            camera->zoomLevel += camera->zoomSpeed;
            if (camera->zoomLevel > 0.0f) camera->zoomLevel = 0.0f;
        }
    }

    float effectiveDist  = camera->followDistance + camera->zoomLevel;
    float zoomHeightComp = -camera->zoomLevel * 0.5f;

    // ---- Target position ----
    float targetX = entityPos->x + sinf(camYaw) * effectiveDist;
    float targetY = entityPos->y + camera->heightOffset
                  + zoomHeightComp + camera->pitchOffset * 2.5f;
    float targetZ = entityPos->z - cosf(camYaw) * effectiveDist;

    // Clamp camera from going underground
    float minCamY = entityPos->y + 0.5f;
    if (targetY < minCamY) targetY = minCamY;

    // ---- Smooth interpolation ----
    camera->position.x = lerp(camera->position.x, targetX, camera->smoothingSpeed);
    camera->position.y = lerp(camera->position.y, targetY, camera->smoothingSpeed);
    camera->position.z = lerp(camera->position.z, targetZ, camera->smoothingSpeed);

    // Look-at slightly above the entity's feet so it's centred in frame
    // (lookLag is non-zero only just after a player <-> boat switch.)
    const float lagDecay = 1.0f - camera->smoothingSpeed * 0.5f;
    camera->lookLag.x *= lagDecay;
    camera->lookLag.y *= lagDecay;
    camera->lookLag.z *= lagDecay;

    camera->look.x = entityPos->x + camera->lookLag.x;
    camera->look.y = entityPos->y + 0.5f + camera->lookLag.y;
    camera->look.z = entityPos->z + camera->lookLag.z;
}