#pragma once
#include <stdint.h>
#include "matrix.h"

enum ConFollowPathForwardAxis
{
	CON_FOLLOW_PATH_FORWARD_X = 1,
	CON_FOLLOW_PATH_FORWARD_Y = 2,
	CON_FOLLOW_PATH_FORWARD_Z = 3,
	CON_FOLLOW_PATH_TRACK_NEGATIVE_X = 4,
	CON_FOLLOW_PATH_TRACK_NEGATIVE_Y = 5,
	CON_FOLLOW_PATH_TRACK_NEGATIVE_Z = 6
};

extern const char* ConFollowPathForwardAxis_Names[];

enum ConFollowPathUpAxis
{
	CON_FOLLOW_PATH_UP_X = 1,
	CON_FOLLOW_PATH_UP_Y = 2,
	CON_FOLLOW_PATH_UP_Z = 3
};

extern const char* ConFollowPathUpAxis_Names[];

enum ConIKType
{
	CON_IK_COPY_POSE = 1,
	CON_IK_DISTANCE = 2
};

extern const char* ConIKType_Names[];

enum ConIKRefAxis
{
	CON_IK_BONE = 1,
	CON_IK_TARGET = 2
};

extern const char* ConIKRefAxis_Names[];

enum ConIKLimitMode
{
	CON_IK_LIMITDIST_INSIDE = 1,
	CON_IK_LIMITDIST_OUTSIDE = 2,
	CON_IK_LIMITDIST_ONSURFACE = 3
};

extern const char* ConIKLimitMode_Names[];

enum ConIKFlags
{
	CON_IK_USE_LOCATION = 1 << 16,
	CON_IK_USE_ROTATION = 1 << 17,
	CON_IK_USE_STRETCH = 1 << 18,
	CON_IK_USE_TAIL = 1 << 19
};

enum ModHookFalloffType
{
	MOD_HOOK_NONE = 1,
	MOD_HOOK_CURVE = 2,
	MOD_HOOK_SMOOTH = 3,
	MOD_HOOK_SPHERE = 4,
	MOD_HOOK_ROOT = 5,
	MOD_HOOK_INVERSE_SQUARE = 6,
	MOD_HOOK_SHARP = 7,
	MOD_HOOK_LINEAR = 8,
	MOD_HOOK_CONSTANT = 9,

	MOD_HOOK_UNIFORM_FLAG = 1<<16
};

extern const char* ModHookFalloffType_Names[];

enum ModArmatureFlags
{
	MOD_ARM_INVERT_VERTEX_GROUP = 1 << 16,
	MOD_ARM_USE_BONE_ENVELOPES = 1 << 17,
	MOD_ARM_USE_DEFORM_PRESERVE_VOLUME = 1 << 18,
	MOD_ARM_USE_VERTEX_GROUPS = 1 << 19
};

enum ModifierType
{
	MOD_ARMATURE = 1,
	MOD_HOOK = 2
};

extern const char* ModifierType_Names[];

enum ModifierFlags
{
	MOD_SHOW_VIEWPORT = 1 << 16,
	MOD_SHOW_RENDER = 1 << 17,
	MOD_SHOW_EDIT = 1 << 18,
	MOD_SHOW_ON_CAGE = 1 << 19
};

enum ConstraintType
{
	CON_FOLLOW_PATH = 1,
	CON_IK = 2
};

extern const char* ConstraintType_Names[];

enum ConstraintFlags
{
	CON_VALID = 1 << 16,
	CON_MUTE = 1 << 17,
	CON_PROXY_LOCAL = 1 << 18
};

enum ObjectType
{
	OBJ_MESH = 1,
	OBJ_CURVE = 2,
	OBJ_ARMATURE = 3,
	OBJ_EMPTY = 4
};

extern const char* ObjectType_Names[];

enum ParentType
{
	PAR_OBJECT = 1,
	PAR_BONE = 2,
	PAR_ARMATURE = 3,
	PAR_VERTEX = 4,
	PAR_VERTEX_3 = 5
};

extern const char* ParentType_Names[];

enum RotationType
{
	ROT_QUATERNION = 1,
	ROT_AXISANGLE = 2,
	ROT_EULER_XYZ = 3,
	ROT_EULER_YZX = 4,
	ROT_EULER_ZXY = 5,
	ROT_EULER_ZYX = 6,
	ROT_EULER_YXZ = 7,
	ROT_EULER_XZY = 8
};

