import os
"""CRC-verified extraction and sequential decompilation of local CSSO gloves."""
from pathlib import Path
import json,subprocess,sys
work=Path(os.environ.get('SA_ANIMATION_WORK','C:\\Users\\SnyX\\Documents\\Codex\\2026-09-28\\c-users-snyx-desktop-pasta-teste\\work'));root=work/'csso-glove-import';root.mkdir(exist_ok=True)
ns={'__file__':str(work/'extract_native_knives.py')}
exec((work/'extract_native_knives.py').read_text().split('selected=')[0],ns)
entries=ns['entries'];payload=ns['payload']
sys.path.insert(0,str(work));from inspect_source_mdl import inspect
models=[n for n in entries if n.startswith('models/weapons/v_models/arms/') and n.endswith('.mdl')]
exe=work/'crowbar-src/Crowbar/bin/x86/Release/Crowbar.exe';results=[]
for name in sorted(models):
 model=root/'original'/name;model.parent.mkdir(parents=True,exist_ok=True)
 stem=name[:-4]
 for sibling in [n for n in entries if n.startswith(stem+'.') and n.endswith(('.mdl','.vvd','.vtx','.phy','.ani'))]:
  target=root/'original'/sibling;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(payload(sibling))
 dest=root/'decompiled'/model.stem;dest.mkdir(parents=True,exist_ok=True)
 if not list(dest.glob('*.qc')):
  run=subprocess.run([str(exe),'--headless-decompile',str(model),str(dest)],capture_output=True,timeout=180,creationflags=subprocess.CREATE_NO_WINDOW)
  (dest/'decompile.log').write_bytes(run.stdout+run.stderr)
  if run.returncode or not list(dest.glob('*.qc')):raise RuntimeError('Failed: '+name)
 entry=inspect(model);results.append(entry);print(model.stem,'bones',entry['numbones'],'skins',entry['numskinfamilies'],flush=True)
(root/'model-audit.json').write_text(json.dumps(results,indent=2),encoding='utf8')
print('Glove models:',len(results),flush=True)
