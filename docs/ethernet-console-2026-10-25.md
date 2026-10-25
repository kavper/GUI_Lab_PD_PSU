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


## Freeze investigation and compact layout follow-up

The user reported repeated complete LCD freezes after the suite release.
Ethernet and core debug access disappeared together while ST-Link still
reported 3.21 V. The earlier short runtime checks did not establish long-term
stability. Reset alone could not always recover access; a full power cycle
and repeated Under Reset connection recovered MCU ID 0x485 / Cortex-M7.

Two distinct corrections were made:

1. HTTP ownership: close_session accessed the TCP PCB after successful
   tcp_close. The lwIP contract revokes ownership at success and may free the
   PCB immediately. Callbacks are now detached before closing, restored only
   on error (ownership retained), and no PCB access follows success.
   test_http_lifetime.c uses the actual bundled lwIP allocator and instruments
   callback setters against revoked ownership. The test fails on the prior
   release and passes with this correction, including close-retry callbacks.
   This HTTP correction alone did not resolve the observed complete freeze.

2. Silicon workaround: the board MPU left the unused GFXMMU window
   0x25000000-0x25FFFFFF subject to default speculative access. ST ES0596 Rev10
   section 2.2.17 documents a whole-system hang from this access, including
   speculation. MPU region 9 now marks the full 16-MiB window Device, XN,
   noncacheable and no-access. On GFXMMU-equipped parts the clock is also
   enabled before CPU caches, applying both ST-recommended measures. GPU2D,
   real framebuffer RAM and all CPU/PSRAM/NOR/display clocks are unchanged.
   Reference: https://www.st.com/resource/en/errata_sheet/es0596-stm32h7rxx7sxx-device-errata-stmicroelectronics.pdf

The second correction is the leading explanation, supported by the missing
workaround and failure symptoms; no fault-register dump was recoverable from
the original locked state, so the exact triggering access was not observed.
After verified programming of the guarded version, DHCP/HTTP and SWD returned,
CFSR/HFSR were zero, and the GUI frame counter advanced. The user confirmed
normal, responsive physical touch/menu behavior during the network stress run.
The 300-second read-only stress run completed 7991 operations with zero errors,
including incomplete/fragmented headers and aborted reads. Median/max mixed
HTML/API response time was 16/63 ms. Final counters: RX 59769, TX 89366,
RX drops 0, TX errors 0. At uptime 325466 ms the GUI frame counter was 10047,
CFSR/HFSR were zero and backlight ready was 1. The user operated the local
screen normally during the test; a subsequent view showed user-enabled
5 V output with actual current around 0.765 A and controller ACK.
Runtime MPU readback confirmed region 9, RBAR=0x25000009,
RASR=0x1005002F. This is a bounded five-minute validation, not a guarantee
against every future fault or traffic pattern.

The desktop console now caps content at 1440 px wide and 700 px high, aligned
under the navigation, instead of stretching to the entire monitor. Preset and
system cards follow their content. The seven tabs still fit 1366x768, and the
Full-HD layout leaves breathing room around the compact console. These are
browser CSS changes only.

Regression/build logs: Board/http-free-regressions.log (includes actual-lwIP
lifetime test), http-free-generate.log, http-free-simulator.log,
gfxmmu-guard-target.log and gfxmmu-guard-flash.log (download verified).
Read-only extended test: tools/test_remote_stability.py; it mixes bounded
parallel GETs with fragmented requests, incomplete headers and RST closes.
It never sends power, setting or output commands.

The final web bundle also tolerates floating-point noise at 1/2/5 tick
boundaries, avoiding a single labelled grid line for an almost-flat voltage.
A regression asserts multiple ticks at 5.001 V. This last adjustment changes
browser JavaScript only; network/MPU code is identical to the five-minute run.
Before the final code-only write, HTTP OUTPUT OFF was sent and confirmed.
Final verified-programming log: Board/gfxmmu-guard-flash-final.log.
The guarded compact build uses 292000 B AXI RAM and 10319392 B PSRAM;
framebuffer allocation and graphics assets remain unchanged.

A subsequent short run with an additional open browser experienced two
network timeouts; the user explicitly confirmed the LCD remained responsive.
The IP and HTTP recovered without a device reset, and CFSR/HFSR remained zero.
This was not evidence of another whole-MCU freeze. Read-only SWD at 4000 kHz
worked when the 1000-kHz debug-read connection failed; network_status.py now
accepts --freq and defaults to 4000 for reads. Programming remains exactly
AP1/Under Reset/hardware reset/1000 kHz/verification.

To avoid making transient TCP/network outages look like a frozen web screen,
each browser request now has a 3-second AbortController timeout covering both
headers and the response body. The existing polling loop retries afterward,
and recovered extended telemetry clears its stale connection error notice.
tests/test_web_api.js covers request abort, stalled response body, timer cleanup,
JSON POST preservation and HTTP error propagation. These browser changes do
not alter MCU display scheduling. The final routine-usage check is recorded
below; the stress tool supports one or two workers to distinguish normal
browser traffic from intentional connection-capacity pressure.


Final combined browser + single-worker 90-second test: 1151 operations,
zero errors, median/max mixed response 16/63 ms; RX drops and TX errors zero.
All seven live browser tabs stayed connected at 1366x768 with no clipped
controls or page scroll; Full-HD content measured 1440x700 under the nav.
At uptime 146270 ms SWD readout showed GUI frame counter 8188, backlight ready,
and CFSR/HFSR=0. Output was confirmed OFF after the final update. The screenshot
was saved as output/web-console-compact-2026-10-25.png in the chat workspace.
