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

	// user should accumulate them
	// physics will zero them only when handled
	float x_impulse;
	float y_impulse;

    // OUTPUT:
    float pos[3];
    float yaw;
    float player_dir;
    int player_stp;
	int dt;

	bool grounded;

	float xyz[64][3];
};

struct Physics;

int Animate(Physics* phys, uint64_t stamp, PhysicsIO* io, int mount/*0-none, 1-ground, 2-fly*/); // return num of time steps handled

Physics* CreatePhysics(Terrain* t, World* w, float pos[3], float dir, float yaw, uint64_t stamp);
void DeletePhysics(Physics* phys);

void SetPhysicsPos(Physics* phys, float pos[3], float vel[3]);
void SetPhysicsYaw(Physics* phys, float yaw, float vel);
void SetPhysicsDir(Physics* phys, float dir);