extern const char* RotationType_Names[];

enum EmptyType
{
	EMP_PLAIN_AXES = 1,
	EMP_ARROWS = 2,
	EMP_SINGLE_ARROW = 3,
	EMP_CUBE = 4,
	EMP_SPHERE = 5,
	EMP_CONE = 6,
	EMP_IMAGE = 7
};

extern const char* EmptyType_Names[];

enum KeyInterpType
{
	KEY_LINEAR = 1,
	KEY_CARDINAL = 2,
	KEY_CATMULL_ROM = 3,
	KEY_BSPLINE = 4
};

extern const char* KeyInterpType_Names[];

enum EdgeFlags
{
	EDGE_USE_FREESTYLE_MARK = 1 << 16,
	EDGE_USE_SMOOTH = 1 << 17,
	EDGE_SELECT = 1 << 18,
	EDGE_HIDE = 1 << 19,
	EDGE_USE_SEAM = 1 << 20,
	EDGE_IS_LOOSE = 1 << 21
};

enum PolyFlags
{
	POLY_USE_FREESTYLE_MARK = 1 << 16,
	POLY_USE_SMOOTH = 1 << 17,
	POLY_SELECT = 1 << 18,
	POLY_HIDE = 1 << 19
};

struct Pump;

#pragma pack(push,4)

typedef float Vector2[2];
typedef float Vector3[3];
typedef float Vector4[4];

struct Header
{
	char sign_ver[8];
	uint32_t inflate_size;
	uint32_t deflate_size;
};


struct ConstraintIK
{
	int32_t target_obj;
	int32_t target_bone; // only if target_obj is armature

	int32_t pole_target_obj;
	int32_t pole_target_bone; // only if pole_target_obj is armature

	uint32_t ik_type; // ConIKType
	uint32_t reference_axis; // ConIKRefAxis
	uint32_t limit_mode; // ConIKLimitMode

	int32_t chain_count;
	int32_t iterations;
	uint32_t flags; // ConIKFlags

	float distance;
	float pole_angle;
	float orient_weight;
	float weight;

	void Dump(Pump* pump);
};


struct ConstraintFollowPath
{
	int32_t curve_obj;
	uint32_t forward_axis; // ConFollowPathForwardAxis
	uint32_t up_axis; // ConFollowPathUpAxis
	float offset;
	float offset_factor;

	void Dump(Pump* pump);
};

struct Constraint
{
	int32_t name_offs;
	uint32_t type; // ConstraintType
	uint32_t flags; // ConstraintFlags

	// constraint data here
	// ...

	void* GetConstraintData()
	{
		return (void*)(this + 1);
	}

	void Dump(Pump* pump);
};

struct Transform
{
	Vector3 position;
	uint32_t rot_type; // RotationType
	Vector4 rotation;
	Vector3 scale;
	void Dump(Pump* pump);
};

struct SplineData
{
	uint32_t mat_and_flags;  // radius_mode(4) | tilt_mode(4) | type(4) | flags(4) | mat_idx(16)
	int32_t resolution_u;
	int32_t first_indice;
	int32_t num_indices; // for bezier splines it is 3x num of points!
};

struct Curve
{
	int32_t name_offs;

	float eval_time;
	float offset;
	float path_duration;
	float twist_smooth;

	int32_t vtx_groups;
	int32_t vtx_groups_offset; // -> offset to VertexGroup[] (not in blender yet)

	int32_t shp_keys; // NOTE: if shp_keys==0 Mesh contains single set of vertex coords (but without ShapeKey info)
	int32_t shp_keys_offset; // -> ShapeKeys

	int32_t vertices;
	int32_t vertices_offset; // -> VertexData list

	int32_t splines;
	int32_t splines_offset; // -> SplineData array

	int32_t materials;
	int32_t material_index[1];

	void Dump(Pump* pump);
};

struct Empty
{
	uint32_t type; // EmptyType
	uint32_t image_side; // todo!
	uint32_t image_depth; // todo!
	float size;
	Vector2 image_ofs;
	void Dump(Pump* pump);
};

