#include <stdint.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "mesh.h"
#include "matrix.h"

struct Line;
struct Face;

#define INST_IN_TREE (1<<31)

struct Vert
{
    Mesh* mesh;

    // in mesh
	Vert* next;
	Vert* prev;

	Face* face_list;
	Line* line_list;

    // regardless of mesh type (2d/3d)
    // we keep z (makes it easier to switch mesh types back and forth)
	float xyzw[4];

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


struct Line
{
    Mesh* mesh;

    // in mesh
	Line* next;
	Line* prev;

	Vert* ab[2];
	Line* share_next[2]; // next line sharing given vertex

	uint32_t visual; // line style & height(depth) offset?

    bool sel;
};

struct Inst;
struct World;
struct Mesh
{
    World* world;
    char* name; // in form of library path?
    void* cookie;

    // in world
	Mesh* next;
	Mesh* prev;

/*
    Face* AddFace(Vert* a, Vert* b, Vert* c);
    void DelFace(uint32_t visual, Face* f);

    Vert* AddVert(float x, float y, float z);
    void DelVert(Vert* v);

    Line* AddLine(Vert* a, Vert* b);
    void DelLine(Line* l);
*/

    enum TYPE
    {
        MESH_TYPE_2D,
        MESH_TYPE_3D
    };

    TYPE type;

	int faces;
	Face* head_face;
	Face* tail_face;

	int lines;
	Line* head_line;
	Line* tail_line;

	int verts;
	Vert* head_vert;
	Vert* tail_vert;

    // untransformed bbox
    float bbox[6];

    Inst* share_list;
};

struct BSP_Node;
struct BSP
{
    enum TYPE
    {
        BSP_TYPE_NODE,
        BSP_TYPE_INST
    };

    TYPE type;    
    BSP_Node* bsp_parent;
    float bbox[6]; // in world coords
};

struct BSP_Node : BSP
{
    BSP* bsp_child[2];
};

struct Inst : BSP
{
    // in world
	Inst* next;
	Inst* prev;    

    Mesh* mesh;
    double tm[16]; // absoulte! mesh->world

    Inst* share_next; // next instance sharing same mesh

    int /*FLAGS*/ flags; 
    char* name;
};



struct World
{
    int meshes;
    Mesh* head_mesh;
    Mesh* tail_mesh;

    Mesh* LoadMesh(const char* path, const char* name);

    Mesh* AddMesh(const char* name = 0, void* cookie = 0)
    {
        Mesh* m = (Mesh*)malloc(sizeof(Mesh));

        m->world = this;
        m->type = Mesh::MESH_TYPE_3D;
        m->name = name ? strdup(name) : 0;
        m->cookie = cookie;

        m->next = 0;
        m->prev = tail_mesh;
        if (tail_mesh)
            tail_mesh->next = m;
        else
            head_mesh = m;
        tail_mesh = m;    

        m->share_list = 0;
 
        m->verts = 0;
        m->head_vert = 0;
        m->tail_vert = 0;
        
        m->faces = 0;
        m->head_face = 0;
        m->tail_face = 0;

        m->lines = 0;
        m->head_line = 0;
        m->tail_line = 0;

        memset(m->bbox,0,sizeof(float[6]));

        meshes++;

        return m;
    }

    bool DelMesh(Mesh* m)
    {
        if (!m || m->world != this)
            return false;

        bool kill_bsp = false;

        // kill sharing insts
        Inst* i = m->share_list;
        while (i)
        {
            Inst* n = i->share_next;
            if (i->prev)
                i->prev->next = i->next;
            else
                head_inst = i->next;

            if (i->next)
                i->next->prev = i->prev;
            else
                tail_inst = i->prev;

            if (i->name)
                free(i->name);

            if (editable == i)
                editable = 0;

            if (i->flags & INST_IN_TREE)
            {
                if (root == i)
                    root = 0;
                else
                if (i->bsp_parent)
                {
                    if (i->bsp_parent->bsp_child[0]==i)
                        i->bsp_parent->bsp_child[0]=0;
                    else
                        i->bsp_parent->bsp_child[1]=0;
                }
                kill_bsp = true;
            }

            free(i);
            insts--;

            i=n;
        }

        Face* f = m->head_face;
        while (f)
        {
            Face* n = f->next;
            free(f);
            f=n;
        }


        Line* l = m->head_line;
        while (l)
        {
            Line* n = l->next;
            free(l);
            l=n;
        }        

        Vert* v = m->head_vert;
        while (v)
        {
            Vert* n = v->next;
            free(v);
            v=n;
        }

        if (m->name)
            free(m->name);

        if (m->prev)
            m->prev->next = m->next;
        else
            head_mesh = m->next;

        if (m->next)
            m->next->prev = m->prev;
        else
            tail_mesh = m->prev;

        free(m);
        meshes--;

        if (kill_bsp && root)
            DeleteBSP(root);

        return true;
    }

