"""Build a separate bonemerge arm mesh from an existing decompiled viewmodel.

Preserves triangle topology and UVs. Optional bind-space retargeting aligns the
skin to the catalog rig. Weapon geometry is excluded; twists use forearm bones.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mesh', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--reference', type=Path, help='Retarget the mesh into this catalog bind rig')
    parser.add_argument('--keep-twists', action='store_true', help='Preserve forearm twist bones for native CSGO rigs')
    parser.add_argument('--model-name', default='c_arms_default')
    args = parser.parse_args()
    original = args.mesh.read_bytes()
    lines = original.decode('utf-8-sig').splitlines()
    begin = lines.index('nodes')
    end = lines.index('end', begin)
    bones = {}
    for line in lines[begin+1:end]:
        match = re.fullmatch(r'\s*(\d+)\s+"([^"]+)"\s+(-?\d+)\s*', line)
        if not match:
            raise ValueError('Malformed SMD node')
        index, name, parent = match.groups()
        bones[int(index)] = (name, int(parent))
    selected = [i for i, (name, _) in bones.items() if '.Bip01_' in name and (args.keep_twists or not name.endswith('_ForeTwist'))]
    mapping = {old: new for new, old in enumerate(selected)}
    by_name = {name: index for index, (name, _) in bones.items()}
    remapped_twists = []
    for i, (name, _) in bones.items():
        if name.endswith('_ForeTwist') and not args.keep_twists:
            arm = by_name[name.replace('_ForeTwist', '_Forearm')]
            mapping[i] = mapping[arm]
            remapped_twists.append(name)
    pose_begin = lines.index('skeleton', end)
    pose_end = lines.index('end', pose_begin)
    pose = {}
    for line in lines[pose_begin+1:pose_end]:
        parts = line.split()
        if parts and parts[0] == 'time':
            if pose:
                raise ValueError('Expected a single bind frame')
            continue
        if len(parts) != 7:
            raise ValueError('Malformed bind pose')
        pose[int(parts[0])] = parts[1:]
    transforms = None
    reference_bones = None
    if args.reference:
        from generate_unified_arms import read_rig, multiply, transform
        _, _, source_world = read_rig(args.mesh)
        reference_bones, reference_pose, reference_world = read_rig(args.reference)
        reference_names = {name: index for index, name, _ in reference_bones}
        transforms = {}
        for old in list(mapping):
            name = re.sub(r'^.*?(Bip01.*)$', r'ValveBiped.\1', bones[old][0])
            if not args.keep_twists: name=name.replace('_ForeTwist','_Forearm')
            target = reference_names[name]
            mapping[old] = target
            src = source_world[old]
            inverse = [[src[j][i] for j in range(3)] + [-sum(src[j][i]*src[j][3] for j in range(3))] for i in range(3)]
            inverse.append([0,0,0,1])
            transforms[old] = multiply(reference_world[target], inverse)
    result = ['version 1', 'nodes']
    names = []
    for old in ([] if reference_bones else selected):
        name, parent = bones[old]
        if parent >= 0 and parent not in mapping:
            raise ValueError('An arm has a weapon parent: ' + name)
        name = re.sub(r'^.*?(Bip01.*)$', r'ValveBiped.\1', name)
        names.append(name)
        result.append(f'{mapping[old]} "{name}" {mapping.get(parent, -1)}')
    if reference_bones:
        for index, name, parent in reference_bones:
            names.append(name)
            result.append(f'{index} "{name}" {parent}')
    result += ['end', 'skeleton', 'time 0']
    if reference_bones:
        result += [str(i) + ' ' + ' '.join(f'{v:.9f}' for v in reference_pose[i]) for i,_,_ in reference_bones]
    else:
        result += [str(mapping[old]) + ' ' + ' '.join(pose[old]) for old in selected]
    result += ['end', 'triangles']
    triangle_begin = lines.index('triangles', pose_end) + 1
    materials = set()
    count = 0
    cursor = triangle_begin
    while lines[cursor].strip() != 'end':
        material = lines[cursor].strip()
        materials.add(material)
        result.append(material)
        for line in lines[cursor+1:cursor+4]:
            parts = line.split()
            if len(parts) < 10:
                raise ValueError('Expected explicit vertex skin weights')
            links = int(parts[9])
            if len(parts) != 10 + links*2:
                raise ValueError('Malformed vertex weights')
            weights = {}
            pairs = [(int(parts[10+i*2]), float(parts[11+i*2])) for i in range(links)]
            if not pairs:
                pairs = [(int(parts[0]), 1.)]
            for old, weight in pairs:
                if old not in mapping:
                    raise ValueError('Arm vertex uses a weapon bone: ' + bones[old][0])
                index = mapping[old]
                weights[index] = weights.get(index, 0.) + weight
            total = sum(weights.values())
            if abs(total - 1.) > 1e-4 or min(weights.values()) < 0:
                raise ValueError('Invalid skin weights')
            ordered = sorted(weights.items(), key=lambda pair: (-pair[1], pair[0]))
            payload = parts[1:9]
            if transforms:
                point, normal = tuple(map(float,parts[1:4])), tuple(map(float,parts[4:7]))
                posed = [0.,0.,0.]
                oriented = [0.,0.,0.]
                for old, weight in pairs:
                    p, n = transform(transforms[old],point), transform(transforms[old],normal,True)
                    for axis in range(3):
                        posed[axis] += weight/total*p[axis]
                        oriented[axis] += weight/total*n[axis]
                length = math.sqrt(sum(v*v for v in oriented))
                if length <= 1e-8: raise ValueError('Degenerate retargeted normal')
                payload = [f'{v:.9f}' for v in posed] + [f'{v/length:.9f}' for v in oriented] + parts[7:9]
            result.append(str(ordered[0][0]) + ' ' + ' '.join(payload) + ' ' + str(len(ordered)) + ' ' +
                          ' '.join(f'{bone} {weight/total:.9f}' for bone, weight in ordered))
        count += 1
        cursor += 4
    args.output.mkdir(parents=True, exist_ok=True)
    mesh = args.output / 'c_arms_default.smd'
    mesh.write_text('\n'.join(result) + '\nend\n', encoding='utf-8')
    if not re.fullmatch(r'[A-Za-z0-9_]+',args.model_name): raise ValueError('Invalid model name')
    qc = [f'$modelname "sourceadvanced/{args.model_name}.mdl"', '$body "arms" "c_arms_default.smd"',
          '$cdmaterials "models/weapons/v_models/arms/"', '$surfaceprop "flesh"',
          '$sequence "idle" "c_arms_default.smd" fps 1 loop']
    qc += [f'$bonemerge "{name}"' for name in names]
    (args.output / 'c_arms_default.qc').write_text('\n'.join(qc) + '\n', encoding='utf-8')
    report = dict(source=str(args.mesh), source_sha256=hashlib.sha256(original).hexdigest(),
                  bones=names, triangles=count, materials=sorted(materials), twist_weights_remapped=remapped_twists,
                  reference_rig=str(args.reference) if args.reference else None,
                  policy='Existing local arm mesh; UVs preserved; bind-space retargeting when a reference is supplied; no weapon geometry')
    (args.output / 'arm_import.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
