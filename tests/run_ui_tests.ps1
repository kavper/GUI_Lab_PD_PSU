param(
  [ValidateSet('INSTRUMENT','USB','BATTERY','THEME','FINGER')]
  [string[]]$Tests = @('INSTRUMENT','USB','BATTERY','THEME','FINGER')
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$app = Join-Path $repo 'Appli/TouchGFX'
$oldVideo = $env:SDL_VIDEODRIVER
$oldRender = $env:SDL_RENDER_DRIVER
try {
  # SDL's dummy/software backend also runs when the Windows desktop is disconnected.
  $env:SDL_VIDEODRIVER = 'dummy'
  $env:SDL_RENDER_DRIVER = 'software'
  foreach ($test in $Tests) {
    $key = 'PSU_' + $test + '_SELFTEST'
    $oldValue = [Environment]::GetEnvironmentVariable($key, 'Process')
    $log = Join-Path $repo ('Board/ui-' + $test.ToLower() + '.log')
    try {
      [Environment]::SetEnvironmentVariable($key, $log, 'Process')
      $process = Start-Process (Join-Path $app 'build/bin/simulator.exe') -WorkingDirectory $app -WindowStyle Hidden -PassThru
      if (!$process.WaitForExit(180000)) {
        $process.Kill()
        throw "$test timed out; see $log"
      }
      Get-Content $log -Tail 3
      if ($process.ExitCode -ne 0) { throw "$test failed ($($process.ExitCode)); see $log" }
    } finally { [Environment]::SetEnvironmentVariable($key, $oldValue, 'Process') }
  }
} finally {
  $env:SDL_VIDEODRIVER = $oldVideo
  $env:SDL_RENDER_DRIVER = $oldRender
}
