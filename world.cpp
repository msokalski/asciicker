#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <assert.h>
#include <float.h>

#include "sprite.h"
#include "world.h"
#include "matrix.h"

#include "terrain.h"
#include "inventory.h"

struct Line;
struct Face;

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

	int vert_id; // index into 
	float xyzw[4];
	unsigned char rgba[4];

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

    bool freestyle;
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
};

struct Inst;
struct World;
struct MeshInst;

struct Mesh
{
    World* world;
    char* name; // in form of library path?
    void* cookie;

    // in world
	Mesh* next;
	Mesh* prev;

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

    MeshInst* share_list;

    bool Update(const char* path);
};

struct BSP
{
    enum TYPE
    {
        BSP_TYPE_NODE,
        BSP_TYPE_NODE_SHARE,
        BSP_TYPE_LEAF,
        BSP_TYPE_INST // mesh or sprite inst
    };

    TYPE type;    
    float bbox[6]; // in world coords
    BSP* bsp_parent; // BSP_Node or BSP_NodeShare or NULL if not attached to tree

	bool InsertInst(World* w, Inst* i);
};

struct BSP_Node : BSP
{
    BSP* bsp_child[2];
};

struct BSP_NodeShare : BSP_Node
{
    // list of shared instances 
    Inst* head; 
    Inst* tail;
};

struct BSP_Leaf : BSP
{
    // list of instances 
   Inst* head; 
   Inst* tail;
};

struct Inst : BSP
{
	enum INST_TYPE
	{
		MESH = 1,
		SPRITE = 2,
		ITEM = 3
	};

	INST_TYPE inst_type;
	char* name;
	int story_id;

    // in BSP_Leaf::inst / BSP_NodeShare::inst
	Inst* next;
	Inst* prev;    

    int /*FLAGS*/ flags; 
};

struct MeshInst : Inst
{
	Mesh* mesh;
	double tm[16]; // absoulte! mesh->world

	MeshInst* share_next; // next instance sharing same mesh

	void UpdateBox()
	{
		float w[4];
		Vert* v = mesh->head_vert;

		if (v)
		{
			Product(tm, v->xyzw, w);
			bbox[0] = w[0];
			bbox[1] = w[0];
			bbox[2] = w[1];
			bbox[3] = w[1];
			bbox[4] = w[2];
			bbox[5] = w[2];
			v = v->next;
		}

		while (v)
		{
			Product(tm, v->xyzw, w);
			bbox[0] = fminf(bbox[0], w[0]);
			bbox[1] = fmaxf(bbox[1], w[0]);
			bbox[2] = fminf(bbox[2], w[1]);
			bbox[3] = fmaxf(bbox[3], w[1]);
			bbox[4] = fminf(bbox[4], w[2]);
			bbox[5] = fmaxf(bbox[5], w[2]);
			v = v->next;
		}
	}

	bool HitFace(double ray[10], double ret[3], double nrm[3], bool positive_only, bool editor, bool solid_only)
	{
		if (!mesh)
			return false;

		bool flag = false;

		Face* f = flags & INST_FLAGS::INST_VISIBLE ? mesh->head_face : 0;
		while (f)
		{
			if (solid_only)
			{
				if ((f->abc[0]->rgba[3] | f->abc[1]->rgba[3] | f->abc[2]->rgba[3]) & 0x80)
				{
					f = f->next;
					continue;
				}
			}

			double v0[4], v1[4], v2[4];
			Product(tm, f->abc[0]->xyzw, v0);
			Product(tm, f->abc[1]->xyzw, v1);
			Product(tm, f->abc[2]->xyzw, v2);

			if (RayIntersectsTriangle(ray, v0, v1, v2, ret, positive_only))
			{
				if (nrm)
				{
					double d1[3] = { v1[0] - v0[0], v1[1] - v0[1], v1[2] - v0[2] };
					double d2[3] = { v2[0] - v0[0], v2[1] - v0[1], v2[2] - v0[2] };
					CrossProduct(d1, d2, nrm);
				}

				flag = true;
			}

			f = f->next;
		}

		return flag;
	}
};

inline bool HitSprite(Sprite* sprite, int anim, int frame, float pos[3], float yaw, double ray[10], double ret[3], bool positive_only)
{
	// calc 4 xyz corners of sprite rect aligned up +Z and ray_xy being perpendicular to it

	// maybe just plane equation first (must pass through pos and its normal is {-ray_x,-ray_y,0}
	double plane[4] = { -ray[3],-ray[4],0,0 };
	double dpos[3] = { pos[0],pos[1],pos[2] };
	plane[3] = -DotProduct(plane, dpos);

	// so we can calc plane ray intersection
	double d = ray[3] * plane[0] + ray[4] * plane[1] + ray[5] * plane[2];
	if (fabs(d) < 0.001)
		return false;
	double n = ray[6] * plane[0] + ray[7] * plane[1] + ray[8] * plane[2] + plane[3];
	double q = -n / d;

	if (positive_only && q < 0)
		return false;
	if (q > ray[9])
		return false;

	double p[3] =
	{
		ray[6] + q * ray[3],
		ray[7] + q * ray[4],
		ray[8] + q * ray[5]
	};

	// get frame from sprite

	double rot_yaw = atan2(ray[4], ray[3]) * 180 / M_PI - 90;
	Sprite* s = sprite;
	float angle = yaw;
	int ang = (int)floor((angle - rot_yaw) * s->angles / 360.0f + 0.5f);
	ang = ang >= 0 ? ang % s->angles : (ang % s->angles + s->angles) % s->angles;

	int i = frame + ang * s->anim[anim].length;
	//if (proj && s->projs > 1)
	//	i += s->anim[anim].length * s->angles;
	Sprite::Frame* f = s->atlas + s->anim[anim].frame_idx[i];

	// transform intersection to cell coords
	float zoom = 2.0 / 3.0;
	float cos30 = cosf(30 * M_PI / 180);
	float dwx = zoom * f->width * 0.5f * cos(rot_yaw*M_PI / 180);
	float dwy = zoom * f->width * 0.5f * sin(rot_yaw*M_PI / 180);
	float dlz = zoom * -f->ref[1] * 0.5f / cos30 * HEIGHT_SCALE;
	float dhz = zoom * (f->height - f->ref[1] * 0.5f) / cos30 * HEIGHT_SCALE;

	float ds = 2.0 * (/*zoom*/ 1.0 * /*scale*/ 3.0) / VISUAL_CELLS * 0.5 /*we're not dbl_wh*/;
	float dz_dy = HEIGHT_SCALE / (cos30 * HEIGHT_CELLS * ds);

	double left = (pos[0] - dwx)*dwx + (pos[1] - dwy)*dwy;
	double right = (pos[0] + dwx)*dwx + (pos[1] + dwy)*dwy;
	double bottom = (pos[2] + dlz);
	double top = (pos[2] + dhz);

	double dot_xy = p[0] * dwx + p[1] * dwy;
	double dot_z = p[2];

	int x = (int)floor((dot_xy - left) / (right - left) * f->width);
	int y = (int)floor((dot_z - bottom) / (top - bottom) * f->height);

	if (x >= 0 && x < f->width && y >= 0 && y < f->height)
	{
		// inside rect
		AnsiCell* ac = f->cell + x + y * f->width;
		if (ac->bk != 255 && ac->gl != 219 || ac->fg != 255 && ac->gl != 0 && ac->gl != 32)
		{
			// not transparent

			// todo later
			// we could do raster check using current font if either fg or bk is transparant
			// ...

			float h = HEIGHT_SCALE / 4 + pos[2] + (2.0*ac->spare + f->ref[2]) * 0.5 * dz_dy;
			if (ret[2] < h)
			{
				// printf("%d,%d\n", x, y);
				ret[0] = p[0];
				ret[1] = p[1];
				ret[2] = h;

				ray[9] = q;
				return true;
			}
		}
	}

	return false;
}

struct SpriteInst : Inst
{
	World* w;
	Sprite* sprite;
	void* data; // player(human) or creature or null
	int anim;
	int frame;
	int reps[4];
	float yaw;
	float pos[3];

	bool Hit(double ray[10], double ret[3], bool positive_only)
	{
		if (flags & INST_FLAGS::INST_VISIBLE)
			return HitSprite(sprite, anim, frame, pos, yaw, ray, ret, positive_only);
		return false;
	}
};

struct ItemInst : Inst
{
	World* w;
	Item* item;
	float yaw;
	float pos[3];

	bool Hit(double ray[10], double ret[3], bool positive_only)
	{
		if (flags & INST_FLAGS::INST_VISIBLE)
			return HitSprite(item->proto->sprite_3d, 0/*anim*/, 0/*frame*/, pos, yaw, ray, ret, positive_only);
		return false;
	}

};

ItemInst* item_inst_cache = 0;

ItemInst* AllocItemInst()
{
	if (!item_inst_cache)
		return (ItemInst*)malloc(sizeof(ItemInst));
	ItemInst* ii = item_inst_cache;
	item_inst_cache = (ItemInst*)ii->next;
	return ii;
}

void FreeItemInst(ItemInst* ii)
{
	ii->next = item_inst_cache;
	item_inst_cache = ii;
}

void PurgeItemInstCache()
{
	ItemInst* ii = item_inst_cache;
	while (ii)
	{
		ItemInst* n = (ItemInst*)ii->next;
		free(ii);
		ii = n;
	}
	item_inst_cache = 0;
}

