
import os
import types
import struct
import bpy
import bmesh

# -----------------------------------------------------------
# for use in blender for attributes discovery
def dump(obj):
   for attr in dir(obj):
	   if hasattr( obj, attr ):
		   print( "obj.%s = %s" % (attr, getattr(obj, attr)))
obj = bpy.context.active_object

# -----------------------------------------------------------

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

def wr_lin(fout,p,tilt,radius): # v3 and tilt+radius
	chunk = struct.pack('<fffff',p[0],p[1],p[2], tilt,radius)
	fout.append(chunk)
	return 20

def wr_bez(fout,l,p,r,tilt,radius): # v3 and tilt+radius
	chunk = struct.pack('<fffffffffff',l[0],l[1],l[2], p[0],p[1],p[2], r[0],r[1],r[2], tilt,radius)
	fout.append(chunk)
	return 44

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

def save_arm_mod(fout,obj,arm_mod,idx_dict):

	size = 0

	# armature object
	arm = None
	if arm_mod.object and arm_mod.object.type == 'ARMATURE':
		arm = arm_mod.object

	# ensure arm is on our export list!
	if arm and not arm in idx_dict:
		arm = None

	if arm:
		size += wr_int(fout, idx_dict[arm])
	else:
		size += wr_int(-1)

	infl_grp = -1
	idx = 0
	for g in obj.vertex_groups:
		if g.name == arm_mod.vertex_group:
			infl_grp = idx
			break
		idx += 1

	size += wr_int(fout, infl_grp)

	flags = 0
	if arm_mod.invert_vertex_group:
		flags |= 1<<16
	if arm_mod.use_bone_envelopes:
		flags |= 1<<17
	if arm_mod.use_deform_preserve_volume:
		flags |= 1<<18
	if arm_mod.use_vertex_groups:
		flags |= 1<<19

	size += wr_int(fout, flags)

	# big thing, if we have armature, write our vtxgrp to its bone idx mappings
	if arm:
		for g in obj.vertex_groups:
			bone_idx = -1
			for b in arm.pose.bones:
				if b.name == g.name:
					bone_idx = idx_dict[b]
					break
			size += wr_int(fout,bone_idx)

	return size

def save_hook_mod(fout,obj,hook_mod,idx_dict):

	size = 0

	if hook_mod.object and hook_mod.object in idx_dict:
		size += wr_int(fout, idx_dict[hook_mod.object])
		sub = -1
		if hook_mod.subtarget and hook_mod.object.type == 'ARMATURE':
			for b in hook_mod.object.pose.bones:
				if b.name == hook_mod.subtarget:
					sub = idx_dict[b]
					break;
		size += wr_int(fout, sub) # bone (subtarget)

	else:
		size += wr_int(fout, -1) # obj
		size += wr_int(fout, -1) # bone (subtarget)

	infl_grp = -1
	idx = 0
	for g in obj.vertex_groups:
		if g.name == hook_mod.vertex_group:
			infl_grp = idx
			break
		idx += 1
	size += wr_int(fout, infl_grp)
	
	type = 0
	if hook_mod.falloff_type == 'NONE':
		type = 1
	elif hook_mod.falloff_type == 'CURVE':
		type = 2
	elif hook_mod.falloff_type == 'SMOOTH':
		type = 3
	elif hook_mod.falloff_type == 'SPHERE':
		type = 4
	elif hook_mod.falloff_type == 'ROOT':
		type = 5
	elif hook_mod.falloff_type == 'INVERSE_SQUARE':
		type = 6
	elif hook_mod.falloff_type == 'SHARP':
		type = 7
	elif hook_mod.falloff_type == 'LINEAR':
		type = 8
	elif hook_mod.falloff_type == 'CONSTANT':
		type = 9

	if hook_mod.use_falloff_uniform:
		type |= 1<<16

	size += wr_int(fout, type)

	falloff_curve_offset = wr_ref(fout)
	size += 4

	size += wr_fp3(fout, hook_mod.center)
	size += wr_flt(fout, hook_mod.falloff_radius)
	size += wr_flt(fout, hook_mod.strength)

	size += wr_mtx(fout, hook_mod.matrix_inverse)

	size += wr_int(fout, len(hook_mod.vertex_indices))
	for v in hook_mod.vertex_indices:
		size += wr_int(fout,v)

	wr_int(falloff_curve_offset,12345) #size)
	# todo falloff_curve
	# ...

	return size

