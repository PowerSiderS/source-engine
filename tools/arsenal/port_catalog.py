"""Offline Source 1 authoring pipeline. Never executes an asset installer.

Inputs are a mesh audit and Crowbar SMD/QC output. Imported assets remain private
and retain their original authors' license; this tool does not grant a license.
"""
import argparse, concurrent.futures, json, math, os, re, shutil, subprocess
from pathlib import Path


def rename_bones(text):
    return text.replace('v_weapon.Bip01', 'ValveBiped.Bip01').replace('"v_weapon"', '"weapon_root"')


def blocks(text, directive):
    pattern = re.compile(r'\$' + directive + r'\s+"([^"]+)"\s*\{', re.I)
    for match in pattern.finditer(text):
        pos = match.end(); depth = 1; quoted = False
        while pos < len(text) and depth:
            char = text[pos]
            if char == '"': quoted = not quoted
            if not quoted:
                if char == '{': depth += 1
                if char == '}': depth -= 1
            pos += 1
        if depth: raise ValueError('Unbalanced QC block')
        yield match.group(1), text[match.start():pos]


def nodes(text):
    section = text.split('nodes\n', 1)[1].split('\nend', 1)[0]
    return [(int(i), name, int(parent)) for i,name,parent in re.findall(r'(\d+)\s+"([^"]+)"\s+(-?\d+)', section)]


def first_pose(text):
    values={}
    for line in text.split('skeleton\n',1)[1].splitlines():
        parts=line.split()
        if line.strip()=='end' or (parts and parts[0]=='time' and values):break
        if len(parts)==7:values[int(parts[0])]=list(map(float,parts[1:]))
    return values


def global_pose(text):
    import numpy as np
    pose=first_pose(text);hierarchy={i:(name,parent) for i,name,parent in nodes(text)};cache={}
    def transform(index):
        if index in cache:return cache[index]
        x,y,z,rx,ry,rz=pose[index];cx,sx=math.cos(rx),math.sin(rx);cy,sy=math.cos(ry),math.sin(ry);cz,sz=math.cos(rz),math.sin(rz)
        local=np.eye(4);local[:3,:3]=np.array([[cz*cy,cz*sy*sx-sz*cx,cz*sy*cx+sz*sx],[sz*cy,sz*sy*sx+cz*cx,sz*sy*cx-cz*sx],[-sy,cy*sx,cy*cx]])
        local[:3,3]=(x,y,z);parent=hierarchy[index][1]
        cache[index]=transform(parent)@local if parent>=0 else local
        return cache[index]
    return {name:transform(index) for index,(name,parent) in hierarchy.items()}


def world_mesh(mesh_texts, idle_text=None):
    import numpy as np
    # The world model is a static weapon-only mesh. Arm vertices never enter it.
    triangles = []
    for text in mesh_texts:
        bind=global_pose(text);pose=global_pose(idle_text) if idle_text else bind
        bone_names={i:name for i,name,_ in nodes(text)}
        transforms={i:pose.get(name,bind[name])@np.linalg.inv(bind[name]) for i,name in bone_names.items()}
        lines = text.split('triangles\n', 1)[1].splitlines()
        for offset in range(0, len(lines)-1, 4):
            if lines[offset] == 'end': break
            vertices=[]
            for line in lines[offset+1:offset+4]:
                parts=line.split();point=np.array([*map(float,parts[1:4]),1.]);normal=np.array(list(map(float,parts[4:7])))
                weights=[(int(parts[10+i*2]),float(parts[11+i*2])) for i in range(int(parts[9]))] if len(parts)>9 else [(int(parts[0]),1.)]
                if not weights:weights=[(int(parts[0]),1.)]
                positioned=sum(weight*(transforms[bone]@point) for bone,weight in weights)
                oriented=sum(weight*(transforms[bone][:3,:3]@normal) for bone,weight in weights)
                oriented/=max(np.linalg.norm(oriented),1e-12)
                vertices.append([parts[0],*map(str,positioned[:3]),*map(str,oriented),*parts[7:9]])
            triangles.append((lines[offset], vertices))
    xyz = [list(map(float,v[1:4])) for _,vs in triangles for v in vs]
    center = [(min(v[i] for v in xyz)+max(v[i] for v in xyz))/2 for i in range(3)]
    result = ['version 1\nnodes\n0 "weapon_bone" -1\nend\nskeleton\ntime 0\n0 0 0 0 0 0 0\nend\ntriangles\n']
    for material, vertices in triangles:
        result.append(material+'\n')
        for vertex in vertices:
            point = [float(vertex[i+1])-center[i] for i in range(3)]
            result.append('0 '+' '.join(f'{v:.6f}' for v in point)+' '+' '.join(vertex[4:9])+' 1 0 1.000000\n')
    result.append('end\n'); return ''.join(result)


