# Ethernet and native browser console, 2026-10-25

## Hardware and identity

The user replaced the faulty transformer. Live LAN8742A BMSR=0x782D,
ID=0x0007/C131, address 0, negotiated 100/full. The user measured 50 MHz on
R29/PD7. STM32H7S7 PD7 REF_CLK is AF4 (not AF11); the driver selects it
explicitly. Other RMII pins use AF11. No PHY reset is needed to preserve an
already negotiated connection. A MAC/DMA rebuild handles link re-entry.

Verified IPv4 DHCP address: **192.168.0.192**. UID-derived, locally administered
unicast MAC: **02:4C:51:09:35:32**. Hostname **LAB-PD-PSU** is inserted in DHCP
DISCOVER and REQUEST option 12. The driver inspected outgoing DHCP packets;
health counter `dhcp_hostname_sent=2` confirms both packets contained the name.
The router may cache or override client labels; its client-table display was
not independently inspected. No mDNS/.local name is claimed.

## Final architecture

Board/board_net.c owns the H7RS HAL port and the only lwIP raw-API task.
Ethernet runs at CMSIS osPriorityNormal, below TouchGFX osPriorityHigh and
UART/panel osPriorityHigh1. DMA TX is asynchronous with four fixed buffers,
polling reclamation after hardware ownership clears; there is no blocking
HAL transmit loop. RX copies into staging before HAL recycles DMA buffers.
Eight RX packets maximum per poll, plus a cycle budget. DMA descriptors and
packets use a linker-reserved, noncacheable AHB SRAM section. The lwIP heap and
three fixed HTTP sessions use a separate NOLOAD PSRAM section, with no overlap
with framebuffer/GPU memory. Software IP/UDP/TCP checksums are enabled.

The user rejected a half-resolution framebuffer preview. It was removed from
the final firmware, including all capture/remote-touch hooks, thumbnail memory,
and GPU involvement. TouchGFXHAL.cpp and Board/touch_controller.cpp have no
functional changes from the existing GUI. The network never captures the LCD.

The browser renders native text, CSS and SVG at the browser resolution. It polls
small JSON state every 250 ms, pauses polling in hidden tabs, draws a 60-second
browser-local V/I history, and shows requested/applied settings, output state,
fault/ack status, temperature, five physical BMS channels, SOC, signed pack
current/power, BMS accumulated charge and active PD contract limits. Missing or
stale peripheral values are null; there is no synthetic telemetry or implied
USB-C current sensor. History is a browser session, not a persistent logger.

GET / and /api/state, GET /health, POST /api/command. Writes use the existing
model and G4 acknowledgement/readiness/fault path. The local Remote control
switch gates writes. Per-client write ownership expires after 10 seconds;
local controls are not locked out by that lease. Cross-origin browser writes
are refused, as are unsupported content types and transfer encodings. Bounds,
partial receives, limited sessions and deadlines apply. This is a LAN HTTP
console, not an authenticated public Internet service or an SCPI implementation.

Strict command JSON accepts either voltage_mv/current_ma together or output
0/1. It rejects duplicate/unknown members, negative/fractional values, overflow,
leading-zero integers, malformed objects and trailing input. One snapshot is
used for common state and web-specific details. Disabling remote writes keeps
the DHCP lease/link, so local network status stays truthful.

## Validation

- Designer Generate, simulator compilation, target compilation succeeded.
- Host suite including strict web command/lease tests passed.
- Simulator INSTRUMENT and FINGER regression suites passed, including all views
  and edge-back navigation.
- Full Designer flash initially programmed Boot/code/graphics and verified all
  three segments. Initial native code-only update used unchanged graphics and the same
  SWD AP1 / Under Reset / hardware reset / 1000 kHz / verify settings. Initial native
  programming log: Board/eth-20261025-code-native.log, verified successfully.
- OUTPUT OFF was sent via the HTTP/model/G4 route and confirmed before restart.
  Later ON/load operation was user initiated; it was not enabled by a test.
- Live final firmware obtains DHCP, serves the actual console and peripheral
  JSON. CFSR/HFSR=0, backlight ready. Browser render was checked on narrow and
  desktop layouts. Confirmed live CC about 2 A, local setpoints 15 V / 2 A,
  BMS and USB-C information visible through the same model.
- 30 sequential /api/state reads: median 4.9 ms, maximum 32.78 ms. Fragmented
  GET=200, missing path=404, oversized request=413, cross-origin POST=403.
- ICMP: 4/4 replies, 0% loss, 0-3 ms. During browser polling, zero RX allocation
  drops and zero TX errors were observed. Counts are observations, not a
  guarantee for every future traffic pattern. `max_poll_us` is wall time and
  includes preemption by the higher-priority GUI/control tasks.
- Initial native target allocation: AXI RAM 291480 B/432 KiB; ETH AHB 12480 B/32 KiB;
  PSRAM 10276384 B/32 MiB; NOR code 655040 B/2 MiB. The existing 50000-byte
  FreeRTOS heap reservation is unchanged; Ethernet adds an 8192-byte task stack
  allocated from it. LCD refresh/GUI input settings are unchanged; no claim of
  a new fixed 60 fps rendering benchmark is made.

