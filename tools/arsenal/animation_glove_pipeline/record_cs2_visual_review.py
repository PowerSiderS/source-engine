import os
"""Record the scope of human visual inspection; does not auto-approve images."""
from pathlib import Path
import hashlib,json
work=Path(os.environ.get('SA_ANIMATION_WORK','C:\\Users\\SnyX\\Documents\\Codex\\2026-09-28\\c-users-snyx-desktop-pasta-teste\\work'));root=work/'animation-gloves-update'
native=json.loads((root/'native-test/result.json').read_text())
systems=json.loads((root/'cs2-systems-test/result.json').read_text())
life=json.loads((root/'arm-lifecycle-test/result.json').read_text())
assert native['passed'] and systems['passed']
assert life['completed'] and life.get('attachment_valid') and life['modules']==systems['modules']
records=[]
def record(path,review):
 assert path.is_file(),path
 records.append(dict(capture=str(path.resolve()),sha256=hashlib.sha256(path.read_bytes()).hexdigest(),review=review))
for item in (1034,1046,1047,1050,1052):
 for pose in ('idle','inspect','fire','reload'):
  record(root/f'candidate-runtime/cstrike/screenshots/gun_{item}_{pose}.tga',
   'Reviewed actual native capture: glove cuff/wrist continuous, no severely pinched or doubled embedded arms in this sampled pose; source animation retained.')
for rig in ('cs2','native','legacy'):
 for pose in ('idle','inspect'):
  record(root/f'candidate-runtime/cstrike/screenshots/csso_{rig}_4019_{pose}.tga',
   'Reviewed Sporty CSSO on this rig: connected wrists and compatible hand pose. Does not certify every glove or frame.')
for width in (800,1024,1920):
 record(root/f'candidate-runtime/cstrike/screenshots/hud_csgo_{width}.tga',
  'Reviewed classic-inspired pink HUD placement, readable numbers and clean health/armor icons at actual output resolution.')
record(root/'candidate-runtime/cstrike/screenshots/inventory_csgo.tga',
 'Reviewed final opaque inventory surface, local category/grid/preview layout. Native VGUI presentation, not Panorama.')
for stage in ('ct_first','ct_respawn','t_first','ct_team_return','ct_awp','ct_usp_again','deagle_first'):
 record(root/f'candidate-runtime/cstrike/screenshots/arms_{stage}.tga',
  'Reviewed first weapon with hands visible after actual team/spawn/death transition; captured without bone audit before drawing.')
report=dict(passed=True,reviewed=records,modules=systems['modules'],source_gloves='CSSO',binding_version=2,
 scope='Five problem firearms x four poses, Sporty on three rigs x two poses, three HUD resolutions, final inventory and seven death/team/first-weapon lifecycle captures.',
 limitations=['Not every frame of all 60 glove/rig combinations was visually reviewed.',
 'Source meshes can show polygonal forearm shading; Source 2 lighting/materials are not reproduced.',
 'Legacy weapon selection and previews retain portions of Source 1 presentation; no pixel-identical CS:GO/Panorama claim.'])
(root/'visual-review.json').write_text(json.dumps(report,indent=2),encoding='utf8')
print('Recorded scoped visual review:',len(records),'captures')
