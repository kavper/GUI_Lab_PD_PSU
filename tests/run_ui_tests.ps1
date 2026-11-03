# Runs the simulator UI tour (every page in every theme, swipe, pull-down menu
# and editing flows). Screenshots land in Appli/TouchGFX/screenshots/ui-tour.
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$app = Join-Path $repo 'Appli/TouchGFX'
$oldVideo = $env:SDL_VIDEODRIVER
$oldRender = $env:SDL_RENDER_DRIVER
$oldTour = $env:PSU_UI_TOUR
$log = Join-Path $repo 'Board/ui-tour.log'
try {
  # SDL's dummy/software backend also runs when the Windows desktop is disconnected.
  $env:SDL_VIDEODRIVER = 'dummy'
  $env:SDL_RENDER_DRIVER = 'software'
  $env:PSU_UI_TOUR = $log
  New-Item -ItemType Directory -Force -Path (Join-Path $app 'screenshots/ui-tour') | Out-Null
  $process = Start-Process (Join-Path $app 'build/bin/simulator.exe') -WorkingDirectory $app -WindowStyle Hidden -PassThru
  if (!$process.WaitForExit(300000)) {
    $process.Kill()
    throw "UI tour timed out; see $log"
  }
  Get-Content $log -Tail 5
  if ($process.ExitCode -ne 0) { throw "UI tour failed ($($process.ExitCode)); see $log" }
} finally {
  $env:SDL_VIDEODRIVER = $oldVideo
  $env:SDL_RENDER_DRIVER = $oldRender
  $env:PSU_UI_TOUR = $oldTour
}