def save_mod(fout,obj,mod,idx_dict,names):

	size = 0

	# write name offs and store name
	name_ofs = wr_ref(fout)
	size += 4
	names.append( (name_ofs,mod.name) )

	type = 0
	if mod.type == 'ARMATURE':
		type = 1
	elif mod.type == 'HOOK':
		type = 2

	size += wr_int(fout,type)

	flags = 0
	if mod.show_viewport:
		flags |= 1<<16
	elif mod.show_render:
		flags |= 1<<17
	elif mod.show_in_editmode:
		flags |= 1<<18
	elif mod.show_on_cage:
		flags |= 1<<19

	size += wr_int(fout,flags)

	if type == 0:
		return size
		
	# MOD DATA

	if type==1:
		size += save_arm_mod(fout,obj,mod,idx_dict)
	elif type==2:
		size += save_hook_mod(fout,obj,mod,idx_dict)

	return size


def proc_msh(obj):

	# here we need to determine which vertices are necessary
	# - are referenced by polygons
	# - or are used as parents for other objects

	msh = obj.data

	grp_dict = dict()
	idx = 0
	for g in obj.vertex_groups: # g is VertexGroup
		grp_dict[g] = idx
		idx += 1

	# fmt_dict[sorted[g1,g2,g3]] = [v1,v2,...]
	fmt_dict = dict()

	# construct fromats from sorted groups
	for v in obj.data.vertices:
		#	# split f into 2 parts:
		#	# - first must include sorted groups with active bones
		#	# - second one must contain all other groups sorted
		#	f1 = []
		#	f2 = []
		#	for g in v.groups: # g is VertexGroupElement, g.group is index to msh.vertex_groups!
		#		group = obj.vertex_groups[g.group]
		#		# check if g has active bone
		#		active = False
		#		if obj.parent and obj.parent.type == 'ARMATURE':
		#			for b in obj.parent.pose.bones:
		#				if b.name == group.name:
		#					active = True
		#					break
		#		if active: 
		#			f1.append(grp_dict[ group ])
		#		else:
		#			f2.append(grp_dict[ group ])
		#	f = tuple(sorted(f1)) + tuple(sorted(f2))

		fk = []
		fg = []

		for g in v.groups:
			group = obj.vertex_groups[g.group]
			fg.append(grp_dict[ group ])

		key_idx = 0
		if msh.shape_keys:
			for k in msh.shape_keys.key_blocks:
				if v.co[0] != k.data[v.index].co[0] or v.co[1] != k.data[v.index].co[1] or v.co[2] != k.data[v.index].co[2]:
					fk.append(key_idx)
				key_idx += 1

		keys = len(fk)
		grps = len(fg)

		if keys>0xFFFF or grps>0xFFFF:
			return None

		f = tuple([keys|(grps<<16)])
		if len(fk):
			f = f + tuple(sorted(fk))
		if len(fg):
			f = f + tuple(sorted(fg))

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

