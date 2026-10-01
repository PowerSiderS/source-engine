import os
"""Check CSSO mesh/UV retention, twist coverage and unchanged weapon motions."""
from pathlib import Path
import json,sys,hashlib,numpy as np
work=Path(os.environ.get('SA_ANIMATION_WORK','C:\\Users\\SnyX\\Documents\\Codex\\2026-09-28\\c-users-snyx-desktop-pasta-teste\\work'))
sys.path.insert(0,r'C:\Users\SnyX\Desktop\projeto clone\source-engine\tools\arsenal')
from port_catalog import nodes,global_pose,rename_bones
gloves=work/'csso-glove-import';guns=work/'cs2-animation-import'
def vertices(text):
 lines=text.split('triangles\n')[1].splitlines();result=[]
 for off in range(0,len(lines)-1,4):
  if lines[off]=='end':break
  for line in lines[off+1:off+4]:
   v=line.split();result.append((lines[off],v))
 return result
report={'styles':[],'weapons':[],'failures':[],'scope':'bind geometry/UVs/bone coverage/numeric motion; native captures remain necessary for visual quality'}
catalog=json.loads((gloves/'asset-audit.json').read_text())['catalog']
for style in catalog:
 name=Path(style['models']['cs2']).stem.removesuffix('_cs2')
 directory=gloves/'decompiled'/name
 qc=next(directory.glob('*.qc')).read_text(encoding='utf-8-sig')
 import re
 original=[]
 for fn in re.findall(r'studio\s+"([^"\r\n]+\.smd)"',qc):original+=vertices((directory/fn).read_text(encoding='utf-8-sig'))
 text=(gloves/'authored'/name/'cs2/arms.smd').read_text();out=vertices(text);bn={i:n for i,n,p in nodes(text)}
 uv_ok=len(original)==len(out) and all(m==n and a[7:9]==b[7:9] for (m,a),(n,b) in zip(original,out))
 weights_ok=True
 for m,v in out:
  weights=[float(v[11+2*j]) for j in range(int(v[9]))]
  weights_ok&=bool(weights) and min(weights)>0 and abs(sum(weights)-1)<2e-6 and np.isfinite(np.array(list(map(float,v[1:7])))).all()
 used=set(bn.values());twists=all('arm_lower_'+s+'_TWIST1' in used for s in ('L','R'))
 item={'style':name,'vertices':len(out),'uvs_and_topology_preserved':uv_ok,'valid_weights_and_geometry':bool(weights_ok),'twist_bones':twists,'bind_sha256':hashlib.sha256(text.encode()).hexdigest()}
 report['styles'].append(item)
 if not(uv_ok and weights_ok and twists):report['failures'].append(name)
ref=global_pose((gloves/'authored/v_glove_sporty/cs2/arms.smd').read_text());required=['arm_lower_'+s+'_TWIST1' for s in ('L','R')]
for model in json.loads((guns/'asset-audit.json').read_text())['models']:
 folder=model['model'];dst=guns/'authored'/folder;src=guns/'decompiled'/folder
 animations=[];bone_coverage=True
 for path in dst.rglob('*.smd'):
  if path.parent==dst:continue
  original=src/path.relative_to(dst)
  a=original.read_text(encoding='utf-8-sig');b=path.read_text(encoding='utf-8-sig')
  numeric=a.split('skeleton\n',1)[1]==b.split('skeleton\n',1)[1]
  names={n for _,n,_ in nodes(b)}
  required=['arm_lower_'+s+'_TWIST1' for s in ('L','R')] if 'arm_lower_L' in names else ['ValveBiped.Bip01_'+s+'_ForeTwist' for s in ('L','R')]
  coverage=all(n in names for n in required);bone_coverage&=coverage
  animations.append({'clip':path.name,'numeric_motion_unchanged':numeric,'twist_coverage':coverage})
 item={'model':folder,'animations':animations,'bone_coverage':bone_coverage,'numeric_motion_unchanged':all(a['numeric_motion_unchanged'] for a in animations)}
 report['weapons'].append(item)
 if not(bone_coverage and item['numeric_motion_unchanged']):report['failures'].append(folder)
report['passed']=len(report['styles'])==20 and len(report['weapons'])==24 and not report['failures']
(work/'animation-gloves-update/wrist-binding-audit.json').write_text(json.dumps(report,indent=2),encoding='utf8')
print('CSSO styles',len(report['styles']),'weapon rigs',len(report['weapons']),'failures',report['failures'],'passed',report['passed'],flush=True)
raise SystemExit(0 if report['passed'] else 1)