struct ShapeKey
{
	int32_t name_offs;
	uint32_t interp; // KeyInterpType 
	int32_t mute;
	int32_t relative_key;
	float min_value;
	float max_value;
	float value;
	int32_t vtx_group;

	void Dump(Pump* pump);
};

struct ShapeKeys
{
	float eval_time;
	int32_t reference_key;
	int32_t use_relative;

	ShapeKey key[1];
};

struct VertexData
{
	int32_t range; // first vertex NOT in this format
	uint32_t keys_and_groups; // number of keys is lower 16 bits, groups is higher 16 bits
	int32_t indexes[1]; // keys first then groups

	// then for each vertex: 
	// - base float[3] vertex coords
	// - float[3] coords for listed shapes in indexes
	// - float vertex weights listed in indexes

	// followed by next VertexData(s) ...
	// until sum of vertex_num is Mesh::vertices
};

struct Edge
{
	uint32_t flags; // EdgeFlags
	int32_t vertices[2];
	void Dump(Pump* pump);
};

struct Indice
{
	int32_t vertex;
	int32_t edge;

	// followed by float[2] coords on all Mesh::tex_channels
	// followed by uint32 colors on all Mesh::col_channels
};

struct PolyData
{
	uint32_t mat_and_flags; // PolyFlags | mat_idx
	int32_t indices;
	Indice indice[1]; 
	// every indice has size = 
	//   sizeof(Indice) + 
	//   Mesh::tex_channels*sizeof(float[2]) + 
	//   Mesh::col_channels*sizeof(uint32_t)

	// indice array is followed by next PolyData(s) ...
	// until have all Mesh::polys 
};

struct VertexGroup
{
	int32_t name_offs;
	int32_t bone_idx; // must be here at least when we have parent type 'ARMATURE' (w/o mod)

	void Dump(Pump* pump);
};

struct Mesh
{
	int32_t name_offs;

	int32_t tex_channels;
	int32_t col_channels;
	
	int32_t vtx_groups;
	int32_t vtx_groups_offset; // -> offset to VertexGroup[]

	int32_t shp_keys; // NOTE: if shp_keys==0 Mesh contains single set of vertex coords (but without ShapeKey info)
	int32_t shp_keys_offset; // -> ShapeKeys

	int32_t vertices;
	int32_t vertices_offset; // -> VertexData list

	int32_t edges;
	int32_t edges_offset; // -> Edge[edges] (all freestyle edges come first)

	int32_t polys;
	int32_t polys_offset; // -> PolyData list (sorted by mat_idx, all freestyle come first in each mat_idx)

	int32_t materials;
	int32_t material_index[1];

	// E must implement:
	// void E::vertex(int vtx, Vector3 shape_coords[], int groups, int group_indexes[], float group_weights[])
	template <typename E> void EnumVertices(E* e)
	{
		VertexData* vd = (VertexData*)((char*)this + vertices_offset);
		int idx = 0;
		while (idx < vertices)
		{
			int keys = vd->keys_and_groups & 0xFFFF;
			int grps = (vd->keys_and_groups>>16) & 0xFFFF;
			int flen = keys + grps;

			float* data = (float*)((char*)vd + sizeof(VertexData) + sizeof(int32_t) * (flen - 1));
			int advance1 = 3 * (keys+1);
			int advance2 = advance1 + grps;

			if (flen)
				for (int end = vd->range; idx < end; idx++, data += advance2)
					e->vertex(idx, keys, grps, vd->indexes, data);
			else
				for (int end = vd->range; idx < end; idx++, data += advance2)
					e->vertex(idx, keys, grps, 0, data);

			vd = (VertexData*)data;
		}
	}

