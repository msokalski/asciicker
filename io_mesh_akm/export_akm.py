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

def proc_msh(obj):

	# here we need to determine which vertices are necessary
	# - are referenced by polygons
	# - or are used as parents for other objects

	grp_dict = dict()
	idx = 0
	for g in obj.vertex_groups:
		grp_dict[g] = idx
		idx += 1

	# fmt_dict[sorted[g1,g2,g3]] = [v1,v2,...]
	fmt_dict = dict()

	# construct fromats from sorted groups
	for v in msh.vertices:
		f = []
		for g in v.groups:
			f.append(grp_dict[g])
		f.sort()
		if not f in fmt_dict:
			fmt_dict[f] = [v]
		else:
			fmt_dict[f].append(v)

	# sort vertices by fmt index
	vtx_dict = dict()
	idx = 0
	for f in fmt_dict:
		for v in f:
			vtx_dict[v] = idx
			idx += 1

	return [ obj, grp_dict, fmt_dict, vtx_dict ]

def save_msh(fout, processed_mesh, idx_dict):
	print("Saving Mesh Data");

	size = 0

	obj = processed_mesh[0]
	msh = obj.data

	grp_dict = processed_mesh[1]
	fmt_dict = processed_mesh[2]
	vtx_dict = processed_mesh[3]

	# num texture coords
	size += wr_int(fout, len(msh.uv_layers))

	# num vertex colors
	size += wr_int(fout, len(msh.vertex_colors))

	# num vertex_groups (weight channels)
	size += wr_int(fout, len(grp_dict))
	# write offset to groups info
	# ...

	# num shape_keys (morph targets)
	keys = len(msh.shape_keys.key_blocks)
	size += wr_int(fout, keys)
	# write offset to keys info
	# ...

	# num verts
	size += wr_int(fout, len(msh.vertices))
	# write offset to vertex data (groupped by fmt)
	# ...

	# num free style edges
	edg_list = []
	for edge in msh.edges:
		if edge.use_free_style:
			edge_list.append(edge)
	size += wr_int(fout,len(edg_list))
	# write offset to free style edges data
	# ...

	# num polygons (we don't triangulate, it should be done after all verts are transformed)
	size += wr_int(fout, len(msh.polygons))
	# write offset to polygon data
	# ...

	# HERE WE CAN STILL MAKE USE OF "tail[1]" array
	# ...

	# GROUPS INFO
	for g in grp_dict:
		bone_idx = -1
		if obj.parent and obj.parent.type == 'ARMATURE':
			for b in obj.parent.pose.bones:
				if b.name == g.name:
					bone_idx = idx_dict[b]
					break
		size += wr_int(fout,bone_idx)

	# KEYS INFO
	print("EVALTIME:",msh.eval_time)
	print("REFERENVE_KEY",msh.shape_keys.reference_key)
	print("USER_RELATIVE",msh.shape_keys.use_relative)
	for k in msh.shape_keys.key_blocks:
		# per key
		print("NAME:", k.name)
		print("INTERPOLATION:", k.interpolation)
		print("MUTE:", k.mute)
		print("RELATIVE_TO:", k.relative_k.name)
		print("SLIDER_MAX:", k.slider_max)
		print("SLIDER_MIN:", k.slider_min)
		print("SLIDER_VAL:", k.value)
		print("VERTEX_GROUP:", k.vertex_group)

	# VERTEX DATA
	for f in fmt_dict:
		# write format length (num of used vertex_groups)
		size += wr_int(fout,len(f))
		for g in f:
			# write vertex group index
			size += wr_int(fout,g) 

		vtx_sublist = fmt_dict[f]
		# num of vertices in this format
		size += wr_int(fout,len(vtx_sublist))
		for v in vtx_sublist:
			
			if keys == 0:
				# base_coords
				size += wr_fp3(fout, v.co)
			else:
				# ALL shape_keys coords (should we exclude first 'basis')?
				for k in msh.shape_keys.key_blocks:
					size += wr_fp3(fout,k.data[v.index].co)

			# weights from groups included in format
			for g in f:
				size += wr_flt(fout,obj.vertex_groups[g].weight(v.index))

	# FREE STYLE EDGES
	for edge in edg_list:
		flags = 0
		if edge.use_freestyle_mark: #MANDATORY!
			mat_and_flags |= 1<<16
		if not edge.use_edge_sharp:
			mat_and_flags |= 2<<16
		if edge.select:
			mat_and_flags |= 4<<16
		if edge.hide:
			mat_and_flags |= 8<<16
		if edge.use_seam:
			mat_and_flags |= 16<<16
		if edge.is_loose:
			mat_and_flags |= 32<<16
		size += wr_int(fout, flags)
		size += wr_int(fout,vtx_dict[edge.vertices[0]])
		size += wr_int(fout,vtx_dict[edge.vertices[1]])


	# POLYGON DATA
	for poly in msh.polygons:
		# poly poly.material_index (0..32767)
		mat_and_flags = poly.material_index
		# we use upper 16 bits for flags:
		if poly.use_freestyle_mark:
			mat_and_flags |= 1<<16
		if poly.use_smooth:
			mat_and_flags |= 2<<16
		if poly.select:
			mat_and_flags |= 4<<16
		if poly.hide:
			mat_and_flags |= 8<<16

		size += wr_int(fout, mat_and_flags)

		# todo: we should rotate loop indices in nicest possible way
		# this is to help build a nice 'fan' if engine can't triangulate dynamicaly
		# estimate/get polygon normal, cast vertices onto plane perpendicular to it
		# choose vertex that can 'see' all others in most possible monotonic angle increments
		# (use 'worth' factor proportional to sum of min(ith_angle, 360/(N-1))

		size += wr_int(fout,poly.loop_total)
		for loop in range(poly.loop_start,poly.loop_start+poly.loop_total):
			size += wr_int(vtx_dict[msh.loops[loop]])
			for uvl in msh.uv_layers:
				size += wr_fp2(fout,uvl.data[loop].uv)
			for col in msh.vertex_colors:
				size += wr_fp4(fout,col.data[loop].color)
	return size

