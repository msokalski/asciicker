
#pragma once

struct World;
struct Mesh;
struct Inst;

World* CreateWorld();
void DeleteWorld(World* w);

Mesh* LoadMesh(World* w, const char* path, const char* name = 0);
void DeleteMesh(Mesh* m);

Mesh* GetFirstMesh(World* w);
Mesh* GetLastMesh(World* w);
Mesh* GetPrevMesh(Mesh* m);
Mesh* GetNextMesh(Mesh* m);

void* GetMeshCookie(Mesh* m);
void  SetMeshCookie(Mesh* m, void* cookie);

int GetMeshName(Mesh* m, char* buf, int size);
void GetMeshBBox(Mesh* m, float bbox[6]);

void QueryMesh(Mesh* m, void (*cb)(float coords[9], uint32_t visual, void* cookie), void* cookie);

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

void QueryWorld(World* w, int planes, double plane[][4], void (*cb)(Mesh* m, const double tm[16], void* cookie), void* cookie);
