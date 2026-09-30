import os
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import subprocess,json
work=Path(os.environ.get('SA_KNIFE_WORK','C:\\Users\\SnyX\\Documents\\Codex\\2026-09-28\\c-users-snyx-desktop-pasta-teste\\work'))
root=work/'native-knife-engine-20260930'
exe=work/'crowbar-src/Crowbar/bin/x86/Release/Crowbar.exe'
jobs=[p for p in (root/'csso-native/models/weapons').glob('v_knife*.mdl') if p.stem!='v_knife_gg']
def run(p):
 dst=root/'decompiled'/p.stem; dst.mkdir(parents=True,exist_ok=True)
 if not list(dst.glob('*.qc')):
  r=subprocess.run([str(exe),'--headless-decompile',str(p),str(dst)],capture_output=True,creationflags=0x08000000,timeout=180)
  (dst/'decompile.log').write_bytes(r.stdout+r.stderr)
  if r.returncode or not list(dst.glob('*.qc')):raise RuntimeError('Decompile failed: '+p.name)
 return p.stem,len(list(dst.rglob('*.smd')))
with ThreadPoolExecutor(max_workers=1) as pool:
 result=list(pool.map(run,jobs))
for row in result:print(*row)
(root/'decompile-results.json').write_text(json.dumps(result,indent=2))
