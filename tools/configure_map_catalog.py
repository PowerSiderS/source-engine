"""Mount one map resource directory and publish its explicit server/menu catalog."""
import argparse
from datetime import datetime
import json
from pathlib import Path
import re
import shutil
import struct

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--game', type=Path, required=True)
parser.add_argument('--maps', type=Path, required=True, help='Resource root containing maps/, models/, materials/')
args = parser.parse_args()
root = args.maps.resolve(strict=True)
maps = sorted(root.glob('maps/*.bsp'))
if not maps:
    raise ValueError('No maps found')
for path in maps:
    if not re.fullmatch(r'(de|cs)_[a-z0-9_-]{1,123}', path.stem):
        raise ValueError('Unsupported map name: ' + path.stem)
    with path.open('rb') as stream:
        header = stream.read(1036)
    if len(header) != 1036 or header[:4] != b'VBSP':
        raise ValueError('Invalid BSP header: ' + str(path))
names = [p.stem for p in maps]
default = 'de_mirage_csgo_new' if 'de_mirage_csgo_new' in names else names[0]
server = args.game / 'Dedicated Server'
backup = args.game / 'backups' / ('map-catalog-' + datetime.now().strftime('%Y%m%d-%H%M%S'))
backup.mkdir(parents=True)

def write(path, text):
    if path.exists():
        relative = path.relative_to(args.game)
        saved = backup / relative
        saved.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, saved)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding='utf-8')

for gameinfo in (args.game / 'cstrike/gameinfo.txt', server / 'runtime/cstrike/gameinfo.txt'):
    text = gameinfo.read_text(encoding='utf-8-sig')
    mount = 'game "' + root.as_posix() + '"'
    if mount not in text:
        text, count = re.subn(r'(SearchPaths\s*\{)', lambda m: m[1] + '\n            // Authoritative map resources.\n            ' + mount, text, count=1)
        if count != 1:
            raise ValueError('SearchPaths missing: ' + str(gameinfo))
        write(gameinfo, text)
catalog = '# Generated from ' + root.as_posix() + '\n' + '\n'.join(names) + '\n'
for path in (args.game / 'cstrike/cfg/sourceadvanced_maps.txt',
             server / 'runtime/cstrike/cfg/sourceadvanced_maps.txt',
             args.game / 'cstrike/maplist.txt', args.game / 'cstrike/mapcycle.txt',
             server / 'config/mapcycle.txt', server / 'runtime/cstrike/mapcycle.txt',
             server / 'runtime/cstrike/maplist.txt', server / 'runtime/cstrike/cfg/mapcycle.txt'):
    write(path, catalog if path.name == 'sourceadvanced_maps.txt' else '\n'.join(names) + '\n')
settings_path = server / 'config/launcher.json'
settings = json.loads(settings_path.read_text(encoding='utf-8-sig'))
settings.update(DefaultMap=default, MapResourceRoot=str(root))
write(settings_path, json.dumps(settings, indent=2) + '\n')
print(json.dumps(dict(default_map=default, maps=names, resource_root=str(root), backup=str(backup)), indent=2))
