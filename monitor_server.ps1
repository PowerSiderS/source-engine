$runtimeDir = "C:\Users\SnyX\Desktop\Source Advanced V1 - PowerSiderS (PC)\Source Advanced V1 - @PowerSiderS (PC)\Dedicated Server\runtime"
Set-Location $runtimeDir

$p = Start-Process -FilePath ".\dedicated_launcher.exe" -ArgumentList "-console -game cstrike -ip 0.0.0.0 -port 25565 +maxplayers 16 +map de_mirage_csgo +sv_lan 1" -PassThru

for ($i = 1; $i -le 15; $i++) {
    Start-Sleep -Seconds 2
    if ($p.HasExited) {
        Write-Host "Process exited at second $($i*2) with code $($p.ExitCode)"
        break
    } else {
        Write-Host "Process running at second $($i*2) - PID: $($p.Id) - WorkingSet: $([math]::Round($p.WorkingSet64 / 1MB, 2)) MB"
    }
}

if (!$p.HasExited) {
    Write-Host "SUCCESS: Process is steadily running after 30 seconds!"
    Get-NetUDPEndpoint -LocalPort 25565 -ErrorAction SilentlyContinue | Format-Table -AutoSize
}
