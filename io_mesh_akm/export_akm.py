# ##### BEGIN GPL LICENSE BLOCK #####
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software Foundation,
#  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# ##### END GPL LICENSE BLOCK #####

# <pep8-80 compliant>

"""
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

		Bone* GetBone(int index)
		{
			if (index<0 || index>=bones)
				return 0;
			return (Bone*) ( (char*)this + bone_offset[index] );
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

		int constraints;
		int constraint_table_offset;

		int object_data_offset;

		int children;
		int child_index[1];

		Constraint* GetConstraint(int index)
		{
			if (index < 0 || index >= constraints)
				return 0;
			int* table = (char*)this + constraint_table_offset;
			return (Constraint*) ( (char*)this + table[index] );
		}

		void* GetObjectData() // cast onto Mesh/Curve/Armature (depending on object_type)
		{
			return (char*)this + object_data_offset;
		}
	};

	struct Scene
	{
		int objects;
		int object_offset[1];

		void Free() const
		{
			free(this);
		}

		static const Scene* Load(const char* path)
		{
			FILE* f = fopen(path,"rb");
			if (!f)
				return 0;

			char type_ver[8];
			if (fread(type_ver, 1, 8, f) != 8)
			{
				fclose(f);
				return 0;
			}

			if (memcmp(type_ver,"AKM-1000",8))
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

		Object* GetObject(int index) const
		{
			if (index < 0 || index >= objects)
				return 0;
			return (Object*) ( (char*)this + obj_ofs[index] );
		}
	};

"""

import os
import types
import struct
import bpy
import bmesh


"""
This script exports Stanford PLY files from Blender. It supports normals,
colors, and texture coordinates per face or per vertex.
"""



"""
# SHAPE KEYS AS MORPH TARGETS
import bpy

obj1 = bpy.context.selected_objects[0]

for key in obj1.data.shape_keys.key_blocks:
	print("NAME:", key.name)
	print("INTERPOLATION:", key.interpolation)
	print("MUTE:", key.mute)
	print("RELATIVE_TO:", key.relative_key.name)
	print("SLIDER_MAX:", key.slider_max)
	print("SLIDER_MIN:", key.slider_min)
	print("SLIDER_VAL:", key.value)
	print("VERTEX_GROUP:", key.vertex_group)
....
	for v in key.data:
		print(v.co)

#for vert in obj1.data.vertices:
#    print(vert.co)  # this is a vertex coord of the mesh
"""


"""
// ISOLATED EDGES ON MESH    
print("---------------------------------")

obj1 = bpy.context.selected_objects[0]

msh = obj1.data

edges = []

for e in msh.edges:
	edges.append( { e.vertices[0], e.vertices[1] } )

for p in msh.polygons:
	
	l1 = p.loop_start
	l2 = p.loop_start + p.loop_total - 1
		
	for l in range( l1, l2 ):
		v1 = msh.loops[l].vertex_index
		v2 = msh.loops[l+1].vertex_index
		try:
			edges.remove( {v1,v2} )
		except:
			pass
		try:
			edges.remove( {v2,v1} )
		except:
			pass
		
	v1 = msh.loops[l1].vertex_index
	v2 = msh.loops[l2].vertex_index

	try:
		edges.remove( {v1,v2} )
	except:
		pass
	try:
		edges.remove( {v2,v1} )
	except:
		pass

for e in edges:
	print( e )
"""    

"""
// ISOLATED EDGES ON BMESH    
obj1 = bpy.context.selected_objects[0]

bm = bmesh.new()
bm.from_mesh(obj1.data)

bmesh.ops.triangulate(bm, faces=bm.faces)

print("---------------------------------")

edges = []

for e in bm.edges:
	edges.append( { e.verts[0].index, e.verts[1].index } )
	
for f in bm.faces:
	for r in f.edges:
		
		r1 = { r.verts[0].index, r.verts[1].index }
		r2 = { r.verts[1].index, r.verts[0].index }
		
		try:
			edges.remove(r1);
		except:
			pass

		try:
			edges.remove(r2);
		except:
			pass
								
for e in edges:
	print( e )
"""

def save_mesh(filepath, mesh, obj): 
# , use_normals=True, use_uv_coords=True, use_colors=True):
	import os
	import bpy

	def rvec3d(v):
		return round(v[0], 6), round(v[1], 6), round(v[2], 6)

