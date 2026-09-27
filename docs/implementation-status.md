# Implementation status

Branch: `cursor/full-product-ui`
Rescue tip of previous `origin/main`: `cursor/rescue-pre-full-product` at `309e782e169b3c59be5ab820b2d8f72551888290`

Host evidence, this tree:

```text
make -C tests/host test
psu host tests passed
```

Compiler: `gcc -std=c11 -Wall -Wextra -Werror`. The TouchGFX simulator and the STM32 target were not built in this environment. `Appli/Middlewares/ST/touchgfx/` is not in the checkout, `generated/` is gitignored, and `arm-none-eabi-gcc` is not installed. No `target.hex` was produced, so there is no SHA-256.

Kacper must pull this branch and run **Generate Code** in TouchGFX Designer 4.26.1 before the new screens exist as C++ base classes. The instrument pages are composed in `GUI_Lab_PD_PSU.touchgfx` from bitmaps in `Appli/TouchGFX/assets/images/` (header bar, rounded cards, icon tiles, step rows). Screen1 is still the keypad face.

## Acceptance checklist

- [x] Cold boot starts with output OFF. `psu_app_init` clears the request, and the host test checks the snapshot. Hardware confirmation still required.
- [ ] USER BUTTON 1 toggles output and LD1 shows the confirmed state. Code polls PC13 (UM3289, pressed = high) and drives LD1 only from `setControllerOutputState`. **IMPLEMENTED, HARDWARE VERIFICATION REQUIRED.** Zephyr maps the same button as active-low.
- [ ] SET/ILIM from the LCD reaches G4 and the readback returns to the LCD. The ASCII command is built and queued. USART1 is not in `STM32H7S78-DK.ioc`, so the bytes are not put on a pin. **IMPLEMENTED, HARDWARE VERIFICATION REQUIRED.**
- [ ] SET/ILIM from the web page reaches G4 and the LCD. The JSON command updates the same model (`tests/host`). There is no Ethernet server task. **BLOCKED: Ethernet is not enabled in the Appli CubeMX configuration.**
- [x] An LCD limit change is visible in the web JSON. Host test posts and reads `requested_mv`.
- [x] G0/G4 loss is stale/offline and does not spin in the GUI. G0 age flags stay in `ldo_protocol.c`. G4 link becomes STALE after 1500 ms without a line. No `HAL_Delay` was added.
- [ ] Five Ethernet unplug cycles do not freeze touch or DHCP. The link state machine is tested for five flaps and never invents an address. The cable itself is **BLOCKED** until ETH/LwIP is added in CubeMX.
- [x] Signed G0 current is decoded from offset 20. Host test uses a negative `int32` and checks it is not taken from offset 24.
- [x] Output power is `Vout(mV) * max(Iout(uA), 0) / 1e6` in 64-bit math. 27 V * 5 A = 135000 mW. Negative current is 0 W on the main display and stays signed in diagnostics.
- [ ] Every dynamic string has glyphs, so `?` never appears. Wildcard sets on `HeaderValue`, `Dynamic`, `LabHero`, `LabBig`, `LabState`, and `LabMid` cover the characters the firmware prints. **Not proven on a display.** Generate Code and check the simulator.
- [x] Presets persist through a CRC A/B RAM record and do not restore output ON. The host test corrupts one copy and loads the other. Target NOR programming is not performed.
- [x] Sequencer supports 1 and 12 steps, add/remove of the selected step, skip, slew, once/N/infinite, and stop/abort. Host test covers the cap, skip, and controller timeout.
- [x] The sequencer emits rate-limited SET/ILIM through `G4Ascii` and aborts on readback timeout. Host test.
- [x] Leaving the sequencer screen does not stop it. The sequencer object lives in `psu_app`, and the target tick is the 1 ms default task, not the screen.
- [x] The main snapshot and the web JSON expose the live sequencer setpoint.
- [ ] BMS shows four cells, delta, temperatures, FETs, balancing, and faults. The screen and commands `BMS`, `BMS OFF`, and `CLR` exist. Cell numbers stay hidden until a proven frame arrives. **IMPLEMENTED, HARDWARE VERIFICATION REQUIRED.**
- [x] BMS OFF, configure, and clear fault are real commands with UI text. They are not acknowledged by a BMS frame. **IMPLEMENTED, HARDWARE VERIFICATION REQUIRED.**
- [x] USB AUTO/SINK/SOURCE enqueue `USB AUTO|SINK|SOURCE`. The shown role is the last request, not a partner readback. **IMPLEMENTED, HARDWARE VERIFICATION REQUIRED.**
- [x] PPS stays locked unless `pps_ctl`, Sink, and APDO are all set. Nothing in this repo sets `pps_ctl`, so the product stays locked. **IMPLEMENTED, HARDWARE VERIFICATION REQUIRED.**
- [x] PDO and RDO are not filled with demo numbers. The USB screen says the list is empty.
- [x] The external charger refuses to start without an explicit profile, confirmation, and polarity check. Host test blocks NiMH and requires confirmation.
- [x] On a direct mock sense stream the charger walks PRECHARGE, CC, CV, COMPLETE. A telemetry drop forces FAULT and `output(0)`.
- [x] Voltage and current limits are the same 0–27.000 V and 0–5.000 A clamps in the LCD path, web JSON, sequencer, and charger.
- [x] Diagnostics still prints the decoded G0 fields, and the service screen keeps raw G4 lines. BMS/PD structured fields are absent, not invented.
- [ ] Simulator build. **BLOCKED** in this VM: TouchGFX package and generated sources are not present.
- [ ] Target build and the memory report. **BLOCKED** in this VM: no ARM GCC and no TouchGFX library. The linker script reserves the last 128 KiB of the 128 MiB ROM so a later link can fail if code grows into the config slots.
- [x] Final commit pushed. `cursor/full-product-ui` is on `origin`. Draft PR: https://github.com/kavper/GUI_Lab_PD_PSU/pull/2

## Do not treat as done

G4 return grammar, USART1 pinmux, Ethernet, NOR erase/program, and every LCD pixel still need the lab bench or Designer Generate.
