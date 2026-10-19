# Instrument UI 1.2

Executable project: `Appli/TouchGFX/GUI_Lab_PD_PSU.touchgfx` in this repository.
Generate/Run Simulator/Run Target use the same Designer layout, handwritten views
and firmware. Generated sources are not maintained by hand.

## Screens and navigation

- Keep the established main screen and grouped Monitor / Tools / System menu.
- Startup uses the active theme, readable device/build information and separate
  supervisor/BMS/USB telemetry states. It continues after about two seconds even
  without telemetry; starting the UI does not enable the output.
- USB-C shows contract limits separately from measured input voltage/current/power,
  all seven available SPR source PDO slots, the RDO-selected row and raw request.
  Fixed, variable, battery and PPS PDOs are decoded; SPR AVS capability current
  ranges are shown explicitly. Unknown APDOs retain their raw value. Unsupported
  requests are not misdecoded as fixed-current requests.
- USB received energy uses distinct valid input ADC samples, trapezoidal integration
  and no integration across invalid/stale gaps. The attach generation resets the
  session even if its detach frame was lost. "Valid time" is integrated sample
  time, not cable-connected wall time. BQ input ADC does not measure source output
  current; the UI reports that measurement as unavailable in SOURCE mode.
- Keep the signed battery power-flow bar. Battery overview puts all five physical
  channels, unused/balancing/min/max status, pack readings, SOC, AFE charge counter,
  onboard charger, FETs, BMS IC temperature, signed energy and trend on one page.
  BMS details remain a separate diagnostic destination, not a set of overview tabs.
- External charger combines profile and session readings/history. A field opens
  the numeric editor; APPLY saves and returns to the overview, tapping the selected
  field cancels a draft. Editing clears polarity confirmation. Existing charging
  validation, stop and shutdown commands remain in force.
- Sequencer initially shows its overview and cycle controls. Choosing a field
  opens the existing animated keypad.
- Appearance keeps theme/accent and sound controls together; the feedback button
  fits its panel. Settings continue to use the verified EEPROM journal.
- Detail screens accept a rightward swipe beginning within the left 28 pixels,
  below the header. Vertical movement remains available to lists. A short or
  cancelled swipe preserves the current view and draft. BMS/USB entered from
  Battery return there; the main/menu/USB interactive transitions are retained.

The physical OUTPUT key is processed in `BoardPanel_Poll`, independently of the
active view: short press toggles output; a two-second hold requests power shutdown.
PF1/PF2 LEDs now follow fresh confirmed output feedback (red OFF, green ON), with
red blinking for a latched fault. Missing confirmation does not imply output ON.

## G4 companion firmware

Companion repository: `https://github.com/kavper/Lab_PD_PSU`, branch
`codex/instrument-ui-telemetry`, based on the existing `cursor/board-flash-0ef3`
binary-link firmware rather than the older ASCII `main` branch.

The new read-only `0x14` frame publishes source PDOs, active RDO/PDO, role and attach
generation every 200 ms. It does not change PD negotiation or power policy, and
retains the 5 ms METER stream and command/ACK priorities. The existing PD frame
uses reserved byte 11 for the low byte of the attach generation. Old H7 ignores
that byte; new H7 without the companion frame says the PDO list is unavailable.
Old-session, stale or mismatched-role lists are never presented as current.

BMS accumulated charge was already implemented on that G4 base: BQ76922 DASTATUS6
passQ is read, converted using the configured current units, and exported in
`passq_mah` with a validity bit. The displayed dQ/Ah is the signed AFE accumulator,
not remaining capacity and not an H7 software current integral. SOC is an estimate
using the existing configured cell chemistry/capacity and passQ delta. Its nominal
capacity requires pack-specific tuning. `session_mah` is reserved and remains zero;
the GUI does not present it as a working session counter.

## Validation and limits

Validated locally on 2026-10-19:

- Designer generation, simulator compilation and Cortex-M7 target compilation.
- Host tests: binary transport/application/startup/automation, EEPROM interrupted
  writes, panel inputs and LED policy, PDO/RDO units and malformed frames, source
  session freshness, USB energy/session boundaries, trend/axes/corners and gestures.
- Simulator: all 14 detail/menu routes and edge returns; real charger keypad touch,
  APPLY/cancel and cancelled-edge draft preservation; stale/previous-session PDO
  hiding; battery physical channels/invalid/stale data and BMS/USB links; USB gauge,
  SOURCE measurement availability, charger STOP -> OFF frame, palettes/sound and
  existing interactive swipe regression.
- G4 Release build, existing BMS SOC unit tests and production getter/serializer
  -> production H7 parser integration test (see companion repository).
- Visual review of actual RGB565 framebuffer captures. SDL window screenshots can
  retain an older frame when a Windows desktop disconnects; the instrument test
  additionally saves the actual framebuffer as PPM for reliable review.

Local commands (after installing the project's normal TouchGFX prerequisites):

```powershell
& 'C:/TouchGFX/4.26.1/designer/tgfx.exe' generate -p "$PWD/Appli/TouchGFX" -v
& 'C:/TouchGFX/4.26.1/designer/tgfx.exe' compile -p "$PWD/Appli/TouchGFX" -s -v
./tests/run_host_tests.ps1
./tests/run_ui_tests.ps1
& 'C:/TouchGFX/4.26.1/designer/tgfx.exe' compile -p "$PWD/Appli/TouchGFX" -t -v
```

Build simulator and target sequentially: they share generated build configuration.
The UI runner supports a disconnected desktop and does not persist fixture settings.
Fixture measurements are simulated, not evidence of working hardware sensors.

Hardware programming and measurements are deferred at the user's request. The
existing AP1 / Under Reset / hardware reset / 1000 kHz / verification programming
recipe is retained. Fonts/text/layout assets changed, so use the full Run Target
programming flow, not a code-only update. A build does not confirm programming.

This is the existing 800x480 project. Cached transitions, NeoChrom path and 200 MHz
PSRAM settings are retained. Simulator results do not prove 60 FPS on hardware or
1024x600 operation; those require the target panel timings, regenerated layout and
measurement on the device. Ethernet PHY bring-up remains deferred and the Network
page reports that status honestly.

PDO encoding reference: Linux USB PD binding definitions,
https://github.com/torvalds/linux/blob/master/include/dt-bindings/usb/pd.h .
