import os
"""Deploy the tested CS2-data/CSGO-presentation update with backup and rollback."""
from pathlib import Path
from datetime import datetime
import hashlib,json,os,shutil,subprocess
work=Path(os.environ.get('SA_ANIMATION_WORK','C:\\Users\\SnyX\\Documents\\Codex\\2026-09-28\\c-users-snyx-desktop-pasta-teste\\work'));root=work/'animation-gloves-update'
repo=Path(r'C:\Users\SnyX\Desktop\projeto clone\source-engine')
game=Path(r'C:\Users\SnyX\Desktop\Source Advanced V1 - PowerSiderS (PC)\Source Advanced V1 - @PowerSiderS (PC)')
ded=game/'Dedicated Server/runtime'
def read(name):return json.loads((root/name).read_text(encoding='utf8'))
def sha(path):
 with path.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
native=read('native-test/result.json');server=read('dedicated-test/result.json');binding=read('wrist-binding-audit.json');package=read('package-validation.json');visual=read('visual-review.json');stage=read('stage-audit.json')
systems=read('cs2-systems-test/result.json');rounds=read('round-economy-test/result.json')
life=read('arm-lifecycle-test/result.json')
hitboxes=json.loads((work/'cs2-hitbox-import/asset-audit.json').read_text())
audio=json.loads((work/'cs2-audio-import/asset-audit.json').read_text())
if not all(r['passed'] for r in (native,server,binding,visual,systems,rounds,hitboxes,audio)):raise RuntimeError('All integration, round rewards, asset and visual audits must pass')
if not life['completed'] or not life.get('attachment_valid') or life['modules']!=systems['modules']:raise RuntimeError('First-weapon respawn/team lifecycle test does not cover the final build')
if visual.get('modules')!=systems['modules']:raise RuntimeError('Visual review does not cover the final systems/UI build')
for relative in ('cstrike/bin/client.dll','cstrike/bin/server.dll'):
 if systems['modules'][relative]!=native['modules'][relative]:raise RuntimeError('Gameplay DLLs changed between systems and firearm tests')
for build in ('build','build-dedicated'):
 deps=json.loads((repo/build/'msvc_header_dependency_audit.json').read_text())
 if not deps or not all(d['weapon_header'] for d in deps):raise RuntimeError('Untracked weapon header dependencies: '+build)
for relative,digest in systems['modules'].items():
 if sha(repo/'output'/relative)!=digest:raise RuntimeError('Build changed since native test: '+relative)
if sha(root/'000_sourceadvanced_catalog.vpk')!=package['sha256']:raise RuntimeError('Catalog changed since CRC verification')
if sha(root/'candidate-runtime/cstrike/custom/000_sourceadvanced_catalog.vpk')!=package['sha256']:raise RuntimeError('Test runtime catalog mismatch')
if sha(root/'candidate-dedicated-runtime/cstrike/custom/000_sourceadvanced_catalog.vpk')!=package['sha256']:raise RuntimeError('Dedicated runtime catalog mismatch')
if sha(repo/'output-dedicated/cstrike/bin/server.dll')!=sha(root/'candidate-dedicated-runtime/cstrike/bin/server.dll'):raise RuntimeError('Dedicated server changed since test')
if server['server_sha256']!=rounds['server_sha256'] or rounds['server_sha256']!=sha(repo/'output-dedicated/cstrike/bin/server.dll'):raise RuntimeError('Round reward test used a different dedicated DLL')
if server['catalog_sha256']!=package['sha256']:raise RuntimeError('Dedicated audit used a different VPK')
env=dict(os.environ,SA_UPDATE_GAME_ROOT=str(game))
guard=subprocess.run(['powershell','-NoProfile','-Command',"$taskPrefix=$env:SA_UPDATE_GAME_ROOT+'\\'; $taskRunning=Get-CimInstance Win32_Process | Where-Object { $_.ExecutablePath -and $_.ExecutablePath.StartsWith($taskPrefix,[StringComparison]::OrdinalIgnoreCase) -and $_.Name -in @('hl2_launcher.exe','dedicated_launcher.exe') }; if($taskRunning){ $taskRunning | Select-Object ProcessId,Name,ExecutablePath | ConvertTo-Json -Compress; exit 1 }"],env=env,capture_output=True,text=True)
if guard.returncode:raise RuntimeError('Main game/server is open; close before deployment: '+guard.stdout.strip())
configs={str(p.relative_to(game)):sha(p) for rt in (game,ded) for p in (rt/'cstrike/cfg').rglob('*') if p.is_file()}
actions=[(repo/'output'/r,game/r) for r in ('cstrike/bin/client.dll','cstrike/bin/server.dll','bin/GameUI.dll')]
actions+=[(repo/'output-dedicated/cstrike/bin/server.dll',ded/'cstrike/bin/server.dll')]
for rt in (game,ded):
 actions+=[(root/'assets/cstrike/scripts/skins_manifest.txt',rt/'cstrike/scripts/skins_manifest.txt'),(root/'000_sourceadvanced_catalog.vpk',rt/'cstrike/custom/000_sourceadvanced_catalog.vpk')]
