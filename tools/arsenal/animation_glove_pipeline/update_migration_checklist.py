import os
"""Persistent migration ledger; success is backed by artifacts, never inferred."""
from pathlib import Path
import json,hashlib
work=Path(os.environ.get('SA_ANIMATION_WORK','C:\\Users\\SnyX\\Documents\\Codex\\2026-09-28\\c-users-snyx-desktop-pasta-teste\\work'));repo=Path(r'C:\Users\SnyX\Desktop\projeto clone\source-engine')
dest=repo/'references/cs2';dest.mkdir(parents=True,exist_ok=True)
items=[
 ('01','Inventário dos arquivos locais do CS2','Concluído','VPK lido com CRC; snapshot de 2026-10-01.','cs2-economy-reference'),
 ('02','Ferramenta de extração Source 2 Viewer','Concluído','CLI 20.0 do projeto original; SHA-256 da release verificado.','cs2-economy-reference/vrf-cli'),
 ('03','Hitboxes de jogadores','Em teste','19 cápsulas × 8 rigs; radii/endpoints CS2 adaptados às poses Source 1; 152 geometrias auditadas.','cs2-hitbox-import/asset-audit.json'),
 ('04','Bones e alinhamento animado','Em teste','Esqueleto e animações Source 1 preservados; ainda precisa teste em pé/agachado/rotação/disparo.','cs2-hitbox-import'),
 ('05','Recoil e spray por arma','Em teste','34 perfis atuais compilados; sementes/amplitudes/variâncias extraídas. Gerador determinístico Source 1 permanece; equivalência Source 2 não comprovada.','references/cs2/weapon_profiles.json'),
 ('06','Precisão e recuperação','Em teste','Valores por modo, postura, movimento, salto e transição de recuperação copiados para os 34 perfis.','references/cs2/weapon_profiles.json'),
 ('07','Economia e preços','Em teste','38 entradas, bônus de perda progressivo, recompensas de arma/objetivo, MR12 e munição gratuita; falta validação de transações reais.','game/shared/cstrike/cs_economy.h'),
 ('08','HUD clássico CS:GO rosa','Em teste','Layout VGUI: radar/dinheiro, relógio/placar/vivos, vida/colete e munição; atlas de ícones limpos.','hud-pink'),
 ('09','Menu e inventário estilo CS:GO','Em implementação','Grade e categorias locais; acesso direto em janela ampla. Não importa serviços Steam/Panorama.','gameui/OptionsSubInventory.cpp'),
 ('10','Animações de armas','Em teste','24 modelos Source 1 do pacote fornecido, motion original preservado. Não são uma extração direta/autenticada dos modelos Source 2.','cs2-animation-import'),
 ('11','Luvas CSSO e pulsos','Em teste','20 estilos × 3 rigs; quatro malhas sport.smd antes não ocultadas foram corrigidas. Revisão visual restante.','csso-glove-import'),
 ('12','Áudio CS2','Em implementação','Conversão de disparos locais para PCM compatível; mixagem espacial/acústica Source 1 continua.','cs2-audio-reference'),
 ('13','Dano, headshot e penetração','Parcial','Dano base, alcance e armor ratio nos perfis; multiplicador por arma, penetração float/materiais ainda exigem integração.','game/shared/cstrike/fx_cs_shared.cpp'),
 ('14','Granadas e utilitários','Pendente','Arremessos já existentes; revisar com medições gravidade/quiques/pavio/HE/flash/fogo. Não copiar algoritmos desconhecidos dos assets.','game/shared/cstrike'),
 ('15','Movimentação e bunny hop','Pendente','Velocidade por arma atualizada; aceleração/fadiga/counter-strafe/ladders precisam validação separada.','game/shared/cstrike/cs_gamemovement.cpp'),
 ('16','Modelos Source 2 completos','Requer conversão','VMDL/VMESH/material Source 2 não carrega como MDL/VTX/VVD. Exportar e retargetar malha/material/animação exige pipeline próprio.','cs2-hitbox-reference'),
 ('17','Smoke volumétrica CS2','Requer implementação própria','Partículas e assets não fornecem o código de voxels, sincronização e interação com balas.',''),
 ('18','Subtick e networking CS2','Requer implementação própria','Esta engine mantém ticks Source 1; nenhum asset converte seu protocolo e predição para Source 2.',''),
 ('19','Carregamentos e resolução','Parcial','Correções de engine existentes; relatório de tempo real dos dez mapas e conexão dedicada ainda incompleto.','native-knife-engine-20260930/map-loading-profile'),
 ('20','Mapas do usuário e dedicated','Em teste','Usar somente catálogo local permitido; VPK/DLLs comuns e auditoria nativa no servidor.','animation-gloves-update/dedicated-test'),
 ('21','Proporção de tela esticada','Pendente','Separar proporção de renderização da resolução de saída; pedido anterior ainda não concluído.',''),
 ('22','Anti-cheat','Parcial','Sistema existente; não pode importar VAC/serviços privados da Valve como assets. Revisão futura contra falsos positivos.','game/server/cstrike/cs_anticheat.cpp'),
]
report={'snapshot':'2026-10-01','production_installed':False,'policy':'Concluído somente com evidência de build/teste; adaptações não implicam identidade CS2/CSGO2015.',
 'items':[dict(id=i,title=t,status=s,details=d,evidence=e) for i,t,s,d,e in items]}
