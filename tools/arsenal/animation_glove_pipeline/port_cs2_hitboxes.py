import os
"""Retarget CRC-verified installed CS2 capsules onto the existing Source 1 rigs.

No mesh, animation, bone order or VVD/VTX checksum is replaced. Rebuilds the
19-entry MDL hitbox tables, flags their bones/ancestors, and writes SCAP metadata.
The output is an adaptation to Source 1 anatomy, not an identical CS2 character.
"""
from pathlib import Path
import json,re,struct,hashlib,numpy as np
work=Path(os.environ.get('SA_ANIMATION_WORK','C:\\Users\\SnyX\\Documents\\Codex\\2026-09-28\\c-users-snyx-desktop-pasta-teste\\work'));reference=work/'cs2-hitbox-reference'
root=work/'cs2-hitbox-import';assets=root/'assets/cstrike/models/player';assets.mkdir(parents=True,exist_ok=True)

def section(text,key,opening,closing):
    a=re.search(r'\b'+re.escape(key)+r'\s*=\s*'+re.escape(opening),text).end()-1
    depth=1;i=a+1
    while depth:
        depth+=(text[i]==opening)-(text[i]==closing);i+=1
    return text[a:i]
def array(text,key):
    return json.loads(re.sub(r',\s*]',']',section(text,key,'[',']')))
def quat(q):
    x,y,z,w=np.array(q)/np.linalg.norm(q)
    return np.array([[1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w)],
      [2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w)],
      [2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y)]])
def source2(name):
    text=(reference/(name+'_agent.data.txt')).read_text();sk=section(text,'m_modelSkeleton','{','}')
    names=array(sk,'m_boneName');parents=array(sk,'m_nParent');positions=array(sk,'m_bonePosParent');rotations=array(sk,'m_boneRotParent');world=[]
    for i,(parent,pos,rot) in enumerate(zip(parents,positions,rotations)):
        mat=np.eye(4);mat[:3,:3]=quat(rot);mat[:3,3]=pos
        world.append(world[parent]@mat if parent>=0 else mat)
    bones={n.lower():m for n,m in zip(names,world)}
    boxes=[]
    hit=section((reference/(name+'_agent.mdat.txt')).read_text(),'m_hitboxsets','[',']')
    for box in re.findall(r'\{\s*m_name =.*?m_nHitBoxIndex = \d+\s*}',hit,re.S):
        name=re.search(r'm_sBoneName = "([^"]+)"',box)[1].lower()
        radius=float(re.search(r'm_flShapeRadius = ([\d.]+)',box)[1])
        group=int(re.search(r'm_nGroupId = (\d+)',box)[1])
        assert int(re.search(r'm_nShapeType = (\d+)',box)[1])==2 and radius>0
        boxes.append(dict(bone=name,start=array(box,'m_vMinBounds'),end=array(box,'m_vMaxBounds'),radius=radius,group=group))
    assert len(boxes)==19
    return bones,boxes

mapping={'pelvis':'Pelvis','spine_0':'Spine','spine_1':'Spine1','spine_2':'Spine2','spine_3':'Spine4','neck_0':'Neck1','head_0':'Head1'}
for side in ('l','r'):
    for src,dst in [('leg_upper','Thigh'),('leg_lower','Calf'),('ankle','Foot'),('hand','Hand'),('arm_upper','UpperArm'),('arm_lower','Forearm')]:
        mapping[src+'_'+side]=side.upper()+'_'+dst
mapping={k:'ValveBiped.Bip01_'+v for k,v in mapping.items()}
children={}
for side in ('l','r'):
    children.update({f'leg_upper_{side}':f'leg_lower_{side}',f'leg_lower_{side}':f'ankle_{side}',f'ankle_{side}':f'ball_{side}',
      f'arm_upper_{side}':f'arm_lower_{side}',f'arm_lower_{side}':f'hand_{side}',f'hand_{side}':f'finger_middle_0_{side}'})
def unit(v):
    assert np.linalg.norm(v)>1e-6
    return v/np.linalg.norm(v)
def body_axes(bones,cs2):
    get=lambda src: bones[src] if cs2 else bones[mapping[src]]
    up=unit(get('head_0')[:3,3]-get('pelvis')[:3,3]);left=unit(get('leg_upper_l')[:3,3]-get('leg_upper_r')[:3,3])
    forward=unit(np.cross(left,up));return up,forward
