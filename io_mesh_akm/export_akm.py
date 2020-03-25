
import os
import types
import struct
import bpy
import bmesh


def wr_str(fout,s):
	chunk = (s + '\0').encode('utf8')
	fout.append(chunk)
	return len(chunk)

def wr_int(fout,i):
	chunk = struct.pack('<i',i)
	fout.append(chunk)
	return 4

def wr_col(fout,f):
	r = int(round(f[0] * 255.0))
	g = int(round(f[1] * 255.0))
	b = int(round(f[2] * 255.0))
	a = int(round(f[3] * 255.0))
	chunk = struct.pack('<I', r | (g<<8) | (b<<16) | (a<<24))
	fout.append(chunk)
	return 4

def wr_fp2(fout,f):
	chunk = struct.pack('<ff',f[0],f[1])
	fout.append(chunk)
	return 8

def wr_fp3(fout,f):
	chunk = struct.pack('<fff',f[0],f[1],f[2])
	fout.append(chunk)
	return 12

def wr_fp4(fout,f):
	chunk = struct.pack('<ffff',f[0],f[1],f[2],f[3])
	fout.append(chunk)
	return 16

def wr_mtx(fout,f):
	chunk = struct.pack('<ffffffffffffffff',
						f[0][0],f[0][1],f[0][2],f[0][3], 
						f[1][0],f[1][1],f[1][2],f[1][3], 
						f[2][0],f[2][1],f[2][2],f[2][3], 
						f[3][0],f[3][1],f[3][2],f[3][3])
	fout.append(chunk)
	return 64

def wr_flt(fout,f):
	chunk = struct.pack('<f',f)
	fout.append(chunk)
	return 4

def wr_ref(fout):
	chunk = []
	fout.append(chunk)
	return chunk

import zlib

def wr_save(data,path):

	# b'AKM-1000' 8 bytes header
	# placeholder for UNCOMPRESSED scene bytes to allocate (4 bytes)
	# placeholder for COMPRESSED bytes to read from file (4 bytes)

	press = zlib.compressobj(zlib.Z_DEFAULT_COMPRESSION, zlib.DEFLATED, -15)

	def wr_press(press,fin,fout):
		size = (0,0)
		for i in fin:
			if type(i) is bytes:
				o = press.compress(i)
				fout.append(o)
				size = (size[0]+len(i),size[1]+len(o))
			else:
				delta = wr_press(press,i,fout)
				size = (size[0]+delta[0],size[1]+delta[1])
		return size

	def wr_flush(press,size,fout):
		o = press.flush()
		fout.append(o)
		size = (size[0],size[1]+len(o))
		return size

	fout = []
	size = wr_press(press,data,fout)
	size = wr_flush(press,size,fout)

	file = open(path, 'wb')
	if not file:
		return -1

	file.write( struct.pack('<8s', b'AKM-1000') )
	file.write( struct.pack('<I', size[0]) )
	file.write( struct.pack('<I', size[1]) )

	for c in fout:
		file.write(c)

	file.close()
	return size[0]

