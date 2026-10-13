# Nowa plytka GUI Lab PD PSU

Branch: `codex/new-board-is25lx-aps256`.
Projekt wykonawczy Designer: `Appli/TouchGFX/GUI_Lab_PD_PSU.touchgfx`.

## Status sprawdzenia

- TouchGFX 4.26.1 `Generate`: PASS (narzedzie tgfx, rzeczywisty projekt wykonawczy).
- Kompilacja simulatora: PASS; uruchomienie simulatora i 14 etapow testu ustawiania V/A: PASS, exit 0.
- Kompilacja Boot, Appli i loadera przez polecenie Compile Target Designera: PASS.
- Istniejace testy protokolu, aplikacji, startupu, automatyzacji i wskaznikow: PASS.
- Test preflight: PASS; poprawne/niepoprawne MCU, brak sondy, blad zapisu i bledny odczyt opcji.
- Test LP8556: PASS; wszystkie awarie transakcji I2C i bledne odczyty kontrolne koncza sie wylaczeniem BL_EN.
- Proba na PCB 2026-10-12: ST-Link wykryl STM32H7RS (Device ID 0x485, Rev B, 3.21 V).
- Preflight ustawil i odczytal kontrolnie wymagane option bytes (XSPI1 HSLV=1, XSPI2 HSLV=0).
- Boot w wewnetrznej Flash zapisany i zweryfikowany przez STM32CubeProgrammer.
- Po poprawce parsera SFDP Flash zostala skasowana i zapisana. Oddzielny odczyt calego kodu (505180 bajtow HEX) i grafiki (3130236 bajtow HEX) potwierdzil zgodnosc bajt po bajcie.
- Run Target / make flash: PASS na PCB. Boot, kod i grafika zapisane i automatycznie zweryfikowane przez -v. Kod i zasoby sa programowane w oddzielnych sesjach, aby ominac blad drugiego segmentu HEX w CubeProgrammer 2.23.
- PSRAM: po poprawce liczby dummy clocks (LC-1) oraz kontrolnym odczycie MR8, test adresow 32 MiB, linii danych, granicy 2 KiB i ostatniej linii przeszedl na PCB. Boot uruchomil Appli.
- BOOT0: uzytkownik usunal R23 do 3.3 V, pozostawil R38=10 kOhm do GND. Po tej poprawce zwykly reset uruchamia program; wczesniej oba rezystory powodowaly start ROM bootloadera.
- Przed wymiana pull-upow LP8556 i GT911 (0x2C, 0x5D, 0x14): NACK. Uzytkownik zmierzyl VDD LP=4.7 V, EN=3.3 V i VLDO=3 V. Test niezaleznego sciagania SDA/SCL nie wykryl zwarcia miedzy liniami. Uzytkownik znalazl blednie zamontowane pull-upy 100 Ohm do 3.3 V zamiast R600/R601=4.7 kOhm; Uzytkownik wymienil pull-upy; LP8556 odpowiada i konfiguracja zostala odczytana kontrolnie.
- Dodano `BoardBacklightDiagnostics` i `BoardBacklightReady`; przy bledzie BL_EN pozostaje niskie, a GUI/UART dalej dzialaja. Po podlaczeniu poprawnego podswietlenia konfiguracja startowa zostala dopasowana do panelu.
- LCD bez podswietlenia: PASS renderowania. LTDC jest aktywny, pozycja skanowania zmienia sie, bufor 0x90000000 zawiera poprawny ekran startowy 800x480 odczytany przez SWD. Tick HAL i RTOS rosna, brak faultow CPU. Po ulotnym ustawieniu boost start=20.2 V, max=25 V oraz jednolitej fazy obu sinkow uzytkownik potwierdzil widoczny obraz na panelu.
- Poprawiono linker loadera: segment kodu/danych miesci sie w DTCM, StorageInfo jest w osobnym segmencie; usunieto nakladanie segmentow i sztuczny zakres 64 MiB. Zapis i automatyczna weryfikacja obu obszarow przeszly na PCB.
- Panel: trzy rownolegle galezie po szesc LED (18 sztuk), wspolne A/K. Uzytkownik potwierdzil polaryzacje i poprosil o 60 mA lacznie. Ulotna konfiguracja 60 mA odczytana kontrolnie, status LP8556=0x30; brak bledow. Prad rzeczywisty wymaga pomiaru miernikiem.
- Widoczny start GUI: POTWIERDZONY przez uzytkownika. Dotyk: NIE POTWIERDZONY; reset obu adresow GT911 bez ACK, skan I2C pokazal 0x2C oraz 0x50. Po podlaczeniu nadal brak ACK GT911. Uzytkownik zglosil urwany przewod i wycofal naprawe dotyku z biezacego zakresu. Trwala poprawka 60 mA: Generate/Compile Target PASS, wgrana i zweryfikowana przez -v.
- Kontrola 2026-10-13 po resecie: LP8556 CURRENT=0xFFF, CURRENT_MAX=30 mA, LED_EN=0x03, jasnosc=0xFF, status=0x30, BoardBacklightReady=1. Nominalnie 60 mA lacznie; rzeczywisty prad nie byl mierzony. LTDC skanuje PSRAM, tick HAL/RTOS rosna, brak faultow CPU. Kod zapisany i zweryfikowany; 3130236 bajtow zasobow HEX nie zmienilo sie. Uzytkownik potwierdzil obraz na panelu.
- Logi rzeczywistych prob: `Board/backlight60_flash.log`, `Board/hardware_flash_split.log`, `Board/hardware_flash_retry.log`, `Board/graphics_readback.log`, `Board/code_readback.log` (lokalne, ignorowane przez Git).

