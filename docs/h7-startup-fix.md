# H7 output startup classification

Current policy is described in [h7-display-and-protection.md](h7-display-and-protection.md).
The sections below record earlier startup fixes; their independent H7
START/OFF/COMMAND timeout latches and ON-NACK latch have been removed.
G4 owns physical protection and start/stop deadlines. H7 retains pre-ON
readiness, explicit shutdown, confirmed G4 fault recovery and no automatic
restart. Failed ON cancels the transaction with OFF but needs no CLEAR;
failed SET stays diagnostic. See the final section for the latest behavior.

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
| Later STARTING ctrl 4..8, still no output/control latch | POWER_KILL may wait for G4's PERMIT retry; measurement/link loss, VIN_LOW and other faults trip OFF/latch. |
| RUNNING (G0 output, ctrl RUNNING or ON ACK) | Measurement/link loss and POWER_KILL trip priority OFF and latch. VIN_LOW and other real fault bits are actionable. |
| Explicit OFF / STOPPING, output already off, ctrl <12 | Delayed want/ctrl/kill convergence is allowed for at most 800 ms; real faults remain actionable. |

### Follow-up: delayed POWER_KILL and explicit stopping

The photo from hardware running the first fix shows a local H7 `G0 POWER_KILL`
latch with current G4 fault=0, ctrl=0, control latch=0, G0 fault=0 and kill=1.
That is the state after the trip, not proof of the exact originating phase. The
unmatched ON NACK UNSAFE is consistent with G4 cancelling ON after H7 OFF;
`HostLink_ApplyOff()` does exactly that while `s_on_wait` is true.

The previous H7 exception ended at ctrl=3. This missed G4 ctrl=7 SEND_OUT_ON,
which explicitly returns to WAIT_PERMIT when `kill_reported` is set, and ctrl=8
WAIT_OUT_ON_ACK, where `Ldo_RejectOutOn()` can return WAIT_PERMIT. METER mixes
G4 control state and the last G0 sample, so a delayed kill bit must not make H7
abort that recovery before G0 output / G4 RUNNING / ON ACK is confirmed.

The corrected policy exempts only POWER_KILL throughout accepted STARTING
ctrl=0..8, with no output and no control latch. MEAS_LOST/VIN_LOW still get only
the earlier ctrl=0..3 exception. Running raw kill=1 remains an emergency even
with a zero G0 fault mask. Actual G4/G0 faults, freshness checks, and 8-second
startup timeout remain active. Explicit OFF now has STOPPING until the complete
stopped telemetry arrives (bounded by 800 ms); delayed want/ctrl/kill flags after
that user OFF do not create a false new latch while output is already off.

Diagnostics preserves the originating METER for the first actionable event and
prints `TRIP phase, ctrl, o/w/p/k, G4, G0, age`, so later idle packets cannot hide
the cause. `o/w/p/k` means output / wanted / permit / kill. Phase numbers are
0=OFF, 1=STARTING, 2=RUNNING, 3=STOPPING. The main SET rejection also displays
the actual latch reason instead of the generic "Blocked - check protection".

Additional tests reproduce late startup kill=1 with G0 fault=0, G4 retry to
WAIT_PERMIT, delayed OFF convergence, raw kill during RUNNING and origin-frame
preservation. This follow-up has not been flashed or validated on hardware.

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

## OFF before G0 output telemetry catches up

G4 removes PERMIT immediately when it receives OFF. G0 publishes output
state on a separate UART tick. Thus an explicit STOPPING phase can receive
`kill=1` while `out=1`, `want=1` and `ctrl=RUNNING` still describe the previous
state. This is expected shutdown, not a new running POWER_KILL fault.

During STOPPING, POWER_KILL is excluded regardless of the reported OUT bit.
MEAS_LOST and VIN_LOW remain actionable until OUT is reported off; other
faults and the G4 fault latch remain actionable throughout. A normal RUNNING
kill still triggers priority OFF and requires manual recovery. A new ON is
blocked until the stopped state is confirmed. Regression coverage includes
raw kill with a zero fault mask and stale ON/want/ctrl during explicit OFF.

Run the host suite on macOS/Linux with `tests/run_host_tests.sh`; the existing
PowerShell runner remains available. Host tests exercise the real H7 parser
and application using binary METER/ACK/NACK frames. They do not replace a
board test of analogue regulation, physical PERMIT polarity or DCDC drivers.

## Coordinated recovery with the paired G4 update

Use this H7 with the G4 CLEAR/heartbeat update, not the previous f70489a.
G4 executes the physical protection and output sequence; G0 retains its
local LDO protection. H7 no longer invents a fault from the raw kill bit.
Explicit G0/G4 faults remain visible and stop UI consumers; CLEAR recovers
through the G4 OFF transaction, without auto ON. A buffered pre-CLEAR
ctrl=FAULT frame cannot invalidate the recovery ACK.

