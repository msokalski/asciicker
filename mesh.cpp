#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include "mesh.h"

const char* ParentType_Names[] = { "","PAR_OBJECT","PAR_BONE","PAR_ARMATURE","PAR_VERTEX","PAR_VERTEX_3" };
const char* ObjectType_Names[] = { "","OBJ_MESH","OBJ_CURVE","OBJ_ARMATURE","OBJ_EMPTY" };
const char* ConstraintType_Names[] = { "","CON_FOLLOW_PATH","CON_IK" };
const char* RotationType_Names[] = { "", "ROT_QUATERNION", "ROT_AXISANGLE", "ROT_EULER_XYZ", "ROT_EULER_YZX", "ROT_EULER_ZXY" , "ROT_EULER_ZYX" , "ROT_EULER_YXZ" , "ROT_EULER_XZY" };
const char* EmptyType_Names[] = { "", "EMP_PLAIN_AXES", "EMP_ARROWS", "EMP_SINGLE_ARROW", "EMP_CUBE", "EMP_SPHERE", "EMP_CONE", "EMP_IMAGE" };

extern "C"
{
	size_t tinfl_decompress_mem_to_mem(void *pOut_buf, size_t out_buf_len, const void *pSrc_buf, size_t src_buf_len, int flags);
}

Scene* Scene::Load(const char* path)
{
	FILE* f = fopen(path, "rb");
	if (!f)
		return 0;

	struct Header
	{
		char sign_ver[8];
		int inflate_size;
		int deflate_size;
	} header;

	if (fread(&header, 1, sizeof(Header), f) != sizeof(Header))
	{
		fclose(f);
		return 0;
	}

	if (memcmp(header.sign_ver, "AKM-1000", 8))
	{
		fclose(f);
		return 0;
	}

	void* deflate = malloc(header.deflate_size);
	if (!deflate)
	{
		fclose(f);
		return 0;
	}

	if (fread(deflate, 1, header.deflate_size, f) != header.deflate_size)
	{
		free(deflate);
		fclose(f);
		return 0;
	}

	fclose(f);

	Scene* s = (Scene*)malloc(header.inflate_size);
	
	if (s)
	{
		size_t size = tinfl_decompress_mem_to_mem(s, header.inflate_size, deflate, header.deflate_size, 0);
		if (size != header.inflate_size)
		{
			free(s);
			s = 0;
		}
	}

	free(deflate);

	return s;
}

void Scene::Free()
{
	free(this);
}

void Mesh::TransformVerts(float xyz[][3], const float* bone_tm[/*all_bones_in_parent_armature*/])
{
	int* bone_ids = (int*)((char*)this + vtx_groups_offset);
	ShapeKeys* sk = (ShapeKeys*)((char*)this + shp_keys_offset);
	VertexData* vd = (VertexData*)((char*)this + vertices_offset);
	int idx = 0;
	while (idx < vertices)
	{
		float* data = (float*)((char*)vd + sizeof(VertexData) + sizeof(int) * (vd->vtx_groups - 1));
		int advance1 = 3 * shp_keys;
		int advance2 = advance1 + vd->vtx_groups;

		for (int end = idx + vd->vertices; idx < end; idx++, data += advance2)
		{
			float pos[3] = { data[0],data[1],data[2] };
			for (int k = 1; k < shp_keys; k++)
			{
				float* key = data + 3 * k;
				float* ref = data + 3 * sk->key[k].relative_key;
				float w = sk->key[k].value;
				pos[0] += w * (key[0] - ref[0]);
				pos[1] += w * (key[1] - ref[1]);
				pos[2] += w * (key[2] - ref[2]);
			}

			if (vd->vtx_groups)
			{
				float* weights = data + advance1;

				float acc[3] = { 0,0,0 };
				float tmp[3];
				for (int g = 0; g < vd->vtx_groups; g++)
				{
					int bone = bone_ids[vd->vtx_group_index[g]];
					if (bone >= 0)
					{
						Product(bone_tm[bone], pos, tmp);
						float w = weights[g];
						acc[0] += w * tmp[0];
						acc[1] += w * tmp[1];
						acc[2] += w * tmp[2];
					}
					else
					{
						// it is guaranteed that all groups in format attached to parent armature's bone
						// come first, so if we reach non-bone group we can skip all other groups in format
						break;
					}
				}

				xyz[idx][0] = acc[0];
				xyz[idx][1] = acc[1];
				xyz[idx][2] = acc[2];
			}
			else
			{
				xyz[idx][0] = pos[0];
				xyz[idx][1] = pos[1];
				xyz[idx][2] = pos[2];
			}
		}

		vd = (VertexData*)data;
	}
}