    // all insts
    int insts;
    Inst* head_inst;
    Inst* tail_inst;

    Inst* AddInst(Mesh* m, const double tm[16], const char* name)
    {
        if (!m || m->world != this)
            return 0;

        Inst* i = (Inst*)malloc(sizeof(Inst));

        if (tm)
            memcpy(i->tm,tm,sizeof(double[16]));
        else
        {
            memset(i->tm,0,sizeof(double[16]));
            i->tm[0] = i->tm[5] = i->tm[10] = i->tm[15] = 1.0;
        }
        
        i->name = name ? strdup(name) : 0;

        i->mesh = m;

        i->type = BSP::BSP_TYPE_INST;
        i->flags = 0; // visible / in-bsp / use-bsp ?
        i->bsp_parent = 0;

        if (m)
        {
            i->share_next = m->share_list;
            m->share_list = i;
        }
        else
            i->share_next = 0;

        i->next = 0;
        i->prev = tail_inst;
        if (tail_inst)
            tail_inst->next = i;
        else
            head_inst = i;
        tail_inst = i;    
        
        // i->bbox[] = ;

        insts++;
        return i;
    }

    bool DelInst(Inst* i)
    {
        if (!i || !i->mesh || i->mesh->world != this)
            return false;

        if (i->mesh)
        {
            Inst** s = &i->mesh->share_list;
            while (*s != i)
                s = &(*s)->share_next;
            *s = (*s)->share_next;
        }

        if (i->prev)
            i->prev->next = i->next;
        else
            head_inst = i->next;

        if (i->next)
            i->next->prev = i->prev;
        else
            tail_inst = i->prev;

        if (i->name)
            free(i->name);

        if (editable == i)
            editable = 0;

        bool kill_bsp = false;

        if (i->flags & INST_IN_TREE)
        {
            if (root == i)
                root = 0;
            else
            {
                if (i->bsp_parent->bsp_child[0]==i)
                    i->bsp_parent->bsp_child[0]=0;
                else
                    i->bsp_parent->bsp_child[1]=0;
            }
            kill_bsp = true;
        }
            
        insts--;
        free(i);

        if (kill_bsp && root)
            DeleteBSP(root);        

        return true;
    }

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

            if (node->bsp_child[0])
                DeleteBSP(node->bsp_child[0]);
            if (node->bsp_child[1])
                DeleteBSP(node->bsp_child[1]);
            free (bsp);
        }
        else
        if (bsp->type == BSP::BSP_TYPE_INST)
        {
            Inst* inst = (Inst*)bsp;
            inst->flags &= ~INST_IN_TREE;
            bsp->bsp_parent = 0;
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
            if (inst->flags & INST_USE_TREE)
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
                        arr[a]->bbox[0] < arr[u]->bbox[0] ? arr[u]->bbox[0] : arr[v]->bbox[0],
                        arr[a]->bbox[1] > arr[u]->bbox[1] ? arr[u]->bbox[1] : arr[v]->bbox[1],
                        arr[a]->bbox[2] < arr[u]->bbox[2] ? arr[u]->bbox[2] : arr[v]->bbox[2],
                        arr[a]->bbox[3] > arr[u]->bbox[3] ? arr[u]->bbox[3] : arr[v]->bbox[3],
                        arr[a]->bbox[4] < arr[u]->bbox[4] ? arr[u]->bbox[4] : arr[v]->bbox[4],
                        arr[a]->bbox[5] > arr[u]->bbox[5] ? arr[u]->bbox[5] : arr[v]->bbox[5]
                    };

                    float vol = (bbox[1]-bbox[0]) * (bbox[3]-bbox[2]) * (bbox[5]-bbox[4]);
                    
