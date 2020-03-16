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



import bpy
import bmesh

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


      
    









def save_mesh(filepath, mesh, obj, use_normals=True, use_uv_coords=True, use_colors=True):
    import os
    import bpy

    def rvec3d(v):
        return round(v[0], 6), round(v[1], 6), round(v[2], 6)

    def rvec2d(v):
        return round(v[0], 6), round(v[1], 6)

    if use_uv_coords and mesh.uv_layers:
        active_uv_layer = mesh.uv_layers.active.data
    else:
        use_uv_coords = False

    if use_colors and mesh.vertex_colors:
        active_col_layer = mesh.vertex_colors.active.data
    else:
        use_colors = False

    # in case
    color = uvcoord = uvcoord_key = normal = normal_key = None

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
    """
    for p in mesh.polygons:
        
        l1 = p.loop_start
        l2 = p.loop_start + p.loop_total - 1
            
        for l in range( l1, l2 ):
            v1 = mesh.loops[l].vertex_index
            v2 = mesh.loops[l+1].vertex_index
            try:
                edges.remove( [ v1,v2 ] )
            except:
                pass
            try:
                edges.remove( [ v2,v1 ] )
            except:
                pass
            
        v1 = mesh.loops[l1].vertex_index
        v2 = mesh.loops[l2].vertex_index

        try:
            edges.remove( [ v1,v2 ] )
        except:
            pass
        try:
            edges.remove( [ v2,v1 ] )
        except:
            pass
    """

    for i, f in enumerate(mesh.polygons):

        smooth = not use_normals or f.use_smooth
        if not smooth:
            normal = f.normal[:]
            normal_key = rvec3d(normal)

        if use_uv_coords:
            uv = [
                active_uv_layer[l].uv[:]
                for l in range(f.loop_start, f.loop_start + f.loop_total)
            ]
        if use_colors:
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

            if smooth:
                normal = v.normal[:]
                normal_key = rvec3d(normal)

            if use_uv_coords:
                uvcoord = uv[j][0], uv[j][1]
                uvcoord_key = rvec2d(uvcoord)

            if use_colors:

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

                #color = col[j]
                #color = (
                #    int(color[0] * 255.0),
                #    int(color[1] * 255.0),
                #    int(color[2] * 255.0),
                #    int(color[3] * 255.0),
                #)

            key = normal_key, uvcoord_key, color

            vdict_local = vdict[vidx]
            pf_vidx = vdict_local.get(key)  # Will be None initially

            if pf_vidx is None:  # Same as vdict_local.has_key(key)
                pf_vidx = vdict_local[key] = vert_count
                ply_verts.append((vidx, normal, uvcoord, color))
                vert_count += 1

            pf.append(pf_vidx)

    #reindex edge verts, add verts if needed
    for e in edges:

        vidx = e[0]

        v = mesh_verts[vidx]
        if smooth:
            normal = v.normal[:]
            normal_key = rvec3d(normal)
        if use_uv_coords:
            uvcoord = uv[j][0], uv[j][1]
            uvcoord_key = rvec2d(uvcoord)
        if use_colors:
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

        key = normal_key, uvcoord_key, color
        vdict_local = vdict[vidx]
        pf_vidx = vdict_local.get(key)  # Will be None initially
        if pf_vidx is None:  # Same as vdict_local.has_key(key)
            pf_vidx = vdict_local[key] = vert_count
            ply_verts.append((vidx, normal, uvcoord, color))
            vert_count += 1

        e[0] = pf_vidx

        vidx = e[1]
        
        v = mesh_verts[vidx]
        if smooth:
            normal = v.normal[:]
            normal_key = rvec3d(normal)
        if use_uv_coords:
            uvcoord = uv[j][0], uv[j][1]
            uvcoord_key = rvec2d(uvcoord)
        if use_colors:
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

        key = normal_key, uvcoord_key, color
        vdict_local = vdict[vidx]
        pf_vidx = vdict_local.get(key)  # Will be None initially
        if pf_vidx is None:  # Same as vdict_local.has_key(key)
            pf_vidx = vdict_local[key] = vert_count
            ply_verts.append((vidx, normal, uvcoord, color))
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
        if use_normals:
            fw(
                "property float nx\n"
                "property float ny\n"
                "property float nz\n"
            )
        if use_uv_coords:
            fw(
                "property float s\n"
                "property float t\n"
            )
        if use_colors:
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
            if use_normals:
                fw(" %.2f %.2f %.2f" % v[1])
            if use_uv_coords:
                fw(" %.3f %.3f" % v[2])
            if use_colors:
                fw(" %u %u %u %u" % v[3])
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


def save(
    operator,
    context,
    filepath="",
    use_selection=False,
    use_mesh_modifiers=True,
    use_normals=True,
    use_uv_coords=True,
    use_colors=True,
    global_matrix=None
):
    import bpy
    import bmesh

    # CRUCIAL so being edited object mesh is updated!
    if bpy.ops.object.mode_set.poll():
        bpy.ops.object.mode_set(mode='OBJECT')

    if use_selection:
        obs = context.selected_objects
    else:
        obs = context.scene.objects

    depsgraph = context.evaluated_depsgraph_get()
    bm = bmesh.new()

    for ob in obs:
        if use_mesh_modifiers:
            ob_eval = ob.evaluated_get(depsgraph)
        else:
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

    if global_matrix is not None:
        mesh.transform(global_matrix)

    if use_normals:
        mesh.calc_normals()

    obj = context.active_object

    ret = save_mesh(
        filepath,
        mesh,
        obj,
        use_normals=use_normals,
        use_uv_coords=use_uv_coords,
        use_colors=use_colors,
    )

    bpy.data.meshes.remove(mesh)

    return ret