H7 sends PING at 100 ms intervals while METER is current. G4 removes
PERMIT locally if no CRC-valid host frame arrives for over 1000 ms during
start or operation. This covers a broken H7-to-G4 direction or panel reset.
An ON transaction has a 10-second host deadline; G4 owns the 8-second
physical startup deadline. The 2-second difference allows its final reply.

G0 owns runtime voltage protection. The persisted H7 voltage ceiling still
limits requested setpoints; it is not an instantaneous measured OVP latch.
This avoids tripping H7 on measurement error at a 27 V setpoint.

Host validation: all three test suites, including CLEAR with an old FAULT
frame, delayed G0 start, expected OFF kill and real running faults. Modified
H7 communication/application units also compile for Cortex-M7. A complete
firmware link requires the project's TouchGFX SDK/generated build files,
which are not present in this Git checkout. No board validation is claimed.

## Final ownership of runtime protection

G0 disables its own LDO on local faults. G4 supervises G0 and DCDC and
publishes ctrl=FAULT / the control latch. H7 now consumes that confirmed
supervisor fault; it does not independently trip on a raw G0 fault/stale
transition or raw kill pin while G4 is still processing the event. Before
ON, H7 still checks readiness and reports why a request cannot be made.
During operation, a lost H7-G4 transport or expired command remains a
communications failure (priority OFF), not a new interpretation of G0's
analogue protection. G4's heartbeat handles failure of H7 itself.

The H7 tests explicitly deliver the G0 transition first and then G4's
supervisor FAULT, verifying that only the latter stops the UI consumers.
The paired G4 is `d1c40fe`; G0 remains `f974390`. Flash the coordinated H7
and G4 pair together. The older G4 has neither heartbeat nor CLEAR recovery.

## METER runtime age (2026-10-10)

H7 no longer queues OFF or latches a fault solely because METER is older than
50 ms during STARTING/RUNNING. Telemetry age remains diagnostic and the existing
freshness check before a new ON remains in place. G4 owns host-link loss shutdown
(1000 ms without a valid host frame). Confirmed G4 faults, command/start/stop
timeouts and explicit OFF remain actionable. This change does not establish the
cause of the observed sub-second shutdown; NACK and fault telemetry are still
needed to distinguish G4/G0 faults from host-link loss.

## Coordinated runtime service (2026-10-10)

* PING every 100 ms is independent of METER reception. It occupies one
  coalesced slot separate from the user command FIFO; OFF stays first.
* Default UART/control task runs at CMSIS priority High1, above the GUI High
  task. Its stack and priorities are also recorded in the CubeMX project.
* RX overflow is determined by unread DMA bytes, not a 25 ms scheduling gap.
* The shared METER readiness/display age is 200 ms. Exceeding it never causes
  an independent H7 runtime OFF. G4 host timeout remains 1000 ms.
* AUX byte 30 reports the retained G4 stop cause: 0 none/legacy, 1 H7 heartbeat,
  2 G0 telemetry, 3 confirmed G0 KILL, 4 G0 fault, 5 G0 command/start failure.
  Byte 31 stays zero and the 32-byte AUX length is unchanged. Diagnostics
  displays this cause until G4 CLEAR or a new explicit ON.

Host tests cover missing METER with a full user FIFO, coalescing, OFF priority,
recovery and the cause decoder. Changed H7 C files compile for Cortex-M7 with
-Wall -Wextra -Werror. Full TouchGFX linking and real board/DMA scheduling
remain unverified here because the TouchGFX SDK/generated assets are missing.

Paired runtime release: G4 `d1c40fe`, G0 `f974390`; H7 runtime source `9e9c9eb`
plus this documentation update. See the G4 `docs/coordinated-link-audit.md`
for test scope and the remaining board validation.

## Minimal H7 supervisor (2026-10-10)

H7 no longer latches a protection fault for an independent startup deadline,
delayed OFF confirmation, received ON NACK or a command timeout. The G4/G0
electrical protection and G4 physical startup deadline are unchanged.

The ON transaction still expires after 10 s if no reply arrives. A matched ON
NACK/timeout cancels the requested ON, stops automation and sends priority OFF.
Fresh stopped telemetry then allows a new manual ON, without CLEAR. Late ACK
or returning data cannot revive the expired request. A failed SET records an
error but does not invent a hardware fault or stop normal G4-supervised output.
An OFF timeout is diagnostic; H7 continues waiting for actual stopped telemetry
and does not pretend the output is OFF just because an ACK/deadline was received.

Diagnostics displays `command_error` separately from fault latch/context and
received NACK TYPE/SEQ/reason. Actual G4 fault bits/control FAULT still latch,
stop automation and require healthy CLEAR recovery without an automatic restart.
Pre-ON freshness, command range caps, power shutdown and priority OFF remain.
Charger/sequence application-specific validation remains; it is not a parallel
electrical supervisor for normal PSU mode.

Regression cases cover retry after NACK without CLEAR, ON expiry/late ACK,
slow OFF followed by healthy confirmation, SET expiry while running, and an
actual G4 control fault received alongside a NACK. Host tests do not prove board
behavior or G4/G0 analogue protection. This policy requires the paired G4
`d1c40fe` and G0 `f974390`.
