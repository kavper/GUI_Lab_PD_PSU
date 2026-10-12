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
- Standardowe `-v` w laczonej probie zglosilo blad odczytu przy 0x70200000; procedura Run Target wymaga dalszego sprawdzenia.
- PSRAM: po poprawce liczby dummy clocks (LC-1) oraz kontrolnym odczycie MR8, test adresow 32 MiB, linii danych, granicy 2 KiB i ostatniej linii przeszedl na PCB. Boot uruchomil Appli.
- BOOT0: uzytkownik usunal R23 do 3.3 V, pozostawil R38=10 kOhm do GND. Po tej poprawce zwykly reset uruchamia program; wczesniej oba rezystory powodowaly start ROM bootloadera.
- LP8556: pierwsze adresowanie I2C (0x2C, zapis rejestru 0x00) zakonczylo sie NACK. SDA/SCL sa w stanie wysokim w spoczynku. Uzytkownik potwierdzil zasilanie zewnetrzne 5 V; zasilanie przy samym LP8556 wymaga sprawdzenia.
- Dodano `BoardBacklightDiagnostics` i `BoardBacklightReady`; przy bledzie BL_EN pozostaje niskie, a nowa wersja pozwala GUI/dotykowi/UART wystartowac.
- Poprawiono linker loadera: segment kodu/danych miesci sie w DTCM, StorageInfo jest w osobnym segmencie; usunieto nakladanie segmentow i sztuczny zakres 64 MiB. Weryfikacja programowania nowym loaderem czeka na probe.
- ST-Link zniknal z USB podczas diagnostyki LP8556. Ostatnie zmiany aplikacji/loadera nie zostaly jeszcze zweryfikowane na PCB.
- Widoczny start GUI, dotyk i pomiar pradu LED: NIE POTWIERDZONO.
- Logi rzeczywistych prob: `Board/hardware_flash_retry.log`, `Board/graphics_readback.log`, `Board/code_readback.log` (lokalne, ignorowane przez Git).

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
| Zegary pamieci | 100 MHz na start, PLL2S=200 MHz i dzielnik 2 |
| PSRAM latencja | MR0/MR4 LC=5; XSPI dummy clocks 4 (LC-1); zapis MR: 0 dodatkowych dummy clocks |
| PSRAM MR8 | 0x44 z maska 0x47: X16, hybrid wrap 16 slow = 32 bajty |
| PSRAM granica / refresh | 2 KiB / 300 cykli = 3 us przy 100 MHz; ponizej tCEM=4 us |
| Mapa Flash | 0x70000000: 2 MiB kodu; 0x70200000: 30 MiB zasobow |
| Mapa PSRAM | 0x90000000: 32 MiB |
| LCD | dotychczasowe timingi, 800x480 RGB888, framebuffer RGB565 |
| Dotyk | GT911, I2C1 PB6/PB9, 0x5D; reset PE0, INT PE3; polling z limitami czasu |
| Podswietlenie | LP8556 0x2C, I2C1; EN PG15; PWM PE5 utrzymywany w stanie niskim |
| Prad LED | LED1 i LED2 polaczone rownolegle: 20 mA kazdy, 40 mA LACZNIE, pelna jasnosc |
| Przycisk OUTPUT | PE6, aktywny stan niski |
| LED OUTPUT R/G | PF1/PF2 |
| UART do plyty glownej | UART7 PE7/PE8, dotychczasowy protokol 460800 baud |

LP8556: LED_EN=0x03, CURRENT=0xFFF, CURRENT_MAX=20 mA, ISET_EN=0,
BRT_MODE=I2C, PWM_DIRECT=0, FAST=1. Ustawienia sa ulotne; EPROM nie jest wypalany.
Prog UVLO jest ustawiony na 2.5 V dla zasilania 5 V. Kazdy zapis jest odczytywany kontrolnie. BL_EN jest wylaczany po bledzie; GUI moze dalej wystartowac, a stan sterownika jest dostepny przez SWD.

IS25LX256 nie udostepnia w SFDP mapy SCCR wymaganej przez ogolny parser ST.
Adapter `Board/sfdp_issi.c` najpierw odczytuje w SPI VCR[1], sprawdza domyslna wartosc 1F/00,
a po przejsciu w Octal DDR ustawia 16 dummy cycles przy zegarze <=100 MHz. Inne bledy pozostaja bledami.
Adapter obejmuje Boot i loader; oryginalny parser ST jest wlaczany tylko do odczytu.

MR8 jest zapisywany osobnym poleceniem z wlasciwym taktowaniem i odczytywany kontrolnie.
Boot resetuje PSRAM poleceniem Global Reset FF i sprawdza adresy w calych 32 MiB,
linie danych, przejscie przez granice 2 KiB oraz ostatnia linie pamieci przed uruchomieniem GUI.
Niepowodzenie zatrzymuje start; test uruchomi sie dopiero na rzeczywistej plytce.

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
Pozostaja SWD ap=1, Under Reset, Hardware reset, 1000 kHz i `-v` po zapisie.
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