def save_emp(fout, emp, idx_dict):


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

def save_tfm(fout, o, delta):
	size = 0
	print('ROTATION',o.rotation_mode);
	p = o.delta_location if delta else o.location
	s = o.delta_scale if delta else o.scale
	if o.rotation_mode == 'QUATERNION':
		r = o.delta_rotation_quaternion if delta else o.rotation_quaternion
		chunk = struct.pack('<fffifffffff',p[0],p[1],p[2],1,r[0],r[1],r[2],r[3],s[0],s[1],s[2])
	elif o.rotation_mode == 'AXIS_ANGLE':
		r = o.delta_rotation_axis_angle if delta else o.rotation_axis_angle
		chunk = struct.pack('<fffifffffff',p[0],p[1],p[2],2,r[0],r[1],r[2],r[3],s[0],s[1],s[2])
	elif o.rotation_mode == 'XYZ':
		r = o.delta_rotation_euler if delta else o.rotation_euler
		chunk = struct.pack('<fffifffffff',p[0],p[1],p[2],3,r[0],r[1],r[2],0,s[0],s[1],s[2])
	elif o.rotation_mode == 'YZX':
		r = o.delta_rotation_euler if delta else o.rotation_euler
		chunk = struct.pack('<fffifffffff',p[0],p[1],p[2],4,r[0],r[1],r[2],0,s[0],s[1],s[2])
	elif o.rotation_mode == 'ZXY':
		r = o.delta_rotation_euler if delta else o.rotation_euler
		chunk = struct.pack('<fffifffffff',p[0],p[1],p[2],5,r[0],r[1],r[2],0,s[0],s[1],s[2])
	elif o.rotation_mode == 'ZYX':
		r = o.delta_rotation_euler if delta else o.rotation_euler
		chunk = struct.pack('<fffifffffff',p[0],p[1],p[2],6,r[0],r[1],r[2],0,s[0],s[1],s[2])
	elif o.rotation_mode == 'YZX':
		r = o.delta_rotation_euler if delta else o.rotation_euler
		chunk = struct.pack('<fffifffffff',p[0],p[1],p[2],7,r[0],r[1],r[2],0,s[0],s[1],s[2])
	elif o.rotation_mode == 'ZXY':
		r = o.delta_rotation_euler if delta else o.rotation_euler
		chunk = struct.pack('<fffifffffff',p[0],p[1],p[2],8,r[0],r[1],r[2],0,s[0],s[1],s[2])
	else:
		chunk = struct.pack('<fffifffffff',0,0,0,0,0,0,0,0,0,0,0)

	fout.append(chunk)
	return 44