def prepare(source, destination, audit):
    destination.mkdir(parents=True, exist_ok=True); catalog=[]
    for entry in audit:
        slug=entry['folder']; original=source/slug; target=destination/slug
        target.mkdir(parents=True, exist_ok=True)
        qc_path=next(original.glob('*.qc')); qc=qc_path.read_text(encoding='utf-8-sig')
        selected=[name for name,mats in entry['meshes'] if 'pearl' not in name.lower() and not any('pearl' in m for m in mats)]
        if not selected: raise ValueError('No default weapon mesh: '+slug)
        mesh_texts=[]
        for name in selected:
            text=rename_bones((original/name).read_text(encoding='utf-8-sig'))
            (target/name).write_text(text, encoding='utf-8'); mesh_texts.append(text)
        for animation in original.rglob('*.smd'):
            if animation.name in selected or animation.parent == original: continue
            output=target/animation.relative_to(original); output.parent.mkdir(parents=True,exist_ok=True)
            output.write_text(rename_bones(animation.read_text(encoding='utf-8-sig')),encoding='utf-8')
        skeleton=nodes(mesh_texts[0]); arm_bones=[name for _,name,_ in skeleton if name.startswith('ValveBiped.')]
        qcout=[f'$modelname "sourceadvanced/c_weapon_{slug}.mdl"\n']
        for i,name in enumerate(selected):
            qcout.append(f'$bodygroup "weapon_part_{i}"\n{{\n studio "{name}"\n}}\n')
        # Definitions preserve the animation rig even after removing arm meshes.
        for line in qc.splitlines():
            if line.lstrip().startswith(('$attachment ', '$definebone ', '$cdmaterials ')):
                qcout.append(rename_bones(line)+'\n')
        qcout += ['$surfaceprop "metal"\n', '$contents "solid"\n']
        qcout += [f'$bonemerge "{name}"\n' for name in arm_bones]
        seqs=[]
        for name,block in blocks(qc,'sequence'):
            # Make activities unambiguous as well as selecting by name in C++.
            if slug.startswith('knife_') and name in ('stab','stab_miss'):
                block=block.replace('ACT_VM_HITCENTER"','ACT_VM_HITCENTER2"').replace('ACT_VM_MISSCENTER"','ACT_VM_MISSCENTER2"')
            qcout.append(rename_bones(block)+'\n');seqs.append(name)
        # Names vary (idle, idle1, idle_unsil); activity is the stable contract.
        idle=None
        for sequence_name,sequence_block in blocks(qc,'sequence'):
            if 'ACT_VM_IDLE' not in sequence_block: continue
            match=re.search(r'"([^"\n]+\.smd)"',sequence_block,re.I)
            if match:
                candidate=target/match.group(1).replace('\\','/')
                if candidate.is_file(): idle=candidate; break
        # Preserve source sequences. Do not substitute an authored inspection
        # for a missing native clip; use a compatible native model instead.
        viewqc=target/'view.qc';viewqc.write_text(''.join(qcout),encoding='utf-8')
        (target/'world.smd').write_text(world_mesh(mesh_texts,idle.read_text(encoding='utf-8') if idle else None),encoding='utf-8')
        material_lines=[line for line in qcout if line.lstrip().startswith('$cdmaterials')]
        (target/'world.qc').write_text(f'$modelname "sourceadvanced/w_weapon_{slug}.mdl"\n$body "weapon" "world.smd"\n'+''.join(material_lines)+'$surfaceprop "metal"\n$sequence "idle" "world.smd" fps 1\n',encoding='utf-8')
        catalog.append({'folder':slug,'view_model':f'models/sourceadvanced/c_weapon_{slug}.mdl','world_model':f'models/sourceadvanced/w_weapon_{slug}.mdl','arm_bones':len(arm_bones),'meshes':selected,'source_sequences':seqs,'custom_sequences':[]})
    (destination/'catalog.json').write_text(json.dumps(catalog,indent=2),encoding='utf-8')
    return catalog


def main():
    parser=argparse.ArgumentParser(); parser.add_argument('--source',type=Path,required=True)
    parser.add_argument('--audit',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--compiler',type=Path);parser.add_argument('--game',type=Path)
    parser.add_argument('--only',help='Compile just this catalog folder')
    parser.add_argument('--skip-prepare',action='store_true');parser.add_argument('--models',nargs='+',choices=['view','world'],default=['view','world'])
    args=parser.parse_args();catalog=json.loads((args.output/'catalog.json').read_text()) if args.skip_prepare else prepare(args.source,args.output,json.loads(args.audit.read_text()))
    if args.compiler:
        if not args.game: parser.error('--game required with --compiler')
        failed=[]
        def compile_item(item):
            errors=[]
            for name in args.models:
                qc=(args.output/item['folder']/(name+'.qc')).resolve()
                env=os.environ.copy();env['SteamAppId']='243750'
                result=subprocess.run([str(args.compiler.resolve()),'-game',str(args.game.resolve()),'-nop4',str(qc)],cwd=args.compiler.parent,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,creationflags=0x08000000)
                (qc.parent/(name+'-compile.log')).write_bytes(result.stdout)
                if result.returncode or b'ERROR:' in result.stdout:errors.append(str(qc))
            print(item['folder'],flush=True)
            return errors
        with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
            for errors in pool.map(compile_item,[item for item in catalog if not args.only or item['folder']==args.only]): failed.extend(errors)
        if failed: raise RuntimeError('Compile failures: '+str(failed))
    print(f'{len(catalog)} view/world model pairs prepared')


if __name__=='__main__': main()
