import os
"""Publish measured local and dedicated loading times, with tested hashes."""
from pathlib import Path
import hashlib,json
work=Path(os.environ.get('SA_ANIMATION_WORK','C:\\Users\\SnyX\\Documents\\Codex\\2026-09-28\\c-users-snyx-desktop-pasta-teste\\work'));root=work/'animation-gloves-update'
repo=Path(r'C:\Users\SnyX\Desktop\projeto clone\source-engine')
game=Path(r'C:\Users\SnyX\Desktop\Source Advanced V1 - PowerSiderS (PC)\Source Advanced V1 - @PowerSiderS (PC)')
dest=repo/'references/cs2';dest.mkdir(exist_ok=True)
local=json.loads((root/'map-loading-profile/result.json').read_text())
ded=json.loads((root/'dedicated-loading-profile/result.json').read_text())
assert local['passed'] and ded['passed']
def sha(p):
 with p.open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()
modules={p:sha(root/'timing-runtime'/p) for p in ('bin/engine.dll','bin/GameUI.dll','cstrike/bin/client.dll','cstrike/bin/server.dll')}
assert all(sha(game/p)==v for p,v in modules.items())
assert sha(root/'connection-client-runtime/cstrike/bin/client.dll')==modules['cstrike/bin/client.dll']
assert sha(root/'connection-server-runtime/cstrike/bin/server.dll')==sha(game/'Dedicated Server/runtime/cstrike/bin/server.dll')
rows=[];byname={r['map']:r for r in ded['maps']}
for r in local['maps']:
 d=byname[r['map']];rows.append(dict(map=r['map'],local_seconds=r['client_active_seconds'],dedicated_seconds=d['client_active_seconds'],local_server_ready=r['server_ready_seconds'],dedicated_server_ready=d['server_ready_seconds']))
def public_report(data):
 return {k:([{a:b for a,b in r.items() if a!='status'} for r in v] if k=='maps' else {a:b for a,b in v.items() if a!='status'} if isinstance(v,dict) else v) for k,v in data.items() if k!='pid'}
report=dict(passed=True,snapshot='2026-10-01',modules=modules,local=public_report(local),dedicated=public_report(ded),summary=rows)
(dest/'loading_times.json').write_text(json.dumps(report,indent=2),encoding='utf8')
lines=['# Tempos reais de carregamento','',
 'Medicao em 2026-10-01 neste PC: cliente privado em janela 800x600, 60 FPS, sem bots; dedicated 128 tick por sockets de loopback, net_usesocketsforloopback=1 e portas distintas. Mapas medidos como changelevel de uma sessao ja ativa. Logs de diagnostico e cl_profile_map_load ativados. Cache de arquivos/shaders aquecido; nao mede primeiro carregamento apos reiniciar o Windows nem latencia de Internet.',
 '',f"Abertura do executavel ate Mirage pronto: {local['bootstrap']['client_active_seconds']:.2f} s. Abertura do cliente separado ate conectar ao dedicated ja iniciado: {ded['initial_connection']['client_active_seconds']:.2f} s.",
 '', '| Mapa | Partida local (s) | Dedicated + cliente (s) |','|---|---:|---:|']
lines += [f"| {r['map']} | {r['local_seconds']:.2f} | {r['dedicated_seconds']:.2f} |" for r in rows]
lines += ['', 'Todos os dez mapas completaram nos dois modos. O fim exige client_ready depois que CL_FullyConnected fecha o carregamento, junto com jogador active no status do servidor. Polling acrescenta aproximadamente 0,5 a 1 segundo. As colunas sao tempos totais independentes; nao devem ser somadas. Os dados de fases e hashes ficam em loading_times.json. server_ready_seconds representa a primeira resposta de status que informa o mapa correto; na partida local, o servidor e o cliente compartilham a thread e esse valor pode incluir bloqueios do cliente.', '', 'Estes tempos mostram que alguns mapas ainda sao pesados. Nao comprovam que a engine inteira foi otimizada, nem uma melhora percentual sem um benchmark anterior nas mesmas condicoes.']
text='\n'.join(lines)+'\n';(dest/'CARREGAMENTOS.md').write_text(text,encoding='utf8');(game/'RELATORIO_CARREGAMENTOS.md').write_text(text,encoding='utf8')
manifest_path=game/'animation_gloves_manifest.json'
manifest=json.loads(manifest_path.read_text(encoding='utf8'))
manifest['loading_test_report']=str(dest/'loading_times.json')
manifest['loading_summary']=rows
manifest_path.write_text(json.dumps(manifest,indent=2),encoding='utf8')
print(text)
