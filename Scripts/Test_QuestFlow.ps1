param(
    [string]$Filter = "DirectorsCut",
    [switch]$Rebuild
)

$ErrorActionPreference = "Stop"
$ProjectDir = Split-Path -Parent $PSScriptRoot
$UProject = Join-Path $ProjectDir "EpicGameJamGame.uproject"
$BuildBat = "C:\Program Files\Epic Games\UE_5.9\Engine\Build\BatchFiles\Build.bat"
$EditorCmd = "C:\Program Files\Epic Games\UE_5.9\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$LogFile = Join-Path $ProjectDir "Saved\Logs\DC_Test_QuestFlow.log"

if ($Rebuild) {
    Write-Host "[1/2] Building EpicGameJamGameEditor..." -ForegroundColor Cyan
    & $BuildBat EpicGameJamGameEditor Win64 Development -Project="$UProject" -WaitMutex
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Build failed with exit code $LASTEXITCODE"
        exit $LASTEXITCODE
    }
}

Write-Host "[Running UE Automation Tests: $Filter]" -ForegroundColor Cyan
$Output = & $EditorCmd "$UProject" -game -ExecCmds="Automation RunTests $Filter; Quit" -unattended -nop4 -nosplash -nullrhi -stdout -FullStdOutLogOutput 2>&1
$TestExit = $LASTEXITCODE
$Output | Out-File -FilePath $LogFile -Encoding utf8

Write-Host "`n=== Director's Cut Quest Flow Test Summary ===" -ForegroundColor Yellow
$Relevant = $Output | Where-Object { $_ -match "LogAutomationController|LogAutomationTest|\[DC_TEST\]|Quest |Narrator:|GoToBeat|Error:|Warning:.*Row " }
foreach ($Line in $Relevant) {
    if ($Line -match "Result=\{Fail\}|Error:") {
        Write-Host $Line -ForegroundColor Red
    } elseif ($Line -match "Result=\{Success\}|\[DC_TEST\]|EXIT CODE: 0") {
        Write-Host $Line -ForegroundColor Green
    } elseif ($Line -match "Warning:") {
        Write-Host $Line -ForegroundColor Yellow
    } else {
        Write-Host $Line
    }
}

Write-Host "`nFull log saved to: $LogFile (exit code $TestExit)" -ForegroundColor Cyan
exit $TestExit
