"""Build an additive gameplay VPK from models exported by the running game.

Run cs_export_player_hitboxes first. No mesh, skeleton or animation is replaced:
only the reserved hitbox words in compatible Source MDLs are extended with SCAP.
The gameplay DLLs interpret SCAP; older DLLs still see the original box bounds.
"""
import argparse
import hashlib
import json
import math
import re
from pathlib import Path
import shutil
import struct
import subprocess
import zlib


def sha(data):
    return hashlib.sha256(data).hexdigest()


def patch_model(data):
    original = bytes(data)
    data = bytearray(data)
    if len(data) < 180 or data[:4] != b'IDST':
        raise ValueError('Not a Source MDL')
    version, checksum = struct.unpack_from('<ii', data, 4)
    if not 44 <= version <= 49 or struct.unpack_from('<i', data, 76)[0] != len(data):
        raise ValueError('Expected intact, compatible version-44..49 MDL')
    bones, bone_offset = struct.unpack_from('<ii', data, 156)
    sets, set_offset = struct.unpack_from('<ii', data, 172)
    if not 0 < bones <= 256 or not 0 < sets <= 32:
        raise ValueError('Invalid bone or hitbox-set count')
    if bone_offset < 180 or bone_offset + bones * 216 > len(data):
        raise ValueError('Bone table outside MDL')
    if set_offset < 180 or set_offset + sets * 12 > len(data):
        raise ValueError('Hitbox-set table outside MDL')
    records = []
    for index in range(sets):
        base = set_offset + index * 12
        _, count, relative = struct.unpack_from('<iii', data, base)
        boxes = base + relative
        if not 0 < count <= 256 or relative < 12 or boxes + count * 68 > len(data):
            raise ValueError('Hitbox table outside MDL')
        for number in range(count):
            offset = boxes + number * 68
            bone, group = struct.unpack_from('<ii', data, offset)
            lo = struct.unpack_from('<3f', data, offset + 8)
            hi = struct.unpack_from('<3f', data, offset + 20)
            if not 0 <= bone < bones or not 0 <= group <= 10:
                raise ValueError('Invalid hitbox bone/group')
            if not all(math.isfinite(v) for v in lo + hi):
                raise ValueError('Non-finite hitbox')
            ext = [(hi[i] - lo[i]) / 2 for i in range(3)]
            if min(ext) <= 0:
                raise ValueError('Degenerate hitbox')
            reserved = data[offset + 36:offset + 68]
            marker = struct.unpack_from('<I', reserved)[0]
            if any(reserved) and marker != 0x53434150:
                raise ValueError('Reserved MDL words already contain foreign data')
            axis = max(range(3), key=lambda i: ext[i])
            radius = min(ext[i] for i in range(3) if i != axis)
            start = [(lo[i] + hi[i]) / 2 for i in range(3)]
            end = list(start)
            half = max(0, ext[axis] - radius)
            start[axis] -= half
            end[axis] += half
            struct.pack_into('<I7f', data, offset + 36, 0x53434150,
                             *start, *end, radius)
            records.append(dict(set=index, hitbox=number, bone=bone,
                                group=group, start=start, end=end, radius=radius))
    # The checksum links MDL/VVD/VTX. Reserved metadata must not change it.
    assert struct.unpack_from('<i', data, 8)[0] == checksum
    allowed = set()
    for index in range(sets):
        base = set_offset + index * 12
        _, count, relative = struct.unpack_from('<iii', data, base)
        for number in range(count):
            allowed.update(range(base + relative + number * 68 + 36,
                                 base + relative + number * 68 + 68))
    assert all(a == b or i in allowed for i, (a, b) in enumerate(zip(original, data)))
    return bytes(data), dict(version=version, bones=bones, checksum=checksum,
                            original_sha256=sha(original), sha256=sha(data),
                            capsules=records)


