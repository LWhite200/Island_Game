// main.c
// Entry point and main game loop.
//
// This is deliberately bare-bones: sail a boat around an ocean full of
// hemisphere islands, or hop out and walk around on foot. That's it --
// no enemies, no collectibles, no HUD. Just the core loop, so it's easy
// to see exactly what's going on before building anything back on top.
//
// Controls
// ------------------------------------------------------------
//  Left stick         - move / steer
//  C-stick left/right - orbit camera around the active entity
//  C-stick up/down    - raise / lower camera angle
//  A button           - regenerate the world (debug)
//  B button           - board / disembark the boat
//  Z / R button       - jump (on foot)
//  D-pad up           - open the debug menu (see debug.c for its controls)
//  START              - quit

#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include "gx_utils.h"
#include "common.h"
#include "water.h"
#include "island.h"
#include "boat.h"
#include "player.h"
#include "camera.h"
#include "boundary.h"
#include "debug.h"

#define GP_FIFO_SIZE (256 * 1024)

int main(void) {
    // ---- Hardware init ----
    PAD_Init();
    void* gpFifo = memalign(32, GP_FIFO_SIZE);
    init_graphics(gpFifo, GP_FIFO_SIZE); // video + GX pipeline, see gx_utils.c

    // ---- World / entities ----
    IslandManager world;
    initIslandManager(&world);
    regenerateIslands(&world);

    Boat   boat;
    Player player;
    Camera camera;
    initBoat(&boat);
    initPlayer(&player);
    initCamera(&camera);
    initDebugMenu();

    bool isPlayerActive = true; // false = sailing the boat, true = on foot

    u32 fb   = 0;
    f32 time = 0.0f;

    // ============================================================
    // Main loop
    // ============================================================

    while (1) {
        PAD_ScanPads();

        if (PAD_ButtonsDown(0) & PAD_BUTTON_START) exit(0);

        // Handle Debug menu inputs
        updateDebugMenu();

        // ---- C-stick (camera orbit) ----
        const float CSTICK_DEAD = 8.0f;
        float cStickX = (float)PAD_SubStickX(0);
        float cStickY = (float)PAD_SubStickY(0);
        float cYaw    = (fabsf(cStickX) > CSTICK_DEAD) ? cStickX / 80.0f : 0.0f;
        float cPitch  = (fabsf(cStickY) > CSTICK_DEAD) ? cStickY / 80.0f : 0.0f;

        // ---- Main stick (movement) ----
        const float DEAD_ZONE = 2.0f;
        float jx = (float)PAD_StickX(0);
        float jy = (float)PAD_StickY(0);
        bool moveFwd   = jy >  DEAD_ZONE;
        bool moveBack  = jy < -DEAD_ZONE;
        bool moveLeft  = jx < -DEAD_ZONE;
        bool moveRight = jx >  DEAD_ZONE;

        // ---- Buttons ----
        bool btnA = (PAD_ButtonsDown(0) & PAD_BUTTON_A)  != 0;
        bool btnB = (PAD_ButtonsDown(0) & PAD_BUTTON_B)  != 0;
        bool btnZ = (PAD_ButtonsDown(0) & PAD_TRIGGER_Z) != 0;
        bool btnR = (PAD_ButtonsDown(0) & PAD_TRIGGER_R) != 0;

        bool jump  = btnR && isPlayerActive;
        bool reset = btnZ;

        // ---- Z: reset player to starting position ----
        if (reset) {
            player.position.x = 0.0f;
            player.position.y = 25.0f;
            player.position.z = 0.0f;

            player.yaw       = 0.0f;
            player.yVelocity = 0.0f;

            // Always return to player mode.
            isPlayerActive = true;

            // Reset the boat as well.
            initBoat(&boat);

            // Reset camera around the player.
            initCamera(&camera);
        }

        // ---- Debug menu open: the pad belongs to the menu, not the game ----
        // (R is the menu's "x10" button, so without this it would also jump.)
        if (debugMenuIsOpen()) {
            cYaw = 0.0f;
            cPitch = 0.0f;
            moveFwd = moveBack = moveLeft = moveRight = false;
            btnA = btnB = jump = false;
        }

        // ---- Debug menu closed with Y: values were applied, make new islands ----
        bool applyNow = debugMenuConsumeApply();

        // ---- A: regenerate world (debug helper, works in either mode) ----
        if (btnA || applyNow) {
            freeAllIslands(&world);
            regenerateIslands(&world);
        }

        // ---- B: board / disembark ----
        
        if (btnB) {
            if (!isPlayerActive) {
                // Disembark: only when the boat is touching land
                Vec3 boatPos = { boat.position.x, boat.position.y, boat.position.z };
                if (checkAllIslandsCollision(&world, boatPos, boat.radius)) {
                    isPlayerActive   = true;
                    player.position  = boat.position;
                    player.yaw       = boat.yaw;
                    player.yVelocity = 0.0f;
                }
            } else {
                // Re-board: only near sea level
                if (player.position.y <= boatChangeY) {
                    boat.position.x = player.position.x;
                    boat.position.z = player.position.z;
                    boat.position.y = 0.0f;
                    boat.yaw        = player.yaw;
                    isPlayerActive  = false;
                    initCamera(&camera);
                }
            }
        }
        

        

        // ---- Jump: only while standing on something ----
        if (jump && playerIsGrounded(&player))
            player.yVelocity = JUMP_FORCE;

        // ---- Update the active entity ----
        bool touchedWater = false;   // on foot: walking into the sea / standing in it
        bool pushedLand   = false;   // sailing: steering forward into an island

        if (isPlayerActive)
            touchedWater = updatePlayer(&player, moveFwd, moveBack, moveLeft, moveRight, &world);
        else
            pushedLand = updateBoat(&boat, moveFwd, moveBack, moveLeft, moveRight, time, &world);

        // ---- PLAYER_SNAP: switch between player and boat at the shoreline ----
        if (PLAYER_SNAP) {
            if (isPlayerActive && touchedWater) {
                // Player -> boat. Keep travelling the way the player was moving
                // (player movement is relative to the camera, hence the offset).
                float heading = player.yaw + g_cameraYawOffset;
                if (moveBack && !moveFwd) heading += 3.14159265f;   // walked in backwards

                boatLaunch(&boat, player.position.x, player.position.z, heading, &world);
                isPlayerActive = false;
                initCamera(&camera);
            }
            else if (!isPlayerActive && pushedLand) {
                // Boat -> player, but only if there is a beach to stand on.
                if (playerLandAhead(&player, boat.position.x, boat.position.z, boat.yaw, &world))
                    isPlayerActive = true;
            }
        }

        // Recompute the tracked position after PLAYER_SNAP so the camera and
        // world rendering use the newly active entity immediately.
        Vec3 trackPos = isPlayerActive
            ? (Vec3){ player.position.x, player.position.y, player.position.z }
            : (Vec3){ boat.position.x,   boat.position.y,   boat.position.z   };

        updateCamera(&camera, &boat, &player, isPlayerActive, &world, cYaw, cPitch);

        // ---- Advance wave time (and wrap so it never grows unbounded) ----
        time += WAVE_SPEED;

        if (time >= 1000.0f)
            time -= 1000.0f;

        // ============================================================
        // Rendering
        // ============================================================

        begin_frame();
        setup_camera(camera.position, camera.up, camera.look);

        drawWater(time, trackPos.x, trackPos.z);
        drawAllIslands(&world, trackPos, 1.5f); // (or whatever radius fits your player/boat size)
        drawWorldBoundary();

        if (isPlayerActive)
            drawPlayer(player.position.x, player.position.y, player.position.z, player.yaw);
        else
            drawBoat(boat.position.x, boat.position.y, boat.position.z, boat.yaw);


        drawDebugMenu();

        end_frame(fb);
        fb ^= 1;
    }

    return 0;
}