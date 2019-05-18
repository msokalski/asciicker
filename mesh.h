
#pragma once

struct World;
struct Mesh;

World* CreateWorld();
void DeleteWorld(World* w);
Mesh* LoadMesh(World* w, const char* path);
void DeleteMesh(Mesh* m);
