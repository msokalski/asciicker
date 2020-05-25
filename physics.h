#pragma once

#include <stdint.h>
#include "terrain.h"
#include "world.h"

struct PhysicsIO
{
    // INPUT:
    // must be thresholded
    float x_force;
    float y_force;
    float torque;
    float water;
    //bool slow;

    // IO:
    bool jump; // will be falsed by Animate() if jump was handled

    // OUTPUT:
    float pos[3];
    float yaw;
    float player_dir;
    int player_stp;
	int dt;

	float xyz[64][3];
};

struct Physics;

int Animate(Physics* phys, uint64_t stamp, PhysicsIO* io, bool mount = false); // return num of time steps handled

Physics* CreatePhysics(Terrain* t, World* w, float pos[3], float dir, float yaw, uint64_t stamp);
void DeletePhysics(Physics* phys);

void SetPhysicsPos(Physics* phys, float pos[3], float vel[3]);
void SetPhysicsYaw(Physics* phys, float yaw, float vel);
void SetPhysicsDir(Physics* phys, float dir);
