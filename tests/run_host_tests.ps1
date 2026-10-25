param([string]$Compiler = 'C:/TouchGFX/4.26.1/env/MinGW/bin/gcc.exe')
$ErrorActionPreference = 'Stop'
$testRepo = Split-Path $PSScriptRoot -Parent
$testOutput = Join-Path ([System.IO.Path]::GetTempPath()) 'GUI_Lab_PD_PSU_host_tests'
New-Item -ItemType Directory -Path $testOutput -Force | Out-Null
$testSources = @('g4_ascii','psu_app','psu_web_features','psu_sim','psu_limits','psu_store','psu_nvstore',
                 'psu_seq','psu_charger','psu_format','psu_edit') |
  ForEach-Object { Join-Path $testRepo "Appli/Core/Src/$_.c" }
foreach ($testName in @('test_binary','test_binary_app','test_startup','test_automation','test_nvstore','test_pd_caps','test_web_commands','test_web_features')) {
  $testInput = Join-Path $PSScriptRoot "$testName.c"
  $testExe = Join-Path $testOutput "$testName.exe"
  $testUnits = @(if ($testName -eq 'test_binary') { $testSources[0] } else { $testSources })
  & $Compiler -std=c11 -Wall -Wextra -Werror -DPSU_SIMULATOR `
    -I (Join-Path $testRepo 'Appli/Core/Inc') $testInput @testUnits -o $testExe
  if ($LASTEXITCODE -ne 0) { throw "Compilation failed: $testName" }
  & $testExe
  if ($LASTEXITCODE -ne 0) { throw "Test failed: $testName" }
}
# Compile the HTTP lifetime regression with the actual bundled lwIP core.
$httpLwipSources = @(Get-ChildItem (Join-Path $testRepo 'Middlewares/Third_Party/LwIP/src/core/*.c'),
  (Join-Path $testRepo 'Middlewares/Third_Party/LwIP/src/core/ipv4/*.c') | ForEach-Object FullName) +
  @(Join-Path $testRepo 'Middlewares/Third_Party/LwIP/src/netif/ethernet.c')
$httpTestExe = Join-Path $testOutput 'test_http_lifetime.exe'
& $Compiler -std=c11 -Wall -Wextra -DPSU_SIMULATOR `
  -I (Join-Path $testRepo 'Board') -I (Join-Path $testRepo 'Appli/Core/Inc') `
  -I (Join-Path $testRepo 'Middlewares/Third_Party/LwIP/src/include') `
  -I (Join-Path $testRepo 'Middlewares/Third_Party/LwIP/system') `
  (Join-Path $PSScriptRoot 'test_http_lifetime.c') @testSources @httpLwipSources -o $httpTestExe
if ($LASTEXITCODE -ne 0) { throw 'Compilation failed: HTTP lifetime' }
& $httpTestExe
if ($LASTEXITCODE -ne 0) { throw 'Test failed: HTTP lifetime' }
$displayCompiler = Join-Path (Split-Path $Compiler -Parent) 'g++.exe'
$panelTest = Join-Path $testOutput 'test_panel_input.exe'
& $Compiler -std=c11 -Wall -Wextra -Werror -I (Join-Path $testRepo 'Appli/Core/Inc') `
  (Join-Path $PSScriptRoot 'test_panel_input.c') (Join-Path $testRepo 'Appli/Core/Src/panel_input.c') -o $panelTest
if ($LASTEXITCODE -ne 0) { throw 'Compilation failed: panel input' }
& $panelTest
if ($LASTEXITCODE -ne 0) { throw 'Test failed: panel input' }
$displayRuntimePath = $env:PATH
try {
  $env:PATH = (Split-Path $displayCompiler -Parent) + ';' + $env:PATH
  foreach ($displayName in @('test_display_voltage','test_power_gauge','test_chart_axis','test_usb_session','test_battery_trend','test_rounded_corners','test_pd_objects','test_edge_back')) {
    $displayTest = Join-Path $testOutput "$displayName.exe"
    & $displayCompiler -std=c++11 -Wall -Wextra -Werror `
      -I (Join-Path $testRepo 'Appli/TouchGFX/gui/include') -I (Join-Path $testRepo 'Appli/Core/Inc') `
      (Join-Path $PSScriptRoot "$displayName.cpp") -o $displayTest
    if ($LASTEXITCODE -ne 0) { throw "Compilation failed: $displayName" }
    & $displayTest
    if ($LASTEXITCODE -ne 0) { throw "Test failed: $displayName" }
  }
} finally { $env:PATH = $displayRuntimePath }
