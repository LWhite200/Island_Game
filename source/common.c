// common.c
//
// Storage and DEFAULT values for every runtime-tunable setting declared
// in common.h. Edit the numbers here to change the defaults; the debug
// menu changes them live while the game runs.

#include "common.h"

// ============================================================
// World / wave
// ============================================================

float g_worldRadius    = 80.0f;   // 80 for a 160x160 patch
float g_waveFrequency  = 0.5f;
float g_waveAmplitude  = 0.15f;
float g_waveSpeed      = 0.03f;
float g_boatChangeY    = 0.5f;
float g_baseY          = -1.0f;
float g_islandBaseY    = -1.0f;

// ============================================================
// Island generation
// ============================================================

int   g_numIslands               = 5;
float g_islandDefaultRadius      = 15.0f;
float g_islandDefaultHeightScale = 0.5f;
int   g_islandDefaultStyle       = 0;    // ISLAND_TROPICAL
int   g_islandLonSegments        = 16;
int   g_islandLatSegments        = 8;

// ============================================================
// Streaming world
// ============================================================

float g_worldStreamDistance = 80.0f;
float g_worldCullDistance   = 160.0f;
float g_islandMinSeparation = 55.0f;

// ============================================================
// Player
// ============================================================

float g_jumpForce  = 0.42f;
bool  g_playerSnap = true;