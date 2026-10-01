import os
"""Rebuild the catalog VPK and independently verify every payload and CRC."""
from pathlib import Path
import hashlib,json,shutil,subprocess,sys
work=Path(os.environ.get('SA_ANIMATION_WORK','C:\\Users\\SnyX\\Documents\\Codex\\2026-09-28\\c-users-snyx-desktop-pasta-teste\\work'));root=work/'animation-gloves-update';base=work/'native-knife-engine-20260930'
repo=Path(r'C:\Users\SnyX\Desktop\projeto clone\source-engine')
sys.path.insert(0,str(repo/'tools'))
from build_gameplay_vpk import verify_vpk
from flatten_vpk import flatten
stage=root/'000_sourceadvanced_catalog';assets=root/'assets/cstrike'
shutil.copytree(base/'000_sourceadvanced_catalog',stage,dirs_exist_ok=True)
for category in ('models','materials','sound','scripts'):
 for path in (assets/category).rglob('*'):
  if not path.is_file() or path.name=='skins_manifest.txt':continue
  dest=stage/path.relative_to(assets);dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(path,dest)
# The editable manifest remains loose in the MOD search path.
assert not (stage/'scripts/skins_manifest.txt').exists()
provenance={kind:json.loads((work/folder/'asset-audit.json').read_text()) for kind,folder in [('firearms','cs2-animation-import'),('gloves','csso-glove-import')]}
(stage/'scripts/sourceadvanced_animation_glove_provenance.json').write_text(json.dumps(provenance,indent=2),encoding='utf8')
exe=Path(r'C:\Users\SnyX\Downloads\Compressed\csso_release_1.1\bin\vpk.exe')
with (root/'vpk-build.log').open('w') as log:
 subprocess.run([str(exe),'-M','-c','100',str(stage)],cwd=exe.parent,check=True,creationflags=subprocess.CREATE_NO_WINDOW,stdout=log,stderr=log)
package=stage.with_suffix('.vpk')
flatten(stage.with_name(stage.name+'_dir.vpk'),package)
entries=verify_vpk(package,stage)
report=dict(files=len(entries),sha256=hashlib.sha256(package.read_bytes()).hexdigest(),entries=entries)
(root/'package-validation.json').write_text(json.dumps(report,indent=2),encoding='utf8')
dest=root/'candidate-runtime/cstrike/custom/000_sourceadvanced_catalog.vpk';shutil.copy2(package,dest)
print('Verified catalog VPK:',len(entries),'files;',round(package.stat().st_size/1048576,1),'MiB',flush=True)
