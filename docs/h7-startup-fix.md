# H7 output startup classification

Base: H7 `cc3b15e`, branch `codex/unify-main-header-controls`.
Reviewed peers (unchanged): [G4 f70489a](https://github.com/kavper/Lab_PD_PSU/tree/f70489a)
and [G0 f974390](https://github.com/kavper/LDO_controller/tree/f974390).

## Cause and reference paths

H7 `g4_ascii.c` previously combined G4 and G0 masks into `event_fault`, excluding
only VIN_LOW. `psu_app_tick()` latched any remaining bit and queued OFF, even at
idle. G0 MEAS_LOST describes sample freshness independently of whether output is
enabled (`Core/Src/uart_protocol.c: uart_fault_flags()`). Healthy METER packets
cleared the active mask, but never the H7 latch. The old ON guard also did not
explicitly require valid measurements.

G4 `Core/Src/host_link.c: HostLink_ApplyOn()` starts DCDC and requests G0 output.
`Core/Src/ldo_link.c: LdoLink_ServiceControl()` waits for link, PERMIT, VIN, SET ACK
and OUT ON ACK. `Core/Src/ldo_prereg.c: LdoPrereg_Update()` owns the regulation and
PERMIT policy. H7 must not send OFF just because early startup has no PERMIT or
G0 still reports POWER_KILL. Binary ON ACK is issued only when G4 reaches RUNNING;
its timeout is 8000 ms. No changes or PERMIT overrides were made in either peer.

## New behavior

| H7 / G4 phase | Classification |
| --- | --- |
| OFF, no output/want, G4 ctrl IDLE or stopping, no control latch | MEAS_LOST, POWER_KILL and VIN_LOW are readiness states. Other fault bits still latch. |
| Accepted user ON, no output, G4 ctrl 0..3 (IDLE/WAIT_LINK/WAIT_PERMIT/WAIT_VIN), no control latch | Allow delayed G0 freshness, MEAS_LOST, POWER_KILL and VIN_LOW while G4 starts its own sequence. METER must remain fresh; whole start is bounded by 8 seconds. |
| Later startup or RUNNING (G0 output, ctrl RUNNING or ON ACK) | Measurement/link loss and POWER_KILL trip priority OFF and latch. VIN_LOW and other real fault bits are actionable. |

ON requires METER age <=50 ms, valid G0 telemetry age <=500 ms, no MEAS_LOST,
no real/control fault, no H7 latch and no shutdown. It does not require PERMIT to
be already asserted. A rejected ON records an explicit local reason. Returning
data never queues ON. Emergencies stop sequencer/charger as well as the output;
CLEAR does not resume them.

Actionable G4 and G0 events are accumulated separately until the app consumes
them. A healthy packet in the same UART batch cannot erase a running fault.
The first H7 latch cause is preserved. CLEAR requires a matching ACK and a later
fresh, healthy METER while OFF. It cannot clear active MEAS_LOST, stale data,
real G4/G0 faults or the G4 control latch. A new actionable fault invalidates a
pending clear. POWER OFF / BMS SHUTDOWN continues to forbid ON.

GUI Diagnostics displays H7 readiness/latch and last local ON rejection, separate
G4/G0 fault masks, G4 ctrl/latch, G0 kill, and last received NACK TYPE/SEQ/reason
with correlation. Local timeouts do not overwrite received NACK diagnostics.
The main screen also displays the reason when TURN ON is rejected.

## Validation

Run `tests/run_host_tests.ps1` (optional `-Compiler` overrides the GCC path).
It builds C11 tests with `-Wall -Wextra -Werror` and runs parser, app/simulator
regressions plus `test_startup.c`. Startup tests cover idle MEAS_LOST recovery,
ON readiness, running measurement/link loss, no automatic restart, expected and
emergency POWER_KILL, the end of the early-start exception, CLEAR with active
faults, delayed G0 through the full start sequence, correlated/unmatched NACK,
an 8-second timeout, multi-frame RX faults and stopping the sequencer.

TouchGFX Designer Generate, compile Simulator and compile Target succeeded.
The Diagnostics layout was inspected in the simulator. The startup test is a
host-side reproduction of the peer state sequence, not a hardware integration
test. This fix has not been programmed or verified on physical H7/G4/G0 hardware.

The generated `Appli/TouchGFX/build/bin/target.hex` contains application code and
graphics in external flash at 0x70000000. For an H7 already running cc3b15e, use
that full image with the STM32H7S78-DK MX66UW1G45G external loader. Internal
bootloader image `intflash.hex` is unchanged. Designer Run Target keeps SWD ap=1,
Under Reset, Hardware reset, 1000 kHz and verification after writing. Build
success does not confirm programming success.
