
#pragma once

struct World;
struct Mesh;
struct Inst;

World* CreateWorld();
void DeleteWorld(World* w);
void RebuildWorld(World* w, bool boxes = false);

Mesh* LoadMesh(World* w, const char* path, const char* name = 0);
void DeleteMesh(Mesh* m);

bool UpdateMesh(Mesh* m, const char* path);

Mesh* GetFirstMesh(World* w);
Mesh* GetLastMesh(World* w);
Mesh* GetPrevMesh(Mesh* m);
Mesh* GetNextMesh(Mesh* m);

void* GetMeshCookie(Mesh* m);
void  SetMeshCookie(Mesh* m, void* cookie);

World* GetMeshWorld(Mesh* m);
int GetMeshName(Mesh* m, char* buf, int size);
void GetMeshBBox(Mesh* m, float bbox[6]);

void QueryMesh(Mesh* m, void (*cb)(float coords[9], uint8_t colors[12], uint32_t visual, void* cookie), void* cookie);

Inst* CreateInst(Mesh* m, int flags, const double tm[16] = 0, const char* name = 0);
void DeleteInst(Inst* i);

Mesh* GetInstMesh(Inst* i);
int GetInstFlags(Inst* i);
void GetInstTM(Inst* i, double tm[16]);
void GetInstBBox(Inst* i, double bbox[6]);

enum INST_FLAGS
{
    INST_VISIBLE = 0x1,
    INST_USE_TREE = 0x2,
};

void QueryWorld(World* w, int planes, double plane[][4], void (*cb)(Mesh* m, double tm[16], void* cookie), void* cookie);
void QueryWorldBSP(World* w, int planes, double plane[][4], void (*cb)(int level, const float bbox[6], void* cookie), void* cookie);

Inst* HitWorld(World* w, double p[3], double v[3], double ret[3], double nrm[3], bool positive_only = false);

void SaveWorld(World* w, FILE* f);
World* LoadWorld(FILE* f);

// during save:
// save those meshes that have instances only (with file names)

// during load:
// load embedded meshes saved in file
// later asciiid can add meshes from external files
// ... then it can ask what to do with collisions (keep embedded, replace with externals)
//     but it should compare them if they really differ (so need to load them regardless everything)


