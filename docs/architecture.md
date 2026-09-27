# Architecture

The H7 is the GUI, the coordinator, and the only place that queues commands. Screens and the web helpers do not call UART HAL themselves.

```text
TouchGFX views / web JSON
        |
        v
   psu_app  (one snapshot, revision counter)
        |
        +-- G4AsciiProtocol   USART1 ASCII, not started: pins are not in the ioc
        +-- LDO protocol      UART7 binary, existing driver
        +-- sequencer
        +-- external charger
        +-- protection latch
        +-- net link state
        +-- preset store
```

`PsuSnapshot` is copied with an odd/even generation counter so a reader retries if a writer is in the middle of an update. The target writer is the default FreeRTOS task (`LDO_ProtocolProcess`, G0 sample, user button, `psu_app_tick`). TouchGFX only reads the snapshot.

Commands carry an id and move through queued, sent, acknowledged, rejected, or timed out. A setpoint is acknowledged only when G0 applied voltage and current match the request. There is no invented G4 acknowledgement.

Simulator measurements live in `psu_sim.c` and compile only with `SIMULATOR` or `PSU_SIMULATOR`. The target makefile does not list that file.

Output is off after `psu_app_init`. A G0 reconnect also queues output off inside the existing `ldo_protocol.c` path. Preset records force the saved output flag off on every load.