                    float u_vol = (arr[u]->bbox[1]-arr[u]->bbox[0]) * (arr[u]->bbox[3]-arr[u]->bbox[2]) * (arr[u]->bbox[5]-arr[u]->bbox[4]);
                    float v_vol = (arr[v]->bbox[1]-arr[v]->bbox[0]) * (arr[v]->bbox[3]-arr[v]->bbox[2]) * (arr[v]->bbox[5]-arr[v]->bbox[4]);
                    
                    vol -= u_vol + v_vol; // minimize volumne expansion

                    // minimize volume difference between children
                    if (u_vol > v_vol)
                        vol += u_vol-v_vol;
                    else
                        vol += v_vol-u_vol;

                    if (vol < e || e<0)
                    {
                        a = u;
                        b = v;
                        e = vol;
                    }
                }
            }

            BSP_Node* node = (BSP_Node*)malloc(sizeof(BSP_Node));

            node->bsp_parent = 0;
            node->type = BSP::BSP_TYPE_NODE;

            node->bsp_child[0] = arr[a];
            node->bsp_child[1] = arr[b];

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

    // closest to 'eye' intersecting face
    struct FaceHit
    {
        Inst* inst;
        Face* face;
        float abc_z[4]; // barycentric and world's z
    };

    // closest to ray isolated vert
    struct IsolHit
    {
        Inst* inst;
        Vert* vert;
    };

    // RAY HIT
    bool Query(double p[3], double v[3], FaceHit* face_hit, IsolHit* isol_hit)
    {
        // hit->abc_z[3] MUST be preinitialized to FAREST z

        // 1. find closest to eye intersecting face
        // 2. closest vertex -> max(a,b,c)
        // 3. closest edge -> min(a,b,c)

        // what about isolated verts?
        // they should form their own list
        // and we can find one closest to ray in 3d

        return false;
    }

    // MESHES IN HULL
    void Query(int planes, double plane[][4], void (*cb)(Mesh* m, const double tm[16], void* cookie), void* cookie)
    {
        // query possibly visible instances

        // temporarily report all insts
        Inst* i = head_inst;
        while (i)
        {
            cb(i->mesh, i->tm, cookie);
            i=i->next;
        }
    }
 
    // FACES IN HULL
    void Query(int planes, double plane[][4], void (*cb)(float coords[9], uint32_t visual, void* cookie), void* cookie)
    {
        // temporarily report all faces
        Inst* i = head_inst;

        float coords[10];

        while (i)
        {
            Face* f = i->mesh->head_face;
            while (f)
            {
                Product(i->tm,f->abc[0]->xyzw,coords+0);
                Product(i->tm,f->abc[1]->xyzw,coords+3);
                Product(i->tm,f->abc[2]->xyzw,coords+6);

                cb(coords, f->visual, cookie);
                f=f->next;
            }

            i=i->next;
        }
    }
};

