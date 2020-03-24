#pragma once
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
	OBJ_EMPTY = 4,
};

extern const char* ObjectType_Names[];

enum ParentType
{
	PAR_OBJECT = 1,
	PAR_BONE = 2,
	PAR_ARMATURE = 3,
	PAR_VERTEX = 4,
	PAR_VERTEX_3 = 5,
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
	ROT_EULER_XZY = 8,
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

enum KeyInterp
{
	KEY_LINEAR = 1,
	KEY_CARDINAL = 2,
	KEY_CATMULL_ROM = 3,
	KEY_BSPLINE = 4,
};

extern const char* KeyInterp_Names[];

struct Constraint
{
	ConstraintType type;
};

struct Transform
{
	float position[3];
	RotationType rot_type;
	float rotation[4];
	float scale[3];
};

struct Empty
{
	EmptyType type;
	float size;
	float image_ofs[2];
};

struct ShapeKeys
{
	float eval_time;
	int reference_key;
	int use_relative;

	struct Key
	{
		KeyInterp interp;
		int mute;
		int relative_key;
		float min_value;
		float max_value;
		float value;
		int vtx_group;
	};

	Key key[1];
};

struct VertexData
{
	int vertices;
	int vtx_groups;
	int vtx_group_index[1];

	// then for each vertex:
	// - float[3] coords for every shape_key in mesh
	// - all vertex weights listed in vtx_group_index[]

	// followed by next VertexData(s) ...
	// until sum of vertex_num is Mesh::vertices
};

struct Edge
{
	int flags;
	int vertices[2];
};

struct PolyData
{
	int mat_and_flags;
	int indices;

	// followed by array of
	struct Indice
	{
		int vertex;
		int edge;

		// followed by float[2] coords on all Mesh::tex_channels
		// followed by uint32 colors on all Mesh::col_channels
	};

	// followed by next PolyData(s) ...
	// until have all Mesh::polys 

	PolyData* Next(int tex_channels, int col_channels)
	{
		size_t size = 
			sizeof(PolyData) + 
			indices * (
				sizeof(Indice) + 
				sizeof(float[2]) * tex_channels + 
				sizeof(int) * col_channels);

		return (PolyData*)((char*)this + size);
	}
};

struct Mesh
{
	int tex_channels;
	int col_channels;
	
	int vtx_groups;
	int vtx_groups_offset; // -> array of integers representing bone indices

	int shp_keys;
	int shp_keys_offset; // -> ShapeKeys

	int vertices;
	int vertices_offset; // -> VertexData list

	int edges;
	int edges_offset; // -> Edge[edges]

	int polys;
	int polys_offset; // -> PolyData list

	int materials;
	int material_index[1];

	// needed only if shape keys > 1 or has vtx_groups with active bones in parent armature
	// hint: bone_tm should point to first bone's tm buffer (which is right after armature object's tm_slot)
	// by pointers
	void TransformVerts(float xyz[][3], const float* bone_tm[/*all_bones_in_parent_armature*/]);
	// or by continous float array
	void TransformVerts(float xyz[][3], const float bone_tm[/*all_bones_in_parent_armature*/][16]);
};

struct Armature
{
	struct Bone
	{
		int tm_slot;
		int parent_index;
		int first_child_index;
		int children;

		Transform transform;
		// bone data here
		// ...

		int constraints;
		int constraint_offset[1];

		Constraint* GetConstraintPtr(int index)
		{
			if (index < 0 || index >= constraints)
				return 0;
			return (Constraint*)((char*)this + constraint_offset[index]);
		}
	};

	int roots;
	int bones;
	int bone_offset[1];

	Bone* GetBonePtr(int index)
	{
		if (index < 0 || index >= bones)
			return 0;
		return (Bone*)((char*)this + bone_offset[index]);
	}
};

struct Object
{
	ObjectType type;

	int tm_slot;
	int parent_index;
	ParentType parent_type;

	union
	{
		struct // parent_type == PAR_BONE
		{
			int parent_bone_index;
		};

		struct // parent_type == PAR_VERTEX
		{
			int parent_vertex;
		};

		struct // parent_type == PAR_VERTEX_3
		{
			int parent_vertex_3[3];
		};
	};

	int first_child_index;
	int children;

	Transform transform;
	Transform delta_transform;
	float matrix_parent_inverse[16];

	int object_data_offset;

	int constraints;
	int constraint_offset[1];

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

	void CalcLocalTransform(float tm[16]);
	void GetParentTransform(float tm[16]);
};

struct Scene
{
	// hints
	int num_transforms; // all tm_slots
	int num_vertices;
	int num_indices;

	// objects
	int roots;
	int objects;
	int object_offset[1];

	/*
		add bufsize hints like:
		- max num bones, 
		- max verts_buf, 
		- max polyverts_buf
		- render_cache size
		
		so after loading all meshes we can allocate single worst case tmp buf
	*/

	static Scene* Load(const char* path);
	void Free();

	Object* GetObjectPtr(int index)
	{
		if (index < 0 || index >= objects)
			return 0;
		return (Object*)((char*)this + object_offset[index]);
	}
};