	// E must implement:
	// void E::triangle(int mat_and_flags, int vertex[3], Vector2* uv[3], uint32_t* colors[3])
	template <typename E> void EnumTriangles(E* e)
	{
		int advance = 
			sizeof(Indice) + 
			tex_channels * sizeof(Vector2) + 
			Mesh::col_channels * sizeof(uint32_t);

		if (tex_channels && col_channels)
		{
			int vertex[3];
			Vector2* uv[3];
			uint32_t* col[3];

			PolyData* pd = (PolyData*)((char*)this + polys_offset);

			for (int poly = 0; poly < polys; poly++)
			{
				Indice* ind = pd->indice;
				vertex[0] = ind->vertex;
				uv[0] = (Vector2*)((char*)ind + sizeof(Indice));
				col[0] = (uint32_t*)((char*)ind + sizeof(Indice) + sizeof(Vector2) * tex_channels);

				ind = (Indice*)((char*)ind + advance);
				vertex[1] = ind->vertex;
				uv[1] = (Vector2*)((char*)ind + sizeof(Indice));
				col[1] = (uint32_t*)((char*)ind + sizeof(Indice) + sizeof(Vector2) * tex_channels);

				int i = 3;

				while (1)
				{
					ind = (Indice*)((char*)ind + advance);
					vertex[2] = ind->vertex;
					uv[2] = (Vector2*)((char*)ind + sizeof(Indice));
					col[2] = (uint32_t*)((char*)ind + sizeof(Indice) + sizeof(Vector2) * tex_channels);

					e->triangle(pd->mat_and_flags, vertex, uv, col);

					if (i == pd->indices)
						break;

					vertex[1] = vertex[2];
					uv[1] = uv[2];
					col[1] = col[2];
					i++;
				}

				pd = (PolyData*)((char*)ind + advance);
			}
		}
		else
		if (tex_channels)
		{
			int vertex[3];
			Vector2* uv[3];

			PolyData* pd = (PolyData*)((char*)this + polys_offset);

			for (int poly = 0; poly < polys; poly++)
			{
				Indice* ind = pd->indice;
				vertex[0] = ind->vertex;
				uv[0] = (Vector2*)((char*)ind + sizeof(Indice));

				ind = (Indice*)((char*)ind + advance);
				vertex[1] = ind->vertex;
				uv[1] = (Vector2*)((char*)ind + sizeof(Indice));

				int i = 3;

				while (1)
				{
					ind = (Indice*)((char*)ind + advance);
					vertex[2] = ind->vertex;
					uv[2] = (Vector2*)((char*)ind + sizeof(Indice));

					e->triangle(pd->mat_and_flags, vertex, uv, 0);

					if (i == pd->indices)
						break;

					vertex[1] = vertex[2];
					uv[1] = uv[2];
					i++;
				}

				pd = (PolyData*)((char*)ind + advance);
			}
		}
		else
		if (col_channels)
		{
			int vertex[3];
			uint32_t* col[3];

			PolyData* pd = (PolyData*)((char*)this + polys_offset);

			for (int poly = 0; poly < polys; poly++)
			{
				Indice* ind = pd->indice;
				vertex[0] = ind->vertex;
				col[0] = (uint32_t*)((char*)ind + sizeof(Indice));

				ind = (Indice*)((char*)ind + advance);
				vertex[1] = ind->vertex;
				col[1] = (uint32_t*)((char*)ind + sizeof(Indice));

				int i = 3;

				while (1)
				{
					ind = (Indice*)((char*)ind + advance);
					vertex[2] = ind->vertex;
					col[2] = (uint32_t*)((char*)ind + sizeof(Indice));

					e->triangle(pd->mat_and_flags, vertex, 0, col);

					if (i == pd->indices)
						break;

					vertex[1] = vertex[2];
					col[1] = col[2];
					i++;
				}

				pd = (PolyData*)((char*)ind + advance);
			}
		}
		else
		{
			int vertex[3];

			PolyData* pd = (PolyData*)((char*)this + polys_offset);

			for (int poly = 0; poly < polys; poly++)
			{
				Indice* ind = pd->indice;
				vertex[0] = ind->vertex;

				ind = (Indice*)((char*)ind + advance);
				vertex[1] = ind->vertex;

				int i = 3;

				while (1)
				{
					ind = (Indice*)((char*)ind + advance);
					vertex[2] = ind->vertex;

					e->triangle(pd->mat_and_flags, vertex, 0, 0);

					if (i == pd->indices)
						break;

					vertex[1] = vertex[2];
					i++;
				}

				pd = (PolyData*)((char*)ind + advance);
			}
		}
	}

	void Dump(Pump* pump);
};

struct Bone
{
	int32_t name_offs;

	int32_t tm_slot;
	int32_t parent_index;
	int32_t first_child_index;
	int32_t children;

	Transform transform;
	// bone data here
	// ...