Unplug/replug recovery is implemented but was not physically exercised during
the active output/load test. Do not use phy_probe.py while this MAC driver is
running; use Board/diagnostics/network_status.py with the matching target ELF.

Primary pin reference: ST STM32H7S7 datasheet DS14359, alternate-function table:
https://www.st.com/resource/en/datasheet/stm32h7s7l8.pdf


## Expanded instrument suite

The final console has seven native tabs: Output, Battery/BMS, USB-C/PD,
External battery charger, Sequencer, Presets, and System. Desktop layouts fit
1366x768 and 1920x1080 without page scrolling. The sequencer shows six editable
rows per page (12 maximum), diagnostics 16 fields per page. Small screens may
scroll and keep the wide step editor within its own horizontal container.

- Output: shared requested/applied values, confirmed output and controller ACK,
  60-second browser history. OFF stops automation through psu_app_shutdown;
  manual limit changes/ON cannot bypass active sequencer or charger locks.
- Battery: five physical channels, inactive channel, SOC, accumulated charge,
  signed current/power, cell spread, FET state, temperature and fault/alarm.
- USB-C: actual contract and port-role commands; full PDO/RDO decoding when G4
  supplies the capability frame, selected-object arrow, cable-generation and
  freshness checks. Sink input ADC current is never presented as outgoing
  SOURCE current. Browser-observed received energy integrates distinct valid
  sink samples only, with gaps on stale/detached data.
- External charger: chemistry/cell/current/termination/target profile, explicit
  polarity confirmation, start/stop, state/reason/elapsed/Ah/Wh and model trace.
  Uses existing G4 readiness, ACK and fault supervision. No load was enabled
  merely to test the web interface.
- Sequencer: edit/add/remove, per-row enable/output/slew/time, repeat/stop policy,
  start/pause/resume/stop. Editing locks during active automation.
- Presets: edit/store/apply limits/save current/restore defaults. Existing
  nonvolatile configuration journal also persists appearance and sequence edits.
- System: physical display theme/accent and buzzer controls/test, network,
  four G4 telemetry records including Sense/AUX, STATUS, fault clear, service
  enable/HELP, BMS initialization, remote-sense selection and power shutdown.

GET /api/features, /api/diagnostics and /api/pd provide these model views.
POST /api/feature uses strict numeric command fields; operation IDs are listed
in psu_web_features.c. A fixed 4-slot single-producer/single-consumer queue
(3 usable jobs) transfers changes to the GUI owner. One job is consumed per
non-transition frame. Remote authorization is checked again at execution.
Appearance invalidation and buzzer application run on the GUI thread. A queued
job ID, model result, and G4 command acknowledgement remain distinct. The web
is not an SCPI endpoint.

HTTP responses have a bounded 16-KiB buffer per fixed session in PSRAM. Requests
are still bounded at 1536 bytes, with command bodies at most 512 bytes. Transport
priority, TX budgets, GUI timings and LCD memory handling are unchanged. The
empty GUI queue poll performs no drawing, peripheral access or blocking wait.

The embedded page is packaged from Board/remote.html, remote.js,
remote_suite.js and remote_suite.css by tools/package_remote.py. It emits the
checked-in remote_page.h and an untracked remote_preview.html for local QA.
The preview helper/fixtures are temporary development data, not device data.

### Chart auto range

Voltage and current have independent browser-side scales, fitted to valid
samples, 15% headroom on each side, minimum visible span (50 mV / 5 mA), and
nice 1/2/5 tick steps. Expansion is immediate; contraction requires five
seconds of consistently narrower data. Values are not anchored to zero.
Flat, zero and negative signals have finite ranges. Tick labels use V/A or
mV/mA with precision derived from the step. Missing data breaks the trace.
The same renderer serves output and charger history; all chart work executes
in the browser, with no LCD capture or GPU involvement.

### Suite verification

- Designer Generate, simulator and target compilation passed. Simulator
  INSTRUMENT/FINGER suites passed all screens and edge returns.
- Host suite includes test_web_features.c: malformed/duplicate/overflow fields,
  bounded queue, execution-time access, preset ranges, charger polarity and
  chemistry, active-automation locks, appearance and 128-point serialization.
- node tests/test_web_charts.js verifies independent ranges, zero/negative/flat
  samples, expansion/contraction and 1/2/5 ticks.
- Browser checked all seven tabs at 1366x768 and 1920x1080; mobile layout was
  checked at 390x844. Local preview used explicit host-test fixtures; subsequent
  device checks used actual HTTP endpoints and G4 telemetry.
- Actual device serves all API paths; diagnostics contains four real records.
  STATUS web job was queued and executed successfully. 30 state reads had
  median 4.91 ms, maximum 26.09 ms; RX drops/TX errors remained zero.
- Current G4 does NOT supply capability frame 0x14: /api/pd valid=0/count=0.
  Contract role/voltage/current and BMS/charger telemetry are live. Full PDO/RDO
  rows require the prepared G4 telemetry update; no fabricated rows are shown.
- Target: NOR code 699264 B/2 MiB, graphics 3373792 B/30 MiB,
  PSRAM 10319392 B/32 MiB, ETH AHB 12480 B/32 KiB. RTOS reservation unchanged.
- Verified code-only programming: Board/web-suite-flash-final.log, AP1, Under
  Reset, hardware reset, 1000 kHz, verification. Graphics were unchanged.
