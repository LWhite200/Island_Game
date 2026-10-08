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

    // Offset between the old and new look target after a player <-> boat
    // switch; it fades out so the view glides to the new target instead of
    // jumping.
    guVector lookLag;
} Camera;

void initCamera   (Camera* camera);

// Call when the tracked entity changes (player <-> boat) INSTEAD of
// initCamera().  Keeps the camera's current heading and position, and eases
// the look target across, so there is no spin or snap.
//   oldCamYaw    -- camera heading before the switch (old entity yaw + g_cameraYawOffset)
//   oldPos/newPos-- position of the old / new entity
//   newEntityYaw -- yaw of the new entity
void cameraRetarget(Camera* camera, float oldCamYaw,
                    const guVector* oldPos, const guVector* newPos,
                    float newEntityYaw);

void updateCamera (Camera* camera, const Boat* boat, const Player* player,
                   bool isPlayerActive, IslandManager* manager,
                   float cYaw, float cPitch);   // C-stick axes