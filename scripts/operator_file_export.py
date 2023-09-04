import bpy
import bmesh
import struct

def write_some_data(context, filepath, use_some_setting):
    print("running write_some_data...")
    f = open(filepath, "wb")

    indices = []
    vertices = []
    

    '''
    for item in bpy.data.objects:  
        if item.type == 'MESH':
            bm = bmesh.from_edit_mesh(item.data)
            uv_layer = bm.loops.layers.uv.active
            for face in item.data.polygons:
                for vert_idx, loop_idx in zip(face.vertices, face.loop_indices):
                    # Get the vertex information
                    vertex = item.data.vertices[vert_idx]
                    vertex_position = item.matrix_world @ vertex.co
                    vertex_normal = item.matrix_world @ face.normal
                    vertex_uv = [0.0, 0.0]
                    # get vertex uv
                    if uv_layer is not None:
						vertex_uv = bm.loops[loop_idx][uv_layer].uv
                    
                    # Fill the vertices list with the information
                    vertices.append((vertex_position[0], vertex_position[1], vertex_position[2],
                                 vertex_normal[0], vertex_normal[1], vertex_normal[2],
                                 vertex_uv[0], vertex_uv[1]))
'''

    obj = bpy.context.active_object
    if obj.type != 'MESH':
		raise ValueError("active object is not a mesh")

    me = obj.data
    bm = bmesh.from_edit_mesh(me)

    uv_layer = bm.loops.layers.uv.active

    for face in bm.faces:
        for loop in face.loops:
			vertex = loop.vert
			vertex_position = obj.matrix_world @ vertex.co
			vertex_normal = obj.matrix_world @ face.normal
			vertex_uv = [0.0, 0.0]
			# get vertex uv
			if uv_layer is not None:
				vertex_uv = loop[uv_layer].uv
			# Fill the vertices list with the information
			vertices.append((vertex_position[0], vertex_position[1], vertex_position[2],
							 vertex_normal[0], vertex_normal[1], vertex_normal[2],
							 vertex_uv[0], vertex_uv[1]))

    if ((len(vertices) % 3) != 0):
        raise ValueError("vertices are not triangular")
    if ((len(indices) % 3) != 0):
        raise ValueError("indices are not triangular")
    

    bytesPerIndex = 2

    # write header
    # bytes per index
    f.write(int(bytesPerIndex).to_bytes(1, "little"))

    for i in range(0, len(vertices)):
        if i > pow(2, bytesPerIndex*8):
            print("index " + str(i) + " is too large")
            raise ValueError("too many vertices")
        indices.append(i)
        
    unique_vertices = {}
    unique_indices = []
    for index in indices:
        vertex = vertices[index]
        if vertex not in unique_vertices:
            unique_vertices[vertex] = len(unique_vertices)
        unique_indices.append(unique_vertices[vertex])
    vertices = list(unique_vertices.keys())
    indices = unique_indices
    
    print("NUM VERTICES: " + str(len(vertices)))
    print("NUM INDICES: " + str(len(indices)))
    f.write(int(len(vertices)).to_bytes(4, "little"))
    f.write(int(len(indices)).to_bytes(4, "little"))
    for vtx in vertices:
        f.write(struct.pack("f", vtx[0])) # pos1
        f.write(struct.pack("f", vtx[1])) # pos2
        f.write(struct.pack("f", vtx[2])) # pos3
        f.write(struct.pack("f", vtx[3])) # norm1
        f.write(struct.pack("f", vtx[4])) # norm 2
        f.write(struct.pack("f", vtx[5])) # norm 3
        f.write(struct.pack("f", vtx[6])) # uv 1
        f.write(struct.pack("f", vtx[7])) # uv 2
        
    for index in indices:
        f.write(int(index).to_bytes(bytesPerIndex, "little"))
        
    f.close()
    print("\n\nDONE")
    return {'FINISHED'}


# ExportHelper is a helper class, defines filename and
# invoke() function which calls the file selector.
from bpy_extras.io_utils import ExportHelper
from bpy.props import StringProperty, BoolProperty, EnumProperty
from bpy.types import Operator


class ExportSomeData(Operator, ExportHelper):
    """This appears in the tooltip of the operator and in the generated docs"""
    bl_idname = "export_test.some_data"  # important since its how bpy.ops.import_test.some_data is constructed
    bl_label = "Export Some Data"

    # ExportHelper mixin class uses this
    filename_ext = ".bin"

    filter_glob: StringProperty(
        default="*.bin",
        options={'HIDDEN'},
        maxlen=255,  # Max internal buffer length, longer would be clamped.
    )

    # List of operator properties, the attributes will be assigned
    # to the class instance from the operator settings before calling.
    use_setting: BoolProperty(
        name="Example Boolean",
        description="Example Tooltip",
        default=True,
    )

    type: EnumProperty(
        name="Example Enum",
        description="Choose between two items",
        items=(
            ('OPT_A', "First Option", "Description one"),
            ('OPT_B', "Second Option", "Description two"),
        ),
        default='OPT_A',
    )

    def execute(self, context):
        return write_some_data(context, self.filepath, self.use_setting)


# Only needed if you want to add into a dynamic menu
def menu_func_export(self, context):
    self.layout.operator(ExportSomeData.bl_idname, text="Text Export Operator")

# Register and add to the "file selector" menu (required to use F3 search "Text Export Operator" for quick access)
def register():
    bpy.utils.register_class(ExportSomeData)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)


def unregister():
    bpy.utils.unregister_class(ExportSomeData)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)


if __name__ == "__main__":
    register()

    # test call
    bpy.ops.export_test.some_data('INVOKE_DEFAULT')
