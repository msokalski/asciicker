#include <stdint.h>

// note:
// this is non-compiled representation of mesh for use by editor
// RT should use BSP!

struct Face;
struct Mesh;
struct Vert
{
    Mesh* mesh;

    // in mesh
	Vert* next;
	Vert* prev;

    // what coord precision?
	int x,y,z;
	Face* share_list;
};

struct Face
{
    Mesh* mesh;

    // in mesh
	Face* next;
	Face* prev;

	Vert* abc[3];
	Face* share_next[3]; // next triangle sharing given vertex

	uint32_t visual; // matid_8bits, 3x{shade_and_mode_8bits(per-vertex)}
};

struct Inst;
struct World;
struct Mesh
{
    World* world;

    // in world
	Mesh* next;
	Mesh* prev;

	// add(Triangle* t)
	// remove(Triangle* t)

	// add(Vertex* v)
	// remove(Vertex* v)

	int faces;
	Face* head_face;
	Face* tail_face;

	int verts;
	Vert* head_vert;
	Vert* tail_vert;

    // untransformed bbox
    int bbox[6];

    Inst* share_list;
    char* name;
};

struct Inst
{
    // in world
	Inst* next;
	Inst* prev;

    Mesh* mesh;
    double tm[16];

    Inst* share_next; // next instance sharing same mesh
    char* name;
};

struct World
{
	// add(Mesh* m)
	// remove(Mesh* m)

    Mesh* head_mesh;
    Mesh* tail_mesh;

	// add(MeshInstance* i)
	// remove(MeshInstance* i)

    Inst* head_inst;
    Inst* tail_inst;
};