	int32_t constraints;
	int32_t constraint_offset[1];

	Constraint* GetConstraintPtr(int index)
	{
		if (index < 0 || index >= constraints)
			return 0;
		return (Constraint*)((char*)this + constraint_offset[index]);
	}

	void Dump(Pump* pump);
};

struct Armature
{
	int32_t name_offs;

	int32_t roots;
	int32_t bones;
	int32_t bone_offset[1];

	Bone* GetBonePtr(int index)
	{
		if (index < 0 || index >= bones)
			return 0;
		return (Bone*)((char*)this + bone_offset[index]);
	}

	void Dump(Pump* pump);
};

struct ModArmature
{
	int32_t armature_obj;
	int32_t influence_grp;
	uint32_t flags; // ModArmatureFlags
	int32_t bone_idx[1]; // [vtx_groups] present only if armature_obj != -1

	void Dump(Pump* pump);
};

struct ModHook
{
	int32_t target_obj;
	int32_t bone_idx; // only if target_obj is armature
	int32_t influence_grp;

	int32_t falloff_type; // ModHookFalloffType | use_falloff_uniform ? 1<<16
	int32_t falloff_curve_offset; // todo falloff_curve

	Vector3 center;
	float falloff_radius;
	float strength;
	float matrix_inverse[16];

	int vertices;
	int vertex_index[1];

	void Dump(Pump* pump);
};

struct Modifier
{
	int32_t name_offs;
	uint32_t type; // ModifierType
	uint32_t flags; // ModifierFlags

	// followed by type specific data
	// ...

	void* GetModifierData()
	{
		return (void*)(this + 1);
	}

	void Dump(Pump* pump);
};

struct Object
{
	int32_t name_offs; // relative to beginning of the names_block

	uint32_t type; // ObjectType 

	int32_t tm_slot;
	int32_t parent_index;
	uint32_t parent_type; // ParentType 

	int32_t parent_bone_or_vert[3];

	int32_t first_child_index;
	int32_t children;

	Transform transform;
	Transform delta_transform;
	float matrix_parent_inverse[16];

	int32_t object_data_offset;

	int32_t modifiers;
	int32_t mod_stack_offset; // -> array of offsets to each modifier

	int32_t constraints;
	int32_t constraint_offset[1];

	Constraint* GetConstraintPtr(int index)
	{
		if (index < 0 || index >= constraints)
			return 0;
		return (Constraint*)((char*)this + constraint_offset[index]);
	}

	Modifier* GetModifierPtr(int index)
	{
		if (index < 0 || index >= modifiers)
			return 0;
		int32_t* stack = (int32_t*)((char*)this + mod_stack_offset);
		return (Modifier*)((char*)this + stack[index]);
	}

	void* GetObjectData() // cast onto Mesh/Curve/Armature/Empty (depending on object_type)
	{
		return (char*)this + object_data_offset;
	}

	void Dump(Pump* pump);
};

struct Scene
{
	uint32_t names_block_offset; // do not swapbytes after this position!

	// hints
	int32_t num_transforms; // all tm_slots
	int32_t num_vertices;
	int32_t num_indices;

	// objects
	int32_t roots;
	int32_t objects;
	int32_t object_offset[1];

	static Scene* Load(const char* path);
	void Free();

	Object* GetObjectPtr(int index)
	{
		if (index < 0 || index >= objects)
			return 0;
		return (Object*)((char*)this + object_offset[index]);
	}

	Object* FindObjectPtr(const char* name)
	{
		return GetObjectPtr(FindObjectIdx(name));
	}

	int FindObjectIdx(const char* name)
	{
		char* names_block = (char*)this + names_block_offset;
		for (int i = 0; i < objects; i++)
		{
			Object* obj = (Object*)((char*)this + object_offset[i]);
			if (strcmp(name, names_block + obj->name_offs) == 0)
				return i;
		}
		return -1;
	}

	void Dump(Pump* pump);
};

#pragma pack(pop)

struct Pump
{
	void Init(void(*f)(Pump* pump, const char* fmt, ...) = Pump::std_flush, void* u = stdout);

	void(*flush)(Pump* pump, const char* fmt, ...);
	void* user;

	Scene* scene;
	Object* object;
	int indent;

	static void std_flush(Pump* pump, const char* fmt, ...);
};
