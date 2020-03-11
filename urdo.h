#pragma once

#include  <stdint.h>
#include "terrain.h"
#include "world.h"
#include "inventory.h"

bool URDO_CanUndo();
bool URDO_CanRedo();

size_t URDO_Bytes();

void URDO_Purge();

void URDO_Undo(int max_depth);
void URDO_Redo(int max_depth);

// groupping
void URDO_Open();
void URDO_Close();

// patches

Patch* URDO_Create(Terrain* t, int x, int y, int z); // replacement for AddTerrainPatch
void URDO_Delete(Terrain* t, Patch* p); // replacement for DelTerrainPatch

void URDO_Patch(Patch* p, bool visual = false); // call before changing height map
void URDO_Diag(Patch* p); // call before flipping diag

// meshes & sprites (instances)

Inst* URDO_Create(World* w, Item* item, int flags, float pos[3], float yaw, int story_id);
Inst* URDO_Create(World* w, Sprite* s, int flags, float pos[3], float yaw, int anim, int frame, int reps[4], int story_id); // replacement for CreateInst
Inst* URDO_Create(Mesh* m, int flags, double tm[16], int story_id); // replacement for CreateInst
void URDO_Delete(Inst* i); // replacement for DeleteInst

