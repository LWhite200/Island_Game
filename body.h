#ifndef BODY_H
#define BODY_H


#include "common.h"
#include "manager.h"

typedef struct {
    guVector position; // x, y, z 
    float yaw;         // rotation, direction looking
    float speed;
    float radius;      // keep circle for now, make oval later
    float rotationSpeed;
    int hp;
    float yVelocity;   // for jumping
    float gravity;
    float jumpForce;
} Body;

void initBody(Body* body, float x, float y, float z);
void updateBody(Body* body, IslandManager* manager, Vec3 playerPos); // make it hop
void drawBody(Body* body);

#endif