def ply_rot(fan):

	num = len(fan)
	if num <= 3:
		return 0

	n = [0,0,0]

	v1 = fan[num-2]
	v2 = fan[num-1]

	for i in range(0,num-1):
		v3 = fan[i]
		n[0] += (v1[1]-v3[1])*(v2[2]-v3[2]) - (v1[2]-v3[2])*(v2[1]-v3[1])
		n[1] += (v1[2]-v3[2])*(v2[0]-v3[0]) - (v1[0]-v3[0])*(v2[2]-v3[2])
		n[2] += (v1[0]-v3[0])*(v2[1]-v3[1]) - (v1[1]-v3[1])*(v2[0]-v3[0])
		v1 = v2
		v2 = v3

	best_smallest = 0
	best_smallest_at = -1
	for rot in range(0,num):
		v1 = fan[(rot + num - 2) % num]
		v2 = fan[(rot + num - 1) % num]

		smallest = 0
		for i in range(0,num-2):
			v3 = fan[(rot + i) % num]
			nx = (v1[1]-v3[1])*(v2[2]-v3[2]) - (v1[2]-v3[2])*(v2[1]-v3[1])
			ny = (v1[2]-v3[2])*(v2[0]-v3[0]) - (v1[0]-v3[0])*(v2[2]-v3[2])
			nz = (v1[0]-v3[0])*(v2[1]-v3[1]) - (v1[1]-v3[1])*(v2[0]-v3[0])
			dot = nx*n[0] + ny*n[1] + nz*n[2]

			if dot < smallest or i == 0:
				smallest = dot

			v2 = v3

		if smallest > best_smallest or rot == 0:
			best_smallest = smallest
			best_smallest_at = rot

	rot = (best_smallest_at + num - 2) % num
	return rot

def save_shp(fout, dat, obj, names):
	shp_dict = dict()
	key_idx = 0
	for k in dat.shape_keys.key_blocks:
		shp_dict[k] = key_idx
		key_idx += 1

	size += wr_flt(fout,dat.shape_keys.eval_time)
	size += wr_int(fout,shp_dict[dat.shape_keys.reference_key])
	if dat.shape_keys.use_relative:
		size += wr_int(fout,1)
	else:
		size += wr_int(fout,0)
	for k in dat.shape_keys.key_blocks:

		# write name offs and store name
		name_ofs = wr_ref(fout)
		size += 4
		names.append( (name_ofs,k.name) )

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
		size += wr_flt(fout, k.slider_min)
		size += wr_flt(fout, k.slider_max)
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

