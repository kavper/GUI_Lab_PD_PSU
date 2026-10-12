param([string]$Compiler = 'C:/TouchGFX/4.26.1/env/MinGW/bin/gcc.exe')
$ErrorActionPreference='Stop'
$testRoot = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$testDir = Join-Path ([IO.Path]::GetTempPath()) 'gui_lab_board_preflight'
New-Item -ItemType Directory -Force $testDir | Out-Null
$testExe = Join-Path $testDir 'mock_programmer.exe'
& $Compiler -std=c11 -Wall -Wextra -Werror "$PSScriptRoot/mock_programmer.c" -o $testExe
if($LASTEXITCODE -ne 0){throw 'Mock compilation failed'}
foreach($case in @('unchanged','needs_update','wrong_mcu','connection_fail','write_fail','readback_fail')) {
 $env:BOARD_MOCK_CASE=$case
 $env:BOARD_MOCK_STATE=Join-Path $testDir ($case+'.state')
 if(Test-Path -LiteralPath $env:BOARD_MOCK_STATE){Remove-Item -LiteralPath $env:BOARD_MOCK_STATE}
 $success=$true
 try { & "$testRoot/Board/preflight.ps1" -Programmer $testExe } catch { $success=$false }
 $expectedSuccess=$case -in @('unchanged','needs_update')
 if($success -ne $expectedSuccess){throw "Unexpected preflight outcome: $case"}
 if($case -in @('unchanged','wrong_mcu','connection_fail') -and (Test-Path -LiteralPath $env:BOARD_MOCK_STATE)){throw "Unexpected option-byte write: $case"}
 if($case -eq 'needs_update') {
  $programmed=Get-Content -LiteralPath $env:BOARD_MOCK_STATE
  foreach($entry in @('XSPI1_HSLV=1','XSPI2_HSLV=0','I2C_NI3C=1','IWDG_HW=1','VDDIO_HSLV=0')) { if($entry -notin $programmed){throw "Missing board option: $entry"} }
  if($programmed.Count -ne 10){throw 'Unexpected programming arguments'}
 }
 Write-Host "PASS preflight: $case"
}
Remove-Item Env:BOARD_MOCK_CASE,Env:BOARD_MOCK_STATE