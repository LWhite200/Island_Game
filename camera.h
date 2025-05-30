// camera.h
#ifndef CAMERA_H
#define CAMERA_H

#include <gccore.h>
#include <stdbool.h>
#include "boat.h"
#include "player.h"

typedef struct {
    guVector position;
    guVector up;
    guVector look;
    float followDistance;
    float heightOffset;

    float zoomLevel;     // Additional zoom modifier
    float minZoom;
    float maxZoom;
    float zoomSpeed;

    float smoothingSpeed;  // Smoothing factor (e.g., 0.1f)
} Camera;


void initCamera(Camera* camera);
void updateCamera(Camera* camera, const Boat* boat, const Player* player, bool isPlayerActive, IslandManager* manager);

#endif // CAMERA_H
