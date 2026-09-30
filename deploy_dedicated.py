import os, shutil

source_root = r'C:\Users\SnyX\Desktop\projeto clone\source-engine'
output_client = os.path.join(source_root, 'output')
output_ded = os.path.join(source_root, 'output-dedicated')

dedicated_suites = [
    r'C:\Users\SnyX\Downloads\Compressed\Source Advanced V1 - PowerSiderS (PC)\Source Advanced V1 - @PowerSiderS (PC)\Dedicated Server Launcher',
    r'C:\Users\SnyX\Desktop\Source Advanced V1 - PowerSiderS (PC)\Source Advanced V1 - @PowerSiderS (PC)\Dedicated Server'
]

launcher_json_content = '''{
  "ServerName": "Source Advanced V1 | Dedicated (128 Tick)",
  "DefaultMap": "de_mirage_csgo",
  "MaxPlayers": 16,
  "Port": 25565,
  "AutoRestartOnCrash": true,
  "RestartDelaySeconds": 5,
  "ExtraArguments": [
    "-tickrate 128",
    "-nohltv",
    "-strictportbind"
  ]
}
'''

mapcycle_content = '''de_dust2_go
de_mirage_csgo
de_inferno_csgo_cssold_fix
de_anubis_go
de_cache_rework
de_overpass_csgo
de_ancient_go
de_nuke_go
de_vertigo_csgo_new
'''

sourceadvanced_servers_content = '''201.43.172.69:25565
192.168.15.29:25565
'''

server_cfg_content = '''// Source Advanced V1 - Servidor Dedicado 128-Tick
hostname "Source Advanced V1 | Dedicated (128 Tick)"
sv_password ""
rcon_password ""

// Taxas de rede e integridade 128-tick
sv_cheats 0
sv_pure 0
sv_consistency 1
sv_region 255
sv_allowdownload 1
sv_allowupload 0
sv_voiceenable 1
sv_alltalk 0
sv_timeout 65
sv_minrate 786432
sv_maxrate 0
sv_minupdaterate 128
sv_maxupdaterate 128
sv_mincmdrate 128
sv_maxcmdrate 128
sv_client_min_interp_ratio 1
sv_client_max_interp_ratio 2
fps_max 0

// Movimentacao estilo CS:GO
sv_accelerate 5.5
sv_friction 5.2
sv_stopspeed 80
sv_airaccelerate 12
sv_maxspeed 320
sv_staminarecoveryrate 60
sv_staminajumpcost 0.080
sv_staminalandcost 0.050
sv_staminamax 80
sv_timebetweenducks 0.4
sv_enablebunnyhopping 0
sv_autobunnyhopping 0
sv_bunnyhop_stamina_threshold 22
weapon_accuracy_model 2

// Regras de Partida Competitiva CS:GO
mp_autoteambalance 1
mp_limitteams 1
mp_friendlyfire 0
mp_freezetime 5
mp_roundtime 1.92
mp_c4timer 40
mp_startmoney 800
mp_buytime 0.20
mp_maxrounds 30
mp_winlimit 16
mp_timelimit 0
mp_chattime 5
mp_allowspectators 1
mp_forcecamera 1
mp_footsteps 1
mp_falldamage 1
mp_tkpunish 0

// Bots
bot_quota 0
bot_quota_mode fill
bot_difficulty 2
bot_chatter normal

// Logs
log on
sv_logfile 1
sv_log_onefile 0
sv_logecho 1
sv_logbans 1
sv_logflush 0
writeid
writeip
'''

iniciar_lan_cmd = '''@echo off
title Source Advanced V1 - Dedicated LAN (128 Tick)
cd /d "%~dp0"
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File ".\\scripts\\start-server.ps1" -Mode lan
echo.
pause
'''

iniciar_online_cmd = '''@echo off
title Source Advanced V1 - Dedicated Online (128 Tick)
cd /d "%~dp0"
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File ".\\scripts\\start-server.ps1" -Mode online
echo.
pause
'''

parar_cmd = '''@echo off
cd /d "%~dp0"
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File ".\\scripts\\stop-server.ps1"
pause
'''

verificar_cmd = '''@echo off
cd /d "%~dp0"
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File ".\\scripts\\validate-server.ps1"
pause
'''