Mesh* World::LoadMesh(const char* path, const char* name)
{
    FILE* f = fopen(path,"rt");
    if (!f)
        return 0;

    Mesh* m = AddMesh(name ? name : path);
    int plannar = 0x7;

    char buf[1024];

    // first pass - scan all verts

    while (fgets(buf,1024,f))
    {
        // note: on every char check for # or \n or \r or 0 -> end of line

        char* p = buf;
        while (*p == ' ' || *p == '\t')
            p++;

        if (*p=='#' || *p=='\r' || *p=='\n' || *p=='\0')
            continue;

        if (p[0] != 'v' || p[1] != ' ')
            continue;

        p += 2;
        // expect 2-4 floats, ignore 4th if present
        float xyzw[4];
        int coords = sscanf(p,"%f %f %f %f", xyzw+0, xyzw+1, xyzw+2, xyzw+3);
        if (coords<2)
            continue;

        if (coords<3)
            xyzw[2] = 0.0f;
        if (coords<4)
            xyzw[3] = 1.0f;

        Vert* v = (Vert*)malloc(sizeof(Vert));
        v->xyzw[0] = xyzw[0];
        v->xyzw[1] = xyzw[1];
        v->xyzw[2] = xyzw[2];
        v->xyzw[3] = xyzw[3];

        if (m->verts && plannar)
        {
            if (plannar&1)
            {
                if (v->xyzw[0] != m->head_vert->xyzw[0])
                    plannar&=~1;
            }
            if (plannar&2)
            {
                if (v->xyzw[1] != m->head_vert->xyzw[1])
                    plannar&=~2;
            }
            if (plannar&4)
            {
                if (v->xyzw[2] != m->head_vert->xyzw[2])
                    plannar&=~4;
            }
        }

        v->mesh = m;
        v->face_list = 0;
        v->line_list = 0;
        v->next = 0;
        v->prev = m->tail_vert;
        if (m->tail_vert)
            m->tail_vert->next = v;
        else
            m->head_vert = v;
        m->tail_vert = v;

        if (!m->verts)
        {
            m->bbox[0] = v->xyzw[0];
            m->bbox[1] = v->xyzw[0];
            m->bbox[2] = v->xyzw[1];
            m->bbox[3] = v->xyzw[1];
            m->bbox[4] = v->xyzw[2];
            m->bbox[5] = v->xyzw[2];
        }
        else
        {
            m->bbox[0] = v->xyzw[0] < m->bbox[0] ? v->xyzw[0] : m->bbox[0];
            m->bbox[1] = v->xyzw[0] > m->bbox[1] ? v->xyzw[0] : m->bbox[1];
            m->bbox[2] = v->xyzw[1] < m->bbox[2] ? v->xyzw[1] : m->bbox[2];
            m->bbox[3] = v->xyzw[1] > m->bbox[3] ? v->xyzw[1] : m->bbox[3];
            m->bbox[4] = v->xyzw[2] < m->bbox[4] ? v->xyzw[2] : m->bbox[4];
            m->bbox[5] = v->xyzw[2] > m->bbox[5] ? v->xyzw[2] : m->bbox[5];
        }

        v->sel = false;
        m->verts++;
    }

    if (m->verts)
    {
        Vert** index = (Vert**)malloc(sizeof(Vert*)*m->verts);
        Vert* v = m->head_vert;
        for (int i=0; i<m->verts; i++)
        {
            index[i] = v;
            v = v->next;
        }

        fseek(f,0,SEEK_SET);

        int verts = 0;

        while (fgets(buf,1024,f))
        {
            char* p = buf;
            while (*p == ' ' || *p == '\t')
                p++;

            if (*p=='#' || *p=='\r' || *p=='\n' || *p=='\0')
                continue;

            if (p[1] != ' ')
                continue;

            int cmd = p[0];
            if (cmd == 'f' || cmd == 'l')
            {
                p += 2;

                // we assume it is 'f' is fan 'l" is linestrip

                int num = 0;
                int abc[3];
                char* end;

                while (abc[num]=(int)strtol(p,&end,0))
                {
                    if (abc[num] > 0)
                        abc[num] -= 1;
                    else
                        abc[num] += verts;

                    if (abc[num] >= 0 && abc[num] < m->verts)
                    {
                        if (cmd == 'f')
                        {
                            if (num<2)
                                num++;
                            else
                            {
                                // add face
                                Face* f = (Face*)malloc(sizeof(Face));

                                f->visual = 0;
                                f->mesh = m;

                                f->next = 0;
                                f->prev = m->tail_face;
                                if (m->tail_face)
                                    m->tail_face->next = f;
                                else
                                    m->head_face = f;
                                m->tail_face = f;

                                for (int i=0; i<3; i++)
                                {
                                    f->abc[i] = index[abc[i]];
                                    f->share_next[i] = f->abc[i]->face_list;
                                    f->abc[i]->face_list = f;
                                }

                                m->faces++;

                                abc[1] = abc[2];
                            }
                        }
                        else
                        if (cmd == 'l')
                        {
                            if (num<1)
                                num++;
                            else
                            {
                                // add line
                                Line* l = (Line*)malloc(sizeof(Line));

                                l->visual = 0;
                                l->mesh = m;

                                l->next = 0;
                                l->prev = m->tail_line;
                                if (m->tail_line)
                                    m->tail_line->next = l;
                                else
                                    m->head_line = l;
                                m->tail_line = l;

                                for (int i=0; i<2; i++)
                                {
                                    l->ab[i] = index[abc[i]];
                                    l->share_next[i] = l->ab[i]->line_list;
                                    l->ab[i]->line_list = l;
                                }

                                m->lines++;

                                abc[0] = abc[1];
                            }
                        }
                        
                    }

                    p = end;

                    // looking for space,digit
                    while (*p!='#' && *p!='\r' && *p!='\n' && *p!='\0')
                    {
                        if ((p[0]==' ' || p[0]=='\t') && (p[1]=='-' || p[1]=='+' || p[1]>='0' && p[1]<='9'))
                        {
                            end = 0;
                            break;
                        }
                        p++;
                    }

                    if (end)
                        break;

                    p++;
                }
            }
            else
            if (cmd == 'v')
            {
                p += 2;
                // expect 2-4 floats, ignore 4th if present
                float xyzw[4];
                int coords = sscanf(p,"%f %f %f %f", xyzw+0, xyzw+1, xyzw+2, xyzw+3);
                if (coords<2)
                    continue;

                // needed for relative indices
                verts++;
            }
        }

        free(index);
    }

    fclose(f);
    return m;
}

