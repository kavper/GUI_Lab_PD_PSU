# Hardware check 2026-10-23

## GUI programming

Old release is retained on Git: `c05f016`, branch
`codex/new-board-is25lx-aps256`. Instrument UI 1.2 is `a237f3d`, branch
`codex/instrument-ui-refinement`.

Executed the actual Designer `tgfx flash` flow. Boot, application code and graphic
assets were programmed, separately verified by CubeProgrammer and reset. Log:
`Board/gui-20261023-flash.log` (local build/hardware artifact). Preserved the AP1,
Under Reset, hardware reset, 1000 kHz and verification programming recipe.

Live checks with the matching ELF symbols: CFSR/HFSR zero, HAL tick advanced by
200 in 200 ms, and `BoardBacklightReady=1`. This confirms code execution after
programming; physical GUI appearance/touch still need the user's observation.

## Ethernet results

- MAC version read `0x3242`.
- LAN8742A responds at MDIO address **0**, ID registers **0x0007 / 0xC131**.
- PE4 reset reads high. PD7 REF_CLK samples have both levels and transitions;
  this proves toggling, **not** a measured 50 MHz clock frequency.
- BMCR `0x3000`, BMSR `0x7809`, partner ability `0x0001`, special status `0x0040`.
  Link is down and autonegotiation incomplete. Explicit negotiation restart did
  not establish link. The user tried another cable/port; LEDs remained dark.
- LAN8742A TDR, forced 100 Mbit full duplex, manual MDI then MDIX: three completed
  tests per pair consistently returned **0x0303**, type **SHORT**, raw electrical
  length **3**. Repeated with RJ45 unplugged: same result on both pairs.
- PHY settings were restored after testing and normal autonegotiation restarted.
- Later clarified resistance measurements: ETH_3V3A-to-GND about **280 ohm**,
  supply **3.3 V** when powered, and R33 confirmed **12.1 kohm**. Approximately
  280 ohm was also reported from differential lines to GND. Near-zero resistance
  from PHY-side differential lines to ETH_3V3A is compatible with the transformer
  center taps being tied to that rail. These measurements do **not** establish a
  hard supply short. The previous approximately 1 ohm indication must not be
  interpreted as a verified line-to-GND short.
- Hardware PHY reset restored TDR register 25 to zero and PHY ID remained
  0x0007/C131 at address 0. Forced 100/full, 100/half, 10/full and 10/half modes
  each remained without external link. Normal autonegotiation also failed.
- A 100 ms internal PHY loopback status test raised BMSR link bit (`0x780D`).
  The user saw a brief LED flash. After restoring normal mode BMSR returned to
  `0x7809` (no external link). This verifies a loopback status response and LED
  behavior, **not** a packet loopback through the MAC/RMII or a router connection.
- The maintained diagnostic wrapper was run in status mode successfully on the
  board. All modified PHY mode settings were restored after the mode tests.

Interpretation: SMI management communication works, but the physical Ethernet
link does not. The short-range TDR indication suggests a local analog-path or
termination/magnetics issue; it does not identify a particular faulty component.
Next checks: actual T1 part/pin orientation and winding connections, ESD paths,
and a scope measurement of REF_CLK frequency. A requested isolation measurement
across the intended KMS-1102NL windings (pins 1-16 and 6-11) is still pending.
Supply resistance and winding DC continuity alone do not identify a fault.
R33=12.1 kohm is confirmed. RMII packet traffic, DHCP and
web/SCPI access are **not confirmed**. The current GUI release still leaves the
Ethernet stack disabled; no IP address or successful packet test is claimed.

## Repeating the PHY test

`Board/diagnostics/phy_probe.py` uses the installed Windows x64 CubeProgrammer
2.23 API. Run only with the Ethernet MAC driver stopped and no concurrent
programmer/debugger. It temporarily enables clocks and muxes MDIO pins, then
restores them. It refuses an active MAC and validates LAN8742A IDs before PHY
register writes. It does not program Flash or change PSU output/setpoints.

```
python Board/diagnostics/phy_probe.py --test status
python Board/diagnostics/phy_probe.py --test restart
python Board/diagnostics/phy_probe.py --test tdr
python Board/diagnostics/phy_probe.py --test modes
```

Use `--serial` if more than one ST-Link is connected. TDR/mode tests are disruptive to
the Ethernet link; report cable-connected/disconnected conditions with the result.
The raw length is a diagnostic counter, not a measured physical distance.

Reference: Microchip LAN8742A datasheet DS00001989A, section 3.8.9.1 and registers
24, 25 and 27: https://www.microchip.com/content/dam/mchp/documents/OTH/ProductDocuments/DataSheets/DS_LAN8742_00001989A.pdf .

Transformer pin reference: ST MB1364 C-01 Ethernet schematic (sheet 6),
https://www.st.com/resource/en/schematic_pack/mb1364-h743zi-c01_schematic.pdf .