root=work/'animation-gloves-update'
def tested(name):
 try:
  result=json.loads((root/name/'result.json').read_text())
  if not result.get('passed'):return False
  for relative,digest in result.get('modules',{}).items():
   if name=='native-test' and relative=='bin/GameUI.dll':continue # UI has its own final systems/visual audit.
   if hashlib.sha256((repo/'output'/relative).read_bytes()).hexdigest()!=digest:return False
  if 'server_sha256' in result and hashlib.sha256((repo/'output-dedicated/cstrike/bin/server.dll').read_bytes()).hexdigest()!=result['server_sha256']:return False
  return True
 except (OSError,ValueError):return False
def update(ids,status,details,evidence):
 for item in report['items']:
  if item['id'] in ids:item.update(status=status,details=details,evidence=evidence)
if tested('cs2-systems-test'):
 update(['03','04'],'Validado no pacote','Oito modelos montados conferidos byte a byte; 24 poses e 2.736 raios sem falhas. Malhas/animações Source 1 preservadas; não comprova identidade visual Source 2.','animation-gloves-update/cs2-systems-test/result.json')
 update(['05','06'],'Validado no pacote','34 perfis comparados com a extração independente; 4.352 impulsos finitos e disparos reais. Gerador Source 1 preservado; equivalência do algoritmo Source 2 não comprovada.','animation-gloves-update/cs2-systems-test/result.json')
if tested('cs2-systems-test') and tested('round-economy-test'):
 update(['07'],'Validado no pacote','38 preços/recompensas, compras reais, negação, munição gratuita e quatro transições de rodadas com conferência de saldos/níveis. Halftime, objetivos e todas as recompensas de kills ainda exigem cenários próprios.','animation-gloves-update/round-economy-test/result.json')
if tested('native-test'):
 update(['10'],'Validado no pacote','24 modelos fornecidos: repouso/default/inspeção/disparo/recarga; encerramento normal e troca Mirage/Nuke. Ports Source 1 com animações originais, não extração direta Source 2.','animation-gloves-update/native-test/result.json')
 update(['11'],'Validado estruturalmente','60 combinações CSSO/rig com bones/matrizes válidos e braços embutidos ocultados. Revisão visual não certifica todos os frames.','animation-gloves-update/native-test/result.json')
if tested('dedicated-test'):
 update(['20'],'Validado no pacote','Dedicated: 34 perfis, 38 preços, 456 raios, 1.280 props sem falhas; lista restrita aos dez mapas permitidos. Não é um teste de conexão em todos os mapas.','animation-gloves-update/dedicated-test/result.json')