#    def rvec2d(v):
#        return round(v[0], 6), round(v[1], 6)
#
#    if use_uv_coords and mesh.uv_layers:
#        active_uv_layer = mesh.uv_layers.active.data
#    else:
#        use_uv_coords = False
#
#    if use_colors and mesh.vertex_colors:
#        active_col_layer = mesh.vertex_colors.active.data
#    else:
#        use_colors = False

	active_col_layer = None

	if mesh.vertex_colors:
		active_col_layer = mesh.vertex_colors.active.data

	# in case
	# color = uvcoord = uvcoord_key = normal = normal_key = None
	color = None

	mesh_verts = mesh.vertices
	# vdict = {} # (index, normal, uv) -> new index
	vdict = [{} for i in range(len(mesh_verts))]
	ply_verts = []
	ply_faces = [[] for f in range(len(mesh.polygons))]
	vert_count = 0


	#prepare edges
	edges = []

	for e in mesh.edges:
		if e.use_freestyle_mark:
			edges.append( [ e.vertices[0], e.vertices[1] ] )

	#no need to remove shared edges with faces, we use use_freestyle_mark!

#    for p in mesh.polygons:
#        
#        l1 = p.loop_start
#        l2 = p.loop_start + p.loop_total - 1
#            
#        for l in range( l1, l2 ):
#            v1 = mesh.loops[l].vertex_index
#            v2 = mesh.loops[l+1].vertex_index
#            try:
#                edges.remove( [ v1,v2 ] )
#            except:
#                pass
#            try:
#                edges.remove( [ v2,v1 ] )
#            except:
#                pass
#            
#        v1 = mesh.loops[l1].vertex_index
#        v2 = mesh.loops[l2].vertex_index
#
#        try:
#            edges.remove( [ v1,v2 ] )
#        except:
#            pass
#        try:
#            edges.remove( [ v2,v1 ] )
#        except:
#            pass

	for i, f in enumerate(mesh.polygons):

#        smooth = not use_normals or f.use_smooth
#        if not smooth:
#            normal = f.normal[:]
#            normal_key = rvec3d(normal)
#
#        if use_uv_coords:
#            uv = [
#                active_uv_layer[l].uv[:]
#                for l in range(f.loop_start, f.loop_start + f.loop_total)
#            ]
#
#        if use_colors:
#            col = [
#                active_col_layer[l].color[:]
#                for l in range(f.loop_start, f.loop_start + f.loop_total)
#            ]

		if active_col_layer:
			col = [
				active_col_layer[l].color[:]
				for l in range(f.loop_start, f.loop_start + f.loop_total)
			]

		pf = ply_faces[i]

		if f.use_freestyle_mark:
			pf.append(-len(f.vertices))
		else:
			pf.append(len(f.vertices))

		for j, vidx in enumerate(f.vertices):
			v = mesh_verts[vidx]

#            if smooth:
#                normal = v.normal[:]
#                normal_key = rvec3d(normal)
#
#            if use_uv_coords:
#                uvcoord = uv[j][0], uv[j][1]
#                uvcoord_key = rvec2d(uvcoord)
#            if use_colors:

			weight = 0.0
			for g in v.groups:
				if g.group == obj.vertex_groups.active.index:
					weight = obj.vertex_groups.active.weight(vidx)
					break

			if active_col_layer:
				color = col[j]
				color = (
					int(round(color[0] * 255.0)),
					int(round(color[1] * 255.0)),
					int(round(color[2] * 255.0)),
					int(round(weight * 255.0)),
				)
			else:
				color = ( 255,255,255, int(round(weight * 255.0)) )

			# key = normal_key, uvcoord_key, color
			key = color

			vdict_local = vdict[vidx]
			pf_vidx = vdict_local.get(key)  # Will be None initially

			if pf_vidx is None:  # Same as vdict_local.has_key(key)
				pf_vidx = vdict_local[key] = vert_count
				#ply_verts.append((vidx, normal, uvcoord, color))
				ply_verts.append((vidx, color))
				vert_count += 1

			pf.append(pf_vidx)

	#reindex edge verts, add verts if needed
	for e in edges:

		vidx = e[0]
		v = mesh_verts[vidx]