## Polaczenie zegara Flash

Zrzut schematu przeslany przez uzytkownika 2026-10-12 potwierdza:
`OCTOSPI_CLK` jest na PN6 (J14), a PN7 (J13) jest niepodlaczony.
PN6 to `XSPIM_P2_CLK`; polaczenie odpowiada konfiguracji firmware i loadera.
Wczesniejsze wskazanie PN7 jako zegara bylo bledne. Poprawka PCB z tego powodu nie jest wymagana.

Na symbolu Flash w PDF jest opis IS25LX064; zgodnie z poleceniem uzytkownika przyjeto IS25LX256-JHLA3.
Loader sprawdza producenta ISSI i pojemnosc 32 MiB. Uklad o innej pojemnosci nie przejdzie inicjalizacji.

## Konfiguracja

| Element | Ustawienie |
| --- | --- |
| MCU | STM32H7S7L8H6H, TFBGA225 HEXA SMPS, bez zmiany rodziny ukladu |
| HSE | modul oscylatora 24 MHz, BYPASS, PH0 |
| CPU / HCLK / PCLK | 600 / 300 / 150 MHz |
| Flash | IS25LX256, XSPI2 / XSPIM P2, 8 linii, 32 MiB, 3.3 V |
| Flash CLK | PN6 / J14; potwierdzone zrzutem schematu uzytkownika |
| PSRAM | APS256XXN-OB9, XSPI1 / XSPIM P1, 16 linii, 32 MiB, 1.8 V |
| Zegary pamieci | NOR 100 MHz; PSRAM inicjalizacja 100 MHz, praca 200 MHz; PLL2S=200 MHz |
| PSRAM latencja | MR0 read LC=7 (variable), MR4 write WLC=7; XSPI dummy clocks 6 (LC-1); zapis MR: 0 dodatkowych dummy clocks |
| PSRAM MR8 | 0x44 z maska 0x47: X16, hybrid wrap 16 slow = 32 bajty |
| PSRAM granica / refresh | 2 KiB / 600 cykli = 3 us przy 200 MHz; ponizej tCEM=4 us |
| Mapa Flash | 0x70000000: 2 MiB kodu; 0x70200000: 30 MiB zasobow |
| Mapa PSRAM | 0x90000000: 32 MiB |
| LCD | dotychczasowe timingi, 800x480 RGB888, framebuffer RGB565 |
| Dotyk | GT911, I2C1 PB6/PB9, 0x5D; reset PE0, INT PE3; polling z limitami czasu |
| Podswietlenie | LP8556 0x2C, I2C1; EN PG15; PWM PE5 utrzymywany w stanie niskim |
| Prad LED | LED1 i LED2 polaczone rownolegle: 30 mA kazdy, 60 mA LACZNIE, pelna jasnosc |
| Przycisk OUTPUT | PE6, aktywny stan niski |
| LED OUTPUT R/G | PF1/PF2 |
| UART do plyty glownej | UART7 PE7/PE8, dotychczasowy protokol 460800 baud |

LP8556: LED_EN=0x03, CURRENT=0xFFF, CURRENT_MAX=30 mA, ISET_EN=0,
BRT_MODE=I2C, PWM_DIRECT=0, PS_MODE=111 (wspolna faza), FAST=1. Boost: range=16 V, start=20.2 V, adaptacyjny, limit nominalny=25 V. FSET wylaczony, boost 1.25 MHz. Ustawienia sa ulotne; EPROM nie jest wypalany.
Prog UVLO jest ustawiony na 2.5 V dla zasilania 5 V. Kazdy zapis jest odczytywany kontrolnie. BL_EN jest wylaczany po bledzie; GUI moze dalej wystartowac, a stan sterownika jest dostepny przez SWD.