firewall_cmd = '''@echo off
cd /d "%~dp0"
for /f %%P in ('powershell.exe -NoProfile -Command "(Get-Content -LiteralPath '.\\config\\launcher.json' -Raw ^| ConvertFrom-Json).Port"') do set SERVER_PORT=%%P
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "Start-Process powershell.exe -Verb RunAs -ArgumentList '-NoProfile -ExecutionPolicy Bypass -Command \"\"New-NetFirewallRule -DisplayName ''''Source Advanced V1 Dedicated UDP'''' -Direction Inbound -Action Allow -Protocol UDP -LocalPort %SERVER_PORT% -ErrorAction SilentlyContinue; New-NetFirewallRule -DisplayName ''''Source Advanced V1 Dedicated TCP'''' -Direction Inbound -Action Allow -Protocol TCP -LocalPort %SERVER_PORT% -ErrorAction SilentlyContinue\"\"'"
echo Solicitacao enviada ao Windows. Encaminhe UDP %SERVER_PORT% no roteador.
pause
'''

atualizar_cmd = '''@echo off
title Compilar e atualizar Dedicated Server
cd /d "%~dp0"
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File ".\\scripts\\build-and-deploy.ps1"
echo.
pause
'''

readme_txt = '''SOURCE ADVANCED V1 - SERVIDOR DEDICADO 128-TICK

INICIO RAPIDO:
1. Use INICIAR SERVIDOR LAN.cmd para jogar na sua rede local (128 Tick).
2. Use INICIAR SERVIDOR ONLINE.cmd para aceitar jogadores pela internet.
3. Use VERIFICAR SERVIDOR.cmd para checar se todos os arquivos e DLLs estao ok.
4. Use PARAR SERVIDOR.cmd para desligar o processo dedicado.

CONFIGURACOES:
- config\\launcher.json: Porta (25565), mapa padrao (de_mirage_csgo), maxplayers e tickrate 128.
- config\\server.cfg: Taxas de 128 tick, movimentacao CS:GO e regras de partida.
- config\\mapcycle.txt: Rotacao dos mapas CS2.
- cstrike\\cfg\\sourceadvanced_servers.txt: lista privada usada somente por esta engine.

O servidor nao e registrado na lista publica/global da Steam. Apenas clientes desta
Source Advanced com o endereco em sourceadvanced_servers.txt recebem a entrada.
'''

start_server_ps1 = '''param(
    [ValidateSet('lan','online')]
    [string]$Mode = 'lan'
)

$ErrorActionPreference = 'Stop'
$serverRoot = Split-Path -Parent $PSScriptRoot
$runtime = Join-Path $serverRoot 'runtime'
$exe = Join-Path $runtime 'dedicated_launcher.exe'
$settingsFile = Join-Path $serverRoot 'config\\launcher.json'
$serverCfg = Join-Path $serverRoot 'config\\server.cfg'
$mapcycle = Join-Path $serverRoot 'config\\mapcycle.txt'

if (-not (Test-Path -LiteralPath $exe)) {
    throw 'Runtime ausente. Execute ATUALIZAR DA SOURCE.cmd ou reinstale o runtime.'
}

$settings = Get-Content -LiteralPath $settingsFile -Raw | ConvertFrom-Json
$map = [string]$settings.DefaultMap
$port = [int]$settings.Port
$maxPlayers = [int]$settings.MaxPlayers
if ($map -notmatch '^[A-Za-z0-9_-]+$') { throw 'DefaultMap possui caracteres invalidos.' }
if ($port -lt 1024 -or $port -gt 65535) { throw 'Port precisa estar entre 1024 e 65535.' }
if ($maxPlayers -lt 1 -or $maxPlayers -gt 64) { throw 'MaxPlayers precisa estar entre 1 e 64.' }

$gameRoot = Split-Path -Parent $serverRoot
$mapFile = Join-Path $gameRoot ('cstrike\\maps\\' + $map + '.bsp')
if (-not (Test-Path -LiteralPath $mapFile)) { throw "Mapa nao encontrado: $mapFile" }

$sameRuntime = Get-CimInstance Win32_Process -Filter "Name = 'dedicated_launcher.exe'" -ErrorAction SilentlyContinue |
    Where-Object { $_.ExecutablePath -and ([IO.Path]::GetFullPath($_.ExecutablePath) -eq [IO.Path]::GetFullPath($exe)) }
if ($sameRuntime) { throw "Servidor ja esta aberto. PID: $($sameRuntime.ProcessId -join ', ')" }

$runtimeCfg = Join-Path $runtime 'cstrike\\cfg'
New-Item -ItemType Directory -Path $runtimeCfg -Force | Out-Null
Copy-Item -LiteralPath $serverCfg -Destination (Join-Path $runtimeCfg 'server.cfg') -Force
Copy-Item -LiteralPath $mapcycle -Destination (Join-Path $runtimeCfg 'mapcycle.txt') -Force
Copy-Item -LiteralPath $mapcycle -Destination (Join-Path $runtime 'cstrike\\mapcycle.txt') -Force
Copy-Item -LiteralPath $mapcycle -Destination (Join-Path $runtime 'cstrike\\maplist.txt') -Force

$lan = if ($Mode -eq 'lan') { '1' } else { '0' }
$arguments = @(
    '-console', '-game', 'cstrike', '-ip', '0.0.0.0', '-port', [string]$port,
    '+maxplayers', [string]$maxPlayers, '+map', $map, '+sv_lan', $lan,
    '+hostname', [string]$settings.ServerName, '+exec', 'server.cfg',
    '-condebug', '-usercon'
)
foreach ($extra in @($settings.ExtraArguments)) {
    if ([string]::IsNullOrWhiteSpace([string]$extra)) { continue }
    if ([string]$extra -match '[\\r\\n]') { throw 'ExtraArguments contem quebra de linha.' }
    $arguments += [string]$extra
}

$env:SteamAppId = '240'
$env:SteamGameId = '240'
$autoRestart = [bool]$settings.AutoRestartOnCrash
$delay = [Math]::Max(1, [Math]::Min(60, [int]$settings.RestartDelaySeconds))

Set-Location -LiteralPath $runtime
Write-Host "Iniciando Source Advanced V1 Dedicated ($Mode) - 128 Tick" -ForegroundColor Cyan
Write-Host "Mapa: $map | Porta: $port | Jogadores: $maxPlayers" -ForegroundColor Cyan
do {
    $proc = Start-Process -FilePath $exe -ArgumentList $arguments -PassThru
    $proc.WaitForExit()
    $exitCode = $proc.ExitCode
    if ($exitCode -eq 0 -or -not $autoRestart) { break }
    Write-Warning "Servidor encerrou com codigo $exitCode. Reiniciando em $delay segundos."
    Start-Sleep -Seconds $delay
} while ($true)

exit $exitCode
'''

