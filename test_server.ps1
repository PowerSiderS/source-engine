$runtimeDir = "C:\Users\SnyX\Desktop\Source Advanced V1 - PowerSiderS (PC)\Source Advanced V1 - @PowerSiderS (PC)\Dedicated Server\runtime"
Set-Location $runtimeDir
if (Test-Path "dedicated_trace.log") {
    Remove-Item "dedicated_trace.log" -Force
}
$p = Start-Process -FilePath ".\dedicated_launcher.exe" -ArgumentList "-console -game cstrike -ip 0.0.0.0 -port 25565 +maxplayers 16 +map de_mirage_csgo +sv_lan 1" -PassThru
Start-Sleep -Seconds 4
if ($p.HasExited) {
    Write-Host "Process exited with code $($p.ExitCode)"
} else {
    Write-Host "Process still running with PID $($p.Id)"
}
if (Test-Path "dedicated_trace.log") {
    Write-Host "--- TRACE LOG ---"
    Get-Content "dedicated_trace.log" -Tail 50
} else {
    Write-Host "No dedicated_trace.log found."
}