def save_msh(fout, processed_mesh, idx_dict, names):

	size = 0

	obj = processed_mesh[0]
	msh = obj.data

	# write name offs and store name
	name_ofs = wr_ref(fout)
	size += 4
	names.append( (name_ofs,msh.name) )

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
	edg_list = sorted(msh.edges, key=lambda e: -int(e.use_freestyle_mark))
	edg_dict = dict()
	eidx = 0
	for e in edg_list:
		edg_dict[e] = eidx
		eidx += 1
	size += wr_int(fout,len(edg_list))
	# write offset edges data
	edge_offs = wr_ref(fout)
	size += 4

	# group polys by material index and split each group by use_freestyle_mark
	ply_list = sorted(msh.polygons, key=lambda p: 2*p.material_index-int(p.use_freestyle_mark))

	# num polygons (we don't triangulate, it should be done after all verts are transformed)
	size += wr_int(fout, len(ply_list))
	# write offset to polygon data
	poly_offs = wr_ref(fout)
	size += 4

	# MATERIALS!
	size += wr_int(fout, 0)

	# GROUPS INFO
	wr_int(groups_offs,size)
	for g in grp_dict:

		# write name offs and store name
		name_ofs = wr_ref(fout)
		size += 4
		names.append( (name_ofs,g.name) )

		bone_idx = -1 # confirmed, we need it (at least for 'ARMATURE' parenting type)
		if obj.parent and obj.parent.type == 'ARMATURE' and obj.parent_type == 'ARMATURE':
			for b in obj.parent.pose.bones:
				if b.name == g.name:
					bone_idx = idx_dict[b]
					break
		size += wr_int(fout,bone_idx)

	# SHAPES INFO
	wr_int(shapes_offs,size)
	if msh.shape_keys:
		save_shp(fout,msh,obj,names)

	# VERTEX DATA
	wr_int(vertex_offs,size)
	end_index = 0
	for f in fmt_dict:
		vtx_sublist = fmt_dict[f]

		# first vertex index that does not fit in this format
		end_index += len(vtx_sublist)
		size += wr_int(fout, end_index)

		# write format lengths (packed num of used keys & vertex_groups)
		# followed by all keys then groups indexes
		for i in f:
			size += wr_int(fout,i) 

		keys = range(1,1+f[0]&0xFFFF)
		grps = range(1+(f[0]&0xFFFF),1+(f[0]&0xFFFF)+((f[0]>>16)&0xFFFF))

		for v in vtx_sublist:

			# always write base_coords
			size += wr_fp3(fout, v.co)
			
			# shape_keys coords
			for j in keys:
				k = msh.shape_keys.key_blocks[f[j]]
				size += wr_fp3(fout,k.data[v.index].co)

			# weights from groups included in format
			for j in grps:
				size += wr_flt(fout,obj.vertex_groups[f[j]].weight(v.index))

	# EDGE DATA
	wr_int(edge_offs,size)
	for edge in edg_list:
		flags = 0
		if edge.use_freestyle_mark:
			flags |= 1<<16
		if not edge.use_edge_sharp:
			flags |= 1<<17
		if edge.select:
			flags |= 1<<18
		if edge.hide:
			flags |= 8<<19
		if edge.use_seam:
			flags |= 1<<20
		if edge.is_loose:
			flags |= 1<<21
		size += wr_int(fout, flags)
		size += wr_int(fout,vtx_dict[ msh.vertices[edge.vertices[0]] ])
		size += wr_int(fout,vtx_dict[ msh.vertices[edge.vertices[1]] ])


	# POLY DATA
	wr_int(poly_offs,size)
	for poly in ply_list:
		# poly poly.material_index (0..32767)
		mat_and_flags = poly.material_index
		# we use upper 16 bits for flags:
		if poly.use_freestyle_mark:
			mat_and_flags |= 1<<16
		if poly.use_smooth:
			mat_and_flags |= 1<<17
		if poly.select:
			mat_and_flags |= 1<<18
		if poly.hide:
			mat_and_flags |= 1<<19

		size += wr_int(fout, mat_and_flags)
		size += wr_int(fout,poly.loop_total)

		# we should rotate loop indices in nicest possible way
		# this is to help build a nice 'fan' if engine can't triangulate dynamicaly
		# estimate/get polygon normal, cast vertices onto plane perpendicular to it
		# choose vertex that can 'see' all others in most possible monotonic angle increments
		fan = []
		for loop in range(poly.loop_start,poly.loop_start+poly.loop_total):
			fan.append( msh.vertices[msh.loops[loop].vertex_index].co )
		rot = ply_rot(fan)

		for loop in range(poly.loop_start,poly.loop_start+poly.loop_total):
			loop_rot = (loop - poly.loop_start + rot) % poly.loop_total + poly.loop_start
			size += wr_int(fout,vtx_dict[ msh.vertices[msh.loops[loop_rot].vertex_index] ])
			size += wr_int(fout,edg_dict[ msh.edges[msh.loops[loop_rot].edge_index] ])
			for uvl in msh.uv_layers:
				size += wr_fp2(fout,uvl.data[loop_rot].uv)
			for col in msh.vertex_colors:
				size += wr_col(fout,col.data[loop_rot].color)
	return size

def save_emp(fout, obj):

	size = 0

	type = 0
	if obj.empty_display_type == 'PLAIN_AXES':
		type = 1
	elif obj.empty_display_type == 'ARROWS':
		type = 2
	elif obj.empty_display_type == 'SINGLE_ARROW':
		type = 3
	elif obj.empty_display_type == 'CUBE':
		type = 4
	elif obj.empty_display_type == 'SPHERE':
		type = 5
	elif obj.empty_display_type == 'CONE':
		type = 6
	elif obj.empty_display_type == 'IMAGE':
		type = 7
	elif obj.empty_display_type == 'CIRCLE':
		type = 8

	size += wr_int(fout, type)

	image_side = 0 # TODO! empty_image_side ['DOUBLE_SIDED', 'FRONT', 'BACK'], default 'DOUBLE_SIDED'
	size += wr_int(fout, image_side)

	image_depth = 0 # TODO! empty_image_depth ['DEFAULT', 'FRONT', 'BACK']
	size += wr_int(fout, image_depth)

	size += wr_flt(fout, obj.empty_display_size)
	size += wr_fp2(fout, obj.empty_image_offset)

	return size

