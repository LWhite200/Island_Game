// camera.c
#include "camera.h"
#include "manager.h"
#include <math.h>

void initCamera(Camera* camera) {
    camera->position.x = 0.0f;
    camera->position.y = 0.5f;
    camera->position.z = 5.0f;
    camera->up.x = 0.0f;
    camera->up.y = 1.0f;
    camera->up.z = 0.0f;
    camera->look.x = 0.0f;
    camera->look.y = 0.0f;
    camera->look.z = 0.0f;
    camera->followDistance = 5.0f;
    camera->heightOffset = 0.5f;

    camera->zoomLevel = 0.0f;
    camera->minZoom = -5.0f; // max zoom in
    camera->maxZoom = 5.0f;  // max zoom out
    camera->zoomSpeed = 0.02f;

    camera->smoothingSpeed = 0.5f;
}

static float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}


void updateCamera(Camera* camera, const Boat* boat, const Player* player, bool isPlayerActive, IslandManager* manager) {
    const guVector* pos;
    float yaw;

    if (isPlayerActive) {
        pos = &player->position;
        yaw = player->yaw;
    }
    else {
        pos = &boat->position;
        yaw = boat->yaw;
    }

    // Convert guVector to Vec3 for compatibility
    Vec3 camPos = { camera->position.x, camera->position.y, camera->position.z };
    Vec3 playerPos = { pos->x, pos->y, pos->z };


    // move until false or equal to player???
    if (checkCameraPlayerCovered(camPos, playerPos, manager)) {
        
        if (camera->zoomLevel <= camera->minZoom) {
            camera->zoomLevel = camera->minZoom;
        } else {
            camera->zoomLevel -= camera->zoomSpeed;
        }
    }
    else {
        if (camera->zoomLevel < 0.0f) {
            camera->zoomLevel += camera->zoomSpeed;
            if (camera->zoomLevel > 0.0f)
                camera->zoomLevel = 0.0f;
        }
        else if (camera->zoomLevel > 0.0f) {
            camera->zoomLevel -= camera->zoomSpeed;
            if (camera->zoomLevel < 0.0f)
                camera->zoomLevel = 0.0f;
        }
    }

    float adjustedDistance = camera->followDistance + camera->zoomLevel;
    float zoomYOffset = -camera->zoomLevel * 0.6f;

    // Target camera position
    float targetX = pos->x + sinf(yaw) * adjustedDistance;
    float targetY = pos->y + camera->heightOffset + zoomYOffset;
    float targetZ = pos->z - cosf(yaw) * adjustedDistance;

    // Smooth interpolation toward target
    camera->position.x = lerp(camera->position.x, targetX, camera->smoothingSpeed);
    camera->position.y = lerp(camera->position.y, targetY, camera->smoothingSpeed);
    camera->position.z = lerp(camera->position.z, targetZ, camera->smoothingSpeed);

    // Look-at point (no need to lerp unless you want a "lazy" camera)
    camera->look.x = pos->x;
    camera->look.y = pos->y;
    camera->look.z = pos->z;

}
