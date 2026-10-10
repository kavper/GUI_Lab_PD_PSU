# H7 voltage display and protection responsibilities

## Voltage display

The main screen input/output voltage uses `DisplayVoltageFilter` in its presenter.
The previous 10 mV hold/hysteresis is removed. The displayed value is now the
rounded arithmetic mean of up to four actual, distinct METER readings observed
by the presenter, retaining at most 80 ms of acquisition history. It never holds
an arbitrary first reading and even a permanent 1 mV change eventually replaces
the entire history. No synthetic samples or setpoint values enter the average.

`PsuSnapshot.meter_serial`/`meter_ms` identify the UART acquisition and change only
when a new METER arrives (or a legacy direct-G0 sample is observed). Repeated GUI
ticks of the same snapshot do not add a reading. The G4 packet has no independent
G0 ADC sequence number, so this is averaging received METER readings, not claiming
that each is a different physical ADC conversion. Frames coalesced before a GUI
tick contribute the latest snapshot only.

This follows the moving-average/step-window approach described in the
[Keithley 6430 reference manual, section 6-12](https://download.tek.com/manual/6430-901-01G_Jan_2021_Ref-2.pdf).
The four-reading count, 80 ms history cap and 50 mV step window are H7 design
choices, not manufacturer recommendations for this PSU. A change >=50 mV between
consecutive readings flushes old history and passes the new sample immediately.
Zero also passes immediately. STARTING/STOPPING bypass output-voltage averaging,
and changing the requested voltage resets its history. Invalid data resets it.

There is a real trade-off: small steady fluctuations are averaged and small slow
ramps have a short delay. At a 20 ms METER period a full four-reading mean lags a
linear ramp by 30 ms and a sub-50 mV step settles after four new samples (60 ms
after its first sample). A large step/fast ramp or startup/shutdown has no added
averaging delay. Large noise spikes are deliberately visible as steps too; the
filter cannot distinguish a real instantaneous jump from noise without more data.
Normal acquisition/UART/GUI latency remains. This is not an oscilloscope or an
accuracy/calibration claim; existing number formatting is unchanged.

Power calculation, diagnostics, telemetry, setpoints, charger and protection
logic continue to consume unfiltered measurements. G4/G0 firmware is unchanged.
Tests cover different initial noise phases converging to the same mean, small DC
changes, duplicate redraws, immediate bidirectional steps, transition bypass,
slow/fast ramps, history expiry, invalid data and clock/serial/integer wrapping.

## Protection and command interlocks in the current H7 code

The active hardware path has `g4_uart_configured=1` and G4 supervises runtime.
The following describes H7 logic, not a certification of physical protections.

1. Cold boot/configuration load leaves output OFF; the first METER causes an
   explicit initial OFF. An ON is blocked while initial OFF is pending.
2. ON needs a valid METER no older than 200 ms, G0 present with age <=500 ms and
   no MEAS_LOST. No pending fault/shutdown/start or stopped automation source
   may be bypassed. Expected idle/start/stop G0 flags are classified by phase.
3. Voltage/current requests are capped at 27 V / 5 A and lower reported hardware
   or stored ceiling limits. Stored OVP/OCP ceilings cap requests; H7 does not
   compare measured output voltage/current to those values to implement a trip.
4. G4 fault bits or confirmed G4 control FAULT/latch cause an H7 latch, cancel
   queued commands and request shutdown. Confirmed G0 MEAS_LOST/POWER_KILL causes
   are labelled when G4 reports that control fault. Raw kill or stale METER alone
   does not independently trip H7 during normal PSU operation.
5. G4 owns physical start/stop deadlines. H7 has no separate START/OFF TIMEOUT
   protection latch. An unconfirmed OFF remains pending, blocking a new ON until
   fresh telemetry confirms the stopped hardware state.
6. Rejection or transport timeout of a still-requested ON cancels the request,
   stops automation and requests OFF, without an H7 protection latch. A new
   manual command can retry after confirmed OFF, without CLEAR.
7. SET rejection/timeout is a command diagnostic, not a reason to shut down or
   latch a fault while G4 reports normal operation. H7 never assumes the new
   setpoint was applied from a failed command.
8. Transport deadlines remain ON 10 s, other commands 800 ms. ON timeout is
   transaction cleanup, not a second physical startup supervisor. Diagnostic
   `command_error` is separate from the confirmed fault latch and NACK fields.
9. Shutdown stops sequencer/charger, cancels pending ON/SET and prioritises OFF.
   Full power shutdown blocks later ON and setpoint writes in this boot session.
10. CLEAR needs its ACK followed by a new, fresh, healthy METER with G0 ready,
    no actionable faults and output/want OFF. Data return/CLEAR does not restart
    the PSU or stopped automation.
11. Sequencer validates step ranges, locks editing while running/paused and
    aborts when applied setpoint readback is missing/mismatched past its 500 ms
    checks; stop action determines OFF/keep/start limits for normal sequence stop.
    An emergency PSU shutdown always forces its OFF stop action.
12. External charger mode validates profile/confirmation/cell count, pack voltage
    and current/C-rate, requires telemetry and PERMIT and a manual polarity check
    unless supplied hardware status covers it. During charging it requests OFF
    on telemetry/PERMIT loss, optional configured temperature limit, or session
    timeout. A manual polarity confirmation is not a measured reverse-polarity
    hardware protection. These checks are specific to charger mode.
13. CRC/length validation drops corrupt UART frames; TYPE/SEQ matching prevents
    unrelated ACK/NACK completing a command. OFF has transport priority and old
    queued ON is cancelled. H7 sends an independent heartbeat every 100 ms; G4
    owns the physical reaction if the heartbeat disappears.
14. Remote access disable blocks web commands. Network transport is future work;
    this is an API access interlock rather than an electrical protection.

There is also a legacy direct-G0/no-G4 branch that latches G0 OFFLINE after a
previously seen G0 disconnects with output requested. It is not the active
G4-supervised hardware path.

H7 has no general local measured OVP/OCP/OTP/short-circuit/remote-sense trip in
this path. Those physical/control protections belong to G4/G0/BMS. H7 displays
their status and acts on confirmed G4 faults rather than creating a third
independent electrical supervisor. Inspect `psu_app.c`, `g4_ascii.c`, `psu_seq.c`
and `psu_charger.c` for the implemented conditions.