int bsp_tests=0;
int bsp_insts=0;
int bsp_nodes=0;

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

        // kill sharing insts
        Inst* i = m->share_list;
        while (m->share_list)
            DelInst(m->share_list);

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

        return true;
    }

    int insts; // all (meshes, sprites, edit items, world items)
	int temp_insts; // only world items

    // only non-bsp
    Inst* head_inst; 
    Inst* tail_inst;

	Inst* AddInst(Item* item, int flags, float pos[3], float yaw, int story_id)
	{
		ItemInst* i = AllocItemInst();
		i->story_id = story_id;
		i->name = 0;
		i->inst_type = Inst::INST_TYPE::ITEM;
		i->w = this;
		i->item = item;

		Sprite* s = item->proto->sprite_3d;

		i->bbox[0] = s->proj_bbox[0] + pos[0];
		i->bbox[1] = s->proj_bbox[1] + pos[0];
		i->bbox[2] = s->proj_bbox[2] + pos[1];
		i->bbox[3] = s->proj_bbox[3] + pos[1];
		i->bbox[4] = s->proj_bbox[4] + pos[2];
		i->bbox[5] = s->proj_bbox[5] + pos[2];

		i->yaw = yaw;
		i->pos[0] = pos[0];
		i->pos[1] = pos[1];
		i->pos[2] = pos[2];

		i->type = BSP::BSP_TYPE_INST;
		i->flags = flags;
		i->bsp_parent = 0;

		i->next = 0;
		i->prev = tail_inst;
		if (tail_inst)
			tail_inst->next = i;
		else
			head_inst = i;
		tail_inst = i;

		// if (item->purpose == Item::WORLD)
		if (flags & INST_FLAGS::INST_VOLATILE)
			temp_insts++;

		insts++;

		return i;
	}

	Inst* AddInst(Sprite* s, int flags, float pos[3], float yaw, int anim, int frame, int reps[4], const char* name, int story_id)
	{
		SpriteInst* i = (SpriteInst*)malloc(sizeof(SpriteInst));
		i->story_id = story_id;
		i->inst_type = Inst::INST_TYPE::SPRITE;
		i->w = this;
		i->sprite = s;
		i->data = 0;

		i->bbox[0] = s->proj_bbox[0] + pos[0];
		i->bbox[1] = s->proj_bbox[1] + pos[0];
		i->bbox[2] = s->proj_bbox[2] + pos[1];
		i->bbox[3] = s->proj_bbox[3] + pos[1];
		i->bbox[4] = s->proj_bbox[4] + pos[2];
		i->bbox[5] = s->proj_bbox[5] + pos[2];

		i->pos[0] = pos[0];
		i->pos[1] = pos[1];
		i->pos[2] = pos[2];

		i->yaw = yaw;
		i->anim = anim;
		i->frame = frame;
		i->reps[0] = reps[0];
		i->reps[1] = reps[1];
		i->reps[2] = reps[2];
		i->reps[3] = reps[3];

		i->name = name ? strdup(name) : 0;

		i->type = BSP::BSP_TYPE_INST;
		i->flags = flags;
		i->bsp_parent = 0;

		i->next = 0;
		i->prev = tail_inst;
		if (tail_inst)
			tail_inst->next = i;
		else
			head_inst = i;
		tail_inst = i;

		if (flags & INST_FLAGS::INST_VOLATILE)
			temp_insts++;

		insts++;
		return i;
	}

    Inst* AddInst(Mesh* m, int flags, const double tm[16], const char* name, int story_id)
    {
        if (!m || m->world != this)
            return 0;

		MeshInst* i = (MeshInst*)malloc(sizeof(MeshInst));

		i->story_id = story_id;
		i->inst_type = Inst::INST_TYPE::MESH;

		if (tm)
		{
			memcpy(i->tm, tm, sizeof(double[16]));
		}
		else
		{
			memset(i->tm, 0, sizeof(double[16]));
			i->tm[0] = i->tm[5] = i->tm[10] = i->tm[15] = 1.0;
			i->bbox[0] = m->bbox[0];
			i->bbox[1] = m->bbox[1];
			i->bbox[2] = m->bbox[2];
			i->bbox[3] = m->bbox[3];
			i->bbox[4] = m->bbox[4];
			i->bbox[5] = m->bbox[5];
		}

        i->name = name ? strdup(name) : 0;

        i->mesh = m;

		i->UpdateBox();

        i->type = BSP::BSP_TYPE_INST;
        i->flags = flags;
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

		if (flags & INST_FLAGS::INST_VOLATILE)
			temp_insts++;
        
        insts++;
        return i;
    }

	bool DelInst(Inst* i)
	{
		if (!i)
			return false;
		if (i->inst_type == Inst::INST_TYPE::MESH)
			return DelInst((MeshInst*)i);
		if (i->inst_type == Inst::INST_TYPE::SPRITE)
			return DelInst((SpriteInst*)i);
		if (i->inst_type == Inst::INST_TYPE::ITEM)
			return DelInst((ItemInst*)i);
		return false;
	}

	bool DelInst(MeshInst* i)
	{
        if (!i || !i->mesh || i->mesh->world != this)
            return false;

        if (i->mesh)
        {
            MeshInst** s = &i->mesh->share_list;
            while (*s != i)
                s = &(*s)->share_next;
            *s = (*s)->share_next;
        }

        if (!i->bsp_parent)
        {
            if (i->prev)
                i->prev->next = i->next;
            else
                head_inst = i->next;

            if (i->next)
                i->next->prev = i->prev;
            else
                tail_inst = i->prev;
        }
        else
        {
            if (i->bsp_parent->type == BSP::BSP_TYPE_LEAF)
            {
                BSP_Leaf* leaf = (BSP_Leaf*)i->bsp_parent;

                if (i->prev)
                    i->prev->next = i->next;
                else
                    leaf->head = i->next;

                if (i->next)
                    i->next->prev = i->prev;
                else
                    leaf->tail = i->prev;

                if (leaf->head == 0)
                {
                    // do ancestors cleanup
                    // ...
                }
            }
            else
            if (i->bsp_parent->type == BSP::BSP_TYPE_NODE_SHARE)
            {
                BSP_NodeShare* share = (BSP_NodeShare*)i->bsp_parent;

                if (share->bsp_child[0] == i)
                    share->bsp_child[0] = 0;
                else
                if (share->bsp_child[1] == i)
                    share->bsp_child[1] = 0;
                else
                {
                    if (i->prev)
                        i->prev->next = i->next;
                    else
                        share->head = i->next;

                    if (i->next)
                        i->next->prev = i->prev;
                    else
                        share->tail = i->prev;
                }

                if (share->head == 0 && share->bsp_child[0]==0 && share->bsp_child[1]==0)
                {
                    // do ancestors cleanup
                    // ...
                }
            }
            else
            if (i->bsp_parent->type == BSP::BSP_TYPE_NODE)
            {
                BSP_Node* node = (BSP_Node*)i->bsp_parent;
                if (node->bsp_child[0] == i)
                    node->bsp_child[0] = 0;
                else
                if (node->bsp_child[1] == i)
                    node->bsp_child[1] = 0;

                if (node->bsp_child[0]==0 && node->bsp_child[1]==0)
                {
                    // do ancestors cleanup
                    // ...
                }
            }
            else
            {
                assert(0);
            }
        }

        if (i->name)
            free(i->name);

        if (editable == i)
            editable = 0;

        if (root == i)
            root = 0;
           
		if (i->flags & INST_FLAGS::INST_VOLATILE)
			temp_insts--;

        insts--;
        free(i);

        return true;
    }

	bool DelInst(SpriteInst* i)
	{
		if (!i)
			return false;

		if (!i->bsp_parent)
		{
			if (i->prev)
				i->prev->next = i->next;
			else
				head_inst = i->next;

			if (i->next)
				i->next->prev = i->prev;
			else
				tail_inst = i->prev;
		}
		else
		{
			if (i->bsp_parent->type == BSP::BSP_TYPE_LEAF)
			{
				BSP_Leaf* leaf = (BSP_Leaf*)i->bsp_parent;

				if (i->prev)
					i->prev->next = i->next;
				else
					leaf->head = i->next;

				if (i->next)
					i->next->prev = i->prev;
				else
					leaf->tail = i->prev;

				if (leaf->head == 0)
				{
					// do ancestors cleanup
					// ...
				}
			}
			else
				if (i->bsp_parent->type == BSP::BSP_TYPE_NODE_SHARE)
				{
					BSP_NodeShare* share = (BSP_NodeShare*)i->bsp_parent;

					if (share->bsp_child[0] == i)
						share->bsp_child[0] = 0;
					else
						if (share->bsp_child[1] == i)
							share->bsp_child[1] = 0;
						else
						{
							if (i->prev)
								i->prev->next = i->next;
							else
								share->head = i->next;

							if (i->next)
								i->next->prev = i->prev;
							else
								share->tail = i->prev;
						}

					if (share->head == 0 && share->bsp_child[0] == 0 && share->bsp_child[1] == 0)
					{
						// do ancestors cleanup
						// ...
					}
				}
				else
					if (i->bsp_parent->type == BSP::BSP_TYPE_NODE)
					{
						BSP_Node* node = (BSP_Node*)i->bsp_parent;
						if (node->bsp_child[0] == i)
							node->bsp_child[0] = 0;
						else
							if (node->bsp_child[1] == i)
								node->bsp_child[1] = 0;

						if (node->bsp_child[0] == 0 && node->bsp_child[1] == 0)
						{
							// do ancestors cleanup
							// ...
						}
					}
					else
					{
						assert(0);
					}
		}

		if (i->name)
			free(i->name);

		if (editable == i)
			editable = 0;

		if (root == i)
			root = 0;

		if (i->flags & INST_FLAGS::INST_VOLATILE)
			temp_insts--;

		insts--;
		free(i);

		return true;
	}


	bool DelInst(ItemInst* i)
	{
		if (!i)
			return false;

		if (!i->bsp_parent)
		{
			if (i->prev)
				i->prev->next = i->next;
			else
				head_inst = i->next;

			if (i->next)
				i->next->prev = i->prev;
			else
				tail_inst = i->prev;
		}
		else
		{
			if (i->bsp_parent->type == BSP::BSP_TYPE_LEAF)
			{
				BSP_Leaf* leaf = (BSP_Leaf*)i->bsp_parent;

				if (i->prev)
					i->prev->next = i->next;
				else
					leaf->head = i->next;

				if (i->next)
					i->next->prev = i->prev;
				else
					leaf->tail = i->prev;

				if (leaf->head == 0)
				{
					// do ancestors cleanup
					// ...
				}
			}
			else
			if (i->bsp_parent->type == BSP::BSP_TYPE_NODE_SHARE)
			{
				BSP_NodeShare* share = (BSP_NodeShare*)i->bsp_parent;

				if (share->bsp_child[0] == i)
					share->bsp_child[0] = 0;
				else
				if (share->bsp_child[1] == i)
					share->bsp_child[1] = 0;
				else
				{
					if (i->prev)
						i->prev->next = i->next;
					else
						share->head = i->next;

					if (i->next)
						i->next->prev = i->prev;
					else
						share->tail = i->prev;
				}

				if (share->head == 0 && share->bsp_child[0] == 0 && share->bsp_child[1] == 0)
				{
					// do ancestors cleanup
					// ...
				}
			}
			else
			if (i->bsp_parent->type == BSP::BSP_TYPE_NODE)
			{
				BSP_Node* node = (BSP_Node*)i->bsp_parent;
				if (node->bsp_child[0] == i)
					node->bsp_child[0] = 0;
				else
				if (node->bsp_child[1] == i)
					node->bsp_child[1] = 0;

				if (node->bsp_child[0] == 0 && node->bsp_child[1] == 0)
				{
					// do ancestors cleanup
					// ...
				}
			}
			else
			{
				assert(0);
			}
		}

		if (i->name)
			free(i->name);

		if (editable == i)
			editable = 0;

		if (root == i)
			root = 0;


		//if (i->item->purpose == Item::WORLD)
		if (i->flags & INST_FLAGS::INST_VOLATILE)
			temp_insts--;

		insts--;

		// item insts are frequently allocated and freed
		// cache em!

		FreeItemInst(i);

		return true;
	}

    // currently selected instance (its mesh) for editting
    // overrides visibility?
    Inst* editable;

    // now we want to form a tree of Insts
    BSP* root;

    void DeleteBSP(BSP* bsp)
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
        if (bsp->type == BSP::BSP_TYPE_NODE_SHARE)
        {
            BSP_NodeShare* share = (BSP_NodeShare*)bsp;

            if (share->bsp_child[0])
                DeleteBSP(share->bsp_child[0]);
            if (share->bsp_child[1])
                DeleteBSP(share->bsp_child[1]);

            if (share->head)
            {
                Inst* i = share->head;
                while (i)
                {
                    i->bsp_parent = 0;
                    i=i->next;
                }

                if (tail_inst)
                {
                    share->head->prev = tail_inst;
                    tail_inst->next = share->head;
                    tail_inst = share->tail;
                }
                else
                {
                    head_inst = share->head; 
                    tail_inst = share->tail;
                }
            }
            free (bsp);
        }        
        else
        if (bsp->type == BSP::BSP_TYPE_LEAF)
        {
            BSP_Leaf* leaf = (BSP_Leaf*)bsp;
            if (leaf->head)
            {
                Inst* i = leaf->head;
                while (i)
                {
                    i->bsp_parent = 0;
                    i=i->next;
                }

                if (tail_inst)
                {
                    leaf->head->prev = tail_inst;
                    tail_inst->next = leaf->head;
                    tail_inst = leaf->tail;
                }
                else
                {
                    head_inst = leaf->head; 
                    tail_inst = leaf->tail;
                }
            }
            free (bsp);
        }        
        else
        if (bsp->type == BSP::BSP_TYPE_INST)
        {
            Inst* inst = (Inst*)bsp;

            bsp->bsp_parent = 0;
            inst->next=0;
            inst->prev = tail_inst;
            if (tail_inst)
                tail_inst->next = inst;
            else
                head_inst = inst;
            tail_inst=inst;
        }
        else
        {
            assert(0);
        }
        
    }

    struct BSP_Item
    {
        Inst* inst;
        float area;
    };

    BSP* SplitBSP(BSP_Item* arr, int num)
    {
        assert(num>0);
        
        if (num == 1)
        {
            Inst* inst = arr[0].inst;
            inst->bsp_parent = 0;
            inst->prev = 0;
            inst->next = 0;
            return inst;
        }

        struct CentroidSorter
        {
            static int sortX(const void* a, const void* b)
            {
                Inst* ia = ((BSP_Item*)a)->inst;
                Inst* ib = ((BSP_Item*)b)->inst;

                float diff = (ia->bbox[0]+ia->bbox[1])-(ib->bbox[0]+ib->bbox[1]);
                if (diff<0)
                    return -1;
                if (diff>0)
                    return +1;
                return 0;
            }
            static int sortY(const void* a, const void* b)
            {
                Inst* ia = ((BSP_Item*)a)->inst;
                Inst* ib = ((BSP_Item*)b)->inst;

                float diff = (ia->bbox[2]+ia->bbox[3])-(ib->bbox[2]+ib->bbox[3]);
                if (diff<0)
                    return -1;
                if (diff>0)
                    return +1;
                return 0;
            }
            static int sortZ(const void* a, const void* b)
            {
                Inst* ia = ((BSP_Item*)a)->inst;
                Inst* ib = ((BSP_Item*)b)->inst;

                float diff = (ia->bbox[4]+ia->bbox[5])-(ib->bbox[4]+ib->bbox[5]);
                if (diff<0)
                    return -1;
                if (diff>0)
                    return +1;
                return 0;
            }
        };

        static int (*sort[3])(const void* a, const void* b) = 
        {
            CentroidSorter::sortX,
            CentroidSorter::sortY,
            CentroidSorter::sortZ
        };

        float best_cost = -1;
        int best_axis = -1;
        int best_item = -1;

        // actualy it could be better to splin only in x and y (axis<2)
        for (int axis=0; axis<3; axis++)
        {
            qsort(arr, num, sizeof(BSP_Item), sort[axis]);

            float lo_bbox[6] =
            {
                arr[0].inst->bbox[0],
                arr[0].inst->bbox[1],
                arr[0].inst->bbox[2],
                arr[0].inst->bbox[3],
                arr[0].inst->bbox[4],
                arr[0].inst->bbox[5]
            };

            for (int i=0; i<num; i++)
            {
                lo_bbox[0] = fminf( lo_bbox[0], arr[i].inst->bbox[0]);
                lo_bbox[1] = fmaxf( lo_bbox[1], arr[i].inst->bbox[1]);
                lo_bbox[2] = fminf( lo_bbox[2], arr[i].inst->bbox[2]);
                lo_bbox[3] = fmaxf( lo_bbox[3], arr[i].inst->bbox[3]);
                lo_bbox[4] = fminf( lo_bbox[4], arr[i].inst->bbox[4]);
                lo_bbox[5] = fmaxf( lo_bbox[5], arr[i].inst->bbox[5]);

                arr[i].area = 
                    (lo_bbox[1]-lo_bbox[0]) * (lo_bbox[3]-lo_bbox[2]) * HEIGHT_SCALE +
                    (lo_bbox[3]-lo_bbox[2]) * (lo_bbox[5]-lo_bbox[4]) +
                    (lo_bbox[5]-lo_bbox[4]) * (lo_bbox[1]-lo_bbox[0]);                
            }

            float hi_bbox[6] =
            {
                arr[num-1].inst->bbox[0],
                arr[num-1].inst->bbox[1],
                arr[num-1].inst->bbox[2],
                arr[num-1].inst->bbox[3],
                arr[num-1].inst->bbox[4],
                arr[num-1].inst->bbox[5]
            };

            for (int i=num-1; i>0; i--)
            {
                hi_bbox[0] = fminf( hi_bbox[0], arr[i].inst->bbox[0]);
                hi_bbox[1] = fmaxf( hi_bbox[1], arr[i].inst->bbox[1]);
                hi_bbox[2] = fminf( hi_bbox[2], arr[i].inst->bbox[2]);
                hi_bbox[3] = fmaxf( hi_bbox[3], arr[i].inst->bbox[3]);
                hi_bbox[4] = fminf( hi_bbox[4], arr[i].inst->bbox[4]);
                hi_bbox[5] = fmaxf( hi_bbox[5], arr[i].inst->bbox[5]);

                float area = 
                    (hi_bbox[1]-hi_bbox[0]) * (hi_bbox[3]-hi_bbox[2]) * HEIGHT_SCALE +
                    (hi_bbox[3]-hi_bbox[2]) * (hi_bbox[5]-hi_bbox[4]) +
                    (hi_bbox[5]-hi_bbox[4]) * (hi_bbox[1]-hi_bbox[0]);

                // cost of {0..i-1}, {i..num-1}
                float cost = arr[i-1].area * i + area * (num-i);
                if (cost < best_cost || best_cost < 0)
                {
                    best_cost = cost;
                    best_item = i;
                    best_axis = axis;
                }
            }
        }

        if (best_axis != -1)
        {
            if (best_cost + arr[num-1].area * 2 > arr[num-1].area * num)
                best_axis = -1;
        }

        if (best_axis == -1)
        {
            // fill final list of instances

            BSP_Leaf* leaf = (BSP_Leaf*)malloc(sizeof(BSP_Leaf));
            leaf->bsp_parent = 0;
            leaf->type = BSP::BSP_TYPE_LEAF;

            leaf->bbox[0] = arr[0].inst->bbox[0];
            leaf->bbox[1] = arr[0].inst->bbox[1];
            leaf->bbox[2] = arr[0].inst->bbox[2];
            leaf->bbox[3] = arr[0].inst->bbox[3];
            leaf->bbox[4] = arr[0].inst->bbox[4];
            leaf->bbox[5] = arr[0].inst->bbox[5];
            
            leaf->head = arr[0].inst;
            leaf->tail = arr[num-1].inst;
            
            arr[0].inst->prev = 0;
            arr[num-1].inst->next = 0;

            arr[0].inst->bsp_parent = leaf;
            if (num>1)
            {
                arr[0].inst->next = arr[1].inst;
                arr[num-1].inst->prev = arr[num-2].inst;
                arr[num-1].inst->bsp_parent = leaf;

                leaf->bbox[0] = fminf( leaf->bbox[0], arr[num-1].inst->bbox[0]);
                leaf->bbox[1] = fmaxf( leaf->bbox[1], arr[num-1].inst->bbox[1]);
                leaf->bbox[2] = fminf( leaf->bbox[2], arr[num-1].inst->bbox[2]);
                leaf->bbox[3] = fmaxf( leaf->bbox[3], arr[num-1].inst->bbox[3]);
                leaf->bbox[4] = fminf( leaf->bbox[4], arr[num-1].inst->bbox[4]);
                leaf->bbox[5] = fmaxf( leaf->bbox[5], arr[num-1].inst->bbox[5]);                
            }

            for (int i=1; i<num-1; i++)
            {
                leaf->bbox[0] = fminf( leaf->bbox[0], arr[i].inst->bbox[0]);
                leaf->bbox[1] = fmaxf( leaf->bbox[1], arr[i].inst->bbox[1]);
                leaf->bbox[2] = fminf( leaf->bbox[2], arr[i].inst->bbox[2]);
                leaf->bbox[3] = fmaxf( leaf->bbox[3], arr[i].inst->bbox[3]);
                leaf->bbox[4] = fminf( leaf->bbox[4], arr[i].inst->bbox[4]);
                leaf->bbox[5] = fmaxf( leaf->bbox[5], arr[i].inst->bbox[5]);  
                                
                arr[i].inst->bsp_parent = leaf;
                arr[i].inst->prev = arr[i-1].inst;
                arr[i].inst->next = arr[i+1].inst;
            }

            assert(leaf->head);
            assert(leaf->tail);
            assert(num==1 && leaf->head == leaf->tail || num!=1 && leaf->head != leaf->tail);
            assert(leaf->head->prev == 0);
            assert(leaf->tail->next == 0);

            Inst* i = leaf->head;
            int c = 0;
            while (i)
            {
                c++;
                i=i->next;
                if (i)
                    assert(i->prev->next == i);
            }

            assert(c==num);

            i = leaf->tail;
            c = 0;
            while (i)
            {
                c++;
                i=i->prev;
                if (i)
                    assert(i->next->prev == i);
            }

            assert(c==num);

            return leaf;
        }
        else
        {
            int axis = best_axis;
            qsort(arr, num, sizeof(BSP_Item), sort[axis]);

            // BSP_Node* node = (BSP_Node*)malloc(sizeof(BSP_Node));
			BSP_Node* node = (BSP_Node*)malloc(sizeof(BSP_NodeShare)); // make it easily changable!

            node->bsp_parent = 0;
            node->type = BSP::BSP_TYPE_NODE;

            node->bsp_child[0] = SplitBSP(arr + 0, best_item);
            node->bsp_child[0]->bsp_parent = node;

            node->bsp_child[1] = SplitBSP(arr + best_item, num - best_item);
            node->bsp_child[1]->bsp_parent = node;

            node->bbox[0] = fminf( node->bsp_child[0]->bbox[0], node->bsp_child[1]->bbox[0]);
            node->bbox[1] = fmaxf( node->bsp_child[0]->bbox[1], node->bsp_child[1]->bbox[1]);
            node->bbox[2] = fminf( node->bsp_child[0]->bbox[2], node->bsp_child[1]->bbox[2]);
            node->bbox[3] = fmaxf( node->bsp_child[0]->bbox[3], node->bsp_child[1]->bbox[3]);
            node->bbox[4] = fminf( node->bsp_child[0]->bbox[4], node->bsp_child[1]->bbox[4]);
            node->bbox[5] = fmaxf( node->bsp_child[0]->bbox[5], node->bsp_child[1]->bbox[5]);               

            return node;
        }
    }

	void Rebuild(bool boxes)
	{
		if (root)
		{
			DeleteBSP(root);
			root = 0;
		}

        if (!insts)
            return;

        // MAY BE SLOW: 1/2 * num^3
        /*
        int num = 0;

        BSP** arr = (BSP**)malloc(sizeof(BSP*) * insts);
        
        for (Inst* inst = head_inst; inst; inst=inst->next)
        {
            if (inst->flags & INST_USE_TREE)
                arr[num++] = inst;
        }

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
                        fminf( arr[u]->bbox[0] , arr[v]->bbox[0] ),
                        fmaxf( arr[u]->bbox[1] , arr[v]->bbox[1] ),
                        fminf( arr[u]->bbox[2] , arr[v]->bbox[2] ),
                        fmaxf( arr[u]->bbox[3] , arr[v]->bbox[3] ),
                        fminf( arr[u]->bbox[4] , arr[v]->bbox[4] ),
                        fmaxf( arr[u]->bbox[5] , arr[v]->bbox[5] )
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

            node->bbox[0] = fminf( arr[a]->bbox[0] , arr[b]->bbox[0] );
            node->bbox[1] = fmaxf( arr[a]->bbox[1] , arr[b]->bbox[1] );
            node->bbox[2] = fminf( arr[a]->bbox[2] , arr[b]->bbox[2] );
            node->bbox[3] = fmaxf( arr[a]->bbox[3] , arr[b]->bbox[3] );
            node->bbox[4] = fminf( arr[a]->bbox[4] , arr[b]->bbox[4] );
            node->bbox[5] = fmaxf( arr[a]->bbox[5] , arr[b]->bbox[5] );

            num--;
            if (b!=num)
                arr[b] = arr[num];

            arr[a] = node;
        }
        root = arr[0];
        free(arr);
        */

        // LET'S TRY:
        // https://graphics.stanford.edu/~boulos/papers/togbvh.pdf
        // http://www.cs.uu.nl/docs/vakken/magr/2016-2017/slides/lecture%2003%20-%20the%20perfect%20BVH.pdf

        // KEY IDEAS
        // leaf nodes need ability of multiple objects encapsulation
        // object can be referenced by both leaf siblings -> use NodeShare in place of Node !!! 
        // ... need only to ensure that union of both leaf bboxes fully encapsulates that object

        BSP_Item* arr = (BSP_Item*)malloc(sizeof(BSP_Item) * insts);

        int count = 0;
        for (Inst* inst = head_inst; inst; )
        {
			// update inst bbox
			if (boxes)
			{
				if (inst->inst_type == Inst::INST_TYPE::MESH)
					((MeshInst*)inst)->UpdateBox();
			}

            Inst* next = inst->next;
            if (inst->flags & INST_USE_TREE)
            {
				if (count==insts)
				{
					int defect=0;
				}
                arr[count++].inst = inst;

                // extract!
                if (inst->prev)
                    inst->prev->next = inst->next;
                else
                    head_inst = inst->next;
                if (inst->next)
                    inst->next->prev = inst->prev;
                else
                    tail_inst = inst->prev;
            }
            inst = next;
        }

        if (count)
        {
            // split recursively!
            root = SplitBSP(arr, count);

            if (!root)
            {
                // if failed we need to put them back
                for (int i=0; i<count; i++)
                {
                    Inst* inst = arr[i].inst;
                    inst->bsp_parent = 0;
                    inst->prev = tail_inst;
                    inst->next = 0;
                    if (tail_inst)
                        tail_inst->next = inst;
                    else
                        head_inst = inst;
                    tail_inst = inst;                        
                }
            }

            free(arr);
        }
    }

	static Inst* HitWorld0(BSP* q, double ray[10], double ret[3], double nrm[3], bool positive_only, bool editor, bool solid_only, bool sprites_too)
	{
		if (!q)
			return 0;

        const float x[2] = {q->bbox[0],q->bbox[1]};
		const float y[2] = {q->bbox[2],q->bbox[3]};
		const float z[2] = {q->bbox[4],q->bbox[5]};

		if (positive_only)
		{
			// do not recurse if all 8 corners projected onto ray are negative
		}

		if (ray[1] - z[0] * ray[3] + ray[5] * x[1] > 0 ||
			ray[5] * y[1] - ray[0] - z[0] * ray[4] > 0 ||
			ray[2] - ray[4] * x[0] + ray[3] * y[1] > 0 ||
			z[1] * ray[3] - ray[5] * x[0] - ray[1] > 0 ||
			ray[0] + z[1] * ray[4] - ray[5] * y[0] > 0 ||
			ray[4] * x[1] - ray[3] * y[0] - ray[2] > 0)
			return 0;

		if (q->type == BSP::TYPE::BSP_TYPE_INST)
		{
			Inst* inst = (Inst*)q;
			if (editor && (inst->flags & INST_VOLATILE) || 
				!editor && !(inst->flags & INST_VOLATILE))
				return 0;

			if (inst->inst_type == Inst::INST_TYPE::MESH)
			{
				if (((MeshInst*)inst)->HitFace(ray, ret, nrm, positive_only, editor, solid_only))
					return inst;
				else
					return 0;
			}
			else
			if (inst->inst_type == Inst::INST_TYPE::SPRITE)
			{
				if (sprites_too && ((SpriteInst*)inst)->Hit(ray, ret, positive_only))
					return inst;
				else
					return 0;
			}
			else
			if (inst->inst_type == Inst::INST_TYPE::ITEM)
			{
				if (sprites_too && ((ItemInst*)inst)->Hit(ray, ret, positive_only))
					return inst;
				else
					return 0;
			}
			return 0;
		}
        else
        if (q->type == BSP::TYPE::BSP_TYPE_NODE)
        {
            BSP_Node* n = (BSP_Node*)q;
            Inst* i = HitWorld0(n->bsp_child[0], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            Inst* j = HitWorld0(n->bsp_child[1], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            i = j ? j : i;
            return i;
        }
        else
        if (q->type == BSP::TYPE::BSP_TYPE_NODE_SHARE)
        {
            BSP_NodeShare* s = (BSP_NodeShare*)q;

            Inst* i = HitWorld0(s->bsp_child[0], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            Inst* j = HitWorld0(s->bsp_child[1], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            i = j ? j : i;

            j = s->head;
            while (j)
            {
				if (editor && (j->flags & INST_VOLATILE) || 
					!editor && !(j->flags & INST_VOLATILE))
				{
					j=j->next;
					continue;
				}

				if (j->inst_type == Inst::INST_TYPE::MESH)
				{
					if (((MeshInst*)j)->HitFace(ray, ret, nrm, positive_only, editor, solid_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::SPRITE && sprites_too)
				{
					if (((SpriteInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::ITEM && sprites_too)
				{
					if (((ItemInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}

                j=j->next;
            }
            return i;
        }
        else
        if (q->type == BSP::TYPE::BSP_TYPE_LEAF)
        {
            BSP_Leaf* l = (BSP_Leaf*)q;

            Inst* i = 0;
            Inst* j = l->head;
            while (j)
            {
				if (editor && (j->flags & INST_VOLATILE) || 
					!editor && !(j->flags & INST_VOLATILE))
				{
					j=j->next;
					continue;
				}

				if (j->inst_type == Inst::INST_TYPE::MESH)
				{
					if (((MeshInst*)j)->HitFace(ray, ret, nrm, positive_only, editor, solid_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::SPRITE && sprites_too)
				{
					if (((SpriteInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::ITEM && sprites_too)
				{
					if (((ItemInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}

                j=j->next;
            }
            return i;
        }
        
		return 0;
	}

	static Inst* HitWorld1(BSP* q, double ray[10], double ret[3], double nrm[3], bool positive_only, bool editor, bool solid_only, bool sprites_too)
	{
		if (!q)
			return 0;

		const float x[2] = { q->bbox[0],q->bbox[1] };
		const float y[2] = { q->bbox[2],q->bbox[3] };
		const float z[2] = { q->bbox[4],q->bbox[5] };

		if (positive_only)
		{
			// do not recurse if all 8 corners projected onto ray are negative
		}

		if (ray[5] * y[1] - ray[0] - z[0] * ray[4] > 0 ||
			z[0] * ray[3] - ray[5] * x[0] - ray[1] > 0 ||
			ray[2] - ray[4] * x[0] + ray[3] * y[0] > 0 ||
			ray[0] + z[1] * ray[4] - ray[5] * y[0] > 0 ||
			ray[1] - z[1] * ray[3] + ray[5] * x[1] > 0 ||
			ray[4] * x[1] - ray[3] * y[1] - ray[2] > 0)
			return 0;

		if (q->type == BSP::TYPE::BSP_TYPE_INST)
		{
			Inst* inst = (Inst*)q;
			if (editor && (inst->flags & INST_VOLATILE) || 
				!editor && !(inst->flags & INST_VOLATILE))
				return 0;

			if (inst->inst_type == Inst::INST_TYPE::MESH)
			{
				if (((MeshInst*)inst)->HitFace(ray, ret, nrm, positive_only, editor, solid_only))
					return inst;
				else
					return 0;
			}
			else
			if (inst->inst_type == Inst::INST_TYPE::SPRITE)
			{
				if (sprites_too && ((SpriteInst*)inst)->Hit(ray, ret, positive_only))
					return inst;
				else
					return 0;
			}
			else
			if (inst->inst_type == Inst::INST_TYPE::ITEM)
			{
				if (sprites_too && ((ItemInst*)inst)->Hit(ray, ret, positive_only))
					return inst;
				else
					return 0;
			}
			return 0;
		}
        else
        if (q->type == BSP::TYPE::BSP_TYPE_NODE)
        {
            BSP_Node* n = (BSP_Node*)q;
            Inst* i = HitWorld1(n->bsp_child[0], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            Inst* j = HitWorld1(n->bsp_child[1], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            i = j ? j : i;
            return i;
        }
        else
        if (q->type == BSP::TYPE::BSP_TYPE_NODE_SHARE)
        {
            BSP_NodeShare* s = (BSP_NodeShare*)q;

            Inst* i = HitWorld1(s->bsp_child[0], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            Inst* j = HitWorld1(s->bsp_child[1], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            i = j ? j : i;

            j = s->head;
            while (j)
            {
				if (editor && (j->flags & INST_VOLATILE) || 
					!editor && !(j->flags & INST_VOLATILE))
				{
					j=j->next;
					continue;
				}

				if (j->inst_type == Inst::INST_TYPE::MESH)
				{
					if (((MeshInst*)j)->HitFace(ray, ret, nrm, positive_only, editor, solid_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::SPRITE && sprites_too)
				{
					if (((SpriteInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::ITEM && sprites_too)
				{
					if (((ItemInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}

				j = j->next;
            }
            return i;
        }
        else
        if (q->type == BSP::TYPE::BSP_TYPE_LEAF)
        {
            BSP_Leaf* l = (BSP_Leaf*)q;

            Inst* i = 0;
            Inst* j = l->head;
            while (j)
            {
				if (editor && (j->flags & INST_VOLATILE) || 
					!editor && !(j->flags & INST_VOLATILE))
				{
					j=j->next;
					continue;
				}

				if (j->inst_type == Inst::INST_TYPE::MESH)
				{
					if (((MeshInst*)j)->HitFace(ray, ret, nrm, positive_only, editor, solid_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::SPRITE && sprites_too)
				{
					if (((SpriteInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::ITEM && sprites_too)
				{
					if (((ItemInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}

                j=j->next;
            }
            return i;
        }

		return 0;
	}

	static Inst* HitWorld2(BSP* q, double ray[10], double ret[3], double nrm[3], bool positive_only, bool editor, bool solid_only, bool sprites_too)
	{
		if (!q)
			return 0;

		const float x[2] = { q->bbox[0],q->bbox[1] };
		const float y[2] = { q->bbox[2],q->bbox[3] };
		const float z[2] = { q->bbox[4],q->bbox[5] };

		if (positive_only)
		{
			// do not recurse if all 8 corners projected onto ray are negative
		}

		if (ray[0] + z[0] * ray[4] - ray[5] * y[0] > 0 ||
			ray[1] - z[0] * ray[3] + ray[5] * x[1] > 0 ||
			ray[2] + ray[3] * y[1] - ray[4] * x[1] > 0 ||
			ray[5] * y[1] - ray[0] - z[1] * ray[4] > 0 ||
			z[1] * ray[3] - ray[5] * x[0] - ray[1] > 0 ||
			ray[4] * x[0] - ray[3] * y[0] - ray[2] > 0)
			return 0;

		if (q->type == BSP::TYPE::BSP_TYPE_INST)
		{
			Inst* inst = (Inst*)q;
			if (editor && (inst->flags & INST_VOLATILE) || 
				!editor && !(inst->flags & INST_VOLATILE))
				return 0;

			if (inst->inst_type == Inst::INST_TYPE::MESH)
			{
				if (((MeshInst*)inst)->HitFace(ray, ret, nrm, positive_only, editor, solid_only))
					return inst;
				else
					return 0;
			}
			else
			if (inst->inst_type == Inst::INST_TYPE::SPRITE)
			{
				if (sprites_too && ((SpriteInst*)inst)->Hit(ray, ret, positive_only))
					return inst;
				else
					return 0;
			}
			else
			if (inst->inst_type == Inst::INST_TYPE::ITEM)
			{
				if (sprites_too && ((ItemInst*)inst)->Hit(ray, ret, positive_only))
					return inst;
				else
					return 0;
			}
			return 0;
		}
        else
        if (q->type == BSP::TYPE::BSP_TYPE_NODE)
        {
            BSP_Node* n = (BSP_Node*)q;
            Inst* i = HitWorld2(n->bsp_child[0], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            Inst* j = HitWorld2(n->bsp_child[1], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            i = j ? j : i;
            return i;
        }
        else
        if (q->type == BSP::TYPE::BSP_TYPE_NODE_SHARE)
        {
            BSP_NodeShare* s = (BSP_NodeShare*)q;

            Inst* i = HitWorld2(s->bsp_child[0], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            Inst* j = HitWorld2(s->bsp_child[1], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            i = j ? j : i;

            j = s->head;
            while (j)
            {
				if (editor && (j->flags & INST_VOLATILE) || 
					!editor && !(j->flags & INST_VOLATILE))
				{
					j=j->next;
					continue;
				}

				if (j->inst_type == Inst::INST_TYPE::MESH)
				{
					if (((MeshInst*)j)->HitFace(ray, ret, nrm, positive_only, editor, solid_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::SPRITE && sprites_too)
				{
					if (((SpriteInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::ITEM && sprites_too)
				{
					if (((ItemInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}

				j = j->next;
			}
            return i;
        }
        else
        if (q->type == BSP::TYPE::BSP_TYPE_LEAF)
        {
            BSP_Leaf* l = (BSP_Leaf*)q;

            Inst* i = 0;
            Inst* j = l->head;
            while (j)
            {
				if (editor && (j->flags & INST_VOLATILE) || 
					!editor && !(j->flags & INST_VOLATILE))
				{
					j=j->next;
					continue;
				}

				if (j->inst_type == Inst::INST_TYPE::MESH)
				{
					if (((MeshInst*)j)->HitFace(ray, ret, nrm, positive_only, editor, solid_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::SPRITE && sprites_too)
				{
					if (((SpriteInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::ITEM && sprites_too)
				{
					if (((ItemInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}

				j = j->next;
            }
            return i;
        }

		return 0;
	}

	static Inst* HitWorld3(BSP* q, double ray[10], double ret[3], double nrm[3], bool positive_only, bool editor, bool solid_only, bool sprites_too)
	{
		if (!q)
			return 0;

		const float x[2] = { q->bbox[0],q->bbox[1] };
		const float y[2] = { q->bbox[2],q->bbox[3] };
		const float z[2] = { q->bbox[4],q->bbox[5] };

		if (positive_only)
		{
			// do not recurse if all 8 corners projected onto ray are negative
		}

		if (z[0] * ray[3] - ray[5] * x[0] - ray[1] > 0 ||
			ray[0] + z[0] * ray[4] - ray[5] * y[0] > 0 ||
			ray[2] - ray[4] * x[1] + ray[3] * y[0] > 0 ||
			ray[1] - z[1] * ray[3] + ray[5] * x[1] > 0 ||
			ray[5] * y[1] - ray[0] - z[1] * ray[4] > 0 ||
			ray[4] * x[0] - ray[3] * y[1] - ray[2] > 0)
			return 0;

		if (q->type == BSP::TYPE::BSP_TYPE_INST)
		{
			Inst* inst = (Inst*)q;
			if (editor && (inst->flags & INST_VOLATILE) || 
				!editor && !(inst->flags & INST_VOLATILE))
				return 0;

			if (inst->inst_type == Inst::INST_TYPE::MESH)
			{
				if (((MeshInst*)inst)->HitFace(ray, ret, nrm, positive_only, editor, solid_only))
					return inst;
				else
					return 0;
			}
			else
			if (inst->inst_type == Inst::INST_TYPE::SPRITE)
			{
				if (sprites_too && ((SpriteInst*)inst)->Hit(ray, ret, positive_only))
					return inst;
				else
					return 0;
			}
			else
			if (inst->inst_type == Inst::INST_TYPE::ITEM)
			{
				if (sprites_too && ((ItemInst*)inst)->Hit(ray, ret, positive_only))
					return inst;
				else
					return 0;
			}

			return 0;
		}
        else
        if (q->type == BSP::TYPE::BSP_TYPE_NODE)
        {
            BSP_Node* n = (BSP_Node*)q;
            Inst* i = HitWorld3(n->bsp_child[0], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            Inst* j = HitWorld3(n->bsp_child[1], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            i = j ? j : i;
            return i;
        }
        else
        if (q->type == BSP::TYPE::BSP_TYPE_NODE_SHARE)
        {
            BSP_NodeShare* s = (BSP_NodeShare*)q;

            Inst* i = HitWorld3(s->bsp_child[0], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            Inst* j = HitWorld3(s->bsp_child[1], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            i = j ? j : i;

            j = s->head;
            while (j)
            {
				if (editor && (j->flags & INST_VOLATILE) || 
					!editor && !(j->flags & INST_VOLATILE))
				{
					j=j->next;
					continue;
				}

				if (j->inst_type == Inst::INST_TYPE::MESH)
				{
					if (((MeshInst*)j)->HitFace(ray, ret, nrm, positive_only, editor, solid_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::SPRITE && sprites_too)
				{
					if (((SpriteInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::ITEM && sprites_too)
				{
					if (((ItemInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}

				j = j->next;
            }
            return i;
        }
        else
        if (q->type == BSP::TYPE::BSP_TYPE_LEAF)
        {
            BSP_Leaf* l = (BSP_Leaf*)q;

            Inst* i = 0;
            Inst* j = l->head;
            while (j)
            {
				if (editor && (j->flags & INST_VOLATILE) || 
					!editor && !(j->flags & INST_VOLATILE))
				{
					j=j->next;
					continue;
				}

				if (j->inst_type == Inst::INST_TYPE::MESH)
				{
					if (((MeshInst*)j)->HitFace(ray, ret, nrm, positive_only, editor, solid_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::SPRITE && sprites_too)
				{
					if (((SpriteInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::ITEM && sprites_too)
				{
					if (((ItemInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}

				j = j->next;
            }
            return i;
        }

		return 0;
	}

	static Inst* HitWorld4(BSP* q, double ray[10], double ret[3], double nrm[3], bool positive_only, bool editor, bool solid_only, bool sprites_too)
	{
		if (!q)
			return 0;

		const float x[2] = { q->bbox[0],q->bbox[1] };
		const float y[2] = { q->bbox[2],q->bbox[3] };
		const float z[2] = { q->bbox[4],q->bbox[5] };

		if (positive_only)
		{
			// do not recurse if all 8 corners projected onto ray are negative
		}

		if (ray[0] + z[1] * ray[4] - ray[5] * y[1] > 0 ||
			-ray[1] + z[1] * ray[3] - ray[5] * x[1] > 0 ||
			-ray[2] + ray[4] * x[1] - ray[3] * y[0] > 0 ||
			-ray[0] - z[0] * ray[4] + ray[5] * y[0] > 0 ||
			ray[1] - z[0] * ray[3] + ray[5] * x[0] > 0 ||
			ray[2] - ray[4] * x[0] + ray[3] * y[1] > 0)
			return 0;

		if (q->type == BSP::TYPE::BSP_TYPE_INST)
		{
			Inst* inst = (Inst*)q;
			if (editor && (inst->flags & INST_VOLATILE) || 
				!editor && !(inst->flags & INST_VOLATILE))
				return 0;

			if (inst->inst_type == Inst::INST_TYPE::MESH)
			{
				if (((MeshInst*)inst)->HitFace(ray, ret, nrm, positive_only, editor, solid_only))
					return inst;
				else
					return 0;
			}
			else
			if (inst->inst_type == Inst::INST_TYPE::SPRITE)
			{
				if (sprites_too && ((SpriteInst*)inst)->Hit(ray, ret, positive_only))
					return inst;
				else
					return 0;
			}
			else
			if (inst->inst_type == Inst::INST_TYPE::ITEM)
			{
				if (sprites_too && ((ItemInst*)inst)->Hit(ray, ret, positive_only))
					return inst;
				else
					return 0;
			}

			return 0;
		}
        else
        if (q->type == BSP::TYPE::BSP_TYPE_NODE)
        {
            BSP_Node* n = (BSP_Node*)q;
            Inst* i = HitWorld4(n->bsp_child[0], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            Inst* j = HitWorld4(n->bsp_child[1], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            i = j ? j : i;
            return i;
        }
        else
        if (q->type == BSP::TYPE::BSP_TYPE_NODE_SHARE)
        {
            BSP_NodeShare* s = (BSP_NodeShare*)q;

            Inst* i = HitWorld4(s->bsp_child[0], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            Inst* j = HitWorld4(s->bsp_child[1], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            i = j ? j : i;

            j = s->head;
            while (j)
            {
				if (editor && (j->flags & INST_VOLATILE) || 
					!editor && !(j->flags & INST_VOLATILE))
				{
					j=j->next;
					continue;
				}

				if (j->inst_type == Inst::INST_TYPE::MESH)
				{
					if (((MeshInst*)j)->HitFace(ray, ret, nrm, positive_only, editor, solid_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::SPRITE && sprites_too)
				{
					if (((SpriteInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::ITEM && sprites_too)
				{
					if (((ItemInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}

				j = j->next;
            }
            return i;
        }
        else
        if (q->type == BSP::TYPE::BSP_TYPE_LEAF)
        {
            BSP_Leaf* l = (BSP_Leaf*)q;

            Inst* i = 0;
            Inst* j = l->head;
            while (j)
            {
				if (editor && (j->flags & INST_VOLATILE) || 
					!editor && !(j->flags & INST_VOLATILE))
				{
					j=j->next;
					continue;
				}

				if (j->inst_type == Inst::INST_TYPE::MESH)
				{
					if (((MeshInst*)j)->HitFace(ray, ret, nrm, positive_only, editor, solid_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::SPRITE && sprites_too)
				{
					if (((SpriteInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::ITEM && sprites_too)
				{
					if (((ItemInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}

				j = j->next;
            }
            return i;
        }

		return 0;
	}

	static Inst* HitWorld5(BSP* q, double ray[10], double ret[3], double nrm[3], bool positive_only, bool editor, bool solid_only, bool sprites_too)
	{
		if (!q)
			return 0;

		const float x[2] = { q->bbox[0],q->bbox[1] };
		const float y[2] = { q->bbox[2],q->bbox[3] };
		const float z[2] = { q->bbox[4],q->bbox[5] };

		if (positive_only)
		{
			// do not recurse if all 8 corners projected onto ray are negative
		}

		if (ray[1] - z[1] * ray[3] + ray[5] * x[0] > 0 ||
			ray[0] + z[1] * ray[4] - ray[5] * y[1] > 0 ||
			-ray[2] + ray[4] * x[1] - ray[3] * y[1] > 0 ||
			-ray[1] + z[0] * ray[3] - ray[5] * x[1] > 0 ||
			-ray[0] - z[0] * ray[4] + ray[5] * y[0] > 0 ||
			ray[2] - ray[4] * x[0] + ray[3] * y[0] > 0)
			return 0;

		if (q->type == BSP::TYPE::BSP_TYPE_INST)
		{
			Inst* inst = (Inst*)q;
			if (editor && (inst->flags & INST_VOLATILE) || 
				!editor && !(inst->flags & INST_VOLATILE))
				return 0;

			if (inst->inst_type == Inst::INST_TYPE::MESH)
			{
				if (((MeshInst*)inst)->HitFace(ray, ret, nrm, positive_only, editor, solid_only))
					return inst;
				else
					return 0;
			}
			else
			if (inst->inst_type == Inst::INST_TYPE::SPRITE)
			{
				if (sprites_too && ((SpriteInst*)inst)->Hit(ray, ret, positive_only))
					return inst;
				else
					return 0;
			}
			else
			if (inst->inst_type == Inst::INST_TYPE::ITEM)
			{
				if (sprites_too && ((ItemInst*)inst)->Hit(ray, ret, positive_only))
					return inst;
				else
					return 0;
			}

			return 0;
		}
        else
        if (q->type == BSP::TYPE::BSP_TYPE_NODE)
        {
            BSP_Node* n = (BSP_Node*)q;
            Inst* i = HitWorld5(n->bsp_child[0], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            Inst* j = HitWorld5(n->bsp_child[1], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            i = j ? j : i;
            return i;
        }
        else
        if (q->type == BSP::TYPE::BSP_TYPE_NODE_SHARE)
        {
            BSP_NodeShare* s = (BSP_NodeShare*)q;

            Inst* i = HitWorld5(s->bsp_child[0], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            Inst* j = HitWorld5(s->bsp_child[1], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            i = j ? j : i;

            j = s->head;
            while (j)
            {
				if (editor && (j->flags & INST_VOLATILE) || 
					!editor && !(j->flags & INST_VOLATILE))
				{
					j=j->next;
					continue;
				}

				if (j->inst_type == Inst::INST_TYPE::MESH)
				{
					if (((MeshInst*)j)->HitFace(ray, ret, nrm, positive_only, editor, solid_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::SPRITE && sprites_too)
				{
					if (((SpriteInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::ITEM && sprites_too)
				{
					if (((ItemInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}

				j = j->next;
            }
            return i;
        }
        else
        if (q->type == BSP::TYPE::BSP_TYPE_LEAF)
        {
            BSP_Leaf* l = (BSP_Leaf*)q;

            Inst* i = 0;
            Inst* j = l->head;
            while (j)
            {
				if (editor && (j->flags & INST_VOLATILE) || 
					!editor && !(j->flags & INST_VOLATILE))
				{
					j=j->next;
					continue;
				}

				if (j->inst_type == Inst::INST_TYPE::MESH)
				{
					if (((MeshInst*)j)->HitFace(ray, ret, nrm, positive_only, editor, solid_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::SPRITE && sprites_too)
				{
					if (((SpriteInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::ITEM && sprites_too)
				{
					if (((ItemInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}

				j = j->next;
            }
            return i;
        }

		return 0;
	}

	static Inst* HitWorld6(BSP* q, double ray[10], double ret[3], double nrm[3], bool positive_only, bool editor, bool solid_only, bool sprites_too)
	{
		if (!q)
			return 0;

		const float x[2] = { q->bbox[0],q->bbox[1] };
		const float y[2] = { q->bbox[2],q->bbox[3] };
		const float z[2] = { q->bbox[4],q->bbox[5] };

		if (positive_only)
		{
			// do not recurse if all 8 corners projected onto ray are negative
		}

		if (-ray[1] + z[1] * ray[3] - ray[5] * x[1] > 0 ||
			-ray[0] - z[1] * ray[4] + ray[5] * y[0] > 0 ||
			-ray[2] + ray[4] * x[0] - ray[3] * y[0] > 0 ||
			ray[1] - z[0] * ray[3] + ray[5] * x[0] > 0 ||
			ray[0] + z[0] * ray[4] - ray[5] * y[1] > 0 ||
			ray[2] - ray[4] * x[1] + ray[3] * y[1] > 0)
			return 0;

		if (q->type == BSP::TYPE::BSP_TYPE_INST)
		{
			Inst* inst = (Inst*)q;
			if (editor && (inst->flags & INST_VOLATILE) || 
				!editor && !(inst->flags & INST_VOLATILE))
				return 0;

			if (inst->inst_type == Inst::INST_TYPE::MESH)
			{
				if (((MeshInst*)inst)->HitFace(ray, ret, nrm, positive_only, editor, solid_only))
					return inst;
				else
					return 0;
			}
			else
			if (inst->inst_type == Inst::INST_TYPE::SPRITE)
			{
				if (sprites_too && ((SpriteInst*)inst)->Hit(ray, ret, positive_only))
					return inst;
				else
					return 0;
			}
			else
			if (inst->inst_type == Inst::INST_TYPE::ITEM)
			{
				if (sprites_too && ((ItemInst*)inst)->Hit(ray, ret, positive_only))
					return inst;
				else
					return 0;
			}

			return 0;
		}
        else
        if (q->type == BSP::TYPE::BSP_TYPE_NODE)
        {
            BSP_Node* n = (BSP_Node*)q;
            Inst* i = HitWorld6(n->bsp_child[0], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            Inst* j = HitWorld6(n->bsp_child[1], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            i = j ? j : i;
            return i;
        }
        else
        if (q->type == BSP::TYPE::BSP_TYPE_NODE_SHARE)
        {
            BSP_NodeShare* s = (BSP_NodeShare*)q;

            Inst* i = HitWorld6(s->bsp_child[0], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            Inst* j = HitWorld6(s->bsp_child[1], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            i = j ? j : i;

            j = s->head;
            while (j)
            {
				if (editor && (j->flags & INST_VOLATILE) || 
					!editor && !(j->flags & INST_VOLATILE))
				{
					j=j->next;
					continue;
				}

				if (j->inst_type == Inst::INST_TYPE::MESH)
				{
					if (((MeshInst*)j)->HitFace(ray, ret, nrm, positive_only, editor, solid_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::SPRITE && sprites_too)
				{
					if (((SpriteInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::ITEM && sprites_too)
				{
					if (((ItemInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}

				j = j->next;
            }
            return i;
        }
        else
        if (q->type == BSP::TYPE::BSP_TYPE_LEAF)
        {
            BSP_Leaf* l = (BSP_Leaf*)q;

            Inst* i = 0;
            Inst* j = l->head;
            while (j)
            {
				if (editor && (j->flags & INST_VOLATILE) || 
					!editor && !(j->flags & INST_VOLATILE))
				{
					j=j->next;
					continue;
				}

				if (j->inst_type == Inst::INST_TYPE::MESH)
				{
					if (((MeshInst*)j)->HitFace(ray, ret, nrm, positive_only, editor, solid_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::SPRITE && sprites_too)
				{
					if (((SpriteInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::ITEM && sprites_too)
				{
					if (((ItemInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}

				j = j->next;
            }
            return i;
        }

		return 0;
	}

	static Inst* HitWorld7(BSP* q, double ray[10], double ret[3], double nrm[3], bool positive_only, bool editor, bool solid_only, bool sprites_too)
	{
		if (!q)
			return 0;

		const float x[2] = { q->bbox[0],q->bbox[1] };
		const float y[2] = { q->bbox[2],q->bbox[3] };
		const float z[2] = { q->bbox[4],q->bbox[5] };

		if (positive_only)
		{
			// do not recurse if all 8 corners projected onto ray are negative
		}

		if (-ray[0] - z[1] * ray[4] + ray[5] * y[0] > 0 ||
			ray[1] - z[1] * ray[3] + ray[5] * x[0] > 0 ||
			-ray[2] + ray[4] * x[0] - ray[3] * y[1] > 0 ||
			ray[0] + z[0] * ray[4] - ray[5] * y[1] > 0 ||
			-ray[1] + z[0] * ray[3] - ray[5] * x[1] > 0 ||
			ray[2] - ray[4] * x[1] + ray[3] * y[0] > 0)
			return 0;

		if (q->type == BSP::TYPE::BSP_TYPE_INST)
		{
			Inst* inst = (Inst*)q;
			if (editor && (inst->flags & INST_VOLATILE) || 
				!editor && !(inst->flags & INST_VOLATILE))
				return 0;

			if (inst->inst_type == Inst::INST_TYPE::MESH)
			{
				if (((MeshInst*)inst)->HitFace(ray, ret, nrm, positive_only, editor, solid_only))
					return inst;
				else
					return 0;
			}
			else
			if (inst->inst_type == Inst::INST_TYPE::SPRITE)
			{
				if (sprites_too && ((SpriteInst*)inst)->Hit(ray, ret, positive_only))
					return inst;
				else
					return 0;
			}
			else
			if (inst->inst_type == Inst::INST_TYPE::ITEM)
			{
				if (sprites_too && ((ItemInst*)inst)->Hit(ray, ret, positive_only))
					return inst;
				else
					return 0;
			}

			return 0;
		}
        else
        if (q->type == BSP::TYPE::BSP_TYPE_NODE)
        {
            BSP_Node* n = (BSP_Node*)q;
            Inst* i = HitWorld7(n->bsp_child[0], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            Inst* j = HitWorld7(n->bsp_child[1], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            i = j ? j : i;
            return i;
        }
        else
        if (q->type == BSP::TYPE::BSP_TYPE_NODE_SHARE)
        {
            BSP_NodeShare* s = (BSP_NodeShare*)q;

            Inst* i = HitWorld7(s->bsp_child[0], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            Inst* j = HitWorld7(s->bsp_child[1], ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
            i = j ? j : i;

            j = s->head;
            while (j)
            {
				if (editor && (j->flags & INST_VOLATILE) || 
					!editor && !(j->flags & INST_VOLATILE))
				{
					j=j->next;
					continue;
				}

				if (j->inst_type == Inst::INST_TYPE::MESH)
				{
					if (((MeshInst*)j)->HitFace(ray, ret, nrm, positive_only, editor, solid_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::SPRITE && sprites_too)
				{
					if (((SpriteInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::ITEM && sprites_too)
				{
					if (((ItemInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}

				j = j->next;
            }
            return i;
        }
        else
        if (q->type == BSP::TYPE::BSP_TYPE_LEAF)
        {
            BSP_Leaf* l = (BSP_Leaf*)q;

            Inst* i = 0;
            Inst* j = l->head;
            while (j)
            {
				if (editor && (j->flags & INST_VOLATILE) || 
					!editor && !(j->flags & INST_VOLATILE))
				{
					j=j->next;
					continue;
				}

				if (j->inst_type == Inst::INST_TYPE::MESH)
				{
					if (((MeshInst*)j)->HitFace(ray, ret, nrm, positive_only, editor, solid_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::SPRITE && sprites_too)
				{
					if (((SpriteInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}
				else
				if (j->inst_type == Inst::INST_TYPE::ITEM && sprites_too)
				{
					if (((ItemInst*)j)->Hit(ray, ret, positive_only))
						i = j;
				}

				j = j->next;
            }
            return i;
        }

		return 0;
	}


    // RAY HIT using plucker
    Inst* HitWorld(double p[3], double v[3], double ret[3], double nrm[3], bool positive_only, bool editor, bool solid_only, bool sprites_too)
    {
		if (!root)
			return 0;

		/*
		double max_z = 0;
		if (positive_only)
		{
			max_z = p[2];
			p[0] += v[0];
			p[1] += v[1];
			p[2] += v[2];
		}
		*/

		// p should be projected to the BOTTOM plane!
		double ray[] =
		{
			p[1] * v[2] - p[2] * v[1],
			p[2] * v[0] - p[0] * v[2],
			p[0] * v[1] - p[1] * v[0],
			v[0], v[1], v[2],
			p[0], p[1], p[2], // used by triangle-ray intersection
			FLT_MAX
		};

		int sign_case = 0;

		if (v[0] >= 0)
			sign_case |= 1;
		if (v[1] >= 0)
			sign_case |= 2;
		if (v[2] >= 0)
			sign_case |= 4;

		// assert((sign_case & 4) == 0); // watching from the bottom? -> raytraced reflections?

		static Inst* (*const func_vect[])(BSP* q, double ray[10], double ret[3], double nrm[3], bool, bool, bool, bool) =
		{
			HitWorld0,
			HitWorld1,
			HitWorld2,
			HitWorld3,

			HitWorld4,
			HitWorld5,
			HitWorld6,
			HitWorld7,
		};

		/*
		if (!positive_only)
		{
			// otherwie ret must be preinitialized
			ret[0] = p[0];
			ret[1] = p[1];
			ret[2] = p[2];
		}
		*/

		Inst* inst = func_vect[sign_case](root, ray, ret, nrm, positive_only, editor, solid_only, sprites_too);
		return inst;
    }

    static void QueryBSP(int level, BSP* bsp, int planes, double plane[][4], void (*cb)(int level, const float bbox[6], void* cookie), void* cookie)
    {
        // temporarily don't check planes
        cb(level, bsp->bbox,cookie);

        if (bsp->type == BSP::BSP_TYPE_NODE)
        {
            BSP_Node* n = (BSP_Node*)bsp;

            if (n->bsp_child[0])
                QueryBSP(level+1, n->bsp_child[0], planes, plane, cb, cookie);
            if (n->bsp_child[1])
                QueryBSP(level+1, n->bsp_child[1], planes, plane, cb, cookie);
        }
        else
        if (bsp->type == BSP::BSP_TYPE_NODE_SHARE)
        {
            BSP_NodeShare* s = (BSP_NodeShare*)bsp;

            if (s->bsp_child[0])
                QueryBSP(level+1, s->bsp_child[0], planes, plane, cb, cookie);
            if (s->bsp_child[1])
                QueryBSP(level+1, s->bsp_child[1], planes, plane, cb, cookie);

            Inst* i = s->head;
            while (i)
            {
                QueryBSP(level+1, i, planes, plane, cb, cookie);
                i=i->next;
            }
        }
        else
        if (bsp->type == BSP::BSP_TYPE_LEAF)
        {
            BSP_Leaf* l = (BSP_Leaf*)bsp;

            Inst* i = l->head;
            while (i)
            {
                QueryBSP(level+1, i, planes, plane, cb, cookie);
                i=i->next;
            }
        }
        else
        {
            assert(bsp->type == BSP::BSP_TYPE_INST);
        }
        
    }

    // MESHES IN HULL

    // recursive no clipping
    static void Query(BSP* bsp, QueryWorldCB* cb, void* cookie)
    {
        if (bsp->type == BSP::BSP_TYPE_LEAF)
        {
            bsp_nodes++;
            Inst* i = ((BSP_Leaf*)bsp)->head;
            while (i)
            {
                Query(i,cb,cookie);
                i=i->next;
            }
        }
        else
        if (bsp->type == BSP::BSP_TYPE_INST)
        {
            bsp_insts++;
            Inst* i = (Inst*)bsp;
			if (i->inst_type == Inst::INST_TYPE::MESH)
				cb->mesh_cb(((MeshInst*)i)->mesh, ((MeshInst*)i)->tm, cookie);
			else
			if (i->inst_type == Inst::INST_TYPE::SPRITE)
			{
				SpriteInst* si = (SpriteInst*)i;
				if (i->flags & INST_FLAGS::INST_VISIBLE)
					cb->sprite_cb(si, si->sprite, si->pos, si->yaw, si->anim, si->frame, si->reps, cookie);
			}
			else
			if (i->inst_type == Inst::INST_TYPE::ITEM)
			{
				ItemInst* si = (ItemInst*)i;
				cb->sprite_cb(si, si->item->proto->sprite_3d, si->pos, si->yaw, -1, si->item->purpose, (int*)si->item, cookie);
			}
        }
        else
        if (bsp->type == BSP::BSP_TYPE_NODE)
        {
            bsp_nodes++;
            BSP_Node* n = (BSP_Node*)bsp;
            if (n->bsp_child[0])
                Query(n->bsp_child[0],cb,cookie);
            if (n->bsp_child[1])
                Query(n->bsp_child[1],cb,cookie);
        }
        else
        if (bsp->type == BSP::BSP_TYPE_NODE_SHARE)
        {
            bsp_nodes++;
            BSP_NodeShare* s = (BSP_NodeShare*)bsp;
            if (s->bsp_child[0])
                Query(s->bsp_child[0],cb,cookie);
            if (s->bsp_child[1])
                Query(s->bsp_child[1],cb,cookie);
            Inst* i = s->head;
            while (i)
            {
                Query(i,cb,cookie);
                i=i->next;
            }                
        }
        else
        {
            assert(0);
        }
        
    }

    // recursive
    static void Query(BSP* bsp, int planes, double* plane[], QueryWorldCB* cb, void* cookie)
    {
        float c[4] = { bsp->bbox[0], bsp->bbox[2], bsp->bbox[4], 1 }; // 0,0,0

        bsp_tests++;

        for (int i = 0; i < planes; i++)
        {
            int neg_pos[2] = { 0,0 };

            neg_pos[PositiveProduct(plane[i], c)] ++;

            c[0] = bsp->bbox[1]; // 1,0,0
            neg_pos[PositiveProduct(plane[i], c)] ++;

            c[1] = bsp->bbox[3]; // 1,1,0
            neg_pos[PositiveProduct(plane[i], c)] ++;

            c[0] = bsp->bbox[0]; // 0,1,0
            neg_pos[PositiveProduct(plane[i], c)] ++;

            c[2] = bsp->bbox[5]; // 0,1,1
            neg_pos[PositiveProduct(plane[i], c)] ++;

            c[0] = bsp->bbox[1]; // 1,1,1
            neg_pos[PositiveProduct(plane[i], c)] ++;

            c[1] = bsp->bbox[2]; // 1,0,1
            neg_pos[PositiveProduct(plane[i], c)] ++;

            c[0] = bsp->bbox[0]; // 0,0,1
            neg_pos[PositiveProduct(plane[i], c)] ++;

            c[2] = bsp->bbox[4]; // 0,0,0

            if (neg_pos[0] == 8)
                return;

            if (neg_pos[1] == 8)
            {
                planes--;
                if (i < planes)
                {
                    double* swap = plane[i];
                    plane[i] = plane[planes];
                    plane[planes] = swap;
                }
                i--;
            }
        }

        if (bsp->type == BSP::BSP_TYPE_INST)
        {
            bsp_insts++;
            Inst* i = (Inst*)bsp;
			if (i->inst_type == Inst::INST_TYPE::MESH)
				cb->mesh_cb(((MeshInst*)i)->mesh, ((MeshInst*)i)->tm, cookie);
			else
			if (i->inst_type == Inst::INST_TYPE::SPRITE)
			{
				SpriteInst* si = (SpriteInst*)i;
				if (i->flags & INST_FLAGS::INST_VISIBLE)
					cb->sprite_cb(si,si->sprite, si->pos, si->yaw, si->anim, si->frame, si->reps, cookie);
			}
			else
			if (i->inst_type == Inst::INST_TYPE::ITEM)
			{
				ItemInst* si = (ItemInst*)i;
				cb->sprite_cb(si, si->item->proto->sprite_3d, si->pos, si->yaw, -1, si->item->purpose, (int*)si->item, cookie);
			}
		}
        else
        if (bsp->type == BSP::BSP_TYPE_NODE)        
        {
            bsp_nodes++;
            BSP_Node* n = (BSP_Node*)bsp;
            if (planes)
            {
                if (n->bsp_child[0])
                    Query(n->bsp_child[0],planes,plane,cb,cookie);
                if (n->bsp_child[1])
                    Query(n->bsp_child[1],planes,plane,cb,cookie);
            }
            else
            {
                if (n->bsp_child[0])
                    Query(n->bsp_child[0],cb,cookie);
                if (n->bsp_child[1])
                    Query(n->bsp_child[1],cb,cookie);
            }
        }
        else
        if (bsp->type == BSP::BSP_TYPE_NODE_SHARE)
        {
            bsp_nodes++;
            BSP_NodeShare* s = (BSP_NodeShare*)bsp;
            if (planes)
            {
                if (s->bsp_child[0])
                    Query(s->bsp_child[0],planes,plane,cb,cookie);
                if (s->bsp_child[1])
                    Query(s->bsp_child[1],planes,plane,cb,cookie);

                Inst* i = s->head;
                while (i)
                {
                    Query(i,planes,plane,cb,cookie);
                    i=i->next;
                }                
            }
            else
            {
                if (s->bsp_child[0])
                    Query(s->bsp_child[0],cb,cookie);
                if (s->bsp_child[1])
                    Query(s->bsp_child[1],cb,cookie);

                Inst* i = s->head;
                while (i)
                {
                    Query(i,cb,cookie);
                    i=i->next;
                }                
            }
        }        
        else
        if (bsp->type == BSP::BSP_TYPE_LEAF)
        {
            bsp_nodes++;
            BSP_Leaf* l = (BSP_Leaf*)bsp;
            if (planes)
            {
                Inst* i = l->head;
                while (i)
                {
                    Query(i,planes,plane,cb,cookie);
                    i=i->next;
                }                
            }
            else
            {
                Inst* i = l->head;
                while (i)
                {
                    Query(i,cb,cookie);
                    i=i->next;
                }                
            }
        }        
        else
        {
            assert(0);
        }
    }

    // main
    void Query(int planes, double plane[][4], QueryWorldCB* cb, void* cookie)
    {
        bsp_tests=0;
        bsp_insts=0;
        bsp_nodes=0;

        // static first
        if (root)
        {
			if (planes > 0)
			{
				//double* pp[4] = { plane[0],plane[1],plane[2],plane[3] };
				double* pp[6] = { plane[0],plane[1],plane[2],plane[3],plane[4],plane[5] };

				Query(root, planes, pp, cb, cookie);
			}
			else
			{
				Query(root, cb, cookie);
			}
        }

		// dynamic after
		Inst* i = head_inst;
		if (planes > 0)
		{
			// double* pp[4] = { plane[0],plane[1],plane[2],plane[3] };
			double* pp[6] = { plane[0],plane[1],plane[2],plane[3],plane[4],plane[5] };

			while (i)
			{
				Query(i, planes, pp, cb, cookie);
				i = i->next;
			}
		}
		else
		{
			while (i)
			{
				Query(i, cb, cookie);
				i = i->next;
			}
		}
	}
};

Item* delete_item_list = 0;
SpriteInst* delete_sprite_list = 0;

static void DeleteItemInsts(BSP* bsp, bool all) 
{
    if (bsp->type == BSP::BSP_TYPE_LEAF)
    {
        Inst* i = ((BSP_Leaf*)bsp)->head;
        while (i)
        {
			if (i->inst_type == Inst::INST_TYPE::ITEM)
			{
				Item* item = ((ItemInst*)i)->item;
				if (all || item->purpose == Item::WORLD)
				{
					item->proto = (ItemProto*)delete_item_list;
					delete_item_list = item;
				}
			}
            i=i->next;
        }
    }
    else
    if (bsp->type == BSP::BSP_TYPE_INST)
    {
        Inst* i = (Inst*)bsp;
		if (i->inst_type == Inst::INST_TYPE::ITEM)
		{
			Item* item = ((ItemInst*)i)->item;
			if (all || item->purpose == Item::WORLD)
			{
				item->proto = (ItemProto*)delete_item_list;
				delete_item_list = item;
			}
		}
	}
    else
    if (bsp->type == BSP::BSP_TYPE_NODE)
    {
        BSP_Node* n = (BSP_Node*)bsp;
        if (n->bsp_child[0])
			DeleteItemInsts(n->bsp_child[0],all);
        if (n->bsp_child[1])
			DeleteItemInsts(n->bsp_child[1],all);
    }
    else
    if (bsp->type == BSP::BSP_TYPE_NODE_SHARE)
    {
        BSP_NodeShare* s = (BSP_NodeShare*)bsp;
        if (s->bsp_child[0])
			DeleteItemInsts(s->bsp_child[0],all);
        if (s->bsp_child[1])
			DeleteItemInsts(s->bsp_child[1],all);
        Inst* i = s->head;
        while (i)
        {
			if (i->inst_type == Inst::INST_TYPE::ITEM)
			{
				Item* item = ((ItemInst*)i)->item;
				if (all || item->purpose == Item::WORLD)
				{
					item->proto = (ItemProto*)delete_item_list;
					delete_item_list = item;
				}
			}
			i=i->next;
        }                
    }
    else
    {
        assert(0);
    }
}

static void DeleteSpriteInsts(BSP* bsp) 
{
    if (bsp->type == BSP::BSP_TYPE_LEAF)
    {
        Inst* i = ((BSP_Leaf*)bsp)->head;
        while (i)
        {
			if (i->inst_type == Inst::INST_TYPE::SPRITE)
			{
				SpriteInst* si = (SpriteInst*)i;
				si->sprite = (Sprite*)delete_sprite_list;
				delete_sprite_list = si;
			}
            i=i->next;
        }
    }
    else
    if (bsp->type == BSP::BSP_TYPE_INST)
    {
        Inst* i = (Inst*)bsp;
		if (i->inst_type == Inst::INST_TYPE::SPRITE)
		{
			SpriteInst* si = (SpriteInst*)i;
			si->sprite = (Sprite*)delete_sprite_list;
			delete_sprite_list = si;
		}
	}
    else
    if (bsp->type == BSP::BSP_TYPE_NODE)
    {
        BSP_Node* n = (BSP_Node*)bsp;
        if (n->bsp_child[0])
			DeleteSpriteInsts(n->bsp_child[0]);
        if (n->bsp_child[1])
			DeleteSpriteInsts(n->bsp_child[1]);
    }
    else
    if (bsp->type == BSP::BSP_TYPE_NODE_SHARE)
    {
        BSP_NodeShare* s = (BSP_NodeShare*)bsp;
        if (s->bsp_child[0])
			DeleteSpriteInsts(s->bsp_child[0]);
        if (s->bsp_child[1])
			DeleteSpriteInsts(s->bsp_child[1]);
        Inst* i = s->head;
        while (i)
        {
			if (i->inst_type == Inst::INST_TYPE::SPRITE)
			{
				SpriteInst* si = (SpriteInst*)i;
				si->sprite = (Sprite*)delete_sprite_list;
				delete_sprite_list = si;
			}
			i=i->next;
        }                
    }
    else
    {
        assert(0);
    }
}

static void CloneItemInsts(World* w, BSP* bsp)
{
    if (bsp->type == BSP::BSP_TYPE_LEAF)
    {
        Inst* i = ((BSP_Leaf*)bsp)->head;
        while (i)
        {
			if (i->inst_type == Inst::INST_TYPE::ITEM)
			{
				Item* item = ((ItemInst*)i)->item;
				if (item->purpose == Item::EDIT)
				{
					Item* clone = CreateItem();
					memcpy(clone, item, sizeof(Item));
					clone->purpose = Item::WORLD;
					clone->inst = 0;
					clone->inst = CreateInst(w, clone, i->flags, ((ItemInst*)i)->pos, ((ItemInst*)i)->yaw, ((ItemInst*)i)->story_id);
				}
			}
            i=i->next;
        }
    }
    else
    if (bsp->type == BSP::BSP_TYPE_INST)
    {
        Inst* i = (Inst*)bsp;
		if (i->inst_type == Inst::INST_TYPE::ITEM)
		{
			Item* item = ((ItemInst*)i)->item;
			if (item->purpose == Item::EDIT)
			{
				Item* clone = CreateItem();
				memcpy(clone, item, sizeof(Item));
				clone->purpose = Item::WORLD;
				clone->inst = 0;
				clone->inst = CreateInst(w, clone, i->flags, ((ItemInst*)i)->pos, ((ItemInst*)i)->yaw, ((ItemInst*)i)->story_id);
			}
		}
	}
    else
    if (bsp->type == BSP::BSP_TYPE_NODE)
    {
        BSP_Node* n = (BSP_Node*)bsp;
        if (n->bsp_child[0])
			CloneItemInsts(w,n->bsp_child[0]);
        if (n->bsp_child[1])
			CloneItemInsts(w,n->bsp_child[1]);
    }
    else
    if (bsp->type == BSP::BSP_TYPE_NODE_SHARE)
    {
        BSP_NodeShare* s = (BSP_NodeShare*)bsp;
        if (s->bsp_child[0])
			CloneItemInsts(w,s->bsp_child[0]);
        if (s->bsp_child[1])
			CloneItemInsts(w,s->bsp_child[1]);
        Inst* i = s->head;
        while (i)
        {
			if (i->inst_type == Inst::INST_TYPE::ITEM)
			{
				Item* item = ((ItemInst*)i)->item;
				if (item->purpose == Item::EDIT)
				{
					Item* clone = CreateItem();
					memcpy(clone, item, sizeof(Item));
					clone->purpose = Item::WORLD;
					clone->inst = 0;
					clone->inst = CreateInst(w, clone, i->flags, ((ItemInst*)i)->pos, ((ItemInst*)i)->yaw, ((ItemInst*)i)->story_id);
				}
			}
			i=i->next;
        }                
    }
    else
    {
        assert(0);
    }
}

void ResetItemInsts(World* w)
{
	delete_item_list = 0;

	if (w)
	{
		// flat first
		Inst* i = w->head_inst;
		while (i)
		{
			if (i->inst_type == Inst::INST_TYPE::ITEM)
			{
				Item* item = ((ItemInst*)i)->item;
				if (/*all || */item->purpose == Item::WORLD)
				{
					item->proto = (ItemProto*)delete_item_list;
					delete_item_list = item;
				}
			}
			i = i->next;
		}

		if (w->root)
			DeleteItemInsts(w->root, false); // prepares list only
	}

	Item* item = delete_item_list;
	while (item)
	{
		Item* n = (Item*)item->proto;
		DestroyItem(item); // destroys inst too!!!
		item = n;
	}

	if (w->root)
		CloneItemInsts(w,w->root); // clones immediately
	RebuildWorld(w);
}

bool Mesh::Update(const char* path)
{
	if (strstr(path,".akm"))
	{
		int bio = 1;
	}
    FILE* f = fopen(path,"rt");
	if (!f)
		return false;

	int plannar = 0x7 | 0x8;

	char buf[1024];
	char tail_str[1024];

	int num_verts = -1;
	int num_faces = -1;
	int element = 0;

	bool face_props = false;
	int vert_props = 0;

	// file header
	while (fgets(buf, 1024, f))
	{
		int len = (int)strlen(buf);
		while (len && (buf[len - 1] == ' ' || buf[len - 1] == '\r' || buf[len - 1] == '\n' || buf[len - 1] == '\t' || buf[len - 1] == '\v'))
			len--;
		if (!len)
			continue;
		buf[len] = 0;


		if (strcmp(buf, "ply"))
		{
			fclose(f);
			return false;
		}
		else
			break;
	}

	while (fgets(buf, 1024, f))
	{
		int len = (int)strlen(buf);
		while (len && (buf[len - 1] == ' ' || buf[len - 1] == '\r' || buf[len - 1] == '\n' || buf[len - 1] == '\t' || buf[len - 1] == '\v'))
			len--;
		if (!len)
			continue;
		buf[len] = 0;

		if (strcmp(buf, "format ascii 1.0"))
		{
			fclose(f);
			return false;
		}
		else
			break;
	}

	// mesh header
	while (fgets(buf, 1024, f))
	{
		int len = (int)strlen(buf);
		while (len && (buf[len - 1] == ' ' || buf[len - 1] == '\r' || buf[len - 1] == '\n' || buf[len - 1] == '\t' || buf[len - 1] == '\v'))
			len--;
		if (!len)
			continue;
		buf[len] = 0;

		if (strncmp(buf, "comment", 7) == 0 && (buf[7] == 0 || buf[7] == ' ' || buf[7] == '\t' || buf[7] == '\r' || buf[7] == '\n'))
			continue;

		if (strncmp(buf, "element vertex ",15) == 0)
		{
			if (num_verts >= 0)
			{
				fclose(f);
				return false;
			}

			if (sscanf(buf+15, "%d", &num_verts) != 1 || num_verts < 0)
			{
				fclose(f);
				return false;
			}

			element = 'V';

			continue;
		}

		if (strncmp(buf, "element face ",13) == 0)
		{
			if (num_faces >= 0)
			{
				fclose(f);
				return false;
			}

			if (sscanf(buf+13, "%d", &num_faces) != 1 || num_faces < 0)
			{
				fclose(f);
				return false;
			}

			element = 'F';

			continue;
		}

		if (strncmp(buf, "property ", 9) == 0)
		{
			if (element == 'F')
			{
				if (strcmp(buf + 9, "list uchar uint vertex_indices") != 0)
				{
					fclose(f);
					return false;
				}

				face_props = true;
				continue;
			}
			else
			if (element == 'V')
			{
				static const char* match[] = 
				{
					"property float x",
					"property float y",
					"property float z",
					"property uchar red",
					"property uchar green",
					"property uchar blue",
					"property uchar alpha",
					0
				};

				if (!match[vert_props] || strcmp(buf,match[vert_props]) != 0)
				{
					fclose(f);
					return false;
				}

				vert_props++;
				continue;
			}
			else
			{
				fclose(f);
				return false;
			}
		}

		if (strcmp(buf, "end_header") == 0)
		{
			if (num_faces <= 0 || num_verts <= 0 || !face_props || vert_props != 3 && vert_props != 7)
			{
				fclose(f);
				return false;
			}
			break;
		}
		else
		{
			fclose(f);
			return false;
		}
	}

	Vert** index = (Vert**)malloc(sizeof(Vert*)*num_verts);

	// verts
	while (fgets(buf, 1024, f))
	{
		int len = (int)strlen(buf);
		while (len && (buf[len - 1] == ' ' || buf[len - 1] == '\r' || buf[len - 1] == '\n' || buf[len - 1] == '\t' || buf[len - 1] == '\v'))
			len--;
		if (!len)
			continue;
		buf[len] = 0;

		if (strncmp(buf, "comment", 7) == 0 && (buf[7] == 0 || buf[7] == ' ' || buf[7] == '\t' || buf[7] == '\r' || buf[7] == '\n'))
			continue;

		float x=0, y=0, z=0;
		int r=255, g=255, b=255, a=255;
		if (vert_props == 3)
		{
			if (sscanf(buf, "%f %f %f %s", &x, &y, &z, tail_str) != 3)
			{
				free(index);
				fclose(f);
				return false;
			}
		}
		else
		{
			if (sscanf(buf, "%f %f %f %d %d %d %d %s", &x, &y, &z, &r, &g, &b, &a, tail_str) != 7)
			{
				free(index);
				fclose(f);
				return false;
			}
		}

		Vert* v = (Vert*)malloc(sizeof(Vert));
		index[verts] = v;

		v->xyzw[0] = x;
		v->xyzw[1] = y;
		v->xyzw[2] = z;
		v->xyzw[3] = 1;
		v->rgba[0] = r;
		v->rgba[1] = g;
		v->rgba[2] = b;
		v->rgba[3] = a;

		if (verts && plannar)
		{
			if (plannar & 1)
			{
				if (v->xyzw[0] != head_vert->xyzw[0])
					plannar &= ~1;
			}
			if (plannar & 2)
			{
				if (v->xyzw[1] != head_vert->xyzw[1])
					plannar &= ~2;
			}
			if (plannar & 4)
			{
				if (v->xyzw[2] != head_vert->xyzw[2])
					plannar &= ~4;
			}
			if (plannar & 8)
			{
				if (v->rgba[0] != head_vert->rgba[0] ||
					v->rgba[1] != head_vert->rgba[1] ||
					v->rgba[2] != head_vert->rgba[2])
				{
					plannar &= ~8;
				}
			}
		}

		v->mesh = this;
		v->face_list = 0;
		v->line_list = 0;
		v->next = 0;
		v->prev = tail_vert;
		if (tail_vert)
			tail_vert->next = v;
		else
			head_vert = v;
		tail_vert = v;

		if (!verts)
		{
			bbox[0] = v->xyzw[0];
			bbox[1] = v->xyzw[0];
			bbox[2] = v->xyzw[1];
			bbox[3] = v->xyzw[1];
			bbox[4] = v->xyzw[2];
			bbox[5] = v->xyzw[2];
		}
		else
		{
			bbox[0] = fminf(bbox[0], v->xyzw[0]);
			bbox[1] = fmaxf(bbox[1], v->xyzw[0]);
			bbox[2] = fminf(bbox[2], v->xyzw[1]);
			bbox[3] = fmaxf(bbox[3], v->xyzw[1]);
			bbox[4] = fminf(bbox[4], v->xyzw[2]);
			bbox[5] = fmaxf(bbox[5], v->xyzw[2]);
		}

		v->sel = false;

		verts++;

		if (verts == num_verts)
			break;
	}

	// faces
    int polys=0;
	while (fgets(buf, 1024, f))
	{
		int len = (int)strlen(buf);
		while (len && (buf[len] == ' ' || buf[len] == '\r' || buf[len] == '\n' || buf[len] == '\t' || buf[len] == '\v'))
			len--;
		if (!len)
			continue;
		buf[len] = 0;

		if (strncmp(buf, "comment", 7) == 0 && (buf[7] == 0 || buf[7] == ' ' || buf[7] == '\t' || buf[7] == '\r' || buf[7] == '\n'))
			continue;

        int vv[4];
		int n;

		if (sscanf(buf, "%d %d %d %d %d %s", &n, vv+0,  vv+1,  vv+2,  vv+3, tail_str) < 3 || 
			n != 2 && n != 3 && n != 4 && n != -3 && n != -4)
		{
 			free(index);
			fclose(f);
			return false;
		}

		// FREESTYLE
		bool freestyle = false;
		if (n<0)
		{
			freestyle = true;
			n=-n;
		}

		if (n>2)
		{
			for (int v1=0; v1<n; v1++)
			{
				if (vv[v1]<0 || vv[v1]>=num_verts)
				{
					free(index);
					fclose(f);
					return false;            
				}  

				if (v1==n-1)
					break;

				for (int v2=v1+1; v2<n; v2++)
				{
					if (vv[v1]==vv[v2])
					{
						free(index);
						fclose(f);
						return false;            
					}            
				}
			}
		
			////////////////
			for (int t=0; t<n-2; t++)
			{
				Face* f = (Face*)malloc(sizeof(Face));

				int abc[3] = { vv[0],vv[t+1],vv[t+2] };

				f->visual = freestyle ? (1<<30) : 0; // highest bit is line, next is freestyle
				f->mesh = this;
				f->freestyle = freestyle;

				f->next = 0;
				f->prev = tail_face;
				if (tail_face)
					tail_face->next = f;
				else
					head_face = f;
				tail_face = f;

				for (int i = 0; i < 3; i++)
				{
					f->abc[i] = index[abc[i]];
					f->share_next[i] = f->abc[i]->face_list;
					f->abc[i]->face_list = f;
				}

				faces++;
			}
		}
		else
		if (n==2)
		{
			if (vv[0]<0 || vv[0]>=num_verts || 
				vv[1]<0 || vv[1]>=num_verts ||
				vv[0] == vv[1])
			{
				free(index);
				fclose(f);
				return false;            
			}  

			Line* l = (Line*)malloc(sizeof(Line));
			l->ab[0] = index[vv[0]];
			l->ab[1] = index[vv[1]];

			l->visual = freestyle ? (3<<30) : (2<<30); // highest bit is line, next is freestyle
			l->mesh = this;

			l->next = 0;
			l->prev = tail_line;
			if (tail_line)
				tail_line->next = l;
			else
				head_line = l;
			tail_line = l;

			lines++;
		}
		
        polys++;
        if (polys == num_faces)
			break;
	}

	free(index);

	// tail
	while (fgets(buf, 1024, f))
	{
		int len = (int)strlen(buf);
		while (len && (buf[len] == ' ' || buf[len] == '\r' || buf[len] == '\n' || buf[len] == '\t' || buf[len] == '\v'))
			len--;
		if (!len)
			continue;
		buf[len] = 0;

		if (strncmp(buf, "comment", 7) == 0 && (buf[7] == 0 || buf[7] == ' ' || buf[7] == '\t' || buf[7] == '\r' || buf[7] == '\n'))
			continue;

		fclose(f);
		return false;
	}

#if 0
	// OLD .obj parser
	
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

		float xyzw[4];
		float rgb[4];

		// expect 6-7 floats 
		// xyz rgb
		// xyzw rgb
		int coords = sscanf(p, "%f %f %f %f %f %f %f", xyzw + 0, xyzw + 1, xyzw + 2, xyzw + 3, rgb+0, rgb+1, rgb+2);
		if (coords < 2)
			continue;

		if (coords < 3)
		{
			xyzw[2] = 0.0f;
			xyzw[3] = 1.0f;
			rgb[0] = rgb[1] = rgb[2] = 1.0f;
		}
		else
		if (coords < 4)
		{
			xyzw[3] = 1.0f;
			rgb[0] = rgb[1] = rgb[2] = 1.0f;
		}
		else
		if (coords < 6)
			rgb[0] = rgb[1] = rgb[2] = 1.0f;
		else
		if (coords < 7)
		{
			rgb[2] = rgb[1];
			rgb[1] = rgb[0];
			rgb[0] = xyzw[3];
			xyzw[3] = 1.0f;
		}

        Vert* v = (Vert*)malloc(sizeof(Vert));
        v->xyzw[0] = xyzw[0];
        v->xyzw[1] = xyzw[1];
        v->xyzw[2] = xyzw[2];
        v->xyzw[3] = xyzw[3];
		v->rgb[0] = rgb[0];
		v->rgb[1] = rgb[1];
		v->rgb[2] = rgb[2];

        if (m->verts && plannar)
        {
            if (plannar&1)
            {
                if (v->xyzw[0] != m->head_vert->xyzw[0])
                    plannar &= ~1;
            }
            if (plannar&2)
            {
                if (v->xyzw[1] != m->head_vert->xyzw[1])
                    plannar &= ~2;
            }
            if (plannar&4)
            {
                if (v->xyzw[2] != m->head_vert->xyzw[2])
                    plannar &= ~4;
            }
			if (plannar & 8)
			{
				if (v->rgb[0] != m->head_vert->rgb[0] ||
					v->rgb[1] != m->head_vert->rgb[1] ||
					v->rgb[2] != m->head_vert->rgb[2])
				{
					plannar &= ~8;
				}
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
            m->bbox[0] = fminf( m->bbox[0] , v->xyzw[0] );
            m->bbox[1] = fmaxf( m->bbox[1] , v->xyzw[0] );
            m->bbox[2] = fminf( m->bbox[2] , v->xyzw[1] );
            m->bbox[3] = fmaxf( m->bbox[3] , v->xyzw[1] );
            m->bbox[4] = fminf( m->bbox[4] , v->xyzw[2] );
            m->bbox[5] = fmaxf( m->bbox[5] , v->xyzw[2] );
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
	#endif // OLD .obj parser

    fclose(f);
    return true;
}

Mesh* World::LoadMesh(const char* path, const char* name)
{
    Mesh* m = AddMesh(name ? name : path);

    if (!m->Update(path))
    {
        DeleteMesh(m);
        return 0;
    }

    return m;
}

World* CreateWorld()
{
    World* w = (World*)malloc(sizeof(World));

    w->meshes = 0;
    w->head_mesh = 0;
    w->tail_mesh = 0;
    w->insts = 0;
	w->temp_insts = 0;
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

    // killing bsp brings all instances to world list


	delete_item_list = 0;
	delete_sprite_list = 0;

	// flat first
	Inst* i = w->head_inst;
	while (i)
	{
		if (i->inst_type == Inst::INST_TYPE::ITEM)
		{
			Item* item = ((ItemInst*)i)->item;
			if (/*all || */item->purpose == Item::WORLD)
			{
				item->proto = (ItemProto*)delete_item_list;
				delete_item_list = item;
			}
		}
		else
		if (i->inst_type == Inst::INST_TYPE::SPRITE)
		{
			SpriteInst* si = (SpriteInst*)i;
			si->sprite = (Sprite*)delete_sprite_list;
			delete_sprite_list = si;
		}

		i = i->next;
	}

	if (w->root)
	{
		DeleteItemInsts(w->root, true); // prepares list only
	}

	Item* item = delete_item_list;
	while (item)
	{
		Item* n = (Item*)item->proto;
		DestroyItem(item); // destroys inst too!!!
		item = n;
	}

	////////////////////////////////////////////////
	// sprite insts

	if (w->root)
		DeleteSpriteInsts(w->root);

	SpriteInst* si = delete_sprite_list;
	while (si)
	{
		SpriteInst* n = (SpriteInst*)si->sprite;
		DeleteInst(si);
		si = n;
	}

	if (w->root)
		w->DeleteBSP(w->root);

	w->root = 0;

    // killing all meshes should kill all insts as well
    while (w->meshes)
        w->DelMesh(w->head_mesh);

	free(w);
}

Mesh* LoadMesh(World* w, const char* path, const char* name)
{
    if (!w)
        return 0;
    return w->LoadMesh(path, name);
}

bool UpdateMesh(Mesh* m, const char* path)
{
    return m->Update(path);
}

void DeleteMesh(Mesh* m)
{
    if (!m)
        return;
    m->world->DelMesh(m);
}

Inst* CreateInst(Mesh* m, int flags, const double tm[16], const char* name, int story_id)
{
    if (!m)
        return 0;
    return m->world->AddInst(m,flags,tm,name,story_id);
}

Inst* CreateInst(World* w, Sprite* s, int flags, float pos[3], float yaw, int anim, int frame, int reps[4], const char* name, int story_id)
{
	if (!s)
		return 0;
	return w->AddInst(s, flags, pos, yaw, anim, frame, reps, name, story_id);
}

Inst* CreateInst(World* w, Item* item, int flags, float pos[3], float yaw, int story_id)
{
	if (!item)
		return 0;
	return w->AddInst(item, flags, pos, yaw, story_id);
}

void DeleteInst(Inst* i)
{
    if (!i)
        return;
	if (i->inst_type == Inst::INST_TYPE::MESH)
		((MeshInst*)i)->mesh->world->DelInst(i);
	else
	if (i->inst_type == Inst::INST_TYPE::SPRITE)
		((SpriteInst*)i)->w->DelInst(i);
	else
	if (i->inst_type == Inst::INST_TYPE::ITEM)
		((ItemInst*)i)->w->DelInst(i);
}

void QueryWorld(World* w, int planes, double plane[][4], QueryWorldCB* cb, void* cookie)
{
    if (!w)
        return;
    w->Query(planes,plane,cb,cookie);
}

void QueryWorldBSP(World* w, int planes, double plane[][4], void (*cb)(int level, const float bbox[6], void* cookie), void* cookie)
{
    if (!w || !w->root)
        return;
    World::QueryBSP(1, w->root,planes,plane,cb,cookie);
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

World* GetMeshWorld(Mesh* m)
{
	return m->world;
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

void QueryMesh(Mesh* m, void (*cb)(float coords[9], uint8_t colors[12], uint32_t visual, void* cookie), void* cookie)
{
    if (!m || !cb)
        return;

    float coords[9];
	uint8_t colors[12];

    Face* f = m->head_face;
    while (f)
    {
        coords[0] = f->abc[0]->xyzw[0];
        coords[1] = f->abc[0]->xyzw[1];
        coords[2] = f->abc[0]->xyzw[2];
		colors[0] = f->abc[0]->rgba[0];
		colors[1] = f->abc[0]->rgba[1];
		colors[2] = f->abc[0]->rgba[2];
		colors[3] = f->abc[0]->rgba[3];

        coords[3] = f->abc[1]->xyzw[0];
        coords[4] = f->abc[1]->xyzw[1];
        coords[5] = f->abc[1]->xyzw[2];
		colors[4] = f->abc[1]->rgba[0];
		colors[5] = f->abc[1]->rgba[1];
		colors[6] = f->abc[1]->rgba[2];
		colors[7] = f->abc[1]->rgba[3];

        coords[6] = f->abc[2]->xyzw[0];
        coords[7] = f->abc[2]->xyzw[1];
        coords[8] = f->abc[2]->xyzw[2];
		colors[8] = f->abc[2]->rgba[0];
		colors[9] = f->abc[2]->rgba[1];
		colors[10] = f->abc[2]->rgba[2];
		colors[11] = f->abc[2]->rgba[3];

        cb(coords, colors, f->visual, cookie);

        f=f->next;
    }

    Line* l = m->head_line;
    while (l)
    {	
        coords[0] = l->ab[0]->xyzw[0];
        coords[1] = l->ab[0]->xyzw[1];
        coords[2] = l->ab[0]->xyzw[2];
		colors[0] = l->ab[0]->rgba[0];
		colors[1] = l->ab[0]->rgba[1];
		colors[2] = l->ab[0]->rgba[2];
		colors[3] = l->ab[0]->rgba[3];

        coords[3] = l->ab[1]->xyzw[0];
        coords[4] = l->ab[1]->xyzw[1];
        coords[5] = l->ab[1]->xyzw[2];
		colors[4] = l->ab[1]->rgba[0];
		colors[5] = l->ab[1]->rgba[1];
		colors[6] = l->ab[1]->rgba[2];
		colors[7] = l->ab[1]->rgba[3];		

        cb(coords, colors, l->visual, cookie);

        l=l->next;		
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

void RebuildWorld(World* w, bool boxes)
{
    if (w)
        w->Rebuild(boxes);
}



static void SaveInst(Inst* inst, FILE* f)
{
	if (inst->flags & INST_FLAGS::INST_VOLATILE)
		return;

	if (inst->inst_type == Inst::INST_TYPE::MESH)
	{
		MeshInst* i = (MeshInst*)inst;

		int mesh_id_len = i->mesh && i->mesh->name ? (int)strlen(i->mesh->name) : 0;

		fwrite(&mesh_id_len, 1, 4, f);
		if (mesh_id_len)
			fwrite(i->mesh->name, 1, mesh_id_len, f);

		int inst_name_len = i->name ? (int)strlen(i->name) : 0;

		fwrite(&inst_name_len, 1, 4, f);
		if (inst_name_len)
			fwrite(i->name, 1, inst_name_len, f);

		fwrite(i->tm, 1, 16 * 8, f);
		fwrite(&i->flags, 1, 4, f);
		fwrite(&i->story_id, 1, 4, f);
	}
	else
	if (inst->inst_type == Inst::INST_TYPE::SPRITE)
	{
		SpriteInst* i = (SpriteInst*)inst;

		int mesh_id_len = -1; // identify sprite
		fwrite(&mesh_id_len, 1, 4, f);

		// sprite id ???
		// ...

		int inst_name_len = i->sprite->name ? (int)strlen(i->sprite->name) : 0;

		fwrite(&inst_name_len, 1, 4, f);
		if (inst_name_len)
			fwrite(i->sprite->name, 1, inst_name_len, f);

		fwrite(i->pos, 1, sizeof(float[3]), f);
		fwrite(&i->yaw, 1, sizeof(float), f);
		fwrite(&i->anim, 1, sizeof(int), f);
		fwrite(&i->frame, 1, sizeof(int), f);
		fwrite(&i->reps, 1, sizeof(int[4]), f);
		fwrite(&i->flags, 1, 4, f);
		fwrite(&i->story_id, 1, 4, f);
	}
	else
	if (inst->inst_type == Inst::INST_TYPE::ITEM)
	{
		// SAVE ONLY EDITOR INSTANCES !!!
		ItemInst* i = (ItemInst*)inst;

		// checked by inst flags
		// if (i->item->purpose == Item::EDIT)

		{
			int mesh_id_len = -2; // identify item
			fwrite(&mesh_id_len, 1, 4, f);

			// sprite id ???
			// ...

			int item_proto_index = (int)(i->item->proto - item_proto_lib);

			fwrite(&item_proto_index, 1, 4, f);

			fwrite(&i->item->count, 1, sizeof(int), f);

			fwrite(i->pos, 1, sizeof(float[3]), f);
			fwrite(&i->yaw, 1, sizeof(float), f);

			fwrite(&i->flags, 1, 4, f);
			fwrite(&i->story_id, 1, 4, f);
		}
	}
}

static void SaveQueryBSP(BSP* bsp, FILE* f) 
{
    if (bsp->type == BSP::BSP_TYPE_LEAF)
    {
        Inst* i = ((BSP_Leaf*)bsp)->head;
        while (i)
        {
            SaveInst(i,f);
            i=i->next;
        }
    }
    else
    if (bsp->type == BSP::BSP_TYPE_INST)
    {
        Inst* i = (Inst*)bsp;
        SaveInst(i,f);
    }
    else
    if (bsp->type == BSP::BSP_TYPE_NODE)
    {
        bsp_nodes++;
        BSP_Node* n = (BSP_Node*)bsp;
        if (n->bsp_child[0])
            SaveQueryBSP(n->bsp_child[0],f);
        if (n->bsp_child[1])
            SaveQueryBSP(n->bsp_child[1],f);
    }
    else
    if (bsp->type == BSP::BSP_TYPE_NODE_SHARE)
    {
        bsp_nodes++;
        BSP_NodeShare* s = (BSP_NodeShare*)bsp;
        if (s->bsp_child[0])
            SaveQueryBSP(s->bsp_child[0],f);
        if (s->bsp_child[1])
            SaveQueryBSP(s->bsp_child[1],f);
        Inst* i = s->head;
        while (i)
        {
            SaveInst(i,f);
            i=i->next;
        }                
    }
    else
    {
        assert(0);
    }
}

void SaveWorld(World* w, FILE* f)
{
	int format_version = -1;

	/*
		VERSION: -1 
		- adds format_version (before num_of_instances which must be >= 0 and version must be < 0) 
		- adds per instance: Inst::story_id
	*/
	fwrite(&format_version, 1, 4, f);

    int num_of_instances = w->insts - w->temp_insts;
    fwrite(&num_of_instances,1,4,f);

    // non bsp first
    Inst* i = w->head_inst;
    while (i)
    {
        SaveInst(i,f);
        i=i->next;
    }

    // then bsp ones
	if (w->root)
		SaveQueryBSP(w->root,f);
}

World* LoadWorld(FILE* f, bool editor)
{
    // load instances, 
    // create empty meshes if mesh-id is used for the first time
    // all subsequent instances should point to that mesh (share!)

    // after loading, asciiid will reload mesh files from ./obj dir
    // then it is responsible to match (by id) & use our empty meshes!

    World* w = CreateWorld();

    int num_of_instances = 0;
    if (1!=fread(&num_of_instances,4,1,f))
    {
        DeleteWorld(w);
        return 0;
    }

	int format_version = 0; // all till y4

	if (num_of_instances < 0)
	{
		format_version = -num_of_instances;
		if (1 != fread(&num_of_instances, 4, 1, f))
		{
			DeleteWorld(w);
			return 0;
		}
	}
    
    for (int i=0; i<num_of_instances; i++)
    {
        int mesh_id_len = 0;
        if (1!=fread(&mesh_id_len, 4,1, f))
        {
            DeleteWorld(w);
            return 0;
        }

		if (mesh_id_len >= 0)
		{
			char mesh_id[256] = "";
			if (mesh_id_len)
			{
				if (1 != fread(mesh_id, mesh_id_len, 1, f))
				{
					DeleteWorld(w);
					return 0;
				}
			}
			mesh_id[mesh_id_len] = 0;

			if (mesh_id_len>=4 && strcmp(mesh_id+mesh_id_len-4,".ply")==0)
				strcpy(mesh_id+mesh_id_len-4,".akm");

			int inst_name_len = 0;
			if (1 != fread(&inst_name_len, 4, 1, f))
			{
				DeleteWorld(w);
				return 0;
			}

			char inst_name[256] = "";
			if (inst_name_len)
			{
				if (1 != fread(inst_name, inst_name_len, 1, f))
				{
					DeleteWorld(w);
					return 0;
				}
			}
			inst_name[inst_name_len] = 0;

			double tm[16] = { 0 };
			if (1 != fread(tm, 16 * 8, 1, f))
			{
				DeleteWorld(w);
				return 0;
			}

			int flags = 0;
			if (1 != fread(&flags, 4, 1, f))
			{
				DeleteWorld(w);
				return 0;
			}

			int story_id = -1;
			if (format_version > 0)
			{
				if (1 != fread(&story_id, 4, 1, f))
				{
					DeleteWorld(w);
					return 0;
				}
			}

			/*
			if (strstr(mesh_id,"untitled"))
			{
				strcpy(mesh_id,"tree-3.akm");
			}
			*/


			// mesh id lookup
			Mesh* m = w->head_mesh;
			while (m && strcmp(m->name, mesh_id))
				m = m->next;

			if (!m)
				m = w->AddMesh(mesh_id);

			if (!editor)
				flags |= INST_FLAGS::INST_VOLATILE;

			Inst* inst = CreateInst(m, flags, tm, inst_name, story_id);
		}
		else
		if (mesh_id_len == -1)
		{
			int inst_name_len = 0;
			if (1 != fread(&inst_name_len, 4, 1, f))
			{
				DeleteWorld(w);
				return 0;
			}

			char inst_name[256] = "";
			if (inst_name_len)
				if (1 != fread(inst_name, inst_name_len, 1, f))
				{
					DeleteWorld(w);
					return 0;
				}
			inst_name[inst_name_len] = 0;

			float pos[3];
			float yaw;
			int anim;
			int frame;
			int reps[4];
			int flags;

			int r;
			r = fread(pos, 1, sizeof(float[3]), f);
			r = fread(&yaw, 1, sizeof(float), f);
			r = fread(&anim, 1, sizeof(int), f);
			r = fread(&frame, 1, sizeof(int), f);
			r = fread(reps, 1, sizeof(int[4]), f);
			r = fread(&flags, 1, 4, f);

			if (!editor)
				flags |= INST_FLAGS::INST_VOLATILE;

			int story_id = -1;
			if (format_version > 0)
			{
				if (1 != fread(&story_id, 4, 1, f))
				{
					DeleteWorld(w);
					return 0;
				}
			}

			Sprite* s = GetFirstSprite();
			while (s)
			{
				if (strcmp(inst_name, s->name) == 0)
				{
					CreateInst(w, s, flags, pos, yaw, anim, frame, reps, 0, story_id);
					break;
				}

				s = s->next;
			}
		}
		else
		if (mesh_id_len == -2)
		{
			int r;

			int item_proto_index = -1;
			r = fread(&item_proto_index, 1, 4, f);

			int count = 0;
			r = fread(&count, 1, sizeof(int), f);

			float pos[3] = { 0,0,0 };
			float yaw = 0;

			r = fread(pos, 1, sizeof(float[3]), f);
			r = fread(&yaw, 1, sizeof(float), f);

			int flags;
			r = fread(&flags, 1, 4, f);

			int story_id = -1;
			if (format_version > 0)
			{
				if (1 != fread(&story_id, 4, 1, f))
				{
					DeleteWorld(w);
					return 0;
				}
			}

			Item* item = CreateItem();

			if (!editor)
				flags |= INST_FLAGS::INST_VOLATILE;

			item->count = count;
			item->proto = item_proto_lib + item_proto_index;
			item->purpose = editor ? Item::EDIT : Item::WORLD;
			item->inst = CreateInst(w, item, flags, pos, yaw, story_id);

			if (editor)
			{
				// create clone for players
				Item* clone = CreateItem(); // (Item*)malloc(sizeof(Item));
				memcpy(clone, item, sizeof(Item));
				clone->purpose = Item::WORLD;
				clone->inst = CreateInst(w, clone, flags | INST_FLAGS::INST_VOLATILE, pos, yaw, story_id);
			}
		}
    }

    return w;
}


Inst* HitWorld(World* w, double p[3], double v[3], double ret[3], double nrm[3], bool positive_only, bool editor, bool solid_only, bool sprites_too)
{
    return w->HitWorld(p,v,ret,nrm, positive_only, editor, solid_only, sprites_too);
}

Mesh* GetInstMesh(Inst* i)
{
	if (i->inst_type == Inst::INST_TYPE::MESH)
		return ((MeshInst*)i)->mesh;
	return 0;
}

int GetInstFlags(Inst* i)
{
	return i->flags;
}

int GetInstStoryID(Inst* i)
{
	return i->story_id;
}

bool GetInstTM(Inst* i, double tm[16])
{
	if (i->inst_type == Inst::INST_TYPE::MESH)
	{
		memcpy(tm, ((MeshInst*)i)->tm, sizeof(double[16]));
		return true;
	}

	return false;
}

int GetMeshFaces(Mesh* m)
{
	return m->faces;
}

void GetInstBBox(Inst* i, double bbox[6])
{
	bbox[0] = i->bbox[0];
	bbox[1] = i->bbox[1];
	bbox[2] = i->bbox[2];
	bbox[3] = i->bbox[3];
	bbox[4] = i->bbox[4];
	bbox[5] = i->bbox[5];
}

World* GetInstWorld(Inst* i)
{
	if (!i)
		return 0;
	if (i->inst_type == Inst::INST_TYPE::SPRITE)
		return ((SpriteInst*)i)->w;
	if (i->inst_type == Inst::INST_TYPE::ITEM)
		return ((ItemInst*)i)->w;
	if (i->inst_type == Inst::INST_TYPE::MESH)
		return ((MeshInst*)i)->mesh ? ((MeshInst*)i)->mesh->world : 0;
	return 0;
}

Sprite* GetInstSprite(Inst* i, float pos[3], float* yaw, int* anim, int* frame, int reps[4])
{
	if (i->inst_type != Inst::SPRITE)
		return 0;
	SpriteInst* si = (SpriteInst*)i;
	if (pos)
	{
		pos[0] = si->pos[0];
		pos[1] = si->pos[1];
		pos[2] = si->pos[2];
	}
	if (yaw)
		*yaw = si->yaw;
	if (anim)
		*anim = si->anim;
	if (frame)
		*frame = si->frame;
	if (reps)
	{
		reps[0] = si->reps[0];
		reps[1] = si->reps[1];
		reps[2] = si->reps[2];
		reps[3] = si->reps[3];
	}
	
	return si->sprite;
}

void* GetInstSpriteData(Inst* i)
{
	if (i->inst_type != Inst::SPRITE)
		return 0;
	SpriteInst* si = (SpriteInst*)i;
	return si->data;
}

bool SetInstSpriteData(Inst* i, void* data)
{
	if (i->inst_type != Inst::SPRITE)
		return false;
	SpriteInst* si = (SpriteInst*)i;
	si->data = data;
	return true;
}

Item* GetInstItem(Inst* i, float pos[3], float* yaw)
{
	if (i->inst_type != Inst::ITEM)
		return 0;

	ItemInst* ii = (ItemInst*)i;
	if (pos)
	{
		pos[0] = ii->pos[0];
		pos[1] = ii->pos[1];
		pos[2] = ii->pos[2];
	}
	if (yaw)
		*yaw = ii->yaw;

	return ii->item;
}

bool BSP::InsertInst(World* w, Inst* i)
{
	// i->bbox must be up to date !

	if (bbox[0] > i->bbox[0] || bbox[1] < i->bbox[1] ||
		bbox[2] > i->bbox[2] || bbox[3] < i->bbox[3] ||
		bbox[4] > i->bbox[4] || bbox[5] < i->bbox[5])
	{
		return false;
	}

	switch (type)
	{
		case BSP::BSP_TYPE_INST:
			return false;

		case BSP::BSP_TYPE_NODE:
		case BSP::BSP_TYPE_NODE_SHARE:
		{
			BSP_Node* n = (BSP_Node*)this;
			if (n->bsp_child[0])
			{
				if (n->bsp_child[0]->InsertInst(w,i))
					return true;
			}
			if (n->bsp_child[1])
			{
				if (n->bsp_child[1]->InsertInst(w,i))
					return true;
			}

			bool ok = false;
			if (!n->bsp_child[0])
			{
				n->bsp_child[0] = i;
				ok = true;
			}
			if (!n->bsp_child[1])
			{
				n->bsp_child[1] = i;
				ok = true;
			}

			if (!ok)
			{
				BSP_NodeShare* s = (BSP_NodeShare*)this;
				if (type != BSP::BSP_TYPE_NODE_SHARE)
				{
					type = BSP::BSP_TYPE_NODE_SHARE;
					s->head = 0;
					s->tail = 0;
				}

				i->bsp_parent = this;

				if (i->prev)
					i->prev->next = i->next;
				else
					w->head_inst = i->next;

				if (i->next)
					i->next->prev = i->prev;
				else
					w->tail_inst = i->prev;

				i->prev = 0;

				i->next = s->head;
				if (s->head)
					s->head->prev = i;
				else
					s->tail = i;

				s->head = i;

				return true;
			}

			if (ok)
			{
				i->bsp_parent = this;

				if (i->prev)
					i->prev->next = i->next;
				else
					w->head_inst = i->next;

				if (i->next)
					i->next->prev = i->prev;
				else
					w->tail_inst = i->prev;

				i->next = 0;
				i->prev = 0;

				return true;
			}

			break;
		}

		case BSP::BSP_TYPE_LEAF:
		{
			BSP_Leaf* l = (BSP_Leaf*)this;

			i->bsp_parent = this;

			if (i->prev)
				i->prev->next = i->next;
			else
				w->head_inst = i->next;

			if (i->next)
				i->next->prev = i->prev;
			else
				w->tail_inst = i->prev;

			i->prev = 0;

			i->next = l->head;
			if (l->head)
				l->head->prev = i;
			else
				l->tail = i;

			l->head = i;

			return true;
		}

	}

	return false;
}

bool DetachInst(World* w, Inst* inst)
{
	// move it to flat list
	if (!inst->bsp_parent && inst != w->root)
		return false; // already out

	if (inst == w->root)
	{
		w->root = 0;
	}
	else
	switch (inst->bsp_parent->type)
	{
		case BSP::BSP_TYPE_NODE_SHARE:
		case BSP::BSP_TYPE_NODE:
		{
			BSP_Node* n = (BSP_Node*)inst->bsp_parent;
			if (n->bsp_child[0] == inst)
				n->bsp_child[0] = 0;
			else
			if (n->bsp_child[1] == inst)
				n->bsp_child[1] = 0;
			else
			{
				BSP_NodeShare* s = (BSP_NodeShare*)inst->bsp_parent;
				if (inst->bsp_parent->type == BSP::BSP_TYPE_NODE_SHARE)
				{
					if (inst->prev)
						inst->prev->next = inst->next;
					else
						s->head = inst->next;
					if (inst->next)
						inst->next->prev = inst->prev;
					else
						s->tail = inst->prev;
				}
				else
					assert(0);
			}
			break;
		}
		case BSP::BSP_TYPE_LEAF:
		{
			BSP_Leaf* l = (BSP_Leaf*)inst->bsp_parent;
			if (inst->prev)
				inst->prev->next = inst->next;
			else
				l->head = inst->next;
			if (inst->next)
				inst->next->prev = inst->prev;
			else
				l->tail = inst->prev;
			break;
		}

		case BSP::BSP_TYPE_INST:
			break;
	}

	inst->next = w->head_inst;
	inst->prev = 0;
	if (w->head_inst)
		w->head_inst->prev = inst;
	else
		w->tail_inst = inst;
	inst->bsp_parent = 0;

	return true;
}

bool AttachInst(World* w, Inst* inst)
{
	// update bbox if needed
	switch (inst->inst_type)
	{
		case Inst::INST_TYPE::SPRITE:
		{
			SpriteInst* si = (SpriteInst*)inst;
			Sprite* s = si->sprite;
			si->bbox[0] = s->proj_bbox[0] + si->pos[0];
			si->bbox[1] = s->proj_bbox[1] + si->pos[0];
			si->bbox[2] = s->proj_bbox[2] + si->pos[1];
			si->bbox[3] = s->proj_bbox[3] + si->pos[1];
			si->bbox[4] = s->proj_bbox[4] + si->pos[2];
			si->bbox[5] = s->proj_bbox[5] + si->pos[2];
			break;
		}
		case Inst::INST_TYPE::ITEM:
		{
			ItemInst* ii = (ItemInst*)inst;
			Sprite* s = ii->item->proto->sprite_3d;
			ii->bbox[0] = s->proj_bbox[0] + ii->pos[0];
			ii->bbox[1] = s->proj_bbox[1] + ii->pos[0];
			ii->bbox[2] = s->proj_bbox[2] + ii->pos[1];
			ii->bbox[3] = s->proj_bbox[3] + ii->pos[1];
			ii->bbox[4] = s->proj_bbox[4] + ii->pos[2];
			ii->bbox[5] = s->proj_bbox[5] + ii->pos[2];
			break;
		}
		case Inst::INST_TYPE::MESH:
			((MeshInst*)inst)->UpdateBox();
			break;
	}

	if (inst->bsp_parent || inst == w->root)
		return false; // already in

	if (!w->root)
		return false; // no place to insert

	return w->root->InsertInst(w,inst);
}

void ShowInst(Inst* i)
{
	i->flags |= INST_FLAGS::INST_VISIBLE;
}

void HideInst(Inst* i)
{
	i->flags &= ~INST_FLAGS::INST_VISIBLE;
}

void UpdateSpriteInst(World* world, Inst* i, Sprite* sprite, const float pos[3], float yaw, int anim, int frame, const int reps[4])
{
	if (!i)
		return;
	assert(i->inst_type == Inst::INST_TYPE::SPRITE);

	// instead of detach/ attach it could be more intelligent:
	// start from current parrent, if bbox ok try its children
	// if not retry with parent's parent ...

	DetachInst(world, i);

	SpriteInst* si = (SpriteInst*)i;
	si->sprite = sprite;
	si->pos[0] = pos[0];
	si->pos[1] = pos[1];
	si->pos[2] = pos[2];
	si->yaw = yaw;
	si->anim = anim;
	si->frame = frame;
	si->reps[0] = reps[0];
	si->reps[1] = reps[1];
	si->reps[2] = reps[2];
	si->reps[3] = reps[3];

	AttachInst(world, i);
}

// undo/redo only!!!
void SoftInstAdd(Inst* i)
{
	World* w = GetInstWorld(i);

	// it is external thing

	i->next = w->head_inst;
	if (w->head_inst)
		w->head_inst->prev = i;
	else
		w->tail_inst = i;
	w->head_inst = i;

	// it is in flat list now

	AttachInst(w, i);

	// if there was place it is in bsp otherwise in flat list
	w->insts++;
}

void SoftInstDel(Inst* i)
{
	World* w = GetInstWorld(i);

	// it is in bsp or flat

	DetachInst(w, i);

	// it is in flat list now.

	if (i->prev)
		i->prev->next = i->next;
	else
		w->head_inst = i->next;

	if (i->next)
		i->next->prev = i->prev;
	else
		w->tail_inst = i->prev;

	// now it is external
	i->next = 0;
	i->prev = 0;

	w->insts--;
}

void HardInstDel(Inst* i)
{
	// assuming it is external !!!

	if (i->inst_type == Inst::INST_TYPE::ITEM)
	{
		// it destroys inst too!
		((ItemInst*)i)->item->inst = 0;
		DestroyItem(((ItemInst*)i)->item);
	}
	else
	if (i->inst_type == Inst::INST_TYPE::MESH)
	{
		MeshInst* m = (MeshInst*)i;
		// unref
		if (m->mesh)
		{
			MeshInst** s = &m->mesh->share_list;
			while (*s != m)
				s = &(*s)->share_next;
			*s = (*s)->share_next;
		}
	}

	if (i->name)
		free(i->name);

	free(i);
}

int AnimateSpriteInst(Inst* i, uint64_t stamp)
{
	if (i->inst_type != Inst::SPRITE)
		return -1;

	SpriteInst* si = (SpriteInst*)i;
	Sprite* sp = si->sprite;

	int anim = si->anim;
	if (anim < 0 || anim >= sp->anims)
		anim = 0;

	int time = 0;

	int len = si->reps[0] + si->reps[1] * sp->anim[anim].length + si->reps[2] + si->reps[3] * sp->anim[anim].length;

	int frame = 0;

	if (len <= 0)
		frame = si->frame % sp->anim[anim].length;
	else
	{
		time = (stamp>>14) /*61.035 FPS*/ % len;

		if (time < si->reps[0])
			frame = 0;
		else
		if (time < si->reps[0] + si->reps[1] * sp->anim[anim].length)
			frame = (time - si->reps[0]) / si->reps[1];
		else
		if (time < si->reps[0] + si->reps[1] * sp->anim[anim].length + si->reps[2])
			frame = sp->anim[anim].length - 1;
		else
			frame = sp->anim[anim].length - 1 - (time - si->reps[0] - si->reps[1] * sp->anim[anim].length - si->reps[2]) / si->reps[3];
	}

	return frame;
}
