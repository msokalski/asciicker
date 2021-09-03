#pragma once

#include <stdio.h>
#include "world.h"

#define DARK_TERRAIN

#define HEIGHT_SCALE 16 // how may z-steps produces 1 visual cell
#define HEIGHT_CELLS 4 // num of verts -1 along patch X and Y axis
#define VISUAL_CELLS 8

#ifdef TEXHEAP
#define TERRAIN_TEXHEAP_CAPACITY (1024 / (VISUAL_CELLS > HEIGHT_CELLS+1 ? VISUAL_CELLS : HEIGHT_CELLS+1))
#endif

struct Terrain;

Terrain* CreateTerrain(int z=-1);
void DeleteTerrain(Terrain* t);

void GetTerrainBase(Terrain* t, int b[2]);
void SetTerrainBase(Terrain* t, const int b[2]);

bool SaveTerrain(const Terrain* t, FILE* f);
Terrain* LoadTerrain(FILE* f);

struct Patch;

Patch* GetTerrainPatch(Terrain* t, int x, int y);
void GetTerrainPatch(Terrain* t, Patch* p, int* x, int* y);

Patch* AddTerrainPatch(Terrain* t, int x, int y, int z);
bool DelTerrainPatch(Terrain* t, int x, int y);

// don't  use, deticated to urdo only!
size_t TerrainDetach(Terrain* t, Patch* p, int* x, int* y);
size_t TerrainAttach(Terrain* t, Patch* p, int x, int y);
size_t TerrainDispose(Patch* p);

Patch* GetTerrainNeighbor(Patch* p, int sign_x, int sign_y);

uint16_t* GetTerrainHeightMap(Patch* p);
uint16_t* GetTerrainVisualMap(Patch* p);

Patch* CalcTerrainGhost(Terrain* t, int x, int y, int z, uint16_t ghost[4 * HEIGHT_CELLS]);

void UpdateTerrainHeightMap(Patch* p);
void UpdateTerrainVisualMap(Patch* p);

void GetTerrainLimits(Patch* p, uint16_t* lo, uint16_t* hi);

#ifdef TEXHEAP
TexHeap* GetTerrainTexHeap(Terrain* t);
TexAlloc* GetTerrainTexAlloc(Patch* p);

struct TexPageBuffer
{
	TexPage* prev;
	TexPage* next;
	int size;

	// large enough to refer to the whole page content by vbo (81920 bytes)
	GLint data[5 * TERRAIN_TEXHEAP_CAPACITY];
};

#endif

int GetTerrainPatches(Terrain* t);
size_t GetTerrainBytes(Terrain* t);

uint16_t GetTerrainDiag(Patch* p);
void SetTerrainDiag(Patch* p, uint16_t diag);

uint16_t GetTerrainHi(Patch* p, uint16_t* lo = 0);

#ifdef DARK_TERRAIN
uint64_t GetTerrainDark(Patch* p);
void SetTerrainDark(Patch* p, uint64_t dark);
void UpdateTerrainDark(Terrain* t, World* w, float lightpos[3], bool editor);
#endif

void QueryTerrain(Terrain* t, double x, double y, double r, int view_flags, void(*cb)(Patch* p, int x, int y, int view_flags, void* cookie), void* cookie);
void QueryTerrain(Terrain* t, int planes, double plane[][4], int view_flags, void (*cb)(Patch* p, int x, int y, int view_flags, void* cookie), void* cookie);
Patch* HitTerrain(Terrain* t, double p[3], double v[3], double ret[4], double nrm[3]=0, bool positive_only = false);

double HitTerrain(Patch* p, double u, double v); // u,v must be normalized

