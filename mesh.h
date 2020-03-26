#pragma once
#include <stdint.h>
#include "matrix.h"

enum ConstraintType
{
	CON_FOLLOW_PATH = 1,
	CON_IK = 2
};

extern const char* ConstraintType_Names[];

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

struct Constraint
{
	int32_t name_offs;
	uint32_t type; // ConstraintType

	// constraint data here
	// ...

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

struct Curve
{
	int32_t name_offs;
	void Dump(Pump* pump);
};

struct Empty
{
	int32_t name_offs;
	uint32_t type; // EmptyType
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
	int32_t vertices;
	int32_t vtx_groups;
	int32_t vtx_group_index[1];

	// then for each vertex:
	// - float[3] coords for every shape_key in mesh
	// - all vertex weights listed in vtx_group_index[]

	// followed by next VertexData(s) ...
	// until sum of vertex_num is Mesh::vertices
};

struct Edge
{
	int32_t flags;
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
	int32_t mat_and_flags;
	int32_t indices;
	Indice indice[1]; 
	// every indice has size = 
	//   sizeof(Indice) + 
	//   Mesh::tex_channels*sizeof(float[2]) + 
	//   Mesh::col_channels*sizeof(uint32_t)

	// array is followed by next PolyData(s) ...
	// until have all Mesh::polys 
};

struct VertexGroup
{
	int32_t name_offs;
	int32_t bone;

	void Dump(Pump* pump);
};

struct Mesh
{
	int32_t name_offs;

	int32_t tex_channels;
	int32_t col_channels;
	
	int32_t vtx_groups;
	int32_t vtx_groups_offset; // -> array of integers representing bone indices

	int32_t shp_keys; // NOTE: if shp_keys==0 Mesh contains single set of vertex coords (but without ShapeKey info)
	int32_t shp_keys_offset; // -> ShapeKeys

	int32_t vertices;
	int32_t vertices_offset; // -> VertexData list

	int32_t edges;
	int32_t edges_offset; // -> Edge[edges]

	int32_t polys;
	int32_t polys_offset; // -> PolyData list

	int32_t materials;
	int32_t material_index[1];

	// E must implement:
	// void E::vertex(int vtx, Vector3 shape_coords[], int groups, int group_indexes[], float group_weights[])
	template <typename E> void EnumVertices(E* e)
	{
		ShapeKeys* sk = (ShapeKeys*)((char*)this + shp_keys_offset);
		VertexData* vd = (VertexData*)((char*)this + vertices_offset);
		int idx = 0;
		while (idx < vertices)
		{
			float* data = (float*)((char*)vd + sizeof(VertexData) + sizeof(int32_t) * (vd->vtx_groups - 1));
			int advance1 = 3 * shp_keys;
			int advance2 = advance1 + vd->vtx_groups;

			if (vd->vtx_groups)
			{
				for (int end = idx + vd->vertices; idx < end; idx++, data += advance2)
				{
					float* weights = data + advance1;
					e->vertex(idx, (Vector3*)data, vd->vtx_groups, vd->vtx_group_index, weights);
				}
			}
			else
			{
				for (int end = idx + vd->vertices; idx < end; idx++, data += advance2)
				{
					float* weights = data + advance1;
					e->vertex(idx, (Vector3*)data, 0, 0, 0);
				}
			}
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

	int32_t constraints;
	int32_t constraint_offset[1];

	Constraint* GetConstraintPtr(int index)
	{
		if (index < 0 || index >= constraints)
			return 0;
		return (Constraint*)((char*)this + constraint_offset[index]);
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
