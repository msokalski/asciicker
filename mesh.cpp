#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "mesh.h"

const char* ParentType_Names[] = { "","PAR_OBJECT","PAR_BONE","PAR_ARMATURE","PAR_VERTEX","PAR_VERTEX_3" };
const char* ObjectType_Names[] = { "","OBJ_MESH","OBJ_CURVE","OBJ_ARMATURE","OBJ_EMPTY" };
const char* ConstraintType_Names[] = { "","CON_FOLLOW_PATH","CON_IK" };
const char* RotationType_Names[] = { "", "ROT_QUATERNION", "ROT_AXISANGLE", "ROT_EULER_XYZ", "ROT_EULER_YZX", "ROT_EULER_ZXY" , "ROT_EULER_ZYX" , "ROT_EULER_YXZ" , "ROT_EULER_XZY" };
const char* EmptyType_Names[] = { "", "EMP_PLAIN_AXES", "EMP_ARROWS", "EMP_SINGLE_ARROW", "EMP_CUBE", "EMP_SPHERE", "EMP_CONE", "EMP_IMAGE" };
const char* KeyInterp_Names[] = { "", "KEY_LINEAR", "KEY_CARDINAL", "KEY_CATMULL_ROM", "KEY_BSPLINE" };

void Pump::std_flush(Pump* pump, const char* fmt, ...)
{
	FILE* f = (FILE*)pump->user;
	for (int i=0; i<pump->indent; i++)
		fprintf(f," ");

	va_list a;
	va_start(a, fmt);
	vfprintf(f,fmt,a);
	va_end(a);

	fprintf(f,"\n");
}

void Pump::Init(void(*f)(Pump* pump, const char* fmt, ...), void* u)
{
	flush = f;
	user = u;
	indent = 0;
}


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

	if (memcmp(header.sign_ver, "AKM-1000", 8) ||
		header.inflate_size < sizeof(Scene) - sizeof(int) ||
		header.deflate_size < 0)
	{
		fclose(f);
		return 0;
	}

	if (!header.inflate_size)
	{
		// uncompressed file
		Scene* s = (Scene*)malloc(header.inflate_size);
		if (fread(s, 1, header.inflate_size, f) != header.inflate_size)
		{
			free(s);
			s = 0;
		}
		fclose(f);
		return s;
	}

	// compressed file

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

void Scene::Dump(Pump* pump)
{
	Scene* push = pump->scene;
	pump->scene = this;

	pump->flush(pump, "Scene");
	pump->flush(pump, "{");
	pump->indent++;
	pump->flush(pump, "num_transforms: %d", num_transforms);
	pump->flush(pump, "num_vertices: %d", num_vertices);
	pump->flush(pump, "num_indices: %d", num_indices);
	pump->flush(pump, "objects: %d", objects);
	pump->flush(pump, "roots: %d", roots);

	pump->flush(pump, "{");
	pump->indent++;
	for (int i = 0; i < roots; i++)
	{
		pump->flush(pump, "{");
		pump->indent++;
		pump->flush(pump, "ID: %d", i);

		GetObjectPtr(i)->Dump(pump);

		pump->indent--;
		pump->flush(pump, "}");
	}
	pump->indent--;
	pump->flush(pump, "}");

	pump->indent--;
	pump->flush(pump, "}");

	pump->scene = push;
}

void Object::Dump(Pump* pump)
{
	pump->flush(pump, "type: %s", ObjectType_Names[type]);
	pump->flush(pump, "tm_slot: %d", tm_slot);
	pump->flush(pump, "parent_index: %d", parent_index);
	pump->flush(pump, "parent_type:  %s", ParentType_Names[parent_type]);
	switch (parent_type)
	{
		case ParentType::PAR_OBJECT: 
		case ParentType::PAR_ARMATURE:
			break;
		case ParentType::PAR_BONE: 
			pump->flush(pump, "parent_bone_index: %d", parent_bone_index);
			break;
		case ParentType::PAR_VERTEX:
			pump->flush(pump, "parent_vertex: %d", parent_vertex);
			break;
		case ParentType::PAR_VERTEX_3:
			pump->flush(pump, "parent_vertex_3: {%d,%d,%d}", 
				parent_vertex_3[0],
				parent_vertex_3[1], 
				parent_vertex_3[2]);
			break;
	}

	pump->flush(pump, "transform:");
	pump->flush(pump, "{");
	pump->indent++;
	transform.Dump(pump);
	pump->indent--;
	pump->flush(pump, "}");

	pump->flush(pump, "delta_transform:");
	pump->flush(pump, "{");
	pump->indent++;
	delta_transform.Dump(pump);
	pump->indent--;
	pump->flush(pump, "}");

	pump->flush(pump, "matrix_parent_inverse:");
	pump->flush(pump, "{");
	pump->indent++;
	for (int row = 0; row < 4; row++)
	{
		float* r = matrix_parent_inverse + 4*row;
		pump->flush(pump, "%8.3f %8.3f %8.3f %8.3f", r[0],r[1],r[2],r[3]);
	}
	pump->indent--;
	pump->flush(pump, "}");

	pump->flush(pump, "%s data:", ObjectType_Names[type]);
	pump->flush(pump, "{");
	pump->indent++;
	void* data = GetObjectData();
	switch (type)
	{
		case OBJ_EMPTY:
			((Empty*)data)->Dump(pump);
			break;
		case OBJ_MESH:
			((Mesh*)data)->Dump(pump);
			break;
	}
	pump->indent--;
	pump->flush(pump, "}");

	pump->flush(pump, "constraints: %d", constraints);
	pump->flush(pump, "{");
	pump->indent++;
	for (int c = 0; c < constraints; c++)
	{
		pump->flush(pump, "{");
		pump->indent++;

		GetConstraintPtr(c)->Dump(pump);

		pump->indent--;
		pump->flush(pump, "}");
	}
	pump->indent--;
	pump->flush(pump, "}");

	pump->flush(pump, "children: %d", children);

	pump->flush(pump, "{");
	pump->indent++;
	for (int i = 0; i < children; i++)
	{
		pump->flush(pump, "{");
		pump->indent++;
		int index = first_child_index + i;
		pump->flush(pump, "ID: %d", index);
		pump->scene->GetObjectPtr(index)->Dump(pump);
		pump->indent--;
		pump->flush(pump, "}");
	}
	pump->indent--;
	pump->flush(pump, "}");
}

