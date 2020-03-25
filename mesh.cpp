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
const char* KeyInterpType_Names[] = { "", "KEY_LINEAR", "KEY_CARDINAL", "KEY_CATMULL_ROM", "KEY_BSPLINE" };

#ifdef ECMA_404 // (ISO/IEC 21778) -> { "prop_enum": "enum", "prop_flag": 3243343 }
#define $K "\""     // "prop":
#define $T "\"%s\"" // "enum"
#define $X "%d"     // 23423242125
#else // ECMA_262 (ISO/IEC 16262)  -> { prop_enum: 'enum', prop_flag: 0x0000ABCD }
#define $K          // prop:
#define $T "'%s'"   // 'enum'
#define $X "0x%08X" // 0x001abc43
#endif

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

inline void SwapBytes(uint32_t* ptr)
{
	uint32_t v = *ptr;
	*ptr = ((v & 0xFF) << 24) | ((v & 0xFF00) << 8) | ((v >> 24) & 0xFF) | ((v >> 8) & 0xFF00);
}

Scene* Scene::Load(const char* path)
{
	FILE* f = fopen(path, "rb");
	if (!f)
		return 0;

	Header header;

	if (fread(&header, 1, sizeof(Header), f) != sizeof(Header))
	{
		fclose(f);
		return 0;
	}

	int big_endian = 0x01;
	*(char*)&big_endian = 0;

	if (big_endian)
	{
		SwapBytes(&header.deflate_size);
		SwapBytes(&header.inflate_size);
	}
	   	 
	if (memcmp(header.sign_ver, "AKM-1000", 8) ||
		header.inflate_size < sizeof(Scene) - sizeof(int32_t) ||
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

		if (big_endian)
			SwapBytes(&s->names_block_offset);

		if (s->names_block_offset & 3)
		{
			// something is misaligned!
			free(s);
			return 0;
		}

		if (big_endian)
		{
			int swaps = s->names_block_offset >> 2;
			for (int swap = 1; swap < swaps; swap++)
				SwapBytes((uint32_t*)s + swap);
		}

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

	if (big_endian)
		SwapBytes(&s->names_block_offset);

	if (s->names_block_offset & 3)
	{
		// something is misaligned!
		free(s);
		return 0;
	}

	if (big_endian)
	{
		int swaps = s->names_block_offset >> 2;
		for (int swap = 1; swap < swaps; swap++)
			SwapBytes((uint32_t*)s + swap);
	}

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

	pump->flush(pump, "{");
	pump->indent++;
	pump->flush(pump, $K "num_transforms" $K ": %d,", num_transforms);
	pump->flush(pump, $K "num_vertices" $K ": %d,", num_vertices);
	pump->flush(pump, $K "num_indices" $K ": %d,", num_indices);
	pump->flush(pump, $K "num_objects" $K ": %d,", objects);
	pump->flush(pump, $K "num_roots" $K ": %d,", roots);

	pump->flush(pump, $K "Roots" $K ": ");
	pump->flush(pump, "[");
	pump->indent++;
	for (int i = 0; i < roots; i++)
	{
		pump->flush(pump, "{");
		pump->indent++;

		GetObjectPtr(i)->Dump(pump);

		pump->indent--;
		pump->flush(pump, i==roots-1 ? "}" : "},");
	}
	pump->indent--;
	pump->flush(pump, "]");

	pump->indent--;
	pump->flush(pump, "}");

	pump->scene = push;
}

void Object::Dump(Pump* pump)
{
	char* utf8_name = (char*)pump->scene + pump->scene->names_block_offset + name_offs;
	pump->flush(pump, $K "name" $K ": \"%s\",", utf8_name);
	pump->flush(pump, $K "type" $K ": " $T ",", ObjectType_Names[type]);
	pump->flush(pump, $K "tm_slot" $K ": %d,", tm_slot);
	pump->flush(pump, $K "parent_index" $K ": %d,", parent_index);
	pump->flush(pump, $K "parent_type" $K ":  " $T ",", ParentType_Names[parent_type]);
	switch (parent_type)
	{
		case ParentType::PAR_OBJECT: 
		case ParentType::PAR_ARMATURE:
			break;
		case ParentType::PAR_BONE: 
			pump->flush(pump, $K "parent_bone_index" $K ": %d,", parent_bone_or_vert[0]);
			break;
		case ParentType::PAR_VERTEX:
			pump->flush(pump, $K "parent_vertex" $K ": %d,", parent_bone_or_vert[0]);
			break;
		case ParentType::PAR_VERTEX_3:
			pump->flush(pump, $K "parent_vertex_3" $K ": [%d,%d,%d],", 
				parent_bone_or_vert[0],
				parent_bone_or_vert[1],
				parent_bone_or_vert[2]);
			break;
	}

	pump->flush(pump, $K "transform" $K ": ");
	pump->flush(pump, "{");
	pump->indent++;
	transform.Dump(pump);
	pump->indent--;
	pump->flush(pump, "},");

	pump->flush(pump, $K "delta_transform" $K ": ");
	pump->flush(pump, "{");
	pump->indent++;
	delta_transform.Dump(pump);
	pump->indent--;
	pump->flush(pump, "},");

	pump->flush(pump, $K "matrix_parent_inverse" $K ": ");
	pump->flush(pump, "[");
	pump->indent++;
	for (int row = 0; row < 4; row++)
	{
		float* r = matrix_parent_inverse + 4*row;
		const char* fmt = row < 3 ? "%8.3f, %8.3f, %8.3f, %8.3f," : "%8.3f, %8.3f, %8.3f, %8.3f";
		pump->flush(pump, fmt, r[0],r[1],r[2],r[3]);
	}
	pump->indent--;
	pump->flush(pump, "],");

	pump->flush(pump, $K "object_data" $K ": ", ObjectType_Names[type]);
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
	pump->flush(pump, "},");

	pump->flush(pump, $K "num_constraints" $K ": %d,", constraints);
	if (constraints)
	{
		pump->flush(pump, $K "Constraints" $K ": ");
		pump->flush(pump, "[");
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
		pump->flush(pump, "],");
	}

	if (children)
	{
		pump->flush(pump, $K "num_children" $K ": %d,", children);
		pump->flush(pump, $K "Children" $K ": ");
		pump->flush(pump, "[");
		pump->indent++;
		for (int i = 0; i < children; i++)
		{
			pump->flush(pump, "{");
			pump->indent++;
			int index = first_child_index + i;
			pump->scene->GetObjectPtr(index)->Dump(pump);
			pump->indent--;
			pump->flush(pump, "}");
		}
		pump->indent--;
		pump->flush(pump, "]");
	}
	else
	{
		pump->flush(pump, $K "num_children" $K ": %d", children);
	}
}

void Empty::Dump(Pump* pump)
{
	pump->flush(pump, $K "type" $K ": " $T ",", EmptyType_Names[type]);
	pump->flush(pump, $K "size" $K ": %f,", size);
	pump->flush(pump, $K "image_ofs" $K ": [%f, %f]", image_ofs[0], image_ofs[1]);
}

void ShapeKey::Dump(Pump* pump)
{
	pump->flush(pump, $K "interp" $K ": " $T ",", KeyInterpType_Names[interp]);
	pump->flush(pump, $K "mute" $K ": %s,", mute ? "true" : "false");
	pump->flush(pump, $K "relative_key" $K ": %d,", relative_key);
	pump->flush(pump, $K "min_value" $K ": %f,", min_value);
	pump->flush(pump, $K "max_value" $K ": %f,", max_value);
	pump->flush(pump, $K "value" $K ": %f,", value);
	pump->flush(pump, $K "vtx_group" $K ": %d", vtx_group);
};

void Mesh::Dump(Pump* pump)
{
	pump->flush(pump, $K "tex_channels" $K ": %d,", tex_channels);
	pump->flush(pump, $K "col_channels" $K ": %d,", col_channels);

	pump->flush(pump, $K "vtx_groups" $K ": %d,", vtx_groups);
	pump->flush(pump, $K "BoneIndex" $K ": ");
	int32_t* bone_idx = (int32_t*)((char*)this + vtx_groups_offset);
	pump->flush(pump, "[");
	pump->indent++;
	for (int g = 0; g < vtx_groups; g++)
		pump->flush(pump, g == vtx_groups-1 ? "%d" : "%d,", bone_idx[g]);
	pump->indent--;
	pump->flush(pump, "],");

	pump->flush(pump, $K "ShapeKeys" $K ": ");
	pump->flush(pump, "{");
	pump->indent++;

	ShapeKeys* sk = (ShapeKeys*)((char*)this + shp_keys_offset);
	pump->flush(pump, $K "eval_time" $K ": %f,", sk->eval_time);
	pump->flush(pump, $K "reference_key" $K ": %d,", sk->reference_key);
	pump->flush(pump, $K "use_relative" $K ": %s,", sk->use_relative ? "true" : "false");
	pump->flush(pump, shp_keys ? $K "num_keys" $K ": %d," : $K "num_keys" $K ": %d", shp_keys);

	if (shp_keys)
	{
		pump->flush(pump, $K "Keys" $K ": ");
		pump->flush(pump, "[");
		pump->indent++;
		for (int i = 0; i < shp_keys; i++)
		{
			pump->flush(pump, "{");
			pump->indent++;

			sk->key[i].Dump(pump);

			pump->indent--;
			pump->flush(pump, i == shp_keys-1 ? "}":"},");
		}
		pump->indent--;
		pump->flush(pump, "]");
	}

	pump->indent--;
	pump->flush(pump, "},");

	pump->flush(pump, $K "num_vertices" $K ": %d,", vertices);

	if (vertices)
	{
		pump->flush(pump, $K "VertexData" $K ": ");
		pump->flush(pump, "[");
		pump->indent++;

		VertexData* vd = (VertexData*)((char*)this + vertices_offset);
		int idx = 0;
		while (idx < vertices)
		{
			pump->flush(pump, "{");
			pump->indent++;

			pump->flush(pump, $K "num_vertices" $K ": %d,", vd->vertices);
			pump->flush(pump, $K "num_groups" $K ": %d,", vd->vtx_groups);
			if (vd->vtx_groups)
			{
				pump->flush(pump, $K "Groups" $K ": ");
				pump->flush(pump, "[");
				pump->indent++;
				for (int g = 0; g < vd->vtx_groups; g++)
					pump->flush(pump, g == vd->vtx_groups - 1 ? "%d" : "%d,", vd->vtx_group_index[g]);
				pump->indent--;
				pump->flush(pump, "],");
			}

			float* data = (float*)((char*)vd + sizeof(VertexData) + sizeof(int32_t) * (vd->vtx_groups - 1));
			int advance1 = 3 * shp_keys;
			int advance2 = advance1 + vd->vtx_groups;

			pump->flush(pump, $K "Vertices" $K ": ");
			pump->flush(pump, "[");
			pump->indent++;
			for (int end = idx + vd->vertices; idx < end; idx++, data += advance2)
			{
				pump->flush(pump, "{");
				pump->indent++;

				pump->flush(pump, $K "KeyCoords" $K ": ", vd->vtx_groups);
				pump->flush(pump, "[");
				pump->indent++;
				for (int k = 0; k < shp_keys; k++)
				{
					float* key = data + 3 * k;
					pump->flush(pump, k == shp_keys - 1 ? "[%f, %f, %f]" : "[%f, %f, %f],", key[0], key[1], key[2]);
				}
				pump->indent--;
				pump->flush(pump, idx==end-1 && !vd->vtx_groups ? "]":"],");

				if (vd->vtx_groups)
				{
					pump->flush(pump, $K "GroupWeights" $K ": ", vd->vtx_groups);
					pump->flush(pump, "[");
					pump->indent++;

					float* weights = data + advance1;

					for (int g = 0; g < vd->vtx_groups; g++)
						pump->flush(pump, g == vd->vtx_groups-1 ? "%f": "%f,", weights[g]);

					pump->indent--;
					pump->flush(pump, "]");
				}

				pump->indent--;
				pump->flush(pump, idx == end - 1 ? "}" : "},");
			}

			pump->indent--;
			pump->flush(pump, "]");

			vd = (VertexData*)data;

			pump->indent--;
			pump->flush(pump, idx == vertices ? "}":"},");
		}

		pump->indent--;
		pump->flush(pump, "],");
	}

	pump->flush(pump, $K "num_edges" $K ": %d,", edges);
	if (edges)
	{
		pump->flush(pump, $K "Edges" $K ": ");
		pump->flush(pump, "[");
		pump->indent++;
		for (int e = 0; e < edges; e++)
		{
			pump->flush(pump, "{");
			pump->indent++;

			((Edge*)((char*)this + edges_offset) + e)->Dump(pump);

			pump->indent--;
			pump->flush(pump, e==edges-1?"}":"},");
		}
		pump->indent--;
		pump->flush(pump, "],");
	}

	pump->flush(pump, $K "num_polys" $K ": %d,", polys);
	if (polys)
	{
		pump->flush(pump, $K "Polys" $K ": ");
		pump->flush(pump, "[");
		pump->indent++;
		PolyData* pd = (PolyData*)((char*)this + polys_offset);
		for (int p = 0; p < polys; p++)
		{
			pump->flush(pump, "{");
			pump->indent++;

			pump->flush(pump, $K "mat_and_flags" $K ": " $X ",", pd->mat_and_flags);
			pump->flush(pump, $K "num_indices" $K ": %d,", pd->indices);
			pump->flush(pump, $K "Indices" $K ": ");
			pump->flush(pump, "[");
			pump->indent++;
			Indice* ind = pd->indice;
			for (int i = 0; i < pd->indices; i++)
			{
				pump->flush(pump, "{");
				pump->indent++;
				pump->flush(pump, $K "vertex" $K ": %d,", ind->vertex);
				pump->flush(pump, tex_channels || col_channels ? $K "edge" $K ": %d,": $K "edge" $K ": %d", ind->edge);

				if (tex_channels)
				{
					pump->flush(pump, $K "TexCoords" $K ": ");
					pump->flush(pump, "[");
					pump->indent++;

					float* data = (float*)((char*)ind + sizeof(Indice));

					for (int t = 0; t < tex_channels; t++)
					{
						const char* fmt = t == tex_channels - 1 ? "[%f, %f]" : "[%f, %f],";
						pump->flush(pump, fmt, data[0], data[1]);
						data += 2;
					}

					pump->indent--;
					pump->flush(pump, col_channels ? "],":"]");
				}

				if (col_channels)
				{
					pump->flush(pump, $K "Colors" $K ": ");
					pump->flush(pump, "[");
					pump->indent++;

					uint32_t* data = (uint32_t*)((char*)ind + sizeof(Indice) + sizeof(float[2])*tex_channels);

					for (int t = 0; t < col_channels; t++)
					{
						const char* fmt = t == col_channels - 1 ? "[%d, %d, %d, %d]" : "[%d, %d, %d, %d],";
						pump->flush(pump, fmt,
							data[0] & 0xFF,
							(data[0] >> 8) & 0xFF,
							(data[0] >> 16) & 0xFF,
							(data[0] >> 24) & 0xFF);
						data += 1;
					}

					pump->indent--;
					pump->flush(pump, "]");
				}

				pump->indent--;
				pump->flush(pump, i == pd->indices-1 ?"}": "},");

				ind = (Indice*)((char*)ind + sizeof(Indice) +
					sizeof(float[2])*tex_channels +
					sizeof(uint32_t)*col_channels);
			}
			pump->indent--;
			pump->flush(pump, "]"); // Indices

			pump->indent--;
			pump->flush(pump, p==polys-1?"}":"},"); // poly

			pd = (PolyData*)ind;
		}
		pump->indent--;
		pump->flush(pump, "],");
	}

	if (materials)
	{
		pump->flush(pump, $K "num_materials" $K ": %d,", materials);
		pump->flush(pump, $K "Materials" $K ": ");
		pump->flush(pump, "[");
		pump->indent++;
		pump->indent--;
		pump->flush(pump, "]");
	}
	else
	{
		pump->flush(pump, $K "num_materials" $K ": %d", materials);
	}
}

void Edge::Dump(Pump* pump)
{
	pump->flush(pump, $K "flags" $K ": " $X ",", flags);
	pump->flush(pump, $K "verts" $K ": [%d,%d]", vertices[0], vertices[1]);
}

void Constraint::Dump(Pump* pump)
{
	pump->flush(pump, $K "type" $K ": " $T, ConstraintType_Names[type]);
}

void Transform::Dump(Pump* pump)
{
	pump->flush(pump, $K "position" $K ": [%f, %f, %f],", position[0], position[1], position[2]);
	pump->flush(pump, $K "rot_type" $K ": " $T ",", RotationType_Names[rot_type]);
	switch (rot_type)
	{
		case ROT_QUATERNION:
		case ROT_AXISANGLE:
			pump->flush(pump, $K "rotation" $K ": [%f, %f, %f, %f],", rotation[0], rotation[1], rotation[2], rotation[3]);
			break;

		default:
			pump->flush(pump, $K "rotation" $K ": [%f, %f, %f],", rotation[0], rotation[1], rotation[2]);
	}
	pump->flush(pump, $K "scale   " $K ": [%f, %f, %f]", scale[0], scale[1], scale[2]);
}

void Mesh::TransformVerts(float xyz[][3], const float* bone_tm[/*all_bones_in_parent_armature*/])
{
	int32_t* bone_ids = (int32_t*)((char*)this + vtx_groups_offset);
	ShapeKeys* sk = (ShapeKeys*)((char*)this + shp_keys_offset);
	VertexData* vd = (VertexData*)((char*)this + vertices_offset);
	int idx = 0;
	while (idx < vertices)
	{
		float* data = (float*)((char*)vd + sizeof(VertexData) + sizeof(int32_t) * (vd->vtx_groups - 1));
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
