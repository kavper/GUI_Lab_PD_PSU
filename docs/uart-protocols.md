# UART protocols

## G0 / LDO — UART7

Taken from the existing driver. Do not change this map without a version bump.

| Item | Value |
| --- | --- |
| Peripheral | UART7 |
| Pins | PE7 RX, PE8 TX, AF7 |
| Format | 460800 8N1 |
| RX | `HAL_UARTEx_ReceiveToIdle_IT` |
| TX | non-blocking |
| SOF | `0xA5 0x5A` |
| Header | length, type, sequence |
| CRC | CRC-16/CCITT over length+type+sequence+payload, init `0xFFFF`, poly `0x1021` |
| Timeout | 100 ms, 2 retries |

Types already in `ldo_protocol.c`: set output `0x03`, set limits `0x05`, get info `0x07`, telemetry `0x80`, ACK `0x81`, NACK `0x82`, info `0x83`.

Telemetry payload is 68 bytes, versions `1, 1`.

| Offset | Field |
| --- | --- |
| 0 | protocol version |
| 1 | telemetry version |
| 2 | `status_flags` uint16 LE. Bit 4 means the current sample is valid. The existing UI also treats bit 0 as the output state. |
| 4 | `fault_flags` uint32 LE |
| 8 | uptime ms |
| 12 | VIN mV |
| 16 | VOUT mV |
| 20 | IOUT int32 LE, microamps. This is the current. |
| 24 | IOUT ADC raw int32 LE. Never display this as amps. |
| 28 | DAC CV readback mV |
| 32 | DAC CC readback mV |
| 36 | requested voltage mV |
| 40 | requested current mA |
| 44 | applied voltage mV |
| 48 | applied current mA |
| 52 | preregulator mV |
| 56 | four int16 LE temperatures, centi-degrees |
| 64 | mode |
| 65 | startup |
| 66 | reserved uint16 |

Info payload is 16 bytes: versions, period, maximum voltage, maximum current, capability bits. Capability bit 0 means current measurement is calibrated.

Presentation uses an EMA with alpha 1/4. Protections and the charger do not use that filter.

## G4 — USART1 ASCII

Specified transport: USART1, 115200 8N1, no flow control, one command per line ending in `\n`. RX accepts `\n` and `\r\n`.

This repository does not contain the G4 firmware or a return-line grammar. `g4_ascii.c` implements the command side and keeps every received line as raw text. It does not decode `TC`, PDO, BMS, or charger fields. PPS stays locked until something proven sets `pps_ctl`, Sink, and APDO together.

| Action | Line |
| --- | --- |
| Output on / off | `ON` / `OFF` |
| Voltage | `SET 12.000` (3 decimals) |
| Current limit | `ILIM 2.000` |
| USB role | `USB AUTO` / `USB SINK` / `USB SOURCE` |
| PPS | `PPS 9.00 2.50` (2 decimals), rejected while locked |
| Permit / remote / verbose | `PERMIT 0|1`, `REMOTE 0|1`, `VERBOSE 0|1` |
| Status, BMS, clear, period | `STATUS`, `BMS`, `BMS OFF`, `CLR`, `TEL` of 0, 100, 250, 500, 1000, 2000 |

`OFF` and `CLR` can evict a normal command when the queue is full. Repeated `SET` or `ILIM` values collapse to the newest one. A SET/ILIM pair is released at most once per 120 ms. After `g4_link_up`, the port sends `TEL 500` and then `STATUS` once, and repeats that pair only after a real reconnect or a 1500 ms silence.

USART1 is not configured in the ioc. The driver is not attached to pins, because guessing the connector would be worse than leaving the link offline.

Service lines such as `HELP` are accepted only while service mode is on.