void Empty::Dump(Pump* pump)
{
	pump->flush(pump, "type: %s", EmptyType_Names[type]);
	pump->flush(pump, "size: %f", size);
	pump->flush(pump, "image_ofs: %f %f", image_ofs[0], image_ofs[1]);
}

void ShapeKeys::Key::Dump(Pump* pump)
{
	pump->flush(pump, "interp: %s", KeyInterp_Names[interp]);
	pump->flush(pump, "mute: %s", mute ? "yes" : "no");
	pump->flush(pump, "relative_key: %d", relative_key);
	pump->flush(pump, "min_value: %f", min_value);
	pump->flush(pump, "max_value: %f", max_value);
	pump->flush(pump, "value: %f", value);
	pump->flush(pump, "vtx_group: %d", vtx_group);
};

void Mesh::Dump(Pump* pump)
{
	pump->flush(pump, "tex_channels: %d", tex_channels);
	pump->flush(pump, "col_channels: %d", col_channels);

	pump->flush(pump, "vtx_groups: %d", vtx_groups);
	int* bone_idx = (int*)((char*)this + vtx_groups_offset);
	pump->flush(pump, "{");
	pump->indent++;
	for (int g = 0; g < vtx_groups; g++)
		pump->flush(pump, "bone: %d", bone_idx[g]);
	pump->indent--;
	pump->flush(pump, "}");

	pump->flush(pump, "ShapeKeys:");
	pump->flush(pump, "{");
	pump->indent++;

	ShapeKeys* sk = (ShapeKeys*)((char*)this + shp_keys_offset);
	pump->flush(pump, "eval_time: %f", sk->eval_time);
	pump->flush(pump, "reference_key: %d", sk->reference_key);
	pump->flush(pump, "use_relative: %s", sk->use_relative ? "yes" : "no");
	pump->flush(pump, "shape_keys: %d", shp_keys);

	pump->flush(pump, "{");
	pump->indent++;
	for (int i = 0; i < shp_keys; i++)
	{
		pump->flush(pump, "{");
		pump->indent++;

		sk->key[i].Dump(pump);

		pump->indent--;
		pump->flush(pump, "}");
	}
	pump->indent--;
	pump->flush(pump, "}");

	pump->indent--;
	pump->flush(pump, "}");

	pump->flush(pump, "vertices: %d", vertices);
	pump->flush(pump, "{");
	pump->indent++;
	pump->indent--;
	pump->flush(pump, "}");

	pump->flush(pump, "edges: %d", edges);
	pump->flush(pump, "{");
	pump->indent++;
	pump->indent--;
	pump->flush(pump, "}");

	pump->flush(pump, "polys: %d", polys);
	pump->flush(pump, "{");
	pump->indent++;
	pump->indent--;
	pump->flush(pump, "}");

	pump->flush(pump, "materials: %d", materials);
	pump->flush(pump, "{");
	pump->indent++;
	pump->indent--;
	pump->flush(pump, "}");
}

void Constraint::Dump(Pump* pump)
{
	pump->flush(pump, "type: %s", ConstraintType_Names[type]);
}

void Transform::Dump(Pump* pump)
{
	pump->flush(pump, "position: %f %f %f", position[0], position[1], position[2]);
	pump->flush(pump, "rot_type: %s", RotationType_Names[rot_type]);
	switch (rot_type)
	{
		case ROT_QUATERNION:
		case ROT_AXISANGLE:
			pump->flush(pump, "rotation: %f %f %f %f", rotation[0], rotation[1], rotation[2], rotation[3]);
			break;

		default:
			pump->flush(pump, "rotation: %f %f %f", rotation[0], rotation[1], rotation[2]);
	}
	pump->flush(pump, "scale   : %f %f %f", scale[0], scale[1], scale[2]);
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