def frame(bones,name,axes,cs2):
    bone=bones[name] if cs2 else bones[mapping[name]]
    up,forward=axes
    if name in children:
        child=children[name]
        target=(child if cs2 else mapping.get(child))
        if not cs2 and child.startswith('ball_'):target='ValveBiped.Bip01_'+child[-1].upper()+'_Toe0'
        if not cs2 and child.startswith('finger_middle'):target='ValveBiped.Bip01_'+child[-1].upper()+'_Finger2'
        shaft=unit(bones[target][:3,3]-bone[:3,3])
    else:shaft=up
    radial=forward-shaft*np.dot(forward,shaft)
    if np.linalg.norm(radial)<1e-4:radial=up-shaft*np.dot(up,shaft)
    radial=unit(radial);third=unit(np.cross(shaft,radial))
    return bone[:3,:3].T@np.column_stack((shaft,radial,third))

report=dict(policy='CS2 capsule radii/endpoints retargeted by anatomical bone frames; Source 1 mesh and animations retained. CS2 neck group 8 maps to Source 1 chest group 2.',references={},models={},failures=[])
refs={}
for name in ('tm_phoenix','ctm_sas'):
    refs[name]=source2(name)
    report['references'][name]=dict(path='agents/models/'+name+'/'+name+'.vmdl_c',sha256=hashlib.sha256((reference/(name+'_agent.vmdl_c')).read_bytes()).hexdigest(),capsules=refs[name][1])
for path in sorted((reference/'source1').glob('*.mdl')):
    original=path.read_bytes();data=bytearray(original);count,offset=struct.unpack_from('<ii',data,156);bones={};indexes={};parents=[]
    for i in range(count):
        b=offset+216*i;j=b+struct.unpack_from('<i',data,b)[0];name=data[j:data.index(0,j)].decode();parent=struct.unpack_from('<i',data,b+4)[0];parents.append(parent)
        inv=np.eye(4);inv[:3,:]=np.array(struct.unpack_from('<12f',data,b+96)).reshape(3,4)
        bones[name]=np.linalg.inv(inv);indexes[name]=i
    refname='ctm_sas' if path.name.startswith('ct_') else 'tm_phoenix';src,boxes=refs[refname];sourceaxes=body_axes(src,True);targetaxes=body_axes(bones,False)
    sets,so=struct.unpack_from('<ii',data,172);records=[];allowed=set()
    for setindex in range(sets):
        base=so+12*setindex;_,num,relative=struct.unpack_from('<iii',data,base);assert num==19
        for i,box in enumerate(boxes):
            name=box['bone'];targetname=mapping[name];idx=indexes[targetname];rotation=frame(bones,name,targetaxes,False)@frame(src,name,sourceaxes,True).T
            assert np.max(np.abs(rotation.T@rotation-np.eye(3)))<1e-4
            start=rotation@box['start'];end=rotation@box['end'];radius=box['radius'];lo=np.minimum(start,end)-radius;hi=np.maximum(start,end)+radius
            o=base+relative+68*i;group=2 if box['group']==8 else box['group']
            struct.pack_into('<ii6f',data,o,idx,group,*lo,*hi)
            struct.pack_into('<I7f',data,o+36,0x53434150,*start,*end,radius)
            allowed.update(range(o,o+32));allowed.update(range(o+36,o+68))
            # Ensure the new spine bones participate in hitbox bone caching.
            boneindex=idx
            while boneindex>=0:
                flagoffset=offset+216*boneindex+160;flags=struct.unpack_from('<I',data,flagoffset)[0];struct.pack_into('<I',data,flagoffset,flags|0x100);allowed.update(range(flagoffset,flagoffset+4));boneindex=parents[boneindex]
            mat=bones[targetname];ws=mat[:3,:3]@start+mat[:3,3];we=mat[:3,:3]@end+mat[:3,3]
            records.append(dict(index=i,bone=targetname,group=group,start=start.tolist(),end=end.tolist(),radius=radius,world_bounds=[(np.minimum(ws,we)-radius).tolist(),(np.maximum(ws,we)+radius).tolist()]))
    assert len(data)==len(original) and data[4:12]==original[4:12]
    assert all(a==b or i in allowed for i,(a,b) in enumerate(zip(original,data)))
    (assets/path.name).write_bytes(data)
    report['models'][path.name]=dict(reference=refname,bones=count,capsules=records,sha256=hashlib.sha256(data).hexdigest())
assert len(report['models'])==8
report['passed']=True
(root/'asset-audit.json').write_text(json.dumps(report,indent=2));provenance=root/'assets/cstrike/scripts/sourceadvanced_cs2_hitbox_import.json';provenance.parent.mkdir(parents=True,exist_ok=True);provenance.write_text(json.dumps(report,indent=2))
print('Retargeted 19 authentic CS2 capsule definitions onto each of 8 existing Source 1 player rigs; mesh/bone order/animations intact.',flush=True)
