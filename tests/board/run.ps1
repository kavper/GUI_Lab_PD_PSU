param([string]$Compiler = 'C:/TouchGFX/4.26.1/env/MinGW/bin/gcc.exe')
$ErrorActionPreference = 'Stop'
$testRoot = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$testExe = Join-Path ([IO.Path]::GetTempPath()) 'gui_lab_board_backlight_test.exe'
& $Compiler -std=c11 -Wall -Wextra -Werror -I $PSScriptRoot -I "$testRoot/Board" "$PSScriptRoot/test_backlight.c" "$testRoot/Board/board_backlight.c" -o $testExe
if ($LASTEXITCODE -ne 0) { throw 'Backlight test compilation failed' }
& $testExe
if ($LASTEXITCODE -ne 0) { throw 'Backlight test failed' }