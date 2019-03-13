#pragma once

struct Terrain;

Terrain* CreateTerrain();
void DeleteTerrain(Terrain* t);

struct Patch;
Patch* GetTerrainPatch(Terrain* t, int x, int y);
Patch* AddTerrainPatch(Terrain* t, int x, int y);
bool DelTerrainPatch(Terrain* t, int x, int y);