World* CreateWorld()
{
    World* w = (World*)malloc(sizeof(World));

    w->meshes = 0;
    w->head_mesh = 0;
    w->tail_mesh = 0;
    w->insts = 0;
    w->head_inst = 0;
    w->tail_inst = 0;
    w->editable = 0;
    w->root = 0;

    return w;
}

void DeleteWorld(World* w)
{
    if (!w)
        return;

    if (w->root)
        w->DeleteBSP(w->root);

    // killing all meshes should kill all insts as well

    while (w->meshes)
        w->DelMesh(w->head_mesh);
}

Mesh* LoadMesh(World* w, const char* path, const char* name)
{
    if (!w)
        return 0;
    return w->LoadMesh(path, name);
}

void DeleteMesh(Mesh* m)
{
    if (!m)
        return;
    m->world->DelMesh(m);
}

Inst* CreateInst(Mesh* m, const double tm[16], const char* name)
{
    if (!m)
        return 0;
    return m->world->AddInst(m,tm,name);
}

void DeleteInst(Inst* i)
{
    if (!i)
        return;
    i->mesh->world->DelInst(i);
}

int GetInstFlags(Inst* i)
{
    if (!i)
        return 0;
    return i->flags & ~INST_IN_TREE;
}

void SetInstFlags(Inst* i, int flags, int mask)
{
    if (!i)
        return;
    mask &= ~INST_IN_TREE;
    i->flags = (i->flags & ~mask) | (flags & mask);
}

void QueryWorld(World* w, int planes, double plane[][4], void (*cb)(Mesh* m, const double tm[16], void* cookie), void* cookie)
{
    if (!w)
        return;
    w->Query(planes,plane,cb,cookie);
}

Mesh* GetFirstMesh(World* w)
{
    if (!w)
        return 0;
    return w->head_mesh;
}

Mesh* GetLastMesh(World* w)
{
    if (!w)
        return 0;
    return w->tail_mesh;
}

Mesh* GetPrevMesh(Mesh* m)
{
    if (!m)
        return 0;
    return m->prev;
}

Mesh* GetNextMesh(Mesh* m)
{
    if (!m)
        return 0;
    return m->next;
}

int GetMeshName(Mesh* m, char* buf, int size)
{
    if (!m)
    {
        if (buf && size>0)
            *buf=0;
        return 0;
    }

    int len = (int)strlen(m->name);

    if (buf && size>0)
        strncpy(buf,m->name,size);

    return len+1;
}

void GetMeshBBox(Mesh* m, float bbox[6])
{
    if (!m || !bbox)
        return;

    bbox[0] = m->bbox[0];
    bbox[1] = m->bbox[1];
    bbox[2] = m->bbox[2];
    bbox[3] = m->bbox[3];
    bbox[4] = m->bbox[4];
    bbox[5] = m->bbox[5];
}

void QueryMesh(Mesh* m, void (*cb)(float coords[9], uint32_t visual, void* cookie), void* cookie)
{
    if (!m || !cb)
        return;

    float coords[9];

    Face* f = m->head_face;
    while (f)
    {
        coords[0] = f->abc[0]->xyzw[0];
        coords[1] = f->abc[0]->xyzw[1];
        coords[2] = f->abc[0]->xyzw[2];

        coords[3] = f->abc[1]->xyzw[0];
        coords[4] = f->abc[1]->xyzw[1];
        coords[5] = f->abc[1]->xyzw[2];

        coords[6] = f->abc[2]->xyzw[0];
        coords[7] = f->abc[2]->xyzw[1];
        coords[8] = f->abc[2]->xyzw[2];

        cb(coords, f->visual, cookie);

        f=f->next;
    }
}

void* GetMeshCookie(Mesh* m)
{
    if (!m)
        return 0;
    return m->cookie;
}

void  SetMeshCookie(Mesh* m, void* cookie)
{
    if (m)
        m->cookie = cookie;
}
