import os
import json
from pathlib import Path
import re
import shutil
import sys

source=Path(r'C:\Users\SnyX\Desktop\projeto clone\source-engine')
original=Path(r'C:\Users\SnyX\Desktop\Source Advanced V1 - PowerSiderS (PC)\Source Advanced V1 - @PowerSiderS (PC)')
root=Path(os.environ.get('SA_KNIFE_WORK','C:\\Users\\SnyX\\Documents\\Codex\\2026-09-28\\c-users-snyx-desktop-pasta-teste\\work'))/'native-knife-engine-20260930'
runtime=root/('candidate-dedicated-runtime' if '--dedicated' in sys.argv else 'candidate-runtime' if '--candidate' in sys.argv else 'baseline-runtime')
runtime.mkdir(parents=True,exist_ok=True)
for path in (original/'bin').iterdir():
    if path.is_file() and path.suffix.lower() in ('.dll','.txt','.cfg'):
        (runtime/'bin').mkdir(exist_ok=True)
        shutil.copy2(path,runtime/'bin'/path.name)
for path in original.iterdir():
    if path.is_file() and (path.suffix.lower()=='.dll' or path.name in ('hl2_launcher.exe','steam_appid.txt')):
        shutil.copy2(path,runtime/path.name)
mod=runtime/'cstrike'
(mod/'bin').mkdir(parents=True,exist_ok=True)
for path in (original/'cstrike/bin').glob('*.dll'):shutil.copy2(path,mod/'bin'/path.name)
shutil.copytree(original/'cstrike/scripts',mod/'scripts',dirs_exist_ok=True)
shutil.copytree(original/'cstrike/cfg',mod/'cfg',dirs_exist_ok=True)
info=(original/'cstrike/gameinfo.txt').read_text(encoding='utf-8-sig')
info=info.replace('|all_source_engine_paths|',original.as_posix()+'/')
info=re.sub(r'(?m)^(\s*(?:game|platform)\s+)(C:/[^\r\n]+)$',r'\1"\2"',info)
for value in ('cstrike/custom/*','cstrike/cstrike_english.vpk','cstrike/cstrike_pak.vpk'):
    info=info.replace(value,'"'+(original/value).as_posix()+'"')
info=re.sub(r'game\+game_write\s+cstrike\s*\n','game "'+(original/'cstrike').as_posix()+'"\n',info)
info=info.replace('SearchPaths\n','SearchPaths\n')
info=re.sub(r'(SearchPaths\s*\{)',r'\1\n            game+mod |gameinfo_path|custom/*\n            game+mod |gameinfo_path|.\n',info,count=1)
(mod/'gameinfo.txt').write_text(info,encoding='utf-8')
if '--candidate' in sys.argv:
    output=source/('output-dedicated' if '--dedicated' in sys.argv else 'output')
    exe='dedicated_launcher.exe' if '--dedicated' in sys.argv else 'hl2_launcher.exe'
    shutil.copy2(output/exe,runtime/exe)
    for path in (output/'bin').glob('*.dll'):shutil.copy2(path,runtime/'bin'/path.name)
    for path in (output/'cstrike/bin').glob('*.dll'):shutil.copy2(path,mod/'bin'/path.name)
    shutil.copytree(root/'assets/cstrike/models',mod/'models',dirs_exist_ok=True)
    shutil.copytree(root/'assets/cstrike/scripts',mod/'scripts',dirs_exist_ok=True)
    if '--dedicated' in sys.argv:
        (mod/'maps').mkdir(exist_ok=True)
        shutil.copy2(Path(r'C:\Users\SnyX\Downloads\Compressed\mapas\mapas\cstrike\maps\de_mirage_csgo_new.nav'),mod/'maps/de_mirage_csgo_new.nav')
print('Private runtime:',runtime,flush=True)
