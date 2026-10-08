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

// How long (in frames) the boat must be touching land / the player must be
// pressing into water before the switch happens. 30 frames = ~0.5s at 60 fps.
#define SNAP_DELAY_FRAMES 30

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

    // Counts up while a PLAYER_SNAP switch is "charging"; drives the darkening.
    int snapFrames = 0;

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

            // Of center because bouncing bug
            player.position.x = 0.25f;
            player.position.y = 25.0f;
            player.position.z = 0.25f;

            player.yaw       = 0.0f;
            player.yVelocity = 0.0f;

            // Always return to player mode.
            isPlayerActive = true;
            snapFrames = 0;

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
            snapFrames = 0;
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
                    float    camYawOld = player.yaw + g_cameraYawOffset;
                    guVector oldPos    = player.position;

                    boat.position.x = player.position.x;
                    boat.position.z = player.position.z;
                    boat.position.y = 0.0f;
                    boat.yaw        = player.yaw;
                    isPlayerActive  = false;

                    // Keep the camera where it is (no initCamera: that resets
                    // it to the world origin and it swings across the map).
                    cameraRetarget(&camera, camYawOld, &oldPos, &boat.position, boat.yaw);
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
        // "Ghost" of the entity we are about to become. It is drawn growing in
        // at the spot where it will appear, while the current one shrinks out.
        bool   ghostValid = false;
        Boat   ghostBoat   = boat;
        Player ghostPlayer = player;

        if (PLAYER_SNAP) {
            // Is a switch being asked for this frame?
            bool wantSwitch = false;
            if (isPlayerActive) {
                wantSwitch = touchedWater;

                // Where the boat will appear (same heading the switch will use).
                float heading = player.yaw + g_cameraYawOffset;
                if (moveBack && !moveFwd) heading += 3.14159265f;   // walking backwards
                if (wantSwitch || snapFrames > 0) {
                    boatLaunch(&ghostBoat, player.position.x, player.position.z, heading, &world);
                    ghostValid = true;
                }
            } else if (pushedLand || snapFrames > 0) {
                // Where the player will appear. Computed on a copy so the real
                // player isn't moved until the switch completes.
                ghostValid = playerLandAhead(&ghostPlayer, boat.position.x, boat.position.z,
                                             boat.yaw, &world);
                wantSwitch = pushedLand && ghostValid;
            }

            // Charge while touching; drain twice as fast when contact is lost,
            // so a brief wobble doesn't lose all progress but backing off cancels.
            if (wantSwitch) {
                snapFrames++;
            } else if (snapFrames > 0) {
                snapFrames -= 2;
                if (snapFrames < 0) snapFrames = 0;
            }

            if (snapFrames >= SNAP_DELAY_FRAMES) {
                snapFrames = 0;

                if (isPlayerActive) {
                    // Player -> boat. Keep travelling the way the player was moving
                    // (player movement is relative to the camera, hence the offset).
                    float    camYawOld = player.yaw + g_cameraYawOffset;
                    guVector oldPos    = player.position;

                    boat = ghostBoat;   // already placed by boatLaunch above
                    isPlayerActive = false;

                    // No initCamera: keep the camera heading/position and glide
                    // the look target over to the boat.
                    cameraRetarget(&camera, camYawOld, &oldPos, &boat.position, boat.yaw);
                }
                else if (ghostValid) {
                    float    camYawOld = boat.yaw + g_cameraYawOffset;
                    guVector oldPos    = boat.position;

                    player = ghostPlayer;   // already placed by playerLandAhead above
                    isPlayerActive = true;

                    cameraRetarget(&camera, camYawOld, &oldPos, &player.position, player.yaw);
                }
            }
        } else {
            snapFrames = 0;
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

        // 0 -> 1 as the switch charges, eased so it starts and ends gently.
        // The current entity darkens and shrinks away; the incoming one grows
        // in from dark at the spot where it will appear.
        float t    = (float)snapFrames / (float)SNAP_DELAY_FRAMES;
        float ease = t * t * (3.0f - 2.0f * t);

        if (isPlayerActive) {
            drawPlayer(player.position.x, player.position.y, player.position.z,
                       player.yaw, ease, 1.0f - ease);
            if (ghostValid && snapFrames > 0)
                drawBoat(ghostBoat.position.x, ghostBoat.position.y, ghostBoat.position.z,
                         ghostBoat.yaw, 1.0f - ease, ease);
        } else {
            drawBoat(boat.position.x, boat.position.y, boat.position.z,
                     boat.yaw, ease, 1.0f - ease);
            if (ghostValid && snapFrames > 0)
                drawPlayer(ghostPlayer.position.x, ghostPlayer.position.y, ghostPlayer.position.z,
                           ghostPlayer.yaw, 1.0f - ease, ease);
        }


        drawDebugMenu();

        end_frame(fb);
        fb ^= 1;
    }

    return 0;
}