#        if smooth:
#            normal = v.normal[:]
#            normal_key = rvec3d(normal)
#        if use_uv_coords:
#            uvcoord = uv[j][0], uv[j][1]
#            uvcoord_key = rvec2d(uvcoord)
#        if use_colors:

		weight = 0.0
		for g in v.groups:
			if g.group == obj.vertex_groups.active.index:
				weight = obj.vertex_groups.active.weight(vidx)
				break
		color = col[j]
		color = (
			int(round(color[0] * 255.0)),
			int(round(color[1] * 255.0)),
			int(round(color[2] * 255.0)),
			int(round(weight * 255.0)),
		)

		# key = normal_key, uvcoord_key, color
		key = color

		vdict_local = vdict[vidx]
		pf_vidx = vdict_local.get(key)  # Will be None initially
		if pf_vidx is None:  # Same as vdict_local.has_key(key)
			pf_vidx = vdict_local[key] = vert_count
			#ply_verts.append((vidx, normal, uvcoord, color))
			ply_verts.append((vidx, color))
			vert_count += 1

		e[0] = pf_vidx

		vidx = e[1]
		
		v = mesh_verts[vidx]

#        if smooth:
#            normal = v.normal[:]
#            normal_key = rvec3d(normal)
#        if use_uv_coords:
#            uvcoord = uv[j][0], uv[j][1]
#            uvcoord_key = rvec2d(uvcoord)
#        if use_colors:

		weight = 0.0
		for g in v.groups:
			if g.group == obj.vertex_groups.active.index:
				weight = obj.vertex_groups.active.weight(vidx)
				break
		color = col[j]
		color = (
			int(round(color[0] * 255.0)),
			int(round(color[1] * 255.0)),
			int(round(color[2] * 255.0)),
			int(round(weight * 255.0)),
		)

		# key = normal_key, uvcoord_key, color
		key = color

		vdict_local = vdict[vidx]
		pf_vidx = vdict_local.get(key)  # Will be None initially
		if pf_vidx is None:  # Same as vdict_local.has_key(key)
			pf_vidx = vdict_local[key] = vert_count
			#ply_verts.append((vidx, normal, uvcoord, color))
			ply_verts.append((vidx, color))
			vert_count += 1

		e[1] = pf_vidx

	with open(filepath, "w", encoding="utf-8", newline="\n") as file:
		fw = file.write

		# Header
		# ---------------------------

		fw("ply\n")
		fw("format ascii 1.0\n")
		fw(
			f"comment Created by Blender {bpy.app.version_string} - "
			f"www.blender.org, source file: {os.path.basename(bpy.data.filepath)!r}\n"
		)

		fw(f"element vertex {len(ply_verts)}\n")
		fw(
			"property float x\n"
			"property float y\n"
			"property float z\n"
		)

#        if use_normals:
#            fw(
#                "property float nx\n"
#                "property float ny\n"
#                "property float nz\n"
#            )
#        if use_uv_coords:
#            fw(
#                "property float s\n"
#                "property float t\n"
#            )
#        if use_colors:

		fw(
			"property uchar red\n"
			"property uchar green\n"
			"property uchar blue\n"
			"property uchar alpha\n"
		)

		fw(f"element face {len(mesh.polygons) + len(edges)}\n")
		fw("property list uchar uint vertex_indices\n")

		fw("end_header\n")

		# Vertex data
		# ---------------------------

		for i, v in enumerate(ply_verts):
			fw("%.3f %.3f %.3f" % mesh_verts[v[0]].co[:])
			#if use_normals:
			#    fw(" %.2f %.2f %.2f" % v[1])
			#if use_uv_coords:
			#    fw(" %.3f %.3f" % v[2])
			#if use_colors:
			#fw(" %u %u %u %u" % v[3])
			fw(" %u %u %u %u" % v[1])
			fw("\n")

		# Face data
		# ---------------------------

		for pf in ply_faces:
			# we have len already in array of indices!
			# fw(f"{len(pf)}")
			for v in pf:
				fw(f"{v} ")
			fw("\n")

		# Edge data
		# ---------------------------

		for e in edges:
			fw(f"{2}")
			fw(f" {e[0]} {e[1]} ")
			fw("\n")            

		print(f"Writing {filepath!r} done")

	return {'FINISHED'}

def wr_str(fout,s,l):
	chunk = struct.pack('<'+str(l)+'s',bytearray(s))
	fout.append(chunk)
	return l

def wr_int(fout,i):
	chunk = struct.pack('<i',i)
	fout.append(chunk)
	return 4

def wr_ref(fout):
	chunk = []
	fout.append(chunk)
	return chunk

