param([string]$Programmer = 'C:/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe')
$ErrorActionPreference = 'Stop'
function Read-BoardOptions {
    $result = & $Programmer -c port=SWD ap=1 mode=UR reset=HWrst freq=1000 -ob displ 2>&1
    $result | Write-Host
    if ($LASTEXITCODE -ne 0) { throw 'Cannot read target option bytes. Flashing stopped.' }
    return ($result -join "`n")
}
$content = Read-BoardOptions
if ($content -notmatch 'Device ID\s*:\s*0x0*485\b') { throw 'Expected STM32H7RS device ID 0x485. Flashing stopped.' }
$expected = [ordered]@{ XSPI1_HSLV=1; XSPI2_HSLV=0; I2C_NI3C=1; IWDG_HW=1; VDDIO_HSLV=0 }
$changes = @()
foreach ($entry in $expected.GetEnumerator()) {
    $pattern = [regex]::Escape($entry.Key) + '\s*:\s*0x0*' + $entry.Value + '\b'
    if ($content -notmatch $pattern) { $changes += "$($entry.Key)=$($entry.Value)" }
}
if ($changes.Count) {
    Write-Host "Configuring board option bytes: $($changes -join ', ')"
    & $Programmer -c port=SWD ap=1 mode=UR reset=HWrst freq=1000 -ob @changes
    if ($LASTEXITCODE -ne 0) { throw 'Board option-byte programming failed. Flashing stopped.' }
    $content = Read-BoardOptions
    foreach ($entry in $expected.GetEnumerator()) {
        $pattern = [regex]::Escape($entry.Key) + '\s*:\s*0x0*' + $entry.Value + '\b'
        if ($content -notmatch $pattern) { throw "Option-byte readback failed: $($entry.Key). Flashing stopped." }
    }
}