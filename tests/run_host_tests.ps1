param([string]$Compiler = 'C:/TouchGFX/4.26.1/env/MinGW/bin/gcc.exe')
$ErrorActionPreference = 'Stop'
$testRepo = Split-Path $PSScriptRoot -Parent
$testOutput = Join-Path ([System.IO.Path]::GetTempPath()) 'GUI_Lab_PD_PSU_host_tests'
New-Item -ItemType Directory -Path $testOutput -Force | Out-Null
$testSources = @('g4_ascii','psu_app','psu_sim','psu_limits','psu_store',
                 'psu_seq','psu_charger','psu_format','psu_edit') |
  ForEach-Object { Join-Path $testRepo "Appli/Core/Src/$_.c" }
foreach ($testName in @('test_binary','test_binary_app','test_startup')) {
  $testInput = Join-Path $PSScriptRoot "$testName.c"
  $testExe = Join-Path $testOutput "$testName.exe"
  $testUnits = @(if ($testName -eq 'test_binary') { $testSources[0] } else { $testSources })
  & $Compiler -std=c11 -Wall -Wextra -Werror -DPSU_SIMULATOR `
    -I (Join-Path $testRepo 'Appli/Core/Inc') $testInput @testUnits -o $testExe
  if ($LASTEXITCODE -ne 0) { throw "Compilation failed: $testName" }
  & $testExe
  if ($LASTEXITCODE -ne 0) { throw "Test failed: $testName" }
}
$displayCompiler = Join-Path (Split-Path $Compiler -Parent) 'g++.exe'
$displayTest = Join-Path $testOutput 'test_display_voltage.exe'
& $displayCompiler -std=c++11 -Wall -Wextra -Werror -static-libgcc -static-libstdc++ `
  -I (Join-Path $testRepo 'Appli/TouchGFX/gui/include') `
  (Join-Path $PSScriptRoot 'test_display_voltage.cpp') -o $displayTest
if ($LASTEXITCODE -ne 0) { throw 'Compilation failed: test_display_voltage' }
& $displayTest
if ($LASTEXITCODE -ne 0) { throw 'Test failed: test_display_voltage' }