def save_arm(fout, arm, pose, idx_dict):
	print("Saving Armature Data");

	size = 0

	# export bones in exact idx_dict order
	bones = sorted(pose.bones, key=lambda b: idx_dict[b])

	num_bones = len(bones)

	# num of root bones
	num_roots = 0
	for b in bones:
		if b.parent:
			break
		num_roots += 1
	size += wr_int(fout,num_roots)

	# num of bones
	size += wr_int(fout,num_bones)

	# bone offs array
	bone_ofs = wr_ref(fout)
	size += 4*num_bones

	for b in bones:
		print("Saving Bone",b.name);

		wr_int(bone_ofs, size)
		
		# parent bone index
		if b.parent:
			size += wr_int(fout,idx_dict[b.parent])
		else:
			size += wr_int(fout,-1)

		first_child = -1
		num_children = 0
		for c in bones:
			if c.parent == b:
				num_children +=1
				if idx_dict[c] < first_child or first_child < 0:
					first_child = idx_dict[c]

		# FIRST CHILD INDEX
		size += wr_int(fout,first_child)

		# NUM OF CHILDREN
		size += wr_int(fout,num_children)

		size += save_tfm(fout, b, False)

		# BONE'S DATA (here, directly without offset)
		# ...

		num_con = len(b.constraints)

		# NUM OF CONSTRAINTS
		size += wr_int(fout,num_con)

		# array of offsets to constraints
		con_ofs = wr_ref(fout)
		size += 4*num_con

		# CONSTRAINTS
		for c in b.constraints:
			wr_int(con_ofs,size)
			size += save_con(fout,c,idx_dict)

		# wr_int(obj_data_ofs,size)

	return size