def save_cur(fout, cur, idx_dict, names):
	size = 0

	# write name offs and store name
	name_ofs = wr_ref(fout)
	size += 4
	names.append( (name_ofs,cur.name) )

	size += wr_flt(fout,cur.eval_time)
	size += wr_flt(fout,cur.offset)
	size += wr_flt(fout,cur.path_duration)
	size += wr_flt(fout,cur.twist_smooth)

	twist_mode = 0
	if cur.twist_mode == 'Z_UP':
		twist_mode = 1
	elif cur.twist_mode == 'MINIMUM':
		twist_mode = 2
	elif cur.twist_mode == 'TANGENT':
		twist_mode = 3
	
	flags = 0

	if cur.use_path:
		flags |= 1 << 16
	if cur.use_path_follow:
		flags |= 1 << 17
	if cur.use_radius:
		flags |= 1 << 18
	if cur.use_stretch:
		flags |= 1 << 19

	twist_mode |= flags

	size += wr_int(fout,twist_mode)

	# num vertex_groups (weight channels)
	size += wr_int(fout, 0)
	# write offset to groups info
	groups_offs = wr_ref(fout)
	size += 4

	# num shape_keys (morph targets)
	keys = len(cur.shape_keys.key_blocks) if cur.shape_keys else 0
	size += wr_int(fout, keys)
	# write offset to keys info
	shapes_offs = wr_ref(fout)
	size += 4

	# num verts
	size += wr_int(fout, len(msh.vertices))

	# write offset to vertex data (groupped by fmt)
	vertex_offs = wr_ref(fout)
	size += 4

	# num splines
	size += wr_int(fout, len(msh.vertices))

	# write offset to spline data array
	spline_offs = wr_ref(fout)
	size += 4

	# MATERIALS!
	size += wr_int(fout, 0)

	# vertex_groups (none)
	wr_int(groups_offs,size)

	# SHAPES INFO
	wr_int(shapes_offs,size)
	if cur.shape_keys:
		save_shp(fout,cur,obj,names)

	# VERTEX DATA
	# 1. determine format for each spline separately!
	wr_int(vertex_offs,size)
	for s in cur.splines:
		fk = []
		key_idx = 0
		for k in cur.shape_keys.key_blocks:
			if s.type == 'BEZIER':
				for p in s.bezier_points
					if p.co[0] != k.data[p.index].co[0] or 
					p.co[1] != k.data[p.index].co[1] or 
					p.co[2] != k.data[p.index].co[2] or
					p.left[0] != k.data[p.index].left[0] or
					p.left[1] != k.data[p.index].left[1] or
					p.left[2] != k.data[p.index].left[2] or
					p.right[0] != k.data[p.index].right[0] or
					p.right[1] != k.data[p.index].right[1] or
					p.right[2] != k.data[p.index].right[2] or
					p.radius != k.data[p.index].tilt or
					p.tilt != k.data[p.index].tilt:
						fk.append(key_idx)
						break
					key_idx += 1
			else:
				for p in s.points
					if p.co[0] != k.data[p.index].co[0] or 
					p.co[1] != k.data[p.index].co[1] or 
					p.co[2] != k.data[p.index].co[2] or
					p.radius != k.data[p.index].tilt or
					p.tilt != k.data[p.index].tilt:
						fk.append(key_idx)
						break
					key_idx += 1

		size += wr_int(???) # end vertex

		keys_and_groups = len(fk) # keys_and_groups (keys in lower 16 bits)
		if s.type == 'BEZIER':
			keys_and_groups |= 1<<31 # add special flag to indicate bezier format
		size += wr_int(keys_and_groups)

		for f in fk:
			size += wr_int(f) # all keys first ...
		# ... then groups (none)

		if s.type == 'BEZIER':
			for p in s.bezier_points
				size += wr_bez(fout)









	wr_int(fout,len(cur.splines))
	for s in cur.splines:

		mat_and_flags = s.material_index
		if s.use_smooth:
			mat_and_flags |= 1 << 16
		if s.use_bezier_u:
			mat_and_flags |= 1 << 17
		if s.use_cyclic_u:
			mat_and_flags |= 1 << 18
		if s.use_endpoint_u:
			mat_and_flags |= 1 << 19

		type = 0 # [‘POLY’, ‘BEZIER’, ‘BSPLINE’, ‘CARDINAL’, ‘NURBS’]
		mat_and_flags |= type << 20

		tilt_interpolation = 0
		if s.tilt_interpolation == 'LINEAR':
			tilt_interpolation = 1
		elif s.tilt_interpolation == 'CARDINAL':
			tilt_interpolation = 2
		elif s.tilt_interpolation == 'BSPLINE':
			tilt_interpolation = 3
		elif s.tilt_interpolation == 'EASE':
			tilt_interpolation = 4

		mat_and_flags |= tilt_interpolation << 24

		radius_interpolation = 0
		if s.radius_interpolation == 'LINEAR':
			radius_interpolation = 1
		elif s.radius_interpolation == 'CARDINAL':
			radius_interpolation = 2
		elif s.radius_interpolation == 'BSPLINE':
			radius_interpolation = 3
		elif s.radius_interpolation == 'EASE':
			radius_interpolation = 4

		mat_and_flags |= radius_interpolation << 28

		size += wr_int(fout, mat_and_flags)
		size += wr_int(fout, s.resolution_u)

		# INDICES!


		if s.type == 'BEZIER':
			size += wr_int(fout, len(s.bezier_points)) # bezier
			for p in s.bezier_points:

				flags = 0
				if p.handle_left_type == 'FREE':
					flags |= 1
				elif p.handle_left_type == 'VECTOR':
					flags |= 2
				elif p.handle_left_type == 'ALIGNED':
					flags |= 3
				elif p.handle_left_type == 'AUTO':
					flags |= 4

				if p.handle_right_type == 'FREE':
					flags |= 1<<8
				elif p.handle_right_type == 'VECTOR':
					flags |= 2<<8
				elif p.handle_right_type == 'ALIGNED':
					flags |= 3<<8
				elif p.handle_right_type == 'AUTO':
					flags |= 4<<8

				if p.hide:
					flags |= 1<<16

				wr_int(fout,flags)

				# base
				wr_fp3(fout,p.left)
				wr_fp3(fout,p.co)
				wr_fp3(fout,p.right)
				wr_flt(fout,p.radius)
				wr_flt(fout,p.tilt)

				# num of keys something is different from base
				# indexes of these keys
				# data for these keys


		else:
			size += wr_int(fout, len(s.points)) # poly or nurbs


	wr_int(shape_keys,size)

	return size

