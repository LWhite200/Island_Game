// camera.h
#pragma once
#include "common.h"
#include "boat.h"
#include "player.h"
#include <gccore.h>
#include <stdbool.h>

// Forward declaration
typedef struct IslandManager IslandManager;

typedef struct {
    guVector position;
    guVector up;
    guVector look;

    float followDistance;
    float heightOffset;

    // Occlusion-driven zoom
    float zoomLevel;
    float minZoom;
    float maxZoom;
    float zoomSpeed;

    // Smoothing
    float smoothingSpeed;

    // Pitch offset (C-stick up/down)
    float pitchOffset;      // Extra height for looking up/down
    float pitchTarget;
} Camera;

void initCamera   (Camera* camera);
void updateCamera (Camera* camera, const Boat* boat, const Player* player,
                   bool isPlayerActive, IslandManager* manager,
                   float cYaw, float cPitch);   // C-stick axes
