"""Generate an original skinned arm mesh from a ValveBiped reference rig.
The supplied SMD is used only for joint transforms, never for its triangles.
"""
import argparse, math, re, struct
from pathlib import Path

def multiply(a,b):
    return [[sum(a[i][k]*b[k][j] for k in range(4)) for j in range(4)] for i in range(4)]

def transform(m,p,direction=False):
    return tuple(sum(m[i][j]*p[j] for j in range(3))+(0 if direction else m[i][3]) for i in range(3))

def matrix(values):
    x,y,z,rx,ry,rz=values; cx,sx=math.cos(rx),math.sin(rx);cy,sy=math.cos(ry),math.sin(ry);cz,sz=math.cos(rz),math.sin(rz)
    return [[cz*cy,cz*sy*sx-sz*cx,cz*sy*cx+sz*sx,x],[sz*cy,sz*sy*sx+cz*cx,sz*sy*cx-cz*sx,y],[-sy,cy*sx,cy*cx,z],[0,0,0,1]]

def read_rig(path):
    lines=path.read_text(encoding='utf-8-sig').splitlines(); ns=lines.index('nodes');ne=lines.index('end',ns)
    bones=[]
    for line in lines[ns+1:ne]:
        match=re.fullmatch(r'\s*(\d+)\s+"([^"]+)"\s+(-?\d+)\s*',line)
        if match:bones.append((int(match[1]),match[2],int(match[3])))
    begin=lines.index('skeleton',ne);pose={}
    for line in lines[begin+1:]:
        if line.strip()=='end':break
        if 'time' in line:
            if pose:break
            continue
        parts=line.split()
        if len(parts)==7:pose[int(parts[0])]=list(map(float,parts[1:]))
    world={};parents={i:p for i,n,p in bones}
    def resolve(i):
        if i not in world:
            local=matrix(pose[i]);world[i]=local if parents[i]<0 else multiply(resolve(parents[i]),local)
        return world[i]
    for i,_,_ in bones:resolve(i)
    return bones,pose,world