def wr_save(fout,path):

	def wr_save_file(fout,file):
		size = 0
		for i in fout:
			if type(i) is bytes:
				size += file.write(i)
			else:
				size += wr_save_file(i,file)
		return size

	file = open(path, 'wb')
	if not file:
		return -1
	size = wr_save_file(fout,file)
	file.close()
	return size

def save_msh(fout, msh, idx_dict):
	print("Saving Mesh Data");
	return 0

def save_cur(fout, cur, idx_dict):
	print("Saving Curve Data");
	return 0

def save_con(fout, con, idx_dict):
	print("Saving Constraint","(",con.type,")");

	size = 0

	if con.type == 'FOLLOW_PATH':
		size += wr_int(fout,1)
	elif con.type == 'IK':
		size += wr_int(fout,2)
	else:
		size += wr_int(fout,0)

	return size

def save_arm(fout, arm, pose, idx_dict):
	print("Saving Armature Data");

	bones = pose.bones

	size = 0
	num_bones = len(bones)

	# num of bones
	size += wr_int(fout,num_bones)

	bone_dict = dict()
	bone_idx = 0
	for b in bones:
		bone_dict[b] = bone_idx
		bone_idx += 1

	for b in bones:
		print("Saving Bone",b.name);
		
		if b.parent:
			size += wr_int(fout,bone_dict[b.parent])
		else:
			size += wr_int(fout,-1)

		num_con = len(b.constraints)

		# NUM OF CONSTRAINTS
		size += wr_int(fout,num_con)

		# offset to array of offsets to constraints
		con_arr_ofs = wr_ref(fout)
		size += 4

		child_idx = []
		for c in bone_dict:
			if c.parent == b:
				child_idx.append(bone_dict[c])

		num_child_bones = len(child_idx)
		size += wr_int(fout,num_child_bones)

		for o in idx_dict:
			if o.parent == b:
				child_idx.append(idx_dict[o])

		num_child_objs = len(child_idx) - num_child_bones
		size += wr_int(fout,num_child_objs)

		for c in child_idx:
			size += wr_int(fout,c)

		wr_int(con_arr_ofs, size)

		con_ofs_table = wr_ref(fout)
		size += 4*num_con

		for c in b.constraints:
			wr_int(con_ofs_table,size)
			size += save_con(fout,c,idx_dict)

	return size

def save_obj(fout, obj, idx_dict):
	print("Saving Object:",obj.name,"(",obj.type,")");

	size = 0

	# ----------------------------------
	# BEGIN OBJ HEADER (8 ints)

	type = 0
	if obj.type == 'MESH':
		type = 1
	elif obj.type == 'CURVE':
		type = 2
	elif obj.type == 'ARMATURE':
		type = 3

	# OBJ TYPE
	size += wr_int(fout,type)

	if obj.parent:
		# PARENT IDX
		size += wr_int(fout,idx_dict[obj.parent])
	else:
		# ROOT IDX
		size += wr_int(fout,-1)

	# PARENTING TYPE and extra info (parenting type dependent, always 3 ints)
	if obj.parent_type == 'OBJECT':
		size += wr_int(fout,1)
		size += wr_int(fout,-1)
		size += wr_int(fout,-1)
		size += wr_int(fout,-1)
	elif obj.parent_type == 'BONE':
		size += wr_int(fout,2)
		armature = obj.parent.data
		bone_idx = 0
		for b in armature.edit_bones:
			if b.name == obj.parent_bone:
				break;
			bone_idx+=1
		size += wr_int(fout,bone_idx)
		size += wr_int(fout,-1)
		size += wr_int(fout,-1)
	elif obj.parent_type == 'ARMATURE':
		size += wr_int(fout,3)
		size += wr_int(fout,-1)
		size += wr_int(fout,-1)
		size += wr_int(fout,-1)
	elif obj.parent_type == 'VERTEX':
		size += wr_int(fout,4)
		size += wr_int(fout,obj.parent_vertices[0])
		size += wr_int(fout,-1)
		size += wr_int(fout,-1)
	elif obj.parent_type == 'VERTEX_3':
		size += wr_int(fout,5)
		size += wr_int(fout,obj.parent_vertices[0])
		size += wr_int(fout,obj.parent_vertices[1])
		size += wr_int(fout,obj.parent_vertices[2])

	num_con = len(obj.constraints)

	# NUM OF CONSTRAINTS
	size += wr_int(fout,num_con)

	# offset to array of offsets to constraints
	con_arr_ofs = wr_ref(fout)
	size += 4

	# offset to obj data (type dependent)
	obj_data_ofs = wr_ref(fout)
	size += 4

	ch_idx = []
	for c in idx_dict:
		if c.parent == obj:
			ch_idx.append(idx_dict[c])

	# NUM OF CHILDREN
	size += wr_int(fout,len(ch_idx))

	# CHILDREN IDX ARRAY
	for c in ch_idx:
		size += wr_int(fout,c)

	# array of offsets into each constraint from beginning of object
	wr_int(con_arr_ofs,size)
	con_ofs = wr_ref(fout)
	size += 4*num_con

	# CONSTRAINTS
	for c in obj.constraints:
		wr_int(con_ofs,size)
		size += save_con(fout,c,idx_dict)

	wr_int(obj_data_ofs,size)

	# depending on object type, save_msh, save_cur or save_arm

	if obj.type == 'MESH':
		size += save_msh(fout,obj.data,idx_dict)
	elif obj.type == 'CURVE':
		size += save_cur(fout,obj.data,idx_dict)
	elif obj.type == 'ARMATURE':
		size += save_arm(fout,obj.data,obj.pose,idx_dict)

	return size