def save_follow_path_con(fout, con, idx_dict):
	
	size = 0

	if con.target and con.target in idx_dict and con.target.type == 'CURVE':
		size += wr_int(fout, idx_dict[con.target])
	else:
		size += wr_int(fout, -1)
	
	forward = 0
	if con.forward_axis == 'FORWARD_X':
		forward = 1
	elif con.forward_axis == 'FORWARD_Y':
		forward = 2
	elif con.forward_axis == 'FORWARD_Z':
		forward = 3
	elif con.forward_axis == 'TRACK_NEGATIVE_X':
		forward = 4
	elif con.forward_axis == 'TRACK_NEGATIVE_Y':
		forward = 5
	elif con.forward_axis == 'TRACK_NEGATIVE_Z':
		forward = 6

	size += wr_int(fout, forward)

	up = 0
	if con.up_axis == 'UP_X':
		up = 1
	elif con.up_axis == 'UP_Y':
		up = 2
	elif con.up_axis == 'UP_Z':
		up = 3

	size += wr_int(fout, up)
	
	size += wr_flt(fout, con.offset)
	
	size += wr_flt(fout, con.offset_factor)

	flags = 0

	if con.use_curve_follow:
		flags |= 1<<16
	if con.use_curve_radius:
		flags |= 1<<17
	if con.use_fixed_location:
		flags |= 1<<18

	size += wr_int(fout, size)

	return size