stop_server_ps1 = '''$ErrorActionPreference = 'Stop'
$serverRoot = Split-Path -Parent $PSScriptRoot
$exe = [IO.Path]::GetFullPath((Join-Path $serverRoot 'runtime\\dedicated_launcher.exe'))
$processes = Get-CimInstance Win32_Process -Filter "Name = 'dedicated_launcher.exe'" -ErrorAction SilentlyContinue |
    Where-Object { $_.ExecutablePath -and ([IO.Path]::GetFullPath($_.ExecutablePath) -eq $exe) }
if (-not $processes) {
    Write-Host 'O servidor dedicado nao esta aberto.'
    exit 0
}
foreach ($process in $processes) {
    Stop-Process -Id $process.ProcessId -Force
    Write-Host "Servidor encerrado. PID $($process.ProcessId)."
}
'''

validate_server_ps1 = '''$ErrorActionPreference = 'Stop'
$serverRoot = Split-Path -Parent $PSScriptRoot
$runtime = Join-Path $serverRoot 'runtime'
$gameRoot = Split-Path -Parent $serverRoot
$settings = Get-Content -LiteralPath (Join-Path $serverRoot 'config\\launcher.json') -Raw | ConvertFrom-Json
$required = @(
    'dedicated_launcher.exe', 'bin\\dedicated.dll', 'bin\\engine.dll',
    'bin\\inputsystem.dll', 'bin\\vgui2.dll', 'bin\\shaderapiempty.dll',
    'cstrike\\bin\\server.dll', 'cstrike\\gameinfo.txt'
)
$failed = $false
foreach ($relative in $required) {
    $path = Join-Path $runtime $relative
    if (Test-Path -LiteralPath $path) { Write-Host "[OK] $relative" -ForegroundColor Green }
    else { Write-Host "[FALTA] $relative" -ForegroundColor Red; $failed = $true }
}
$map = Join-Path $gameRoot ('cstrike\\maps\\' + [string]$settings.DefaultMap + '.bsp')
if (Test-Path -LiteralPath $map) { Write-Host "[OK] mapa $($settings.DefaultMap)" -ForegroundColor Green }
else { Write-Host "[FALTA] mapa $($settings.DefaultMap)" -ForegroundColor Red; $failed = $true }

$serverDll = Join-Path $runtime 'cstrike\\bin\\server.dll'
if (Test-Path -LiteralPath $serverDll) {
    $hash = (Get-FileHash -LiteralPath $serverDll -Algorithm SHA256).Hash
    Write-Host "server.dll SHA256: $hash"
}
$port = [int]$settings.Port
$used = Get-NetUDPEndpoint -LocalPort $port -ErrorAction SilentlyContinue
if ($used) {
    Write-Host "[INFO] UDP $port esta em uso. Consultando o servidor..." -ForegroundColor Yellow
    try {
        & (Join-Path $PSScriptRoot 'query-server.ps1') -Port $port
    } catch {
        Write-Host "[FALHA] A2S: $($_.Exception.Message)" -ForegroundColor Red
        $failed = $true
    }
}
else { Write-Host "[OK] UDP $port esta livre." -ForegroundColor Green }

if ($failed) { exit 1 }
Write-Host 'Validacao concluida com sucesso.' -ForegroundColor Cyan
'''