def proc_msh(obj):

	# here we need to determine which vertices are necessary
	# - are referenced by polygons
	# - or are used as parents for other objects

	grp_dict = dict()
	idx = 0
	for g in obj.vertex_groups: # g is VertexGroup
		grp_dict[g] = idx
		idx += 1

	# fmt_dict[sorted[g1,g2,g3]] = [v1,v2,...]
	fmt_dict = dict()

	# construct fromats from sorted groups
	for v in obj.data.vertices:

		# split f into 2 parts:
		# - first must include sorted groups with active bones
		# - second one must contain all other groups sorted
		f1 = []
		f2 = []
		for g in v.groups: # g is VertexGroupElement, g.group is index to msh.vertex_groups!
			group = obj.vertex_groups[g.group]
			# check if g has active bone
			active = False
			if obj.parent and obj.parent.type == 'ARMATURE':
				for b in obj.parent.pose.bones:
					if b.name == group.name:
						active = True
						break

			if active: 
				f1.append(grp_dict[ group ])
			else:
				f2.append(grp_dict[ group ])

		f = tuple(sorted(f1)) + tuple(sorted(f2))

		if not f in fmt_dict:
			fmt_dict[f] = [v]
		else:
			fmt_dict[f].append(v)

	# sort vertices by fmt index
	vtx_dict = dict()
	idx = 0
	for f in fmt_dict:
		for v in fmt_dict[f]:
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
	groups_offs = wr_ref(fout)
	size += 4

	# num shape_keys (morph targets)
	keys = len(msh.shape_keys.key_blocks) if msh.shape_keys else 0
	size += wr_int(fout, keys)
	# write offset to keys info
	shapes_offs = wr_ref(fout)
	size += 4

	# num verts
	size += wr_int(fout, len(msh.vertices))
	# write offset to vertex data (groupped by fmt)
	vertex_offs = wr_ref(fout)
	size += 4

	# num edges
	edg_list = [] # just in case we'd like to sort'em
	for edge in msh.edges:
		edg_list.append(edge)
	size += wr_int(fout,len(edg_list))
	# write offset edges data
	edge_offs = wr_ref(fout)
	size += 4

	ply_list = [] # just in case we'd like to sort'em
	for poly in msh.polygons:
		ply_list.append(poly)
	# num polygons (we don't triangulate, it should be done after all verts are transformed)
	size += wr_int(fout, len(ply_list))
	# write offset to polygon data
	poly_offs = wr_ref(fout)
	size += 4

	# HERE WE CAN STILL MAKE USE OF "tail[1]" array
	# let's use this for material index array
	# size += wr_int(fout, len(msh.materials))
	# for m in msh.materials:
	size += wr_int(fout, 0)

	# GROUPS INFO
	wr_int(groups_offs,size)
	for g in grp_dict:
		bone_idx = -1
		if obj.parent and obj.parent.type == 'ARMATURE':
			for b in obj.parent.pose.bones:
				if b.name == g.name:
					bone_idx = idx_dict[b]
					break
		size += wr_int(fout,bone_idx)

	# SHAPES INFO
	wr_int(shapes_offs,size)
	if msh.shape_keys:
		shp_dict = dict()
		key_idx = 0
		for k in msh.shape_keys.key_blocks:
			shp_dict[k] = key_idx
			key_idx += 1

		size += wr_flt(fout,msh.shape_keys.eval_time)
		size += wr_int(fout,shp_dict[msh.shape_keys.reference_key])
		if msh.shape_keys.use_relative:
			size += wr_int(fout,1)
		else:
			size += wr_int(fout,0)
		for k in msh.shape_keys.key_blocks:
			interp = 0
			if k.interpolation == 'KEY_LINEAR':
				interp = 1
			elif k.interpolation == 'KEY_CARDINAL':
				interp = 2
			elif k.interpolation == 'KEY_CATMULL_ROM':
				interp = 3
			elif k.interpolation == 'KEY_BSPLINE':
				interp = 4

			size += wr_int(fout,interp)
			if k.mute:
				size += wr_int(fout,1)
			else:
				size += wr_int(fout,0)

			size += wr_int(fout, shp_dict[k.relative_key])
			size += wr_flt(fout, k.slider_max)
			size += wr_flt(fout, k.slider_min)
			size += wr_flt(fout, k.value)

			grp = None
			for g in obj.vertex_groups:
				if g.name == k.vertex_group:
					grp = g
					break
			if grp:
				size += wr_int(fout, grp_dict[grp])
			else:
				size += wr_int(fout, -1)
	else:
		size += wr_flt(fout,0.0)
		size += wr_int(fout,-1)
		size += wr_int(fout,0)

	# VERTEX DATA
	wr_int(vertex_offs,size)
	print("NUM FORMATS:",len(fmt_dict))
	for f in fmt_dict:
		vtx_sublist = fmt_dict[f]
		print("  NUM VERTS:",len(vtx_sublist),", fmt:",f)
		# num of vertices in this format
		size += wr_int(fout,len(vtx_sublist))

		# write format length (num of used vertex_groups)
		size += wr_int(fout,len(f))
		for g in f:
			# write vertex group index
			size += wr_int(fout,g) 

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

	# EDGE DATA
	wr_int(edge_offs,size)
	print("NUM EDGES:",len(edg_list))
	for edge in edg_list:
		flags = 0
		if edge.use_freestyle_mark:
			flags |= 1<<16
		if not edge.use_edge_sharp:
			flags |= 2<<16
		if edge.select:
			flags |= 4<<16
		if edge.hide:
			flags |= 8<<16
		if edge.use_seam:
			flags |= 16<<16
		if edge.is_loose:
			flags |= 32<<16
		size += wr_int(fout, flags)
		size += wr_int(fout,vtx_dict[ msh.vertices[edge.vertices[0]] ])
		size += wr_int(fout,vtx_dict[ msh.vertices[edge.vertices[1]] ])


	# POLY DATA
	wr_int(poly_offs,size)
	print("NUM POLYS:",len(ply_list))
	for poly in ply_list:
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
			size += wr_int(fout,vtx_dict[ msh.vertices[ msh.loops[loop].vertex_index] ])
			size += wr_int(fout,msh.loops[loop].edge_index) # WARNING: in future it may require reindexing!
			for uvl in msh.uv_layers:
				size += wr_fp2(fout,uvl.data[loop].uv)
			for col in msh.vertex_colors:
				size += wr_col(fout,col.data[loop].color)
	return size