def save(
	operator,
	context,
	filepath=""
#    use_selection=False,
#    use_mesh_modifiers=True,
#    use_normals=True,
#    use_uv_coords=True,
#    use_colors=True,
#    global_matrix=None
):

	# CRUCIAL so being edited object mesh is updated!
	if bpy.ops.object.mode_set.poll():
		bpy.ops.object.mode_set(mode='OBJECT')


#	if use_selection:
#        obs = context.selected_objects
#    else:

	obs = context.scene.objects

	print("ALL OBJECTS=",len(obs))

	#create list of exportable objects:
	# all meshes and armatures

	# it may happen that mesh vertices are hooked to other objects or even individual bones in armatures
	# BUT WE SKIP IT, same results in more generic way can be atchieved by ARMATURE parent_type and vertex group weights


	obj_list = []

	for o in obs:

		# ensure object type
		if o.type != 'ARMATURE' and o.type != 'MESH' and o.type != 'CURVE':
			print(o.name, "skipping - unsupported type", o.type, "of", o.name)
			continue #unknown object type, sorry

		#ensure parent relation type
		p = o
		while p:
			if p.parent_type != 'VERTEX' and p.parent_type != 'VERTEX_3' and p.parent_type != 'OBJECT' and p.parent_type != 'BONE' and p.parent_type != 'ARMATURE':
				break
			p = p.parent
		if p:
			print(o.name, "skipping - unsupported parent relation", p.parent_type, "in", p.name)
			continue

		if o.parent_type == 'ARMATURE':
			if o.parent.type != 'ARMATURE':
				print("skipping - ARMATURE relation requires ARMATURE parent for",o.name)
				continue

		ok = True
		if o.parent_type == 'BONE':
			if o.parent.type != 'ARMATURE':
				print("skipping - BONE relation requires ARMATURE parent for",o.name)
				continue
			armature = o.parent.data
			for b in armature.edit_bones:
				if b.name == o.parent_bone:
					ok = False
					break
			if not ok:
				print("skipping - BONE not found in parent ARMATURE for",o.name)
				continue

		if o.parent_type == 'VERTEX':
			if o.parent.type != 'MESH':
				print("skipping - VERTEX relation requires MESH parent for",o.name)
				continue				
			mesh = o.parent.data
			size = len(mesh.vertices)
			if size <= o.parent_vertices[0]:
				print("skipping - VERTEX not found in parent MESH for",o.name)
				continue
			
		if o.parent_type == 'VERTEX_3':
			if o.parent.type != 'MESH':
				print("skipping - VERTEX_3 relation requires MESH parent for",o.name)
				continue				
			mesh = o.parent.data
			size = len(mesh.vertices)
			if size <= o.parent_vertices[0] or size <= o.parent_vertices[1] or size <= o.parent_vertices[2]:
				print("skipping - VERTEX not found in parent MESH for",o.name)
				continue

		#test constraints
		ok = True
		for c in o.constraints:
			if c.type != 'FOLLOW_PATH':
				ok = False
				print("skipping - unsupported constraint",c.type,"in",o.name)
				break

			if not c.target or c.target.type != 'CURVE':
				print("skipping - constraint target is not CURVE for in",o.name)
				ok = False
				break

		if not ok:
			continue #unknown constraint type, sorry

		# test bones
		ok = True
		if o.type == 'ARMATURE':
			armature = o.data
			bones = o.pose.bones
			for b in bones:
				for c in b.constraints:
					if c.type != 'FOLLOW_PATH' and c.type != 'IK':
						ok = False
						print("skipping - unsupported constraint",c.type,"in",o.name,".",b.name)
						break

					if c.type == 'FOLLOW_PATH':
						if not c.target or c.target.type != 'CURVE':
							print("skipping - constraint target is missing or is not CURVE for",o.name,".",b.name)
							ok = False
							break

					if c.type == 'IK':
						if not c.target or not c.pole_target:
							print("skipping - constraint target or pole target is missing for",o.name,".",b.name)
						
				if not ok:
					break
						
		if not ok:
			continue #unknown bone's constraint type, sorry

		obj_list.append(o)

	# now we have to check obj references
	# - parent, bone name exist

	ok = False
	while not ok:
		ok = True
		obj_check_list = []
		for o in obj_list:
			if o.parent:
				try:
					idx = obj_list.index(o.parent)
				except:
					print("skipping - skipped parent",o.parent.name,"of",o.name)
					ok = False
					continue

			for c in o.constraints:
				if c.type == 'FOLLOW_PATH':
					try:
						idx = obj_list.index(c.target)
					except:
						print("skipping - constraint target is skipped for in",o.name)
						ok = False
					if not ok:
						break

			if o.type == 'ARMATURE':
				armature = o.data
				bones = o.pose.bones
				for b in bones:
					for c in b.constraints:
						if c.type == 'FOLLOW_PATH':
							try:
								idx = obj_list.index(c.target)
							except:
								print("skipping - constraint target is skipped for",o.name,".",b.name)
								ok = False
								break

						if c.type == 'IK':
							try:
								idx = obj_list.index(c.target)
							except:
								print("skipping - constraint target is skipped for",o.name,".",b.name)
								ok = False
								break
							try:
								idx = obj_list.index(c.pole_target)
							except:
								print("skipping - constraint pole_target is skipped for",o.name,".",b.name)
								ok = False
								break
						
					if not ok:
						break

			# passed, add to next pass check
			obj_check_list.append(o)

		obj_list = obj_check_list
			
	print("-------------------------")

	obj_dict = dict()
	obj_num = 0
	for o in obj_list:
		obj_dict[o] = obj_num
		obj_num += 1

	#fout = open(filepath, 'wb')
	#if not fout:
	#	return {'FINISHED'}

	fout = []
	fpos = 0

	wr_str(fout,b'AKM-1000',8)

	# placeholder for scene bytes to read
	size_of_scene_data = wr_ref(fout)

	# from now (after 12 bytes) we start accumulating fpos	

	# num of objects in scene
	fpos += wr_int(fout,len(obj_list))

	# placeholder for array of offsets to each object
	obj_ofs = wr_ref(fout)
	fpos += 4 * obj_num

	for o in obj_list:
		wr_int(obj_ofs,fpos)
		fpos += save_obj(fout,o,obj_dict)

	wr_int(size_of_scene_data, fpos)

	size = wr_save(fout,filepath)

	if size != fpos + 12:
		print("EXPORT ERROR!")
		return {'ERROR'}

	print("EXPORT OK!")
	return {'FINISHED'}

	depsgraph = context.evaluated_depsgraph_get()
	bm = bmesh.new()

	for ob in obs:

#        if use_mesh_modifiers:
#            ob_eval = ob.evaluated_get(depsgraph)
#        else:

		ob_eval = ob

		try:
			me = ob_eval.to_mesh()
		except RuntimeError:
			continue

		me.transform(ob.matrix_world)
		bm.from_mesh(me)
		ob_eval.to_mesh_clear()

	mesh = bpy.data.meshes.new("TMP PLY EXPORT")

	#pray!
	bmesh.ops.triangulate(bm, faces=bm.faces)
	#, quad_method=0, ngon_method=0)

	bm.to_mesh(mesh)
	bm.free()

#    if global_matrix is not None:
#        mesh.transform(global_matrix)
#    if use_normals:
#        mesh.calc_normals()

	obj = context.active_object

	ret = save_mesh(
		filepath,
		mesh,
		obj
		# use_normals=use_normals,
		# use_uv_coords=use_uv_coords,
		# use_colors=use_colors,
	)

	bpy.data.meshes.remove(mesh)

	return ret
