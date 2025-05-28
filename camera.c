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
    camera->minZoom = -3.5f; // max zoom in
    camera->maxZoom = 3.0f;  // max zoom out
    camera->zoomSpeed = 0.025f;

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
        camera->zoomLevel -= camera->zoomSpeed;
        if (camera->zoomLevel < camera->minZoom)
            camera->zoomLevel = camera->minZoom;
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

    camera->position.x = pos->x + sinf(yaw) * adjustedDistance;
    camera->position.y = pos->y + camera->heightOffset + zoomYOffset;
    camera->position.z = pos->z - cosf(yaw) * adjustedDistance;

    camera->look.x = pos->x;
    camera->look.y = pos->y;
    camera->look.z = pos->z;
}
