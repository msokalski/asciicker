#include <stdio.h>
#include <string.h>
#include "mesh.h"

const char* ParentType_Names[] = { "","PAR_OBJECT","PAR_BONE","PAR_ARMATURE","PAR_VERTEX","PAR_VERTEX_3" };
const char* ObjectType_Names[] = { "","OBJ_MESH","OBJ_CURVE","OBJ_ARMATURE" };
const char* ConstraintType_Names[] = { "","CON_FOLLOW_PATH","CON_IK" };

Scene* Scene::Load(const char* path)
{
	FILE* f = fopen(path, "rb");
	if (!f)
		return 0;

	char type_ver[8];
	if (fread(type_ver, 1, 8, f) != 8)
	{
		fclose(f);
		return 0;
	}

	if (memcmp(type_ver, "AKM-1000", 8))
	{
		fclose(f);
		return 0;
	}

	int size;
	if (fread(&size, 1, 4, f) != 4)
	{
		fclose(f);
		return 0;
	}

	Scene* s = (Scene*)malloc(size);
	if (!s)
	{
		fclose(f);
		return 0;
	}

	if (fread(s, 1, size, f) != size)
	{
		free(s);
		fclose(f);
		return 0;
	}

	fclose(f);
	return s;
}
