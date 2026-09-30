"""Run with Blender --background --python ... -- --catalog ... --output ... ."""
import argparse, json, math, sys
from pathlib import Path
import bpy
from mathutils import Vector


def render_mesh(path,output,arms=False):
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    vertices=[];faces=[];material_ids=[];names=[]
    lines=path.read_text().split('triangles\n',1)[1].splitlines()
    for i in range(0,len(lines)-1,4):
        if lines[i]=='end':break
        name=lines[i]
        if name not in names:names.append(name)
        material_ids.append(names.index(name));faces.append(tuple(range(len(vertices),len(vertices)+3)))
        vertices.extend(tuple(map(float,line.split()[1:4])) for line in lines[i+1:i+4])
    center=Vector(tuple((min(v[k] for v in vertices)+max(v[k] for v in vertices))/2 for k in range(3)))
    vertices=[tuple(Vector(v)-center) for v in vertices]
    mesh=bpy.data.meshes.new('AuthoringMesh');mesh.from_pydata(vertices,[],faces);mesh.update()
    obj=bpy.data.objects.new('Weapon',mesh);bpy.context.collection.objects.link(obj)
    for name in names:
        mat=bpy.data.materials.new(name);mat.diffuse_color=(0.07,0.09,0.065,1) if 'sleeve' in name else (0.025,0.03,0.035,1) if 'glove' in name else (0.26,0.3,0.36,1)
        mat.use_nodes=True;shader=mat.node_tree.nodes.get('Principled BSDF');shader.inputs['Base Color'].default_value=mat.diffuse_color
        shader.inputs['Roughness'].default_value=0.8 if arms else 0.35;shader.inputs['Metallic'].default_value=0 if arms else 0.65
        mesh.materials.append(mat)
    for i,polygon in enumerate(mesh.polygons):polygon.material_index=material_ids[i];polygon.use_smooth=True
    scene=bpy.context.scene;scene.render.engine='BLENDER_EEVEE';scene.render.resolution_x=768 if arms else 256;scene.render.resolution_y=512 if arms else 192;scene.render.resolution_percentage=100
    scene.render.image_settings.file_format='PNG';scene.render.film_transparent=not arms
    scene.world.color=(0.055,0.06,0.07)
    extent=max(max(v[k] for v in vertices)-min(v[k] for v in vertices) for k in range(3))
    camera_data=bpy.data.cameras.new('Camera');camera=bpy.data.objects.new('Camera',camera_data);bpy.context.collection.objects.link(camera)
    direction=Vector((1,-2,0.9)) if arms else Vector((0.25,-1,0.45))
    camera.location=direction.normalized()*extent*3;camera.rotation_euler=(-camera.location).to_track_quat('-Z','Y').to_euler();camera_data.type='ORTHO';camera_data.ortho_scale=extent*1.25;scene.camera=camera
    for index,(location,energy,size) in enumerate([((1,-2,3),1200,4),((-2,-1,1),700,3),((1,2,2),1000,3)]):
        light_data=bpy.data.lights.new('Light'+str(index),'AREA');light=bpy.data.objects.new(light_data.name,light_data);bpy.context.collection.objects.link(light)
        light.location=Vector(location)*extent;light.rotation_euler=(-light.location).to_track_quat('-Z','Y').to_euler();light_data.energy=energy*(extent/3)**2;light_data.shape='DISK';light_data.size=size*extent
    output.parent.mkdir(parents=True,exist_ok=True);scene.render.filepath=str(output.resolve());bpy.ops.render.render(write_still=True)
    for datablock in list(bpy.data.meshes):
        if datablock.users==0:bpy.data.meshes.remove(datablock)
    for datablock in list(bpy.data.materials):
        if datablock.users==0:bpy.data.materials.remove(datablock)


parser=argparse.ArgumentParser();parser.add_argument('--catalog',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--arms',type=Path)
args=parser.parse_args(sys.argv[sys.argv.index('--')+1:])
if args.arms:render_mesh(args.arms,args.output/'unified_arms_preview.png',True)
for entry in json.loads(args.catalog.read_text()):
    render_mesh(args.catalog.parent/entry['folder']/'world.smd',args.output/(entry['folder']+'.png'))
