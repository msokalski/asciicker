#pragma once
#include "malloc.h"

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

struct Armature
{
	struct Bone
	{
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