def generate(reference,destination):
    bones,pose,world=read_rig(reference)
    selected=[b for b in bones if 'Bip01' in b[1]]
    mapping={old:i for i,(old,name,parent) in enumerate(selected)}
    names={re.sub(r'^.*?(Bip01.*)$',r'ValveBiped.\1',name):old for old,name,parent in selected}
    vertices=[];triangles=[]
    def triangle(material,points):
        a,b,c=[v[0] for v in points];ab=[b[i]-a[i] for i in range(3)];ac=[c[i]-a[i] for i in range(3)]
        cross=(ab[1]*ac[2]-ab[2]*ac[1],ab[2]*ac[0]-ab[0]*ac[2],ab[0]*ac[1]-ab[1]*ac[0])
        if sum(cross[i]*sum(v[1][i] for v in points) for i in range(3))<0:points=[points[0],points[2],points[1]]
        if sum(v*v for v in cross)>1e-12:triangles.append((material,points))
    def vertex(old,p,n,uv,child=None,blend=0):
        point=transform(world[old],p);normal=transform(world[old],n,True);length=math.sqrt(sum(v*v for v in normal));normal=tuple(v/length for v in normal)
        weights=[(mapping[old],1-blend)]
        if child is not None and blend>0:weights.append((mapping[child],blend))
        return (point,normal,uv,weights)
    def tube(old,length,radius_y,radius_z,material,child=None,end_scale=0.85):
        count=16; rings=[]
        for segment in range(7):
            t=segment/6;scale=(1-t)+t*end_scale;ring=[]
            for index in range(count):
                a=2*math.pi*index/count;p=(t*length,radius_y*scale*math.cos(a),radius_z*scale*math.sin(a))
                n=(0,math.cos(a)/radius_y,math.sin(a)/radius_z)
                blend=max(0,(t-0.65)/0.35)*0.5 if child is not None else 0
                ring.append(vertex(old,p,n,(index/count,t),child,blend))
            rings.append(ring)
        for k in range(len(rings)-1):
            for i in range(count):
                j=(i+1)%count
                triangle(material,[rings[k][i],rings[k+1][i],rings[k+1][j]])
                triangle(material,[rings[k][i],rings[k+1][j],rings[k][j]])
        for end,x in [(0,0),(6,length)]:
            center=vertex(old,(x,0,0),(-1 if end==0 else 1,0,0),(0.5,0.5))
            for i in range(count):
                j=(i+1)%count
                points=[center,rings[end][j],rings[end][i]] if end==0 else [center,rings[end][i],rings[end][j]]
                triangle(material,points)
    def ellipsoid(old,center,radii,material):
        rows=[]
        for lat in range(13):
            theta=math.pi*lat/12;row=[]
            for lon in range(24):
                phi=2*math.pi*lon/24;u=(math.cos(theta),math.sin(theta)*math.cos(phi),math.sin(theta)*math.sin(phi))
                p=tuple(center[i]+radii[i]*u[i] for i in range(3));n=tuple(u[i]/radii[i] for i in range(3))
                row.append(vertex(old,p,n,(lon/24,lat/12)))
            rows.append(row)
        for lat in range(12):
            for lon in range(24):
                nxt=(lon+1)%24
                if lat>0:triangle(material,[rows[lat][lon],rows[lat+1][lon],rows[lat][nxt]])
                if lat<11:triangle(material,[rows[lat][nxt],rows[lat+1][lon],rows[lat+1][nxt]])
    for side in ['L','R']:
        def bone(part):return names[f'ValveBiped.Bip01_{side}_{part}']
        upper,fore,hand=bone('UpperArm'),bone('Forearm'),bone('Hand')
        fore_length=abs(pose[hand][0]);upper_length=abs(pose[fore][0])
        tube(upper,upper_length,1.75,1.9,'unified_sleeve',fore,0.85)
        tube(fore,fore_length,1.42,1.5,'unified_sleeve',hand,0.62)
        ellipsoid(fore,(0,0,0),(1.25,1.35,1.4),'unified_sleeve')
        ellipsoid(hand,(1.95,0,0),(2.4,0.78,1.65),'unified_glove')
        # Raised knuckle padding, placed on the back of the glove.
        ellipsoid(hand,(2.8,-0.65,0),(1.05,0.22,1.3),'unified_glove')
        for finger in range(5):
            for joint in range(3):
                suffix=str(finger)+('' if joint==0 else str(joint))
                old=bone('Finger'+suffix)
                next_old=bone('Finger'+str(finger)+str(joint+1)) if joint<2 else None
                length=abs(pose[next_old][0]) if next_old is not None else (0.7 if finger==4 else 0.9)
                radius=0.3 if finger==0 else 0.26
                tube(old,length,radius,radius,'unified_glove',next_old,0.88)
                ellipsoid(old,(0,0,0),(0.32,radius,radius),'unified_glove')
    destination.mkdir(parents=True,exist_ok=True)
    lines=['version 1','nodes']
    for old,name,parent in selected:
        name=re.sub(r'^.*?(Bip01.*)$',r'ValveBiped.\1',name);lines.append(f'{mapping[old]} "{name}" {mapping.get(parent,-1)}')
    lines+=['end','skeleton','time 0']
    for old,name,parent in selected:
        # The removed weapon root is identity in this reference rig.
        if parent>=0 and parent not in mapping and any(abs(v)>0.00001 for v in pose[parent]):
            raise ValueError('nonidentity root: choose a neutral ValveBiped reference')
        lines.append(str(mapping[old])+' '+' '.join(f'{v:.9f}' for v in pose[old]))
    lines+=['end','triangles']
    for material,points in triangles:
        lines.append(material)
        for p,n,uv,weights in points:
            lines.append(str(weights[0][0])+' '+' '.join(f'{v:.9f}' for v in (*p,*n,*uv))+' '+str(len(weights))+' '+' '.join(f'{b} {w:.9f}' for b,w in weights))
    lines+=['end',''];(destination/'c_arms_default.smd').write_text('\n'.join(lines),encoding='utf-8')
    qc=['$modelname "sourceadvanced/c_arms_default.mdl"','$body "arms" "c_arms_default.smd"','$cdmaterials "models/sourceadvanced/arms/"','$surfaceprop "cloth"','$sequence "idle" "c_arms_default.smd" fps 1 loop']
    qc += [f'$bonemerge "{name}"' for name in names]
    (destination/'c_arms_default.qc').write_text('\n'.join(qc)+'\n',encoding='utf-8')
    print('Original arm mesh:',len(selected),'bones',len(triangles),'triangles')

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('reference',type=Path);parser.add_argument('destination',type=Path)
    args=parser.parse_args();generate(args.reference,args.destination)