def save_obj(fout, obj, idx_dict, obj_list, msh_dict):
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
	elif obj.type == 'EMPTY':
		type = 4

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
		bone_idx = -1
		print("bone lookup for",obj.parent_bone)
		for b in obj.parent.pose.bones:
			print("maybe",b.name)
			if b.name == obj.parent_bone:
				bone_idx = idx_dict[b]
				break

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

	#find smallest index of child object
	first_child = -1
	num_children = 0
	for c in obj_list:
		if c.parent == obj:
			num_children += 1
			if idx_dict[c] < first_child or first_child < 0:
				first_child = idx_dict[c]

	# FIRST CHILD INDEX
	size += wr_int(fout,first_child)

	# NUM OF CHILDREN
	size += wr_int(fout,num_children)

	size += save_tfm(fout, obj, False)
	size += save_tfm(fout, obj, True)

	# offset to obj data (type dependent)
	obj_data_ofs = wr_ref(fout)
	size += 4

	num_con = len(obj.constraints)

	# NUM OF CONSTRAINTS
	size += wr_int(fout,num_con)

	# array of offsets to constraints
	con_ofs = wr_ref(fout)
	size += 4*num_con

	# CONSTRAINTS
	for c in obj.constraints:
		wr_int(con_ofs,size)
		size += save_con(fout,c,idx_dict)

	wr_int(obj_data_ofs,size)

	# depending on object type, save_msh, save_cur or save_arm

	if obj.type == 'MESH':
		size += save_msh(fout,msh_dict[obj],idx_dict)
	elif obj.type == 'CURVE':
		size += save_cur(fout,obj.data,idx_dict)
	elif obj.type == 'ARMATURE':
		size += save_arm(fout,obj.data,obj.pose,idx_dict)
	elif obj.type == 'EMPTY':
		size += save_emp(fout,obj.data,idx_dict)

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

			print("bone lookup for",o.parent_bone)

			ok = False
			for b in o.parent.pose.bones:
				print("maybe",b.name)
				if b.name == o.parent_bone:
					ok = True
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

			# ensure target is not descendant
			t = c.target
			while t:
				if t == o:
					print("WARNING - constraint target points to descendant",o.name,"->",c.target)
					break
				t=t.parent

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
						print("SKIPPING - unsupported constraint",c.type,"in",o.name,".",b.name)
						break

					if c.type == 'FOLLOW_PATH':
						if not c.target or c.target.type != 'CURVE':
							print("skipping - constraint target is missing or is not CURVE for",o.name,".",b.name)
							ok = False
							break
						# ensure target is not descendant
						t = c.target
						while t:
							if t == o:
								print("WARNING - constraint target points to descendant",o.name,".",b.name,"->",c.target)
								break
							t=t.parent

					if c.type == 'IK':
						if not c.target:
							print("SKIPPING - constraint target is missing for",o.name,".",b.name)
							ok = False
							break
						elif not c.pole_target:
							print("SKIPPING - constraint pole_target is missing for",o.name,".",b.name)
							ok = False
							break
						# warn if target is not descendant of bone
						t = c.target
						while t:
							if t == o:
								print("WARNING - constraint target points to descendant",o.name,".",b.name,"->",c.target)
								break
							t=t.parent
						# warn if pole_target is descendant of bone
						t = c.pole_target
						while t:
							if t == o:
								print("WARNING - constraint pole_target points to descendant",o.name,".",b.name,"->",c.pole_target)
								break
							t=t.parent
						
				if not ok:
					break
						
		if not ok:
			continue #unknown bone's constraint type, sorry

		obj_list.append(o)

	ok = False
	while not ok:
		ok = True
		obj_check_list = []
		for o in obj_list:
			if o.parent:
				try:
					idx = obj_list.index(o.parent)
				except:
					print("SKIPPING - skipped parent",o.parent.name,"of",o.name)
					ok = False
					continue

			for c in o.constraints:
				if c.type == 'FOLLOW_PATH':
					try:
						idx = obj_list.index(c.target)
					except:
						print("SKIPPING - constraint target is skipped for in",o.name)
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
								print("SKIPPING - constraint target is skipped for",o.name,".",b.name)
								ok = False
								break

						if c.type == 'IK':
							try:
								idx = obj_list.index(c.target)
							except:
								print("SKIPPING - constraint target is skipped for",o.name,".",b.name)
								ok = False
								break
							try:
								idx = obj_list.index(c.pole_target)
							except:
								print("SKIPPING - constraint pole_target is skipped for",o.name,".",b.name)
								ok = False
								break
						
					if not ok:
						break

			# passed, add to next pass check
			obj_check_list.append(o)

		obj_list = obj_check_list
			
	print("-------------------------")

	def sort_obj(obj_curr,obj_root,obj_desc):
		i1 = len(obj_root)
		for o in obj_desc:
			if o.parent == obj_curr:
				obj_root.append(o)
		i2 = len(obj_root)
		for i in range(i1,i2):
			sort_obj(obj_root[i],obj_root,obj_desc)
		return i2-i1

	obj_root = []
	root_num = sort_obj(None,obj_root,obj_list)

	if len(obj_root) != len(obj_list):
		print("SORT ERROR")
	obj_list = obj_root

	obj_dict = dict()
	obj_num = 0
	for o in obj_list:
		obj_dict[o] = obj_num
		obj_num += 1

	msh_dict = dict()

	# similary to sorting objects we need to sort bones in each armature
	# and add them right into obj_dict (but keyed with PoseBone and with index value local in its armature)
	# it must be done prior to saving anything so we have access to
	# reindexed dictionary of everything
	for o in obj_list:
		if o.type == 'ARMATURE':
			bone_root = []
			bone_root_num = sort_obj(None,bone_root,o.pose.bones)
			if len(bone_root) != len(o.pose.bones):
				print("SORT ERROR")
			bone_num = 0
			for b in o.pose.bones:
				obj_dict[b] = bone_num
				bone_num += 1
		if o.type == 'MESH':
			# time to process meshes and store them in mesh_dict[obj]
			# so writing VERTEX parenting can use reindexed vertices
			msh_dict[o] = proc_msh(o)

	fout = []
	fpos = 0

	wr_str(fout,b'AKM-1000',8)

	# placeholder for scene bytes to read
	size_of_scene_data = wr_ref(fout)

	# from now (after 12 bytes) we start accumulating fpos	

	# num of root objects
	fpos += wr_int(fout,root_num)

	# num of all objects in scene
	fpos += wr_int(fout,obj_num)

	# placeholder for array of offsets to each object
	obj_ofs = wr_ref(fout)
	fpos += 4 * obj_num

	for o in obj_list:
		wr_int(obj_ofs,fpos)
		fpos += save_obj(fout,o,obj_dict,obj_list,msh_dict)

	wr_int(size_of_scene_data, fpos)

	size = wr_save(fout,filepath)

	if size != fpos + 12:
		print("EXPORT ERROR!")
		return {'FINISHED'}

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