IS25LX256 nie udostepnia w SFDP mapy SCCR wymaganej przez ogolny parser ST.
Adapter `Board/sfdp_issi.c` najpierw odczytuje w SPI VCR[1], sprawdza domyslna wartosc 1F/00,
a po przejsciu w Octal DDR ustawia 16 dummy cycles przy zegarze <=100 MHz. Inne bledy pozostaja bledami.
Adapter obejmuje Boot i loader; oryginalny parser ST jest wlaczany tylko do odczytu.

MR8 jest zapisywany osobnym poleceniem z wlasciwym taktowaniem i odczytywany kontrolnie.
Boot resetuje PSRAM poleceniem Global Reset FF i sprawdza adresy w calych 32 MiB,
linie danych, przejscie przez granice 2 KiB oraz ostatnia linie pamieci przed uruchomieniem GUI.
Niepowodzenie zatrzymuje start; test uruchomi sie dopiero na rzeczywistej plytce.

## Start GUI bez telemetrii

Ekran powitalny przechodzi automatycznie do glownego widoku po okolo 3 sekundach, niezaleznie od swiezosci telemetrii. CONTINUE pozwala przejsc wczesniej. Przejscie jest zglaszane tylko raz, a licznik resetowany przy wejsciu na splash. Brak telemetrii jest nadal pokazywany jako G4 OFFLINE / NO LINK i brak pomiarow.

Kontrola 2026-10-13: Generate, Compile Target i Compile Simulator PASS; istniejacy test GUI 14 etapow PASS. Nowy kod zapisany i zweryfikowany przez -v, zasoby Flash bez zmian. Na PCB odczytano z aktywnego bufora LCD poprawny ekran glowny bez telemetrii. Logi lokalne: Board/startup_generate.log, startup_target.log, startup_simulator.log, startup_simulator_selftest.log, startup_flash.log.

## Panel controls and mounting (2026-10-13)

Voltage encoder: PC6/PC7 AF2, TIM3 hardware quadrature x4; push PE2.
Current encoder: PD12/PD13 AF2, TIM4 hardware quadrature x4; push PD14.
Each detent changes its setpoint; pressing selects independent 0.01 / 0.1 / 1 steps.
Hardware input filters and a 30 ms button debounce reject contact bounce.
Events cross an RTOS queue to the main GUI; no GUI methods run in the input task.
User confirmed both encoders work on the PCB. PA5/TIM2_CH1 gives short 4 kHz,
8% duty piezo feedback; user confirmed the volume is appropriate.

OUTPUT uses PE6, active low. Short release toggles the requested output; a
2-second hold requests output OFF and G4/BMS shutdown once, without a second
short-click event on release. With no G4/BMS this cannot cut external 5 V.
SWD diagnostics confirmed both short clicks and the long hold.

The physical display is rotated 180 degrees with a GPU2D RGB565 copy into
two separate scanout buffers; TouchGFX retains its original double buffers and
animation storage. Rotation runs in the GUI task before VSYNC swapping, not
inside the LCD interrupt. GT911 coordinates are transformed to (799-x, 479-y).
The simulator keeps the logical GUI orientation. No generated source is edited.

Generate, Target, Simulator and host tests passed. Code programming passed
CubeProgrammer verification. SWD showed active rotated scanout, increasing
rotation/tick counters and zero CFSR/HFSR faults; framebuffer inspection confirmed
180-degree rotation. Both original framebuffers previously showed no AGH logo
and LTDC layer 2 was disabled; this excludes a stale logo in those buffers, but
does not establish the physical panel's condition.

Run Target retains ap=1, Under Reset, hardware reset, 1000 kHz and verification
for both code and assets. A software reset follows the final hardware reset so
the application starts with fresh runtime state after the external loader.

Rotation approach: ST employee example at
https://community.st.com/stm32-mcus-touchgfx-and-gui-33/ltdc-rotation-180-degres-156813/index1.html

## Performance (2026-10-13)

The initial 100 MHz memory configuration was a bring-up baseline, not evidence
of failed operation at a higher clock. CPU remains 600 MHz and GPU2D/NeoChrom
plus its cache are enabled. NOR remains 100 MHz (the 3.3 V IS25LX part is rated
to 133 MHz); PSRAM now runs at 200 MHz after checked MR0/MR4 latency writes at
100 MHz. Boot retains the full-capacity alias/data-line tests and additionally
checks sequential and reverse pseudo-random data across 64 KiB of physical rows.

