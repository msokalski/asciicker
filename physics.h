#pragma once

#include <stdint.h>
#include "terrain.h"
#include "mesh.h"

struct PhysicsIO
{
    // INPUT:
    // must be thresholded
    float x_force;
    float y_force;
    float torque;
    float water;
    bool slow;

    // IO:
    bool jump; // will be falsed by Animate() if jump was handled

    // OUTPUT:
    float pos[3];
    float yaw;
    float player_dir;
    int player_stp;
};

struct Physics;

void Animate(Physics* phys, uint64_t stamp, PhysicsIO* io);

Physics* CreatePhysics(Terrain* t, World* w, float pos[3], float dir, float yaw, uint64_t stamp);
void DeletePhysics(Physics* phys);