query_server_ps1 = '''param(
    [int]$Port = 25565,
    [int]$TimeoutMs = 4000
)

$ErrorActionPreference = 'Stop'
$address = Get-NetIPAddress -AddressFamily IPv4 -AddressState Preferred |
    Where-Object { $_.IPAddress -notlike '127.*' -and $_.IPAddress -notlike '169.254.*' } |
    Sort-Object InterfaceMetric |
    Select-Object -First 1 -ExpandProperty IPAddress
if (-not $address) { throw 'Nenhum endereco IPv4 LAN ativo foi encontrado.' }

[byte[]]$query = @(0xff, 0xff, 0xff, 0xff, 0x54) +
    [Text.Encoding]::ASCII.GetBytes('Source Engine Query') + 0
$client = [Net.Sockets.UdpClient]::new()
$client.Client.ReceiveTimeout = $TimeoutMs
try {
    $client.Connect($address, $Port)
    [void]$client.Send($query, $query.Length)
    $remote = [Net.IPEndPoint]::new([Net.IPAddress]::Any, 0)
    [byte[]]$response = $client.Receive([ref]$remote)
} finally {
    $client.Dispose()
}

if ($response.Length -lt 7 -or $response[0] -ne 0xff -or $response[4] -ne 0x49) {
    throw 'O servidor respondeu com um pacote A2S invalido.'
}

function Read-A2SString {
    param([byte[]]$Data, [ref]$Offset)
    $start = $Offset.Value
    while ($Offset.Value -lt $Data.Length -and $Data[$Offset.Value] -ne 0) { $Offset.Value++ }
    if ($Offset.Value -ge $Data.Length) { throw 'Resposta A2S truncada.' }
    $value = [Text.Encoding]::UTF8.GetString($Data, $start, $Offset.Value - $start)
    $Offset.Value++
    return $value
}

$offset = 6
$name = Read-A2SString $response ([ref]$offset)
$map = Read-A2SString $response ([ref]$offset)
$folder = Read-A2SString $response ([ref]$offset)
$game = Read-A2SString $response ([ref]$offset)
Write-Host ("[OK] A2S respondeu em {0}:{1}" -f $address, $Port) -ForegroundColor Green
Write-Host "Servidor: $name"
Write-Host "Mapa: $map | Jogo: $game | Pasta: $folder"
'''

build_and_deploy_ps1 = '''$ErrorActionPreference = 'Stop'
$serverRoot = Split-Path -Parent $PSScriptRoot
$runtime = Join-Path $serverRoot 'runtime'
$source = 'C:\\Users\\SnyX\\Desktop\\projeto clone\\source-engine'
$output = Join-Path $source 'output-dedicated'

if (-not (Test-Path -LiteralPath (Join-Path $source 'waf'))) { throw "Source nao encontrada: $source" }
Set-Location -LiteralPath $source
python .\\waf configure -T release --build-games=cstrike --enable-opus --disable-warns --prefix=.\\output-dedicated --out=build-dedicated -d
if ($LASTEXITCODE) { exit $LASTEXITCODE }
python .\\waf build -j12 --out=build-dedicated
if ($LASTEXITCODE) { exit $LASTEXITCODE }
python .\\waf install --out=build-dedicated
if ($LASTEXITCODE) { exit $LASTEXITCODE }

New-Item -ItemType Directory -Path (Join-Path $runtime 'bin') -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $runtime 'cstrike\\bin') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $output 'dedicated_launcher.exe') -Destination $runtime -Force
Get-ChildItem -LiteralPath (Join-Path $output 'bin') -File -Filter '*.dll' |
    Copy-Item -Destination (Join-Path $runtime 'bin') -Force
Copy-Item -LiteralPath (Join-Path $output 'cstrike\\bin\\server.dll') -Destination (Join-Path $runtime 'cstrike\\bin\\server.dll') -Force
Write-Host 'Build dedicado compilado e implantado com sucesso.' -ForegroundColor Green
'''