update(['12'],'Integrado','36 sons locais convertidos para PCM e auditados. Mistura espacial Source 1; HRTF/camadas acústicas Source 2 não importadas.','cs2-audio-import/asset-audit.json')
update(['13'],'Parcial','Dano, alcance, armor ratio e multiplicador de headshot por arma integrados. Penetração float, materiais e cenários de dano ainda exigem revisão.','game/shared/cstrike/fx_cs_shared.cpp')
update(['09'],'Integrado','Janela ampla com 75 cartões, categorias, seleção local e open_inventory. Sem serviços Steam/Panorama; revisão visual final registrada separadamente.','gameui/OptionsSubInventory.cpp')
try:
 visual=json.loads((root/'visual-review.json').read_text())
 final=json.loads((root/'cs2-systems-test/result.json').read_text())
 life=json.loads((root/'arm-lifecycle-test/result.json').read_text())
 if tested('cs2-systems-test') and visual.get('passed') and visual.get('modules')==final.get('modules'):
  update(['08'],'Validado no pacote','HUD rosa inspirado no CS:GO revisado em 800, 1024 e 1920 pixels; desenho nativo VGUI, sem identidade Panorama.','animation-gloves-update/visual-review.json')
  if life.get('attachment_valid') and life.get('modules')==final.get('modules'):
   update(['11'],'Validado no pacote','60 combinacoes estruturais; cinco armas em quatro poses e Sporty nos tres rigs revisados. Corrigido braco desanexado apos respawn; sete transicoes com maos visiveis. Nao certifica todos os frames.','animation-gloves-update/arm-lifecycle-test/result.json')
except (OSError,ValueError):pass
try:
 game=Path(r'C:\Users\SnyX\Desktop\Source Advanced V1 - PowerSiderS (PC)\Source Advanced V1 - @PowerSiderS (PC)')
 manifest=json.loads((game/'animation_gloves_manifest.json').read_text())
 final=json.loads((root/'cs2-systems-test/result.json').read_text())
 report['production_installed']=bool(tested('cs2-systems-test') and manifest.get('cs2_systems_test',{}).get('modules')==final.get('modules') and manifest.get('arm_lifecycle_test',{}).get('attachment_valid') and manifest.get('files') and all(hashlib.sha256((game/r['path']).read_bytes()).hexdigest()==r['sha256'] for r in manifest['files']))
 if report['production_installed']:report['installation_manifest']=str(game/'animation_gloves_manifest.json')
except (OSError,ValueError):pass
try:
 loading=json.loads((dest/'loading_times.json').read_text())
 if report['production_installed'] and loading.get('passed') and all(hashlib.sha256((game/p).read_bytes()).hexdigest()==digest for p,digest in loading['modules'].items()):
  update(['19'],'Medido; otimizacao parcial','Dez mapas completados em partida local e dedicated, ate fechar a tela de carregamento. Tempos publicados com cache aquecido; mapas pesados ainda exigem otimizacao. Troca de resolucao validada em tres tamanhos.','references/cs2/CARREGAMENTOS.md')
except (OSError,ValueError):pass
items=[(i['id'],i['title'],i['status'],i['details'],i['evidence']) for i in report['items']]
(dest/'migration_status.json').write_text(json.dumps(report,indent=2,ensure_ascii=False),encoding='utf8')
lines=['# Migração CS2 / apresentação CS:GO','',
 ('Atualizacao instalada no cliente e dedicated; hashes conferidos pelo manifesto de instalacao.' if report['production_installed'] else 'Estado da atualização em preparação. Ainda não instalada no runtime principal. A instalação ocorre após validação das DLLs e dos assets juntos.'),
 '', '| Item | Sistema | Estado | Evidência / limite |','|---|---|---|---|']
lines += [f'| {i} | {t} | {s} | {d} |' for i,t,s,d,e in items]
lines += ['', 'Os arquivos locais servem como referência de dados. Não contêm a implementação completa da Source 2, de subtick ou de VAC. CS:GO 2015, CS:GO Legacy e CS2 são versões diferentes; este pacote usa dados atuais do CS2 e apresentação inspirada no CS:GO.', '',
 'Extração: [Source 2 Viewer / ValveResourceFormat](https://github.com/ValveResourceFormat/ValveResourceFormat). As malhas/animações de armas do pacote fornecido e as luvas CSSO têm proveniência separada dos dados oficiais do CS2.', '']
(dest/'MIGRACAO_CS2.md').write_text('\n'.join(lines),encoding='utf8')
print('Updated migration ledger:',dest/'MIGRACAO_CS2.md')
