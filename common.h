#ifndef COMMON_H
#define COMMON_H

#include <gccore.h>
 
#define DEFAULT_FIFO_SIZE    (256*1024)
#define WATER_SIZE           100
#define WAVE_SPEED           0.2f
#define WAVE_FREQUENCY       0.3f
#define WAVE_AMPLITUDE       0.25f

#define ISLAND_MIN_RADIUS    10.0f
#define ISLAND_MAX_RADIUS    70.0f
#define ISLAND_MIN_HEIGHT    4.0f
#define ISLAND_MAX_HEIGHT    18.0f
#define ISLAND_FLATTENING    0.3f


#define JOYSTICK_DEADZONE    15
#define MAX_JOYSTICK_VALUE   70

#define cRadius      1.0f

// The bottom of the world islands
#define BASE_Y       -0.5f

#endif