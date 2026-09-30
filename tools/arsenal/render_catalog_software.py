"""Portable orthographic model thumbnails from the actual authored SMD mesh."""
import argparse, json, math
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw
from create_stage_assets import write_vtf


def render(mesh_path,output,width=256,height=256,arms=False):
    lines=mesh_path.read_text().split('triangles\n',1)[1].splitlines();triangles=[];names=[]
    for i in range(0,len(lines)-1,4):
        if lines[i]=='end':break
        names.append(lines[i]);triangles.append([list(map(float,line.split()[1:4])) for line in lines[i+1:i+4]])
    points=np.array(triangles,dtype=np.float64);center=(points.reshape(-1,3).min(0)+points.reshape(-1,3).max(0))/2;points-=center
    extent=points.reshape(-1,3).ptp(0) if hasattr(np.ndarray,'ptp') else np.ptp(points.reshape(-1,3),axis=0)
    long_axis=int(np.argmax(extent));short_axis=int(np.argmin(extent));up_axis=next(i for i in range(3) if i not in (long_axis,short_axis)) if long_axis!=short_axis else 2
    forward=np.zeros(3);forward[short_axis]=1;forward[long_axis]=0.3;forward[up_axis]=0.25;forward/=np.linalg.norm(forward)
    right=np.zeros(3);right[long_axis]=1;right-=np.dot(right,forward)*forward;right/=np.linalg.norm(right)
    up=np.cross(forward,right);up/=np.linalg.norm(up)
    projected=np.stack((points@right,points@up),axis=-1);depth=points@forward
    pmin=projected.reshape(-1,2).min(0);pmax=projected.reshape(-1,2).max(0);offset=(pmin+pmax)/2
    scale=min((width-24)/(pmax[0]-pmin[0]),(height-24)/(pmax[1]-pmin[1]))
    projected=(projected-offset)*scale;projected[:,:,0]+=width/2;projected[:,:,1]=height/2-projected[:,:,1]
    normals=np.cross(points[:,1]-points[:,0],points[:,2]-points[:,0]);lengths=np.linalg.norm(normals,axis=1);normals/=np.maximum(lengths[:,None],1e-10)
    light=forward*0.8+up*0.65-right*0.3;light/=np.linalg.norm(light);shade=0.38+0.62*np.maximum(0,normals@light)
    image=Image.new('RGBA',(width,height),(25,30,37,255) if arms else (0,0,0,0));draw=ImageDraw.Draw(image)
    for i in np.argsort(depth.mean(1)):
        if normals[i]@forward < -0.01:continue
        color=(74,82,66) if 'sleeve' in names[i] else (48,52,59) if 'glove' in names[i] else (162,173,188)
        rgba=tuple(int(v*shade[i]) for v in color)+(255,)
        draw.polygon([tuple(p) for p in projected[i]],fill=rgba)
    output.parent.mkdir(parents=True,exist_ok=True);image.save(output);return image


parser=argparse.ArgumentParser();parser.add_argument('--catalog',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--stage',type=Path,required=True);parser.add_argument('--arms',type=Path)
args=parser.parse_args()
if args.arms:render(args.arms,args.output/'unified_arms_preview.png',768,512,True)
for entry in json.loads(args.catalog.read_text()):
    slug=entry['folder'];image=render(args.catalog.parent/slug/'world.smd',args.output/(slug+'.png'))
    material='vgui/sourceadvanced/inventory/'+slug
    write_vtf(args.stage/'materials'/(material+'.vtf'),image.width,image.height,image.tobytes())
    (args.stage/'materials'/(material+'.vmt')).write_text('"UnlitGeneric"\n{\n "$basetexture" "'+material+'"\n "$translucent" "1"\n "$ignorez" "1"\n "$vertexcolor" "1"\n "$vertexalpha" "1"\n}\n')
print('All model icons rendered from geometry.')
