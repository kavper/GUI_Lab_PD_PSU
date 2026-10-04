# H7 / G4: protokol binarny i GUI

Projekt wykonawczy: C:/TouchGFXProjects/GUI_Lab_PD_PSU/Appli/TouchGFX/GUI_Lab_PD_PSU.touchgfx
Specyfikacja: https://github.com/kavper/Lab_PD_PSU/blob/f70489a/docs/h7-update.md

UART7 PE7 RX / PE8 TX, 460800 8N1. Ramki A5 5A, CRC16 CCITT-FALSE, transakcje z SEQ i ACK, latest-wins SET, priorytet OFF, niezalezne terminy waznosci METER/BMS/PD/AUX. Transport H7S7 wykorzystuje HPDMA i obsluguje cache oraz odzyskiwanie po bledach UART. Utrata komunikacji nie powoduje automatycznego ON po powrocie danych.

Battery: cztery fizyczne kanaly 1/2/3/5, SOC, licznik ladunku, temperatury i bilansowanie. NET ENERGY jest lokalna, podpisana calka Vpack * Ipack w Wh od startu H7; G4 nie przesyla Wh.
Diagnostics -> SENSE: lokalne i zdalne napiecia, spadki P/N, kod testu, zadanie REMOTE, przelacznik K1, zatrzask bledu, PWM/RPM wentylatora i cztery temperatury NTC. NOT_READY przy niskim napieciu nie potwierdza poprawnego podlaczenia przewodow.
PSU OFF zatrzymuje wyjscie. Battery -> POWER OFF wysyla binarne TEXT_CMD BMS SHUTDOWN: zatrzymanie toru PSU i shutdown BQ76922. Wybudzenie: krotkie nacisniecie TS2.

Walidacja 2026-10-04:
- Generate, kompilacja Simulator i Target: sukces.
- Testy parsera/CRC/fragmentacji/SEQ/ACK/timeout/priorytetu OFF i logiki aplikacji: sukces.
- Widoki symulatora sprawdzone wizualnie.
- Firmware zapisany na STM32H7S78-DK i zweryfikowany po zapisie; wykonano reset sprzetowy.
- Zachowane SWD ap=1, Under Reset, Hardware reset, 1000 kHz i weryfikacja.

Ograniczenie walidacji na sprzecie: UART odbiera bajty i HPDMA pracuje, ale wystepuja bledy framingu i brak poprawnych ramek A5 5A przy 460800. Komunikacja z rzeczywistym G4 nie zostala potwierdzona. Nalezy sprawdzic wersje firmware G4 zgodna ze specyfikacja oraz polaczenie UART.

Zmienione zrodla, konfiguracja Designera, teksty i testy sa zsynchronizowane w tym katalogu. Pliki generowane GUI powstaly przez Generate.
