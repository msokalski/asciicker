#include <stdint.h>
#include <malloc.h>

struct Face;
struct Mesh;
struct Vert
{
    Mesh* mesh;

    // in mesh
	Vert* next;
	Vert* prev;

	Face* share_list;

    void Set(float x, float y, float z);

    // regardless of mesh type (2d/3d)
    // we keep z (makes it easier to switch mesh types back and forth)
	float x,y,z;

    bool sel;
};

struct Face
{
    Mesh* mesh;

    // in mesh
	Face* next;
	Face* prev;

	Vert* abc[3];
	Face* share_next[3]; // next triangle sharing given vertex

	uint32_t visual; // matid_8bits, 3 x {shade_7bits,elev_1bit}

    bool sel;
};

struct Inst;
struct World;
struct Mesh
{
    World* world;
    char* name; // in form of library path?

    // in world
	Mesh* next;
	Mesh* prev;

	// add(Triangle* t)
	// remove(Triangle* t)

    Face* AddFace(Vert* a, Vert* b, Vert* c);
    void DelFace(uint32_t visual, Face* f);

    Vert* AddVert(float x, float y, float z);
    void DelVert(Vert* v);

    enum TYPE
    {
        MESH_TYPE_2D,
        MESH_TYPE_3D
    };

    TYPE type;

	int faces;
	Face* head_face;
	Face* tail_face;

	int verts;
	Vert* head_vert;
	Vert* tail_vert;

    // untransformed bbox
    int bbox[6];

    Inst* share_list;
};

struct BSP
{
    enum TYPE
    {
        BSP_TYPE_NODE,
        BSP_TYPE_INST
    };

    TYPE type;    
    BSP* parent;
    float bbox[6]; // in world coords
};

struct BSP_Node : BSP
{
    BSP* child[2];
};

struct Inst : BSP
{
    World* world;

    // in world
	Inst* next;
	Inst* prev;    

    Mesh* mesh;
    double tm[16]; // absoulte! mesh->world

    Inst* share_next; // next instance sharing same mesh

    enum FLAGS
    {
        INST_VISIBLE = 0x1,
        INST_IN_TREE = 0x2,
        INST_USE_TREE = 0x4,
    };

    int /*FLAGS*/ flags; 
};

/*
sometime we'll add:
struct Object
{
    // contains hierarchy of Nodes with Insts attached
    // ...

    // changing tm of nodes updates tm in instances
    // in this way world does not need to know anything
    // about objects and nodes
}
*/

struct World
{
	// add(Mesh* m)
	// remove(Mesh* m)

    // mesh lib
    Mesh* head_mesh;
    Mesh* tail_mesh;

    Mesh* AddMesh(Mesh::TYPE type, const char* name = 0);
    void DelMesh(Mesh* m);

    // all insts
    int insts;
    Inst* head_inst;
    Inst* tail_inst;

    Inst* AddInst(double tm[16], const char* name = 0);
    void DelInst(Inst* i);

    // currently selected instance (its mesh) for editting
    // overrides visibility?
    Inst* editable;

    // now we want to form a tree of Insts
    BSP* root;

    static void DeleteBSP(BSP* bsp)
    {
        if (bsp->type == BSP::BSP_TYPE_NODE)
        {
            BSP_Node* node = (BSP_Node*)bsp;

            // 1 child can be null if inst was removed
            // because its TM changed or its Mesh was modified

            if (node->child[0])
                DeleteBSP(node->child[0]);
            if (node->child[1])
                DeleteBSP(node->child[1]);
            free (bsp);
        }
        else
        if (bsp->type == BSP::BSP_TYPE_INST)
        {
            Inst* inst = (Inst*)bsp;
            inst->flags &= ~Inst::INST_IN_TREE;
            bsp->parent = 0;
        }
    }

    void Rebuild()
    {
        if (root)
            DeleteBSP(root);

        int num = 0;
        BSP** arr = (BSP**)malloc(sizeof(BSP*) * insts);
        
        for (Inst* inst = head_inst; inst; inst=inst->next)
        {
            if (inst->flags & Inst::INST_USE_TREE)
                arr[num++] = inst;
        }

        // MAY BE SLOW: 1/2 * num^3
        while (num>1)
        {
            int a = 0;
            int b = 1;
            float e = -1;

            for (int u=0; u<num-1; u++)
            {
                for (int v=u+1; v<num; v++)
                {
                    float bbox[6] =
                    {
                        arr[a]->bbox[0] < arr[b]->bbox[0] ? arr[a]->bbox[0] : arr[b]->bbox[0],
                        arr[a]->bbox[1] > arr[b]->bbox[1] ? arr[a]->bbox[1] : arr[b]->bbox[1],
                        arr[a]->bbox[2] < arr[b]->bbox[2] ? arr[a]->bbox[2] : arr[b]->bbox[2],
                        arr[a]->bbox[3] > arr[b]->bbox[3] ? arr[a]->bbox[3] : arr[b]->bbox[3],
                        arr[a]->bbox[4] < arr[b]->bbox[4] ? arr[a]->bbox[4] : arr[b]->bbox[4],
                        arr[a]->bbox[5] > arr[b]->bbox[5] ? arr[a]->bbox[5] : arr[b]->bbox[5]
                    };

                    float vol = (bbox[1]-bbox[0]) * (bbox[3]-bbox[2]) * (bbox[5]-bbox[4]);

                    if (vol < e || e<0)
                    {
                        a = u;
                        b = v;
                        e = vol;
                    }
                }
            }

            BSP_Node* node = (BSP_Node*)malloc(sizeof(BSP_Node));

            node->parent = 0;
            node->type = BSP::BSP_TYPE_NODE;

            node->child[0] = arr[a];
            node->child[1] = arr[b];

            node->bbox[0] = arr[a]->bbox[0] < arr[b]->bbox[0] ? arr[a]->bbox[0] : arr[b]->bbox[0];
            node->bbox[1] = arr[a]->bbox[1] > arr[b]->bbox[1] ? arr[a]->bbox[1] : arr[b]->bbox[1];
            node->bbox[2] = arr[a]->bbox[2] < arr[b]->bbox[2] ? arr[a]->bbox[2] : arr[b]->bbox[2];
            node->bbox[3] = arr[a]->bbox[3] > arr[b]->bbox[3] ? arr[a]->bbox[3] : arr[b]->bbox[3];
            node->bbox[4] = arr[a]->bbox[4] < arr[b]->bbox[4] ? arr[a]->bbox[4] : arr[b]->bbox[4];
            node->bbox[5] = arr[a]->bbox[5] > arr[b]->bbox[5] ? arr[a]->bbox[5] : arr[b]->bbox[5];

            num--;
            if (b!=num)
                arr[b] = arr[num];

            arr[a] = node;
        }

        free(arr);
    }

    struct Hit
    {
        Inst* inst;
        Face* face;
        float abc_z[4]; // barycentric and world's z
    };

    // RAY HIT
    bool Query(double p[3], double v[3], Hit* hit)
    {
        // hit->abc_z[3] MUST be preinitialized to FAREST z
    }

    // INSTS IN HULL
    void Query(int planes, double plane[][4], void (*cb)(Inst* inst, void* cookie), void* cookie)
    {
        // query possibly visible instances
    }
 
    // FACES IN HULL
    void Query(Inst* inst, int planes, double plane[][4], void (*cb)(Face* face, void* cookie), void* cookie)
    {
        // query possibly visible faces of instance's mesh
    }
};

