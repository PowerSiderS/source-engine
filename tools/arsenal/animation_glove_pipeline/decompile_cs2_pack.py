import os
"""Read-only local pack import. Retain the original MDL/QC/SMD and provenance."""
from pathlib import Path
import hashlib,json,subprocess
work=Path(os.environ.get('SA_ANIMATION_WORK','C:\\Users\\SnyX\\Documents\\Codex\\2026-09-28\\c-users-snyx-desktop-pasta-teste\\work'))
source=Path(r'C:\Users\SnyX\Downloads\Compressed\cs2 pack v1\cs2 test pack')
root=work/'cs2-animation-import';root.mkdir(exist_ok=True)
compiler=work/'crowbar-src/Crowbar/bin/x86/Release/Crowbar.exe'
results=[]
for model in sorted((source/'models').rglob('*.mdl')):
 target=root/'decompiled'/model.stem;target.mkdir(parents=True,exist_ok=True)
 if not list(target.glob('*.qc')):
  result=subprocess.run([str(compiler),'--headless-decompile',str(model),str(target)],capture_output=True,timeout=180,creationflags=subprocess.CREATE_NO_WINDOW)
  (target/'decompile.log').write_bytes(result.stdout+result.stderr)
  if result.returncode or not list(target.glob('*.qc')):raise RuntimeError('Decompile failed: '+str(model))
 entry=dict(model=str(model),sha256=hashlib.sha256(model.read_bytes()).hexdigest(),qc=[str(p) for p in target.glob('*.qc')],smd_count=len(list(target.rglob('*.smd'))))
 results.append(entry);print(model.stem,entry['smd_count'],'SMDs',flush=True)
(root/'decompile-results.json').write_text(json.dumps(results,indent=2),encoding='utf8')
print('Decompiled',len(results),'models; source pack unchanged.',flush=True)
