
#pragma once

struct World;
struct Mesh;
struct Inst;
struct Sprite;
struct Item;

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

int GetMeshFaces(Mesh* m);
void QueryMesh(Mesh* m, void (*cb)(float coords[9], uint8_t colors[12], uint32_t visual, void* cookie), void* cookie);

Inst* CreateInst(World* w, Item* item, int flags, float pos[3], float yaw, int story_id);
Inst* CreateInst(World* w, Sprite* s, int flags, float pos[3], float yaw, int anim, int frame, int reps[4], const char* name, int story_id);
Inst* CreateInst(Mesh* m, int flags, const double tm[16], const char* name, int story_id);
void DeleteInst(Inst* i);

World* GetInstWorld(Inst* i);

Mesh* GetInstMesh(Inst* i);
int GetInstFlags(Inst* i);
int GetInstStoryID(Inst* i);
bool GetInstTM(Inst* i, double tm[16]);
void GetInstBBox(Inst* i, double bbox[6]);

void UpdateSpriteInst(World* world, Inst* i, Sprite* sprite, const float pos[3], float yaw, int anim, int frame, const int reps[4]);
Sprite* GetInstSprite(Inst* i, float pos[3], float* yaw, int* anim, int* frame, int reps[4]);
bool SetInstSpriteData(Inst* i, void* data);
void* GetInstSpriteData(Inst* i);

Item* GetInstItem(Inst* i, float pos[3], float* yaw);

int AnimateSpriteInst(Inst* i, uint64_t stamp);

void ShowInst(Inst* i);
void HideInst(Inst* i);

enum INST_FLAGS
{
    INST_VISIBLE = 0x1,
    INST_USE_TREE = 0x2,
	INST_VOLATILE = 0x4
};

// new
// void QueryWorld(World* w, int planes, double plane[][4], void(*cb)(Sprite* s, float pos[3], float yaw, int anim, float frame, void* cookie), void* cookie);

struct QueryWorldCB
{
	void(*mesh_cb)(Mesh* m, double tm[16], void* cookie);
	void(*sprite_cb)(Inst* inst, Sprite* s, float pos[3], float yaw, int anim, int frame, int reps[4], void* cookie);
};

void QueryWorld(World* w, int planes, double plane[][4], QueryWorldCB* cb, void* cookie);
void QueryWorldBSP(World* w, int planes, double plane[][4], void (*cb)(int level, const float bbox[6], void* cookie), void* cookie);


// if editor==true -> ignore volatile instances
Inst* HitWorld(World* w, double p[3], double v[3], double ret[3], double nrm[3], bool positive_only = false, bool editor = false, bool solid_only = false, bool sprites_too = true);

void SaveWorld(World* w, FILE* f);

// editor==true  clones items for test-players
// editor==false changes items purpose directly for player(s)
World* LoadWorld(FILE* f, bool editor); 

void PurgeItemInstCache();
void ResetItemInsts(World* w);

bool AttachInst(World* w, Inst* i); // tries to move from flat list to bsp

// undo/redo only!!!
void SoftInstAdd(Inst* i);
void SoftInstDel(Inst* i);
void HardInstDel(Inst* i);

