
bl_info = {
    "name": "Asciicker AKM format",
    "author": "Gumix",
    "version": (1, 0, 0),
    "blender": (2, 82, 0),
    "location": "File > Import-Export",
    "description": "Import-Export AKM mesh data",
    "wiki_url": "https://asciicker.com/",
    "support": 'YOURSELF',
    "category": "Import-Export",
}

# Copyright (C) 2020: Gumix

if "bpy" in locals():
    import importlib
    if "export_akm" in locals():
        importlib.reload(export_akm)
    if "import_akm" in locals():
        importlib.reload(import_akm)


import bpy
from bpy.props import (
    CollectionProperty,
    StringProperty,
    BoolProperty,
    FloatProperty,
)
from bpy_extras.io_utils import (
    ImportHelper,
    ExportHelper,
#    axis_conversion,
#    orientation_helper,
)


class ImportAKM(bpy.types.Operator, ImportHelper):
    """Load a AKM geometry file"""
    bl_idname = "import_mesh.akm"
    bl_label = "Import AKM"
    bl_options = {'UNDO'}

    files: CollectionProperty(
        name="File Path",
        description=(
            "File path used for importing "
            "the AKM file"
        ),
        type=bpy.types.OperatorFileListElement)

    directory: StringProperty()

    filename_ext = ".akm"
    filter_glob: StringProperty(default="*.akm", options={'HIDDEN'})

    def execute(self, context):
        import os

        paths = [os.path.join(self.directory, name.name)
                 for name in self.files]
        if not paths:
            paths.append(self.filepath)

        from . import import_akm

        for path in paths:
            import_akm.load(self, context, path)

        return {'FINISHED'}


# @orientation_helper(axis_forward='Y', axis_up='Z')

class ExportAKM(bpy.types.Operator, ExportHelper):
    bl_idname = "export_mesh.akm"
    bl_label = "Export AKM"
    bl_description = "Export as Asciicker mesh"

    filename_ext = ".akm"
    filter_glob: StringProperty(default="*.akm", options={'HIDDEN'})

#    use_selection: BoolProperty(
#        name="Selection Only",
#        description="Export selected objects only",
#        default=False,
#    )
#    use_mesh_modifiers: BoolProperty(
#        name="Apply Modifiers",
#        description="Apply Modifiers to the exported mesh",
#        default=True,
#    )
#    use_normals: BoolProperty(
#        name="Normals",
#        description=(
#            "Export Normals for smooth and "
#            "hard shaded faces "
#            "(hard shaded faces will be exported "
#            "as individual faces)"
#        ),
#        default=True,
#    )
#    use_uv_coords: BoolProperty(
#        name="UVs",
#        description="Export the active UV layer",
#        default=True,
#    )
#    use_colors: BoolProperty(
#        name="Vertex Colors",
#        description="Export the active vertex color layer",
#        default=True,
#    )
#
#    global_scale: FloatProperty(
#        name="Scale",
#        min=0.01, max=1000.0,
#        default=1.0,
#    )
    

    def execute(self, context):
        from . import export_akm

#        from mathutils import Matrix
#
#        keywords = self.as_keywords(
#            ignore=(
#                "axis_forward",
#                "axis_up",
#                "global_scale",
#                "check_existing",
#                "filter_glob",
#            )
#        )
#        global_matrix = axis_conversion(
#            to_forward=self.axis_forward,
#            to_up=self.axis_up,
#        ).to_4x4() @ Matrix.Scale(self.global_scale, 4)
#        keywords["global_matrix"] = global_matrix

        filepath = self.filepath
        filepath = bpy.path.ensure_ext(filepath, self.filename_ext)

        return export_akm.save(self, context, filepath) #, **keywords)

    def draw(self, context):
        pass

#class AKM_PT_export_include(bpy.types.Panel):
#    bl_space_type = 'FILE_BROWSER'
#    bl_region_type = 'TOOL_PROPS'
#    bl_label = "Include"
#    bl_parent_id = "FILE_PT_operator"
#
#    @classmethod
#    def poll(cls, context):
#        sfile = context.space_data
#        operator = sfile.active_operator
#
#        return operator.bl_idname == "EXPORT_MESH_OT_akm"
#
#    def draw(self, context):
#        layout = self.layout
#        layout.use_property_split = True
#        layout.use_property_decorate = False  # No animation.
#
#        sfile = context.space_data
#        operator = sfile.active_operator
#        layout.prop(operator, "use_selection")
#
#class AKM_PT_export_transform(bpy.types.Panel):
#    bl_space_type = 'FILE_BROWSER'
#    bl_region_type = 'TOOL_PROPS'
#    bl_label = "Transform"
#    bl_parent_id = "FILE_PT_operator"
#
#    @classmethod
#    def poll(cls, context):
#        sfile = context.space_data
#        operator = sfile.active_operator
#
#        return operator.bl_idname == "EXPORT_MESH_OT_akm"
#
#    def draw(self, context):
#        layout = self.layout
#        layout.use_property_split = True
#        layout.use_property_decorate = False  # No animation.
#
#        sfile = context.space_data
#        operator = sfile.active_operator
#
#        layout.prop(operator, "axis_forward")
#        layout.prop(operator, "axis_up")
#        layout.prop(operator, "global_scale")
#
#class AKM_PT_export_geometry(bpy.types.Panel):
#    bl_space_type = 'FILE_BROWSER'
#    bl_region_type = 'TOOL_PROPS'
#    bl_label = "Geometry"
#    bl_parent_id = "FILE_PT_operator"
#
#    @classmethod
#    def poll(cls, context):
#        sfile = context.space_data
#        operator = sfile.active_operator
#
#        return operator.bl_idname == "EXPORT_MESH_OT_akm"
#
#    def draw(self, context):
#        layout = self.layout
#        layout.use_property_split = True
#        layout.use_property_decorate = False  # No animation.
#
#        sfile = context.space_data
#        operator = sfile.active_operator
#
#        layout.prop(operator, "use_mesh_modifiers")
#        layout.prop(operator, "use_normals")
#        layout.prop(operator, "use_uv_coords")
#        layout.prop(operator, "use_colors")

def menu_func_import(self, context):
    self.layout.operator(ImportAKM.bl_idname, text="Asciicker (.akm)")


def menu_func_export(self, context):
    self.layout.operator(ExportAKM.bl_idname, text="Asciicker (.akm)")


classes = (
    ImportAKM,
    ExportAKM,
#    AKM_PT_export_include,
#    AKM_PT_export_transform,
#    AKM_PT_export_geometry,
)


def register():
    for cls in classes:
        bpy.utils.register_class(cls)

    bpy.types.TOPBAR_MT_file_import.append(menu_func_import)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)


def unregister():
    for cls in classes:
        bpy.utils.unregister_class(cls)

    bpy.types.TOPBAR_MT_file_import.remove(menu_func_import)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)


if __name__ == "__main__":
    register()
