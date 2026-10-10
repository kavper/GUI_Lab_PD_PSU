# H7 voltage display and protection responsibilities

## Voltage display

The main screen input/output voltage uses `DisplayVoltageFilter` in its presenter.
It is a 10 mV display hysteresis, not a temporal low-pass filter. The displayed
value is held while a valid raw sample differs by at most 10 mV; outside that band
the exact new sample is displayed on the same update, including voltage collapse.
Zero always passes immediately. Entry, invalid telemetry and a pause over 500 ms
reset the state. This does not add settling time to a voltage step or ramp.
The filter error relative to each valid sample is bounded by 10 mV before existing
screen number formatting (1 mV digits below 10 V, 10 mV digits above).

This suppresses only small fluctuations. Noise exceeding that band remains
visible, deliberately: smoothing larger fluctuations could hide real changes.
The normal acquisition/UART/GUI refresh delay remains; this is not an oscilloscope.
Power calculation, diagnostics, telemetry, setpoints, charger and protection
logic continue to consume unfiltered measurements. G4/G0 firmware is unchanged.

Tests cover jitter, immediate steps in both directions, collapse, same-timestamp
updates, 1/5/10/11/100/1000 mV rising/falling ramps, invalid data and reset.

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
5. An uncompleted start exceeding 10 s causes H7 START TIMEOUT and shutdown.
6. An uncompleted stop exceeding 800 ms causes H7 OFF TIMEOUT and shutdown.
7. G4 rejection of a still-requested ON causes H7 ON REJECTED BY G4 and shutdown.
8. Transport timeout of ON or LIMITS causes H7 COMMAND TIMEOUT and shutdown
   (transport deadlines: ON 10 s, other commands 800 ms).
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
