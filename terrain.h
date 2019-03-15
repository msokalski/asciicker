#pragma once

#define HEIGHT_CELLS 4 // num of verts -1 along patch X and Y axis
#define VISUAL_CELLS (HEIGHT_CELLS*4)

struct Terrain;

Terrain* CreateTerrain(int z=-1);
void DeleteTerrain(Terrain* t);

struct Patch;
Patch* GetTerrainPatch(Terrain* t, int x, int y);
Patch* AddTerrainPatch(Terrain* t, int x, int y, int z);
bool DelTerrainPatch(Terrain* t, int x, int y);

int GetTerrainPatches(Terrain* t);

template <typename P>
void QueryTerrain(Terrain* t, int planes, P plane[][4], void (*cb)(Patch* p, int x, int y, void* cookie), void* cookie);

