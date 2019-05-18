
#pragma once

struct World;
struct Mesh;
struct Inst;

World* CreateWorld();
void DeleteWorld(World* w);

Mesh* LoadMesh(World* w, const char* path);
void DeleteMesh(Mesh* m);

Inst* CreateInst(Mesh* m, const double tm[16] = 0, const char* name = 0);
void DeleteInst(Inst* i);

enum INST_FLAGS
{
    INST_VISIBLE = 0x1,
    INST_USE_TREE = 0x2,
};

int GetInstFlags(Inst* i);
void SetInstFlags(Inst* i, int flags, int mask);

void GetInstTM(Inst* i, double tm[16]);
void SetInstTM(Inst* i, const double tm[16]);

void QueryWorld(World* w, int planes, double plane[][4], void (*cb)(float coords[9], uint32_t visual, void* cookie), void* cookie);
