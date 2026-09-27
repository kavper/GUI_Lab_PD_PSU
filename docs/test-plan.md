# Test plan

## Run on a PC

```text
make -C tests/host test
```

`tests/host/test_psu.c` covers:

- G0 CRC, resync after garbage, bad CRC, 68-byte signed current at offset 20, ADC raw at offset 24
- power at 27 V / 5 A and negative current
- voltage text below and above 10 V, first-key replace, 0–27 V and 0–5 A clamps
- G4 session `TEL 500` then `STATUS`, SET coalesce, ILIM/SET rate limit, `OFF` when the queue is full, PPS lock, split lines
- sequencer length 12, remove, skip, no automatic output on, controller timeout
- charger NiMH block, PRECHARGE to COMPLETE, telemetry-loss FAULT and output off
- preset JSON sync, lease, five link flaps, DHCP timeout, HTTP chunking, CRC recovery

Result recorded from this workspace: `psu host tests passed` with `-Wall -Wextra -Werror`.

## Not run here

1. TouchGFX Generate Code
2. Simulator build (`make -f simulator/gcc/Makefile`)
3. Target build (`make -f gcc/makefile_appli` or CubeIDE)
4. Flash and bench tests: button, LD1, UART7 frame, USART1 once the pins are known, Ethernet unplug, charger on a real pack

Do not flash from this branch until Generate Code has been run and the target link is clean.

## Bench notes already fixed in code

- Main-screen current hides invalid and negative samples. Diagnostics keeps the signed microamp value.
- Output LED follows the confirmed status bit, not the finger, after the next model publish.
- Charger start reads live telemetry. It does not invent a pack voltage.