def verify_vpk(path, stage):
    """Independent VPK v1/v2 directory, payload, preload and CRC validation."""
    raw = path.read_bytes()
    magic, version, size = struct.unpack_from('<III', raw)
    if magic != 0x55AA1234 or version not in (1, 2):
        raise ValueError('Unsupported VPK')
    header = 12 if version == 1 else 28
    end = header + size
    cursor = header
    entries = {}

    def string():
        nonlocal cursor
        stop = raw.index(0, cursor, end)
        result = raw[cursor:stop].decode('utf-8')
        cursor = stop + 1
        return result

    while (extension := string()):
        while (folder := string()):
            while (name := string()):
                crc, preload, archive, offset, length, term = struct.unpack_from('<IHHIIH', raw, cursor)
                cursor += 18
                if term != 0xFFFF or archive != 0x7FFF:
                    raise ValueError('Expected a single-file VPK')
                payload = raw[cursor:cursor + preload]
                cursor += preload
                payload += raw[end + offset:end + offset + length]
                key = ('' if folder == ' ' else folder + '/') + name + ('' if extension == ' ' else '.' + extension)
                if key in entries or zlib.crc32(payload) != crc:
                    raise ValueError('Duplicate or corrupt VPK entry: ' + key)
                expected = (stage / key).read_bytes()
                if payload != expected:
                    raise ValueError('VPK does not match staging: ' + key)
                entries[key] = sha(payload)
    # Valve's Windows VPK builder normalizes resource and font paths to lowercase.
    expected_paths = [p.relative_to(stage).as_posix().lower() for p in stage.rglob('*') if p.is_file()]
    expected_names = set(expected_paths)
    if len(expected_paths) != len(expected_names):
        raise ValueError('Case-colliding staging paths')
    if {name.lower() for name in entries} != expected_names:
        raise ValueError('VPK file inventory mismatch')
    return entries


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--export', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--vpk', type=Path, required=True)
    parser.add_argument('--game', type=Path, required=True, help='Installed game supplying compatible weapon content definitions')
    args = parser.parse_args()
    models = sorted(args.export.glob('models/player/*.mdl'))
    if len(models) != 8:
        raise ValueError(f'Expected eight mounted player models, found {len(models)}')
    stage = args.output / '000_sourceadvanced_gameplay'
    if stage.exists():
        raise ValueError('Use a fresh output directory to keep the previous build recoverable')
    manifest = dict(format='SCAP-1', model_policy='Existing mesh/bones/animations preserved; authored circular capsules contained in original boxes.',
                    gameplay='Recoil, spread, movement, grenade and anticheat rules require the matching client/server DLLs.', models={})
    for model in models:
        relative = model.relative_to(args.export)
        target = stage / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        data, info = patch_model(model.read_bytes())
        target.write_bytes(data)
        manifest['models'][relative.as_posix()] = info
        for extension in ('.vvd', '.dx90.vtx', '.phy', '.ani'):
            companion = model.with_suffix(extension)
            if not companion.exists():
                if extension in ('.vvd', '.dx90.vtx'):
                    raise ValueError('Missing model companion: ' + str(companion))
                continue
            content = companion.read_bytes()
            if extension in ('.vvd', '.dx90.vtx'):
                checksum_offset = 8 if extension == '.vvd' else 16
                if struct.unpack_from('<i', content, checksum_offset)[0] != info['checksum']:
                    raise ValueError('Model companion checksum mismatch: ' + str(companion))
            shutil.copy2(companion, target.with_suffix(extension))
    (stage / 'scripts').mkdir()
    # Additional compiled entities need content definitions even when their
    # cosmetic catalog is disabled. Reuse installed Source-compatible models,
    # animations and sounds; the matching DLLs supply their own ballistics.
    aliases = {'cz75': 'p228', 'tec9': 'glock', 'p2000': 'usp', 'm4a4': 'm4a1',
               'mp7': 'mp5navy', 'bizon': 'p90', 'mag7': 'm3',
               'sawedoff': 'm3', 'negev': 'm249', 'revolver': 'deagle'}
    for weapon, base in aliases.items():
        source = args.game / ('cstrike/scripts/weapon_' + base + '.txt')
        text = source.read_text(encoding='utf-8-sig')
        for key, value in {'printname': weapon.upper(), 'Team': 'ANY'}.items():
            text = re.sub(r'("' + key + r'"\s+)"[^"]*"', lambda m: m[1] + '"' + value + '"', text, flags=re.I)
        (stage / ('scripts/weapon_' + weapon + '.txt')).write_text(text, encoding='utf-8')
    manifest['additional_weapon_content_aliases'] = aliases
    (stage / 'scripts/sourceadvanced_gameplay.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    (stage / 'cfg').mkdir()
    (stage / 'cfg/sourceadvanced_gameplay.cfg').write_text(
        '// Matching compiled gameplay DLLs required. Server settings.\n'
        'sv_player_hitbox_capsules 2\nsv_gravity 800\nsv_maxspeed 250\n'
        'sv_accelerate 5.5\nsv_airaccelerate 12\nsv_friction 5.2\nsv_stopspeed 80\n'
        'sv_enablebunnyhopping 0\nsv_autobunnyhopping 0\nsv_bunnyhop_stamina_threshold 22\n'
        'weapon_accuracy_model 2\nsv_simple_ac 1\nsv_simple_ac_action 1\nsm_map_player_change 0\n', encoding='ascii')
    subprocess.run([str(args.vpk.resolve()), str(stage.resolve())], cwd=args.vpk.parent, check=True)
    package = stage.with_suffix('.vpk')
    entries = verify_vpk(package, stage)
    report = dict(vpk=str(package), sha256=sha(package.read_bytes()), files=len(entries),
                  models=len(models), capsules=sum(len(v['capsules']) for v in manifest['models'].values()), entries=entries)
    (args.output / 'verification.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps({k: v for k, v in report.items() if k != 'entries'}, indent=2))


if __name__ == '__main__':
    main()
