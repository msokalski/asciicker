#pragma once

#define HEIGHT_SCALE 16 // how may z-steps produces 1 visual cell
#define HEIGHT_CELLS 4 // num of verts -1 along patch X and Y axis
#define VISUAL_CELLS 4

struct Terrain;

Terrain* CreateTerrain(int z=-1);
void DeleteTerrain(Terrain* t);

struct Patch;
Patch* GetTerrainPatch(Terrain* t, int x, int y);
Patch* AddTerrainPatch(Terrain* t, int x, int y, int z);
bool DelTerrainPatch(Terrain* t, int x, int y);

uint16_t* GetTerrainHeightMap(Patch* p);
void UpdateTerrainPatch(Patch* p);

void GetTerrainLimits(Patch* p, uint16_t* lo, uint16_t* hi);

#ifdef TEXHEAP
TexHeap* GetTerrainTexHeap(Terrain* t);
TexAlloc* GetTerrainTexAlloc(Patch* p);

struct TexPageBuffer
{
	TexPage* prev;
	TexPage* next;
	int size;
	GLint data[4 * 789];
};

#endif

int GetTerrainPatches(Terrain* t);
size_t GetTerrainBytes(Terrain* t);

void QueryTerrain(Terrain* t, double x, double y, double r, int view_flags, void(*cb)(Patch* p, int x, int y, int view_flags, void* cookie), void* cookie);
void QueryTerrain(Terrain* t, int planes, double plane[][4], int view_flags, void (*cb)(Patch* p, int x, int y, int view_flags, void* cookie), void* cookie);
Patch* HitTerrain(Terrain* t, double p[3], double v[3], double ret[4]);
