# Memory map

Active Appli linker script: `STM32CubeIDE/Appli/STM32H7S7L8HXH_RAMxspi1_ROMxspi2.ld`.

| Region | Address | Size | Use |
| --- | --- | --- | --- |
| FLASH (ROM, XSPI2 map) | `0x70000000` | 128 MiB minus 128 KiB | Code, constants, TouchGFX assets in `.rodata` |
| PSU_CONFIG | `0x77FE0000` | 128 KiB | Two config slots. `NOLOAD`, not part of the firmware image |
| EXTRAM (XSPI1 map) | `0x90000000` | 32 MiB | External RAM |
| RAM | `0x24000000` | `0x71C00` | AXI SRAM |
| RAM_NONCACHEABLEBUFFER | after RAM | `0x400` | Non-cache buffer |
| DTCM | `0x20000000` | 64 KiB | Stack and heap check |
| ITCM | `0x00000000` | 64 KiB | — |
| SRAMAHB | `0x30000000` | 32 KiB | — |
| BKPSRAM | `0x38800000` | 4 KiB | — |

The config origin is `__FLASH_BEGIN + 0x08000000 - 128KiB`. Shrinking `FLASH` makes the linker fail if code or assets grow into that tail. The slots are not programmed. A memory-mapped store would not erase a NOR sector safely, so `psu_config_save` on the target has no NOR driver and the tested backend is RAM.

Record layout, 16-byte header plus payload:

| Offset | Field |
| --- | --- |
| 0 | magic `PSU1` (`0x31555350`) |
| 4 | schema uint16 = 1 |
| 6 | payload length uint16 |
| 8 | generation uint32 |
| 12 | CRC-32 of the payload |
| 16 | `PsuBlob` |

The loader keeps the valid copy with the higher generation. A bad CRC falls back to the other copy, then to defaults. `output_on` in the blob is cleared on save and on load.