Rotation copies the current/previous dirty area union rather than a complete
frame for every small update, retaining separate double-buffered scanout.
Finger transitions draw the destination once into a dynamic RGB565 bitmap.
All Designer wipe routes, including Splash -> Main, use a cached destination
while preserving their direction and easing. The generated files are unchanged;
FrontendApplication supplies the same entry-point names with handwritten logic.
Rounded corner textures use the same masks/colors, generated once and copied
by the GPU instead of issuing many tiny fill commands every frame. Texture
storage is stable while GPU commands execute; cache exhaustion falls back to
the original renderer. Bitmap storage is 2 MiB with 516 metadata slots.

Hardware full-redraw benchmark improved from about 94 ms to 26 ms in the
tested main view. This is a render-time measurement, not a claim of 60 FPS on
every screen. The user confirmed both finger settling and the initial post-splash
wipe are now smooth. CPU fault registers remained zero. SWD profiling/benchmark
requests default to zero and do not change output or setpoints.

Transitions now pre-rotate outgoing/destination snapshots once, then compose
their visible regions directly into scanout with GPU blits. Wipes copy the
complementary old/new regions, avoiding overdraw. Logical buffer cloning is
skipped only during this direct composition; both logical framebuffers are
explicitly redrawn afterward, including the zero-delta final easing tick.
This fixes the observed flashing after release. User confirmed the stable
idle image. Ten static sample pixels matched both logical buffers and rotated
scanout after the fix, with zero CFSR/HFSR. During measured motion, fast frame
render times were ~10.7 ms at ~16.7 ms intervals (60 FPS); preparation and final
live-view reconstruction still have slower frames. This is not an unconditional
60 FPS full-redraw guarantee.

Animation/scanout/transition storage has capacity for 1024x600 and uses active
Designer width/height for pitch, clipping, rotation and touch coordinates.
Current total EXTRAM use is about 9.7 MiB of 32 MiB. A future 1024x600 panel
still requires its specified pixel clock/porches/sync and GUI layout; 60 FPS on
that physical panel remains to be measured. Current 25 MHz / (821*507) scanout
is ~60.1 Hz. The AliExpress listing did not provide an accessible panel timing
datasheet, so the vendor's nominal refresh was not independently confirmed.
Unchanged ASCII status/measurement text and color no longer invalidate their
widgets at every model tick; changed and UTF-8 text retains the existing update
path. This avoids unnecessary redraws across the GUI.

CubeProgrammer 2.23 rejected incremental download for this device/loader and
fell back to legacy programming. Fast verification also provided no speedup in
the tested external-NOR path. Neither unvalidated mode is retained. At the
required 1000 kHz SWD, the tested ~500 KiB code download took ~23 s and byte
verification ~11 s; full Run Target additionally writes/verifies ~3 MiB assets.
The performance changes themselves kept graphics unchanged. The subsequent UI
refresh changes texts/fonts and is programmed with both code and graphics verification.
Boot at PSRAM 200 MHz passed the enlarged memory test and verified programming.

## Generate / Run Simulator / Run Target

Zachowano polecenia projektu `.touchgfx`; makefile wykonawczy wybiera adaptery z `Board/`.
Generator moze aktualizowac GUI: adaptery nie sa w `generated/`.
Pliki generowane CubeMX sa wlaczane tylko do odczytu dla wspolnych inicjalizatorow;
wejscia Boot/Appli, GPIO, MPU i pamieci wybiera wlasna warstwa plytki.
Plik `.ioc` ma zaktualizowane pojemnosci, rodzaj Flash, HSE, GPIO, MPU i opcje HSLV.
Wlasne ustawienia PSRAM w `Board/board_memory.c` sa konfiguracja wykonawcza.
Regeneracja calego projektu w CubeMX nie byla testowana (CubeMX nie jest zainstalowany).
Wgranie przez Designer nie wymaga recznego konfigurowania pamieci w CubeMX.

Run Target buduje `gcc/IS25LX256_GUI_Lab_PD_PSU.stldr`, sprawdza MCU i odczytuje opcje.
Jesli potrzeba, ustawia tylko XSPI1_HSLV=1, XSPI2_HSLV=0, I2C_NI3C=1, IWDG_HW=1 i VDDIO_HSLV=0,
po czym ponownie odczytuje je kontrolnie. Blad odczytu/zapisu przerywa programowanie.
Loader pochodzi z tego projektu; nie trzeba kopiowac go do instalacji STM32CubeProgrammer.
Pozostaja SWD ap=1, Under Reset, Hardware reset, 1000 kHz i `-v` po kazdym zapisie. Kod i zasoby sa wydzielane z ELF przez objcopy, po czym kazdy obszar jest programowany i weryfikowany w osobnym uruchomieniu CLI. BSS loadera jest zawarte w obrazie do zaladowania, aby programator nie nadpisywal jego zmiennych roboczych.
Po podlaczeniu zasilania oraz ST-Linka trzeba sprawdzic odczyt Flash, zapis i weryfikacje HEX,
a potem start GUI, dotyk oraz zmierzyc prad pojedynczego stringu LED.

