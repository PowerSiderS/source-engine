param(
    [string]$GameRoot = 'C:\Users\SnyX\Desktop\Source Advanced V1 - PowerSiderS (PC)\Source Advanced V1 - @PowerSiderS (PC)',
    [string]$BuildDirectory = 'build'
)

$ErrorActionPreference = 'Stop'
$sourceRoot = Split-Path -Parent $PSScriptRoot
$GameRoot = (Resolve-Path -LiteralPath $GameRoot).Path
$running = Get-CimInstance Win32_Process | Where-Object {
    $_.ExecutablePath -and $_.ExecutablePath.StartsWith($GameRoot + '\', [StringComparison]::OrdinalIgnoreCase) -and
    $_.Name -eq 'hl2_launcher.exe'
}
if ($running) { throw 'Feche o cliente do jogo antes de atualizar suas DLLs.' }

Push-Location $sourceRoot
try {
    $env:PYTHONIOENCODING = 'utf-8'
    # IFileSystem is shared by the engine and its separately loaded module.
    # Always rebuild and deploy both sides after interface/header changes.
    # Waf's lock file selects the configured output directory. --out on a build
    # invocation alone does not switch away from a dedicated/test configuration.
    & python waf configure -T release --build-games=cstrike --enable-opus --disable-warns --prefix=.\output "--out=$BuildDirectory"
    if ($LASTEXITCODE -ne 0) { throw "Falha na configuracao: $LASTEXITCODE" }
    # Native modules share interface headers. Install a matching full release,
    # including the launcher and GameUI map selector, rather than mixing builds.
    & python waf build install -j12
    if ($LASTEXITCODE -ne 0) { throw "Falha na compilacao: $LASTEXITCODE" }
    $relativeFiles = @('hl2_launcher.exe', 'cstrike\bin\client.dll', 'cstrike\bin\server.dll')
    $relativeFiles += @(Get-ChildItem -LiteralPath (Join-Path $sourceRoot 'output\bin') -File -Filter '*.dll' | ForEach-Object { 'bin\' + $_.Name })
    foreach ($relative in $relativeFiles) {
        if (-not (Test-Path -LiteralPath (Join-Path $sourceRoot "output\$relative"))) {
            throw "Binario ausente em output: $relative"
        }
    }
    $backup = Join-Path $GameRoot ('backups\client-update-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
    New-Item -ItemType Directory -Path $backup -Force | Out-Null
    foreach ($relative in $relativeFiles) {
        $built = Join-Path $sourceRoot "output\$relative"
        $installed = Join-Path $GameRoot $relative
        if (Test-Path -LiteralPath $installed) {
            Copy-Item -LiteralPath $installed -Destination (Join-Path $backup $relative.Replace('\', '_'))
        }
        Copy-Item -LiteralPath $built -Destination $installed -Force
        if ((Get-FileHash -LiteralPath $built).Hash -ne (Get-FileHash -LiteralPath $installed).Hash) {
            throw "Verificacao de copia falhou: $relative"
        }
        Write-Host "Atualizado e verificado: $relative"
    }
    Write-Host "Backup: $backup"
} finally {
    Pop-Location
}
