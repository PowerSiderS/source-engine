"""Read-only audit of installed SCAP models. Does not certify historical parity.

The historical Valve announcement documents a shape type, not capsule coordinates.
An original CS:GO build and pose/trace measurements are still required for parity.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import math
from pathlib import Path
import struct
import zlib


def digest(data):
    return hashlib.sha256(data).hexdigest()


def read_package(path):
    data = path.read_bytes()
    magic, version, tree_size = struct.unpack_from('<III', data)
    if magic != 0x55AA1234 or version not in (1, 2):
        raise ValueError('Unsupported VPK')
    cursor = 12 if version == 1 else 28
    end = cursor + tree_size
    if end > len(data):
        raise ValueError('Truncated VPK tree')
    entries = {}

    def string():
        nonlocal cursor
        stop = data.index(0, cursor, end)
        text = data[cursor:stop].decode('utf-8')
        cursor = stop + 1
        return text

    while (extension := string()):
        while (folder := string()):
            while (name := string()):
                if cursor + 18 > end:
                    raise ValueError('Truncated VPK entry')
                crc, preload, archive, offset, size, terminator = struct.unpack_from('<IHHIIH', data, cursor)
                cursor += 18
                if terminator != 0xFFFF or archive != 0x7FFF:
                    raise ValueError('Expected single-file gameplay VPK')
                if cursor + preload > end or end + offset + size > len(data):
                    raise ValueError('Truncated VPK payload')
                payload = data[cursor:cursor + preload] + data[end + offset:end + offset + size]
                cursor += preload
                key = ('' if folder == ' ' else folder + '/') + name + ('' if extension == ' ' else '.' + extension)
                if key in entries or zlib.crc32(payload) != crc:
                    raise ValueError('Duplicate/corrupt entry: ' + key)
                entries[key] = payload
    return entries


def parse_model(data):
    if len(data) < 180 or data[:4] != b'IDST':
        raise ValueError('Invalid MDL')
    version, checksum = struct.unpack_from('<ii', data, 4)
    if version != 44 or struct.unpack_from('<i', data, 76)[0] != len(data):
        raise ValueError('Audit supports the installed version-44 MDLs only')
    bone_count, bone_offset = struct.unpack_from('<ii', data, 156)
    set_count, set_offset = struct.unpack_from('<ii', data, 172)
    if not 0 < bone_count <= 256 or not 0 < set_count <= 32:
        raise ValueError('Invalid counts')
    if bone_offset < 180 or bone_offset + bone_count * 216 > len(data):
        raise ValueError('Invalid bone table')
    if set_offset < 180 or set_offset + set_count * 12 > len(data):
        raise ValueError('Invalid hitbox table')
    bones = []
    for i in range(bone_count):
        base = bone_offset + i * 216
        name_relative, parent = struct.unpack_from('<ii', data, base)
        name_offset = base + name_relative
        if not 0 <= name_offset < len(data) or not -1 <= parent < bone_count:
            raise ValueError('Invalid bone name/parent')
        name = data[name_offset:data.index(0, name_offset)].decode('utf-8')
        bones.append(dict(index=i, name=name, parent=parent,
                          bind_position=struct.unpack_from('<3f', data, base + 32),
                          bind_quaternion=struct.unpack_from('<4f', data, base + 44)))
    boxes = []
    reserved_offsets = set()
    for i in range(set_count):
        base = set_offset + i * 12
        _, count, relative = struct.unpack_from('<iii', data, base)
        first = base + relative
        if not 0 < count <= 256 or relative < 12 or first + count * 68 > len(data):
            raise ValueError('Invalid hitboxes')
        for j in range(count):
            offset = first + j * 68
            bone, group = struct.unpack_from('<ii', data, offset)
            if not 0 <= bone < bone_count:
                raise ValueError('Invalid hitbox bone')
            lo, hi = struct.unpack_from('<3f', data, offset + 8), struct.unpack_from('<3f', data, offset + 20)
            marker = struct.unpack_from('<I', data, offset + 36)[0]
            capsule = struct.unpack_from('<7f', data, offset + 40) if marker == 0x53434150 else None
            boxes.append(dict(set=i, index=j, bone=bone, bone_name=bones[bone]['name'],
                              group=group, minimum=lo, maximum=hi, capsule=capsule))
            reserved_offsets.update(range(offset + 36, offset + 68))
    return dict(version=version, checksum=checksum, bones=bones, hitboxes=boxes), reserved_offsets


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    package = args.game / 'cstrike/custom/000_sourceadvanced_gameplay.vpk'
    entries = read_package(package)
    provenance = json.loads(entries['scripts/sourceadvanced_gameplay.json'])
    report = dict(created_utc=datetime.now(timezone.utc).isoformat(),
                  package=str(package), package_sha256=digest(package.read_bytes()),
                  historical_reference='CS:GO after the 2015-09-15 animation/capsule update; exact build not supplied',
                  official_source='https://blog.counter-strike.net/2015/09/12496/',
                  exact_2015_parity='NOT_VERIFIED', original_2015_model_reference_available=False,
                  comparison_failures=[], integrity_failures=[], models={})
    for key, payload in sorted(entries.items()):
        if not key.startswith('models/player/') or not key.endswith('.mdl'):
            continue
        model, reserved = parse_model(payload)
        original = args.game / 'cstrike/sourceadvanced_gameplay' / key
        if not original.is_file():
            raise ValueError('Missing exported Source baseline: ' + str(original))
        baseline = original.read_bytes()
        metadata = provenance['models'][key]
        outside_changes = sum(a != b for i, (a, b) in enumerate(zip(payload, baseline)) if i not in reserved)
        preserved = len(payload) == len(baseline) and outside_changes == 0
        if digest(payload) != metadata['sha256'] or digest(baseline) != metadata['original_sha256'] or not preserved:
            report['integrity_failures'].append(key + ': provenance/baseline mismatch')
        all_derived, contained, marked = True, True, 0
        for box in model['hitboxes']:
            cap = box['capsule']
            if cap is None:
                report['integrity_failures'].append(key + ': missing SCAP capsule')
                all_derived = contained = False
                continue
            marked += 1
            lo, hi = box['minimum'], box['maximum']
            extents = [(b-a)*.5 for a, b in zip(lo, hi)]
            center = [(a+b)*.5 for a, b in zip(lo, hi)]
            axis = max(range(3), key=lambda a: extents[a])
            radius = min(extents[a] for a in range(3) if a != axis)
            start, finish = list(center), list(center)
            start[axis] -= extents[axis] - radius
            finish[axis] += extents[axis] - radius
            expected = start + finish + [radius]
            finite = all(math.isfinite(v) for v in cap) and cap[6] > 0
            matches = finite and max(abs(a-b) for a, b in zip(cap, expected)) < 1e-5
            inside = finite and all(min(cap[a], cap[a+3])-cap[6] >= lo[a]-1e-5 and
                                    max(cap[a], cap[a+3])+cap[6] <= hi[a]+1e-5 for a in range(3))
            all_derived &= matches
            contained &= inside
            if not inside:
                report['integrity_failures'].append(f'{key}: capsule {box["index"]} invalid/outside bounds')
        model.update(sha256=digest(payload), baseline_sha256=digest(baseline),
                     source_baseline_preserved_outside_reserved_words=preserved,
                     bytes_changed_outside_reserved_words=outside_changes,
                     scap_capsules=marked, capsules_derived_from_source_boxes=all_derived,
                     capsules_contained_in_source_boxes=contained)
        report['models'][key] = model
    if len(report['models']) != 8:
        report['integrity_failures'].append('Expected eight player models')
    report['total_capsules'] = sum(m['scap_capsules'] for m in report['models'].values())
    report['internal_integrity_passed'] = not report['integrity_failures']
    report['comparison_failures'] = [
        'The installed geometry is derived from Source boxes, not authenticated 2015 capsule coordinates.',
        'The Source skeleton/mesh/animation data is preserved; no original 2015 rig was imported.',
        'Original 2015 pose matrices and trace results are absent; animation and lag-compensation parity cannot be compared.',
    ]
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / 'hitbox_2015_audit.json').write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding='utf-8')
    lines = ['# Auditoria de hitbox: referência CS:GO 2015', '',
             '**Resultado: não validada como idêntica ao CS:GO de 2015.**', '',
             'Referência provisória: após a atualização de 15/09/2015. A Valve documenta cápsulas, mudanças no esqueleto e nas animações, mas não publica nessa nota as coordenadas ou os raios.', '',
             '[Nota oficial da Valve](https://blog.counter-strike.net/2015/09/12496/)', '',
             f'Integridade interna: {"PASSOU" if report["internal_integrity_passed"] else "FALHOU"}. VPK lido diretamente, CRC de cada arquivo verificado e hashes comparados com o manifesto.', '',
             '| Modelo | Bones | Cápsulas | Derivadas das caixas Source | Dados fora da extensão preservados |',
             '|---|---:|---:|---|---|']
    for key, model in report['models'].items():
        lines.append(f'| {Path(key).stem} | {len(model["bones"])} | {model["scap_capsules"]} | {"Sim" if model["capsules_derived_from_source_boxes"] else "Não"} | {"Sim" if model["source_baseline_preserved_outside_reserved_words"] else "Não"} |')
    lines += ['', f'Total: {report["total_capsules"]} cápsulas.', '',
              'O teste cs_validate_hitboxes lança seis raios em direção ao centro de cada caixa. Ele aceita uma colisão com qualquer hitgroup válido do jogador, inclusive outro membro antes do centro pretendido. Zero falhas nesse teste não certifica cada borda, cada membro ou equivalência histórica.', '',
              'A visualização existente desenha caixas de referência, não as cápsulas efetivamente testadas.', '',
              'Para uma comparação exata faltam os modelos, esqueleto e animações originais de uma build identificada de 2015 e os resultados de referência dos mesmos tiros/poses. Comparar endpoints, raios, bone/hitgroup, poses parado/agachado/correndo/pulando/mirando e rewinds de lag compensation; não usar packs comunitários ou versões recentes como prova histórica.', '',
              'Nenhuma DLL, modelo instalado, configuração ou processo do jogo foi alterado por esta auditoria.', '',
              f'VPK SHA-256: `{report["package_sha256"]}`', '']
    (args.output / 'HITBOX_2015_VALIDACAO.md').write_text('\n'.join(lines), encoding='utf-8')
    print(json.dumps({k: v for k, v in report.items() if k != 'models'}, indent=2))


if __name__ == '__main__':
    main()