def save_emp(fout, obj):
	type = 0
	if obj.empty_draw_type == 'PLAIN_AXES':
		type = 1
	elif obj.empty_draw_type == 'ARROWS':
		type = 2
	elif obj.empty_draw_type == 'SINGLE_ARROW':
		type = 3
	elif obj.empty_draw_type == 'CUBE':
		type = 4
	elif obj.empty_draw_type == 'SPHERE':
		type = 5
	elif obj.empty_draw_type == 'CONE':
		type = 6
	elif obj.empty_draw_type == 'IMAGE':
		type = 7
	wr_int(fout, type)
	wr_flt(fout, obj.empty_draw_size)
	wr_fp2(fout, obj.empty_image_offset)

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

tm_slot = 0

def save_arm(fout, arm, pose, idx_dict):

	global tm_slot
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

		# tm_slot
		size += wr_int(fout, tm_slot)
		tm_slot += 1
		
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

def save_obj(fout, obj, idx_dict, obj_list, msh_dict, names):

	global tm_slot
	print("Saving Object:",obj.name,"(",obj.type,")");

	size = 0

	# write name offs and store name
	name_ofs = wr_ref(fout)
	size += 4

	names.append( (name_ofs,obj.name) )

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

	# tm_slot
	size += wr_int(fout, tm_slot)
	tm_slot += 1

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

	# parenting inverse
	print(len(obj.matrix_parent_inverse))
	size += wr_mtx(fout, obj.matrix_parent_inverse)

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
	global tm_slot

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

	# extra hints for allocating engine's buffers
	num_transforms = 0
	num_vertices = 0
	num_indices = 0

	# similary to sorting objects we need to sort bones in each armature
	# and add them right into obj_dict (but keyed with PoseBone and with index value local in its armature)
	# it must be done prior to saving anything so we have access to
	# reindexed dictionary of everything

	for o in obj_list:
		num_transforms += 1
		if o.type == 'ARMATURE':
			num_transforms += len(o.pose.bones)
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
			num_vertices += len(o.data.vertices)
			for poly in o.data.polygons:
				num_indices += poly.loop_total

	fout = []
	fpos = 0
	
	# every object and every bone will come with its own tm_slot index
	tm_slot = 0 # GLOBAL VARIABLE !!!!

	# placeholder for beginning of names block
	names_block_offset = wr_ref(fout)
	fpos += 4

	# hints first
	fpos += wr_int(fout,num_transforms)
	fpos += wr_int(fout,num_vertices)
	fpos += wr_int(fout,num_indices)

	# num of root objects
	fpos += wr_int(fout,root_num)

	# num of all objects in scene
	fpos += wr_int(fout,obj_num)

	# placeholder for array of offsets to each object
	obj_ofs = wr_ref(fout)
	fpos += 4 * obj_num

	names = [] # tuples ( wr_ref, string )
	for o in obj_list:
		wr_int(obj_ofs,fpos)
		fpos += save_obj(fout,o,obj_dict,obj_list,msh_dict,names)


	names_base = fpos
	wr_int(names_block_offset,names_base)

	for n in names:
		wr_int(n[0], fpos - names_base)
		fpos += wr_str(fout, n[1])

	size = wr_save(fout,filepath)

	if size != fpos or tm_slot != num_transforms:
		print("EXPORT ERROR!")
		return {'FINISHED'}

	print("EXPORT OK!")
	return {'FINISHED'}