for suite in dedicated_suites:
    print(f'Configuring Dedicated Server Suite in {suite}...')
    parent_game = os.path.dirname(suite)
    runtime = os.path.join(suite, 'runtime')
    config_dir = os.path.join(suite, 'config')
    scripts_dir = os.path.join(suite, 'scripts')
    logs_dir = os.path.join(suite, 'logs')
    runtime_bin = os.path.join(runtime, 'bin')
    runtime_cstrike = os.path.join(runtime, 'cstrike')
    runtime_cstrike_bin = os.path.join(runtime_cstrike, 'bin')
    runtime_cstrike_cfg = os.path.join(runtime_cstrike, 'cfg')
    
    os.makedirs(config_dir, exist_ok=True)
    os.makedirs(scripts_dir, exist_ok=True)
    os.makedirs(logs_dir, exist_ok=True)
    os.makedirs(runtime_bin, exist_ok=True)
    os.makedirs(runtime_cstrike_bin, exist_ok=True)
    os.makedirs(runtime_cstrike_cfg, exist_ok=True)
    
    # Write config files
    with open(os.path.join(config_dir, 'launcher.json'), 'w', encoding='utf-8') as f:
        f.write(launcher_json_content)
    with open(os.path.join(config_dir, 'mapcycle.txt'), 'w', encoding='utf-8') as f:
        f.write(mapcycle_content)
    with open(os.path.join(config_dir, 'server.cfg'), 'w', encoding='utf-8') as f:
        f.write(server_cfg_content)
        
    # Write root CMDs
    with open(os.path.join(suite, 'INICIAR SERVIDOR LAN.cmd'), 'w', encoding='utf-8') as f:
        f.write(iniciar_lan_cmd)
    with open(os.path.join(suite, 'INICIAR SERVIDOR ONLINE.cmd'), 'w', encoding='utf-8') as f:
        f.write(iniciar_online_cmd)
    with open(os.path.join(suite, 'PARAR SERVIDOR.cmd'), 'w', encoding='utf-8') as f:
        f.write(parar_cmd)
    with open(os.path.join(suite, 'VERIFICAR SERVIDOR.cmd'), 'w', encoding='utf-8') as f:
        f.write(verificar_cmd)
    with open(os.path.join(suite, 'CONFIGURAR FIREWALL.cmd'), 'w', encoding='utf-8') as f:
        f.write(firewall_cmd)
    with open(os.path.join(suite, 'ATUALIZAR DA SOURCE.cmd'), 'w', encoding='utf-8') as f:
        f.write(atualizar_cmd)
    with open(os.path.join(suite, 'README.txt'), 'w', encoding='utf-8') as f:
        f.write(readme_txt)
        
    # Write scripts
    with open(os.path.join(scripts_dir, 'start-server.ps1'), 'w', encoding='utf-8') as f:
        f.write(start_server_ps1)
    with open(os.path.join(scripts_dir, 'stop-server.ps1'), 'w', encoding='utf-8') as f:
        f.write(stop_server_ps1)
    with open(os.path.join(scripts_dir, 'validate-server.ps1'), 'w', encoding='utf-8') as f:
        f.write(validate_server_ps1)
    with open(os.path.join(scripts_dir, 'query-server.ps1'), 'w', encoding='utf-8') as f:
        f.write(query_server_ps1)
    with open(os.path.join(scripts_dir, 'build-and-deploy.ps1'), 'w', encoding='utf-8') as f:
        f.write(build_and_deploy_ps1)
        
    # Copy dedicated server binaries from output-dedicated
    shutil.copy2(os.path.join(output_ded, 'dedicated_launcher.exe'), os.path.join(runtime, 'dedicated_launcher.exe'))
    shutil.copy2(os.path.join(output_ded, 'cstrike', 'bin', 'server.dll'), os.path.join(runtime_cstrike_bin, 'server.dll'))
    for f in os.listdir(os.path.join(output_ded, 'bin')):
        if f.endswith('.dll'):
            shutil.copy2(os.path.join(output_ded, 'bin', f), os.path.join(runtime_bin, f))
            
    # Also copy any client bin dependencies like inputsystem.dll, vgui2.dll into runtime/bin
    for f in ['inputsystem.dll', 'vgui2.dll', 'vguimatsurface.dll']:
        src = os.path.join(output_client, 'bin', f)
        if os.path.exists(src):
            shutil.copy2(src, os.path.join(runtime_bin, f))
            
    # Gameinfo for dedicated runtime
    gameinfo_content = f'''"GameInfo"
{{
    game "Source Advanced V1 Dedicated"
    title "Source Advanced V1 Dedicated"
    type multiplayer_only
    bots 1
    nodegraph 0

    FileSystem
    {{
        SteamAppId 240
        SearchPaths
        {{
            game+mod "{parent_game}\\cstrike\\custom\\*"
            game+mod "{parent_game}\\cstrike\\cstrike_english.vpk"
            game+mod "{parent_game}\\cstrike\\cstrike_pak.vpk"
            game "{parent_game}\\hl2\\hl2_textures.vpk"
            game "{parent_game}\\hl2\\hl2_sound_vo_english.vpk"
            game "{parent_game}\\hl2\\hl2_sound_misc.vpk"
            game "{parent_game}\\hl2\\hl2_misc.vpk"
            platform "{parent_game}\\platform\\platform_misc.vpk"
            game+mod "{parent_game}\\cstrike"
            game "{parent_game}\\hl2"
            platform "{parent_game}\\platform"
            mod+mod_write+default_write_path |gameinfo_path|.
            game+game_write |gameinfo_path|.
            gamebin |gameinfo_path|bin
        }}
    }}
}}
'''
    with open(os.path.join(runtime_cstrike, 'gameinfo.txt'), 'w', encoding='utf-8') as f:
        f.write(gameinfo_content)
        
    # Copy server.cfg and mapcycle into runtime
    shutil.copy2(os.path.join(config_dir, 'server.cfg'), os.path.join(runtime_cstrike_cfg, 'server.cfg'))
    shutil.copy2(os.path.join(config_dir, 'mapcycle.txt'), os.path.join(runtime_cstrike, 'mapcycle.txt'))
    shutil.copy2(os.path.join(config_dir, 'mapcycle.txt'), os.path.join(runtime_cstrike, 'maplist.txt'))
    shutil.copy2(os.path.join(config_dir, 'mapcycle.txt'), os.path.join(runtime_cstrike_cfg, 'mapcycle.txt'))

    # Private server discovery used only by this custom engine.
    parent_cfg = os.path.join(parent_game, 'cstrike', 'cfg')
    os.makedirs(parent_cfg, exist_ok=True)
    with open(os.path.join(parent_cfg, 'sourceadvanced_servers.txt'), 'w', encoding='utf-8') as f:
        f.write(sourceadvanced_servers_content)
    with open(os.path.join(runtime_cstrike_cfg, 'sourceadvanced_servers.txt'), 'w', encoding='utf-8') as f:
        f.write(sourceadvanced_servers_content)
    
    # Deploy updated engine.dll, client.dll and server.dll to parent game
    parent_bin = os.path.join(parent_game, 'bin')
    parent_cstrike_bin = os.path.join(parent_game, 'cstrike', 'bin')
    if os.path.exists(parent_bin):
        try:
            shutil.copy2(os.path.join(output_client, 'bin', 'engine.dll'), os.path.join(parent_bin, 'engine.dll'))
            shutil.copy2(os.path.join(output_client, 'bin', 'filesystem_stdio.dll'), os.path.join(parent_bin, 'filesystem_stdio.dll'))
            print(f'Deployed engine.dll and filesystem_stdio.dll to {parent_bin}')
        except PermissionError:
            print(f'Binaries in {parent_bin} currently in use by active game.')
    if os.path.exists(parent_cstrike_bin):
        try:
            shutil.copy2(os.path.join(output_client, 'cstrike', 'bin', 'client.dll'), os.path.join(parent_cstrike_bin, 'client.dll'))
            shutil.copy2(os.path.join(output_client, 'cstrike', 'bin', 'server.dll'), os.path.join(parent_cstrike_bin, 'server.dll'))
            print(f'Deployed client.dll and server.dll to {parent_cstrike_bin}')
        except PermissionError:
            print(f'Binaries in {parent_cstrike_bin} currently in use by active game.')
        
    print(f'Done configuring {suite}.')

print('All dedicated server suites fully configured.')