## Zrodla

- Potwierdzenie zegara Flash: `C:/Users/kacpe/OneDrive/Desktop/Screenshot 2026-10-12 at 21.33.13.png`.
- Schemat uzytkownika: `C:/Users/kacpe/OneDrive/Desktop/Untitled.pdf`, arkusze 3, 4, 7, 8.
- ISSI: https://www.issi.com/WW/pdf/25LX-WX256-128.pdf
- AP Memory, rev. 1.2: https://www.apmemory.com/tw/downloadFiles/0325021716tb786912
- LP8556, rev. M: https://www.ti.com/lit/ds/symlink/lp8556.pdf
- STM32H7S7, rev. 7: https://www.st.com/resource/en/datasheet/stm32h7s7i8.pdf
- Middleware loadera ST: STM32Cube_FW_H7RS_V1.3.0 / STM32_ExtMem_Loader, zachowana licencja ST.

## Blue theme, grouped menu and sound controls (2026-10-13)

Boot theme is Dark / Blue. Menu columns group Control & automation, Power & USB-C,
and Device & service. USB-C retains independent negotiated limits and actual ADC
readings, adds input-contract utilization, and input session energy/duration/peak.
Session integration uses distinct valid TC input ADC samples, trapezoidal integration,
and skips stale/invalid gaps; fresh detach/reconnect starts a new session. SOURCE
current remains unavailable because BQ input ADC does not measure outgoing current.
The session continues when the USB page is not visible.

Presenter activation can write legacy light-theme colors after setupScreen. Both
cached transitions now synchronize theme colors before capturing the destination,
so main measurements and the OUTPUT action label are visible during the reveal.
Cached wipe duration is 30 ticks (~0.5 s at 60 Hz); finger settling is 20 ticks
(~0.33 s). Finger tracking remains one pixel per pixel.

Appearance has Sound on/mute, volume 0..100, tone 4/2/1 kHz, feedback selection
(touch + controls, touch only, controls only), and a 350 ms test. Default volume
16 maps to the original 8% duty at 4 kHz; maximum volume is 50% duty. Volume is a
relative PWM control, not calibrated acoustic dB. Target requests are
queued through a bounded IRQ critical section (no LDREX in shareable AXI SRAM) to the 1 ms panel task; no GUI delay. Test is bounded to 500 ms by firmware,
and mute stops an active tone. Rotation feedback cannot truncate the longer test.
Sound settings survive page navigation in this boot, but are not saved to NOR;
startup always returns to the blue theme and quiet sound defaults. The simulator
exercises the same GUI settings/callbacks without an electrical piezo output.

Linker allocation after UI refresh: AXI main 242176/442368 B (54.75%), GPU command
pool 24360/24576 B (99.12%, fixed reservation), RTOS DTCM 50000/53248 B (93.90%),
DTCM heap/stack reservation 8192/12288 B (66.67%), PSRAM 9928 KiB/32 MiB (30.30%).
NOR application 518340 B and graphics/text/fonts 3130528 B: total 3.48 MiB/32 MiB
(10.87%). These are linker allocations, not measured runtime CPU/heap utilization.
Host tests cover USB energy sample identity/gaps/reconnect/timer wrap; simulator
covers 32 theme/navigation/sound stages, finger routes/cancellation/editor, USB ADC
validity/SOURCE behavior, battery gauge sign/range and charger session STOP.

Final target validation: Generate and target/simulator builds succeeded, all host
and simulator cases passed. Run Target verified Boot, code and graphics separately.
The new buzzer mailbox initially faulted on LDREX in shareable AXI SRAM; this was
replaced by the bounded IRQ critical section and the corrected code was reprogrammed
with verification. All asset bytes compare identical to the fully programmed image.
AP0 read-only diagnostics and the bounded firmware tone request confirm PA5 AF1,
TIM2 CH1 enabled, PSC=299, ARR=249, requested duty=20, automatic return to zero,
and CFSR/HFSR=0. After restart 10 static points agree in both logical buffers and
rotated scanout. AP1 Under Reset / Hardware reset / 1000 kHz is retained for all
programming; AP0 is only used for runtime diagnostic access. Audible confirmation
requires the user's observation, not a register read.
