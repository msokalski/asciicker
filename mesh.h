#pragma once
#include "malloc.h"

enum ConstraintType
{
	CON_FOLLOW_PATH = 1,
	CON_IK = 2
};

enum ObjectType
{
	OBJ_MESH = 1,
	OBJ_CURVE = 2,
	OBJ_ARMATURE = 3,
};

enum ParentType
{
	PAR_OBJECT = 1,
	PAR_BONE = 2,
	PAR_ARMATURE = 3,
	PAR_VERTEX = 4,
	PAR_VERTEX_3 = 5,
};

struct Constraint
{
	ConstraintType type;
};

struct Armature
{
	struct Bone
	{
		int parent_index;
		int constraints;
		int constraint_table_offset;
		int bone_children;
		int object_children;
		int child_index[1]; // bones first then objects
	};

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

	int object_data_offset;

	int constraints;
	int constraint_offset[1];

	Constraint* GetConstraintPtr(int index)
	{
		if (index < 0 || index >= constraints)
			return 0;
		return (Constraint*)((char*)this + constraint_offset[index]);
	}

	void* GetObjectData() // cast onto Mesh/Curve/Armature (depending on object_type)
	{
		return (char*)this + object_data_offset;
	}
};

struct Scene
{
	int roots;
	int objects;
	int object_offset[1];

	static Scene* Load(const char* path);

	void Free()
	{
		free(this);
	}

	Object* GetObjectPtr(int index)
	{
		if (index < 0 || index >= objects)
			return 0;
		return (Object*)((char*)this + object_offset[index]);
	}
};