def save_ik_con(fout, con, idx_dict):

	size = 0
	if con.target and con.target in idx_dict:
		size += wr_int(fout, idx_dict[con.target])
		sub = -1
		if con.subtarget and con.target.type == 'ARMATURE':
			for b in con.target.pose.bones:
				if b.name == con.subtarget:
					sub = idx_dict[b]
					break;
		size += wr_int(fout, sub)
	else:
		size += wr_int(fout, -1)
		size += wr_int(fout, -1)

	if con.pole_target and con.pole_target in idx_dict:
		size += wr_int(fout, idx_dict[con.pole_target])
		sub = -1
		if con.pole_subtarget and con.pole_target.type == 'ARMATURE':
			for b in con.pole_target.pose.bones:
				if b.name == con.pole_subtarget:
					sub = idx_dict[b]
					break;
		size += wr_int(fout, sub)
	else:
		size += wr_int(fout, -1)
		size += wr_int(fout, -1)

	ik_type	= 0
	if con.ik_type == 'COPY_POSE':
		ik_type = 1
	elif con.ik_type == 'DISTANCE':
		ik_type = 2

	size += wr_int(fout, ik_type)

	reference_axis = 0
	if con.reference_axis == 'BONE':
		reference_axis = 1
	elif con.reference_axis == 'TARGET':
		reference_axis = 2

	size += wr_int(fout, reference_axis)

	limit_mode = 0
	if con.limit_mode == 'LIMITDIST_INSIDE':
		limit_mode = 1
	elif con.limit_mode == 'LIMITDIST_OUTSIDE':
		limit_mode = 2
	elif con.limit_mode == 'LIMITDIST_ONSURFACE':
		limit_mode = 3

	size += wr_int(fout, limit_mode)
	size += wr_int(fout, con.chain_count)
	size += wr_int(fout, con.iterations)

	flags = 0
	if con.use_location:
		flags |= 1<<16
	if con.use_rotation:
		flags |= 1<<17
	if con.use_stretch:
		flags |= 1<<18
	if con.use_tail:
		flags |= 1<<19

	size += wr_int(fout, flags)

	size += wr_flt(fout, con.distance)
	size += wr_flt(fout, con.pole_angle)
	size += wr_flt(fout, con.orient_weight)
	size += wr_flt(fout, con.weight)
	
	return size

def save_con(fout, con, idx_dict, names):

	size = 0

	# write name offs and store name
	name_ofs = wr_ref(fout)
	size += 4
	names.append( (name_ofs,con.name) )

	type = 0
	if con.type == 'FOLLOW_PATH':
		type = 1
	elif con.type == 'IK':
		type = 2

	size += wr_int(fout,type)

	flags = 0
	if con.is_valid:
		flags |= 1<<16
	if con.mute:
		flags |= 1<<17
	if con.is_proxy_local:
		flags |= 1<<18

	size += wr_int(fout,flags)

	if type == 1:
		size += save_follow_path_con(fout, con, idx_dict)
	elif type == 2:
		size += save_ik_con(fout, con, idx_dict)

	return size

def save_tfm(fout, o, delta):
	size = 0
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