notes=root/'ATUALIZACAO_ANIMACOES_E_LUVAS.md'
notes.write_text('''# Dados CS2, HUD CS:GO e animações/luvas

Dados extraídos do CS2 instalado em 2026-10-01: 34 perfis de armas (recoil, precisão, recuperação, dano, headshot e velocidade), 38 entradas de economia, 19 cápsulas de referência adaptadas para oito esqueletos Source 1 e 36 sons de disparo convertidos para PCM. Cliente e servidor dedicado recebem os mesmos dados. O gerador determinístico de recoil e o protocolo de rede continuam Source 1; os testes não estabelecem identidade com CS2 ou CS:GO 2015.

Economia: preços compilados prevalecem sobre packs cosméticos; bônus de derrota progressivo, recompensas de armas e objetivos, munição gratuita e MR12. Os testes verificam compras reais, negação por equipe, arma já possuída, munição e transições de vitória/derrota com o saldo de cada jogador.

HUD VGUI inspirado no CS:GO, rosa: radar/dinheiro à esquerda, relógio/placar/vivos no topo, vida/colete e munição nos cantos inferiores. Inventário amplo com categorias e grade local; comando open_inventory. Não contém serviços de inventário Steam nem Panorama.

O rastreamento de headers do MSVC em português foi corrigido no Waf. Uma auditoria do build exige o header de dados das armas nas dependências do parser e dos disparos, evitando misturar objetos com estruturas antigas.

24 modelos de armas do pacote local foram recompilados preservando os quadros numéricos originais. As 21 facas existentes continuam com as animações nativas já implementadas. Os arquivos fornecidos são ports Source 1; esta atualização não autentica arquivos Valve nem promete correspondência com uma versão específica do CS:GO.

O Inventário inclui **Gloves**, com 20 estilos CSSO. A seleção é aplicada durante a partida. Cada estilo tem versões para os três rigs encontrados nos modelos, e a engine escolhe a versão compatível. `inventory_gloves 4000..4019` seleciona por console; `inventory_gloves 0` restaura CSSO Sporty (4019).

O pulso no rig CS2 foi corrigido com frames anatômicos de palma/antebraço e pesos nos ossos TWIST1. A malha é ancorada no punho; o braço direito respeita a inversão de eixos. As UVs e a topologia do CSSO permanecem preservadas. As mãos embutidas no pacote de armas são ocultadas após carregar a luva CSSO válida.

Corrigido o desaparecimento das mãos na primeira arma após morte/respawn: a engine recria a entidade de braços quando ela perde a ligação com o viewmodel. A validação cobre morte, renascimento e troca CT/T com capturas sem auditoria de ossos antes do desenho.

O shader Character do CSSO não existe nesta engine; as texturas originais usam VertexLitGeneric. A aparência de iluminação pode diferir do CSSO. São estilos de luvas/modelos, não uma importação de todos os paint kits.

Validação: 24 armas em repouso/inspeção/disparo/recarga; 60 combinações de luva/rig; resoluções; Mirage e Nuke; encerramento normal. AWP preservada com 5 balas. Dedicated: 34 armas sem falhas, auditoria de hitboxes/cápsulas sem falhas e catálogo restrito de 10 mapas. As auditorias não provam que todo frame de toda luva esteja visualmente perfeito; as capturas de revisão ficam no manifesto.

Os arquivos foram empacotados com o VPK local e todos os CRCs/payloads foram verificados. O manifesto de inventário permanece solto e editável. Os scripts reproduzíveis ficam em tools/arsenal/animation_glove_pipeline no repositório.

Esta atualização altera hitboxes, dados de armas, economia e apresentação. Configurações pessoais são preservadas. Penetração, granadas, movimentação completa, relatório de tempos dos dez mapas e proporção esticada continuam com validação própria pendente. Modelos Source 2 completos, smoke volumétrica e subtick não foram importados. A lista em references/cs2/MIGRACAO_CS2.md registra os limites e o restante do trabalho.
''',encoding='utf8')
actions.append((notes,game/notes.name))
backup=game/'backups'/('animation-gloves-'+datetime.now().strftime('%Y%m%d-%H%M%S'));backup.mkdir(parents=True)
records=[];done=[]
try:
 for built,dest in actions:
  digest=sha(built);relative=dest.relative_to(game);existed=dest.is_file()
  if existed and sha(dest)==digest:records.append({'path':relative.as_posix(),'sha256':digest,'changed':False});continue
  saved=backup/relative
  if existed:saved.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(dest,saved)
  dest.parent.mkdir(parents=True,exist_ok=True);done.append((dest,saved,existed));shutil.copy2(built,dest)
  if sha(dest)!=digest:raise RuntimeError('Copy mismatch: '+str(relative))
  records.append({'path':relative.as_posix(),'sha256':digest,'changed':True,'previous_file':existed,'bytes':dest.stat().st_size})
 for name,digest in configs.items():
  if not (game/name).is_file() or sha(game/name)!=digest:raise RuntimeError('User cfg changed: '+name)
 manifest=game/'animation_gloves_manifest.json'
 if manifest.exists():shutil.copy2(manifest,backup/manifest.name)
 manifest.write_text(json.dumps({'backup':str(backup),'files':records,'native_test':native,'dedicated_test':server,'cs2_systems_test':systems,'round_rewards_test':rounds,'arm_lifecycle_test':life,'hitbox_asset_audit':hitboxes,'audio_asset_audit':audio,'binding_audit':binding,'visual_review':visual,'catalog_sha256':package['sha256'],'user_cfg_hashes':configs,'pipeline':str(repo/'tools/arsenal/animation_glove_pipeline')},indent=2),encoding='utf8')
except Exception:
 for dest,saved,existed in reversed(done):
  if existed:shutil.copy2(saved,dest)
  else:dest.unlink(missing_ok=True)
 raise
print(json.dumps({'installed':sum(r['changed'] for r in records),'backup':str(backup),'manifest':str(manifest)},indent=2),flush=True)
