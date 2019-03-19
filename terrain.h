#pragma once

#define HEIGHT_CELLS 4 // num of verts -1 along patch X and Y axis
#define VISUAL_PER_HEIGHT_CELLS 4

#define VISUAL_CELLS (HEIGHT_CELLS*VISUAL_PER_HEIGHT_CELLS)

struct Terrain;

Terrain* CreateTerrain(int z=-1);
void DeleteTerrain(Terrain* t);

struct Patch;
Patch* GetTerrainPatch(Terrain* t, int x, int y);
Patch* AddTerrainPatch(Terrain* t, int x, int y, int z);
bool DelTerrainPatch(Terrain* t, int x, int y);

uint16_t* GetTerrainHeightMap(Patch* p);

#ifdef TEXHEAP
TexAlloc* GetTerrainTexAlloc(Patch* p);
#endif

int GetTerrainPatches(Terrain* t);

template <typename P>
void QueryTerrain(Terrain* t, int planes, P plane[][4], int view_flags, void (*cb)(Patch* p, int x, int y, int view_flags, void* cookie), void* cookie);