def save_bon(fout, b, bones, idx_dict, names):
	global tm_slot
	size = 0
	# write name offs and store name
	name_ofs = wr_ref(fout)
	size += 4
	names.append( (name_ofs,b.name) )

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
		size += save_con(fout,c,idx_dict,names)

	return size

def save_arm(fout, arm, pose, idx_dict, names):

	global tm_slot
	size = 0

	# write name offs and store name
	name_ofs = wr_ref(fout)
	size += 4
	names.append( (name_ofs,arm.name) )


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
		wr_int(bone_ofs, size)
		size += save_bon(fout,b,bones,idx_dict,names)

	return size

def save_obj(fout, obj, idx_dict, obj_list, msh_dict, names):

	global tm_slot

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
		for b in obj.parent.pose.bones:
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
	size += wr_mtx(fout, obj.matrix_parent_inverse)

	# offset to obj data (type dependent)
	obj_data_ofs = wr_ref(fout)
	size += 4

	# modifiers stack size
	size += wr_int( fout, len(obj.modifiers) )
	# offset to modifiers stack
	modifier_stack_offs = wr_ref(fout)
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
		size += save_con(fout,c,idx_dict,names)

	wr_int(obj_data_ofs,size)

	# depending on object type, save_msh, save_cur or save_arm

	if obj.type == 'MESH':
		size += save_msh(fout,msh_dict[obj],idx_dict,names)
	elif obj.type == 'CURVE':
		size += save_cur(fout,obj.data,idx_dict,names)
	elif obj.type == 'ARMATURE':
		size += save_arm(fout,obj.data,obj.pose,idx_dict,names)
	elif obj.type == 'EMPTY':
		size += save_emp(fout,obj) # NO NAME!

	wr_int(modifier_stack_offs,size)
	modifier_offs = wr_ref(fout)
	size += 4*len(obj.modifiers)

	for m in obj.modifiers:
		wr_int(modifier_offs,size)
		size += save_mod(fout,obj,m,idx_dict,names)

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

	#create list of exportable objects:
	# all meshes and armatures

	# it may happen that mesh vertices are hooked to other objects or even individual bones in armatures
	# BUT WE SKIP IT, same results in more generic way can be atchieved by ARMATURE parent_type and vertex group weights


	obj_list = []

	# TODO: 
	# NEW TACTICS: ----------------------------------------------------------------------------
	
	# use global UI flags if we want cameras, light, meshes, armatures, empties, latices etc...
	# UI could be made automaticaly from python's enum!!!

	# this flags should be overridable by adding AKM_EXPORT custom prop with 0 or 1 value
	# also we can add such flag right into object data (if we want to export node but its mesh)

	# same with constraints
	# same with modifiers
	# same with materials

	# mesh data could also have 
	# AKM_EXPORT_EDGES 0,1,2     (0-none, 1-all, 2-only freestyle)
	# AKM_EXPORT_MODIFIERS 0,1,2 (0-none, 1-all, 2-apply all to mesh data)

	# scene could override UI with
	# AKM_EXPORT_CAMERAS
	# AKM_EXPORT_MESHES
	# AKM_EXPORT_CONSTRAINTS
	# AKM_EXPORT_MODIFIERS
	# AKM_EXPORT_MATERIALS
	# ...

	# -----------------------------------------------------------------------------------------

	for o in obs:

		# ensure object type
		if o.type != 'ARMATURE' and o.type != 'MESH' and o.type != 'CURVE' and o.type != 'EMPTY':
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

			ok = False
			for b in o.parent.pose.bones:
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
			bone_num = 0
			for b in o.pose.bones:
				obj_dict[b] = bone_num
				bone_num += 1
		if o.type == 'MESH':
			# time to process meshes and store them in mesh_dict[obj]
			# so writing VERTEX parenting can use reindexed vertices
			processed = proc_msh(o)
			if not processed:
				print("EXPORT ERROR! Mesh inside",o.name,"uses too many groups or shapes at once")
				return {'FINISHED'}

			msh_dict[o] = processed
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
