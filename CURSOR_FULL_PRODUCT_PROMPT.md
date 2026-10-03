# Zadanie: odtworzenie i dokończenie kompletnego produktu Digital PD PSU

## Instrukcja wykonawcza dla agenta

To nie jest prośba o analizę, koncepcję, makietę ani listę sugestii. Masz samodzielnie wykonać pracę w repozytorium. Po krótkim audycie i planie od razu przejdź do implementacji i kontynuuj, dopóki kryteria odbioru nie zostaną spełnione albo nie pojawi się rzeczywisty brak informacji sprzętowej grożący uszkodzeniem urządzenia.

Nie kończ odpowiedzi po napisaniu planu. Nie pozostawiaj widocznych przycisków bez działania, placeholderów, ekranów `coming soon`, pustych handlerów, sztucznej telemetrii na target ani komentarzy `TODO` zamiast implementacji. Mock jest dozwolony wyłącznie w symulatorze i ma być odseparowany od builda targetowego.

Jeżeli zadanie przekracza jeden przebieg kontekstu:

1. prowadź w repozytorium `docs/implementation-status.md` z checklistą i dowodami;
2. przed końcem każdego etapu doprowadź kod do stanu kompilowalnego;
3. wykonaj commit etapu;
4. w następnym przebiegu kontynuuj od checklisty, zamiast zaczynać od nowa;
5. nie ogłaszaj ukończenia, dopóki funkcja nie ma kodu, walidacji, feedbacku UI i testu.

Każde stwierdzenie `działa` musi mieć dowód: wynik testu, wynik kompilacji, screenshot symulatora albo potwierdzoną telemetrię sprzętową. Funkcje zależne od nieobecnego firmware drugiego MCU oznacz `IMPLEMENTED, HARDWARE VERIFICATION REQUIRED`, a nie `DONE`.

Pracujesz w repozytorium:

`C:\TouchGFXProjects\GUI_Lab_PD_PSU`

Docelowy sprzęt to STM32H7S78-DK z ekranem dotykowym 800×480, TouchGFX 4.26.1, zewnętrznym NOR Flash i PSRAM. H7 odpowiada za GUI, Ethernet, koordynację systemu i komunikację z kontrolerami wykonawczymi. Projekt jest pracą inżynierską AGH autorstwa Kacpra Gawła i ma wyglądać oraz działać jak gotowy przyrząd laboratoryjny, a nie demonstrator UI.

Załączone zrzuty przeglądarkowego `Lab PSU Console` są wyłącznie referencją wizualną i funkcjonalną. Nie kopiuj ich bezmyślnie na ekran 800×480. Strona WWW może być gęstym dashboardem, natomiast GUI TouchGFX ma być czytelne z odległości, dotykowe i zoptymalizowane pod mały ekran.

## 0. Najważniejsze zasady bezpieczeństwa pracy z repozytorium

1. Nie używaj `git reset --hard`, `git clean`, `checkout -- .`, nie kasuj plików nieśledzonych i nie regeneruj projektu „od zera”.
2. Najpierw wykonaj audyt: `git status`, historia, wszystkie pliki projektu, wygenerowane źródła TouchGFX i katalog odzyskiwania.
3. Ostatnia działająca wersja binarna i wygenerowane pliki znajdują się również w:
   `C:\TouchGFXProjects\GUI_Lab_PD_PSU_RECOVERY_2026-08-19`
4. Zachowaj obecny stan na osobnej gałęzi ratunkowej. Następnie pracuj na nowej gałęzi `cursor/full-product-ui`.
5. Nie nadpisuj ręcznego kodu przez generator. Elementy wizualne dodawaj do `.touchgfx`, a logikę tylko do klas użytkownika poza `generated/`.
6. Po każdym działającym etapie wykonaj mały, opisowy commit. Po pełnej weryfikacji wypchnij gałąź do `origin`. Nigdy nie zostawiaj całej pracy wyłącznie w working tree.
7. Najpierw odtwórz ostatni działający stan na podstawie katalogu recovery, wygenerowanych klas i istniejącego kodu. Dopiero potem rozwijaj funkcje.
8. Nie udawaj obsługi sprzętu. Jeżeli dla danej funkcji brakuje protokołu lub sterownika, zaprojektuj i zaimplementuj go po obu stronach, jeśli źródła obu MCU są dostępne. Jeżeli nie są dostępne, przygotuj jednoznaczną specyfikację protokołu, adapter i tryb mock/simulator; kontrolka ma być wtedy oznaczona jako niedostępna, a nie wyglądać na działającą.

## 1. Definicja ukończenia

Produkt jest ukończony dopiero wtedy, gdy:

- wszystkie ekrany TouchGFX mają kompletną logikę, a nie teksty poglądowe;
- strona WWW obsługuje te same funkcje i ten sam stan co ekran fizyczny;
- zmiany z LCD, WWW, sekwencera i UART są synchronizowane dwukierunkowo;
- sterowanie sprzętem jest nieblokujące, odporne na rozłączenia i walidowane;
- Ethernet można wielokrotnie odłączać i podłączać bez zamrożenia GUI;
- aplikacja nie pokazuje `?`, śmieci z pamięci ani surowych, nieważnych pomiarów;
- symulator i target kompilują się bez błędów;
- istnieją testy parserów, ograniczeń, sekwencera, synchronizacji i stanów błędów;
- dokumentacja opisuje połączenia, protokoły, mapę pamięci i sposób kompilacji;
- repozytorium zawiera commit i push ostatniego działającego stanu.

## 2. Architektura — jedna prawda dla całego systemu

Zaprojektuj centralny model `PsuSystemState` oraz bezpieczny `CommandBroker`.

Źródła danych:

- telemetria regulatora G0/LDO;
- telemetria przetwornicy/DC-DC i toru mocy;
- BMS BQ76922 dla pakietu 4S;
- kontroler ładowarki/USB-C PD;
- lokalne ustawienia i konfiguracja;
- stan Ethernetu, DHCP i klienta WWW.

Konsumenci:

- TouchGFX;
- serwer WWW/API;
- diagnostyka;
- sekwencer;
- logger i wykresy.

Nie pozwalaj ekranom ani handlerom HTTP wywoływać HAL/UART bezpośrednio. Wszystkie polecenia mają przechodzić przez kolejkę/broker, mieć identyfikator, stan `queued/sent/acknowledged/rejected/timed out`, ograniczenia i wynik. Model publikuje rewizję/generację stanu. GUI i WWW odświeżają tylko dane, które faktycznie się zmieniły.

Zadbaj o atomowe snapshoty między ISR, zadaniami FreeRTOS i TouchGFX. Nie trzymaj długich sekcji krytycznych. Nie używaj dynamicznej alokacji w szybkich pętlach.

## 3. G0/LDO — zachowaj istniejący, działający protokół

Aktualne połączenie H7:

- UART7, 460800 baud, 8N1;
- PE7/PE8, AF7;
- odbiór `ReceiveToIdle_IT`, transmisja nieblokująca;
- ramka: `0xA5 0x5A`, długość, typ, sekwencja, payload, CRC16-CCITT;
- ACK/NACK, timeout 100 ms, maksymalnie 2 ponowienia;
- komendy istniejące: output, limits, get info.

Telemetria G0 ma payload dokładnie 68 bajtów. Nie zmieniaj mapy bez wersjonowania.

Wymagania krytyczne:

- `status_flags`: little-endian, offset 2;
- pomiar prądu: `int32_t`, little-endian, offset 20, jednostka µA;
- pole z offsetu 24 to RAW ADC i nigdy nie może być używane jako prąd;
- pomiar prądu jest ważny tylko, gdy `status_flags & (1U << 4)`;
- capability bit 0 potwierdza dostępność pomiaru prądu;
- ujemny lub nieważny prąd wyjściowy na głównym ekranie pokaż jako `0.000 A`; zachowaj wartość signed w diagnostyce;
- moc wyjściową licz jako `Vout × max(Iout, 0)` z poprawnym skalowaniem jednostek;
- EMA służy tylko do stabilizacji prezentacji, nie do zabezpieczeń i decyzji sterujących;
- po starcie, reconnect lub utracie łącza wyjście musi przejść do bezpiecznego OFF;
- pokaż online/stale/offline, wiek ostatniej ramki, liczniki CRC, ACK, NACK, timeout i reconnect.

Nie naruszaj działającego parsera, chyba że test dowodzi błędu.

## 3A. G4 — rzeczywisty interfejs poleceń ASCII

Drugim, niezależnym interfejsem jest komunikacja GUI/H7 z G4. Nie zastępuj jej protokołem binarnym i nie mieszaj jej z UART7 przeznaczonym dla G0.

Parametry transportu G4:

- USART1;
- 115200 baud;
- 8N1;
- bez hardware flow control;
- komendy ASCII;
- każda komenda kończy się pojedynczym `\n`;
- odbiór musi tolerować zarówno `\n`, jak i `\r\n`;
- TX ma korzystać z jednej serializowanej kolejki — ramki z kilku producentów nie mogą się przeplatać;
- RX ma obsługiwać fragmentację linii, wiele linii w jednym buforze, overflow, nieznany typ oraz reconnect bez restartowania GUI.

Aktualnie obsługiwane komendy G4:

```text
ON
OFF

SET <V>
ILIM <A>

USB AUTO
USB SINK
USB SOURCE

PPS <V> <A>

PERMIT 0
PERMIT 1

REMOTE 0
REMOTE 1

VERBOSE 0
VERBOSE 1

STATUS
BMS
BMS OFF
CLR

TEL 0
TEL 100
TEL 250
TEL 500
TEL 1000
TEL 2000
```

Znaczenie i wymagane mapowanie:

- output enable → `ON\n`;
- output disable → `OFF\n`;
- voltage setpoint → `SET 12.000\n`;
- current limit → `ILIM 2.000\n`;
- USB role → `USB AUTO\n`, `USB SINK\n`, `USB SOURCE\n`;
- PPS request → `PPS 9.00 2.50\n`;
- power path permission → `PERMIT 0/1\n`;
- remote sense → `REMOTE 0/1\n`;
- verbose diagnostics → `VERBOSE 0/1\n`;
- immediate telemetry → `STATUS\n`;
- configure/start BMS → `BMS\n`;
- stop BMS → `BMS OFF\n`;
- clear latched fault → `CLR\n`;
- telemetry period → dokładnie jeden z obsługiwanych wariantów `TEL <ms>\n`;
- service console może dodatkowo wysyłać ręczne `?\n`, `HELP\n` i inne jawnie wpisane komendy.

Po pierwszym poprawnym uruchomieniu interfejsu lub ponownym wykryciu G4 wyślij:

```text
TEL 500
STATUS
```

Nie wysyłaj `TEL 500` w nieskończonej pętli. Śledź stan inicjalizacji sesji i ponów go dopiero po rzeczywistym reconnect/timeout.

### Warstwa komend G4

Utwórz osobny sterownik, np. `G4AsciiProtocol`, z następującymi cechami:

- statyczna kolejka TX bez `malloc`;
- priorytet dla `OFF`, fault clear i poleceń bezpieczeństwa;
- koaleskowanie SET/ILIM: jeżeli kilka nowych nastaw czeka w kolejce, wyślij tylko najnowszą;
- limit aktualizacji nastaw z suwaka i sekwencera: maksymalnie jedna para SET/ILIM mniej więcej co 120 ms;
- zawsze używaj kropki dziesiętnej i kontrolowanego formatowania — 3 miejsca dla SET/ILIM, 2 lub 3 dla PPS zgodnie z wymaganiem firmware;
- walidacja długości i zakresu przed `snprintf`;
- stan polecenia oraz timestamp wysłania;
- jeżeli firmware nie ma jawnego ACK, potwierdzeniem jest odpowiadający readback w następnej poprawnej telemetrii;
- rozróżniaj `requested`, `sent`, `reported/applied`;
- po timeout pokaż błąd i nie udawaj, że zmiana została zastosowana;
- `OFF` ma być możliwe nawet przy pełnej kolejce — zarezerwuj miejsce lub zastosuj kolejkę priorytetową.

### Parser odpowiedzi G4

Najpierw odczytaj rzeczywisty kod firmware G4 lub aktualną specyfikację ramek zwrotnych. Nie zgaduj układu telemetrii na podstawie samego wyglądu WWW. Zaimplementuj parser każdego istniejącego typu linii, w tym `TC`, list PDO, BMS, charger, faults i logów verbose. Zachowaj również raw line w diagnostyce.

Parser powinien:

- aktualizować jeden atomowy snapshot G4;
- oznaczać osobno ważność i wiek każdej grupy danych;
- dekodować wszystkie flagi na nazwy, ale zachowywać raw fields;
- ignorować niekompletną/uszkodzoną linię bez utraty synchronizacji kolejnych linii;
- mieć ograniczenie długości linii i licznik overflow/parse errors;
- traktować pole `pps_ctl=1` w ramce `TC` jako capability sterowania PPS;
- odblokować PPS tylko gdy `pps_ctl=1`, port jest w roli Sink oraz wybrane PDO jest PPS/APDO;
- publikować listę PDO jako osobny snapshot z generacją;
- nie odświeżać kontrolek z logów tekstowych, jeżeli istnieje dedykowane pole telemetrii.

### Konsola serwisowa

Konsola jest osobnym ekranem diagnostycznym, nie głównym sposobem sterowania. Powinna mieć historię RX/TX, timestampy, filtr telemetry/log/command/error, pause/autoscroll, clear oraz pole ręcznej komendy. Ręczne komendy mają działać tylko w wyraźnie oznaczonym `SERVICE MODE`; pokaż ostrzeżenie, że omijają część wysokopoziomowych zabezpieczeń GUI.

## 4. Fizyczne GUI TouchGFX — wspólny design system

Cały interfejs urządzenia ma być po angielsku.

Wymagania wizualne:

- rozdzielczość 800×480;
- jeden wspólny ciemny motyw instrumentu laboratoryjnego;
- na każdym ekranie identyczny górny pasek o wysokości 64 px i identyczna linia podziału — animacje między ekranami nie mogą powodować skoku tła;
- wspólne kolory: cyan dla wartości/akcji, green dla OK/active, amber dla CC/warning, red wyłącznie dla fault/danger/output active, neutral dark dla nawigacji;
- minimalny aktywny obszar dotyku około 44×44 px;
- żadnych nachodzących przycisków, przypadkowych marginesów i tekstów uciętych;
- wszystkie elementy mają być widoczne i edytowalne w TouchGFX Designerze;
- runtime może tworzyć listy/wykresy, ale nie może zastępować całej kompozycji ekranu ręcznym kodem;
- zapewnij wszystkie znaki wildcard wymagane przez dynamiczne teksty, aby nigdy nie pojawiały się `?`.

### Splash screen

- czas około 2 s, bez blokującego delay;
- logo Digital PD PSU;
- wyraźne oficjalne logo AGH, nie mikroskopijne;
- `Designed by Kacper Gaweł`;
- firmware version, build identifier i release date pobierane z jednego pliku wersji;
- profesjonalny komunikat typu `STARTING POWER SYSTEM` i pasek postępu;
- tło może mieć delikatny motyw ścieżek PCB, bez sztucznych plam.

### Main screen

- duże `MEASURED VOLTAGE` i `MEASURED CURRENT`;
- obok wyraźne, dotykowe kafle `VOLTAGE SET` i `CURRENT LIMIT`;
- kliknięcie całego kafla wybiera edycję;
- pierwszy klawisz zastępuje poprzednią wartość, a nie dopisuje do niej;
- aktywny kafel ma mocne, trwałe zaznaczenie;
- klawiatura numeryczna, `CLR`, `DEL`, `.`, `APPLY`;
- zakres napięcia 0–27.000 V, prądu 0–5.000 A;
- poniżej 10 V wyświetlaj 3 miejsca po przecinku, od 10 V 2 miejsca, jeśli wymaga tego szerokość;
- pokaż limity pod nastawami;
- nagłówek: CV/CC, VIN, MOS temperature, PCB temperature, output power, stan łącza;
- CV ma cyan, CC amber, oba ze spójnym zaokrągleniem;
- OUTPUT na LCD jest wskaźnikiem stanu, nie mylącym przyciskiem. Fizyczne przełączenie wyjścia realizuje USER BUTTON 1; LD1 ma odzwierciedlać faktyczny stan potwierdzony przez kontroler;
- presety 5 V/1 A, 12 V/2 A, 20 V/3 A z trwałym zaznaczeniem wybranego;
- swipe góra/dół na aktywnym kaflu może precyzyjnie zwiększać/zmniejszać wartość z akceleracją i ograniczeniami.

### Control Center

Zbuduj prawdziwe menu prowadzące do: Presets, Sequencer, Measurements, BMS, USB-C PD, Protection Limits, Diagnostics, Network/System. Żaden ekran nie może być pustym placeholderem.

Informacje energetyczne podziel logicznie, zamiast wrzucać wszystko do jednej przewijanej strony:

- `BATTERY PACK` — stan wewnętrznego pakietu: V/I/P, SOC/SOH, capacity, coulomb counter, cycles, temperatures;
- `BMS / CELLS` — C1–C4, delta, balancing, CHG/DSG FET, protections, alerts, configuration;
- `CHARGER & POWER PATH` — ładowarka urządzenia, battery/USB/system paths, requested/applied currents, permit, remote sense i faults;
- `USB-C PD` — role, attach, PDO/RDO, PPS/APDO, contract i cable data;
- `EXTERNAL BATTERY CHARGER` — osobna maszyna stanów bezpiecznego ładowania baterii podłączonej do wyjścia.

Ekrany mogą mieć podzakładki, ale użytkownik zawsze ma wiedzieć, czy ogląda wewnętrzny pakiet urządzenia, kontroler BMS, ładowarkę wejściową czy zewnętrzną ładowaną baterię.

### Presets

- co najmniej 3 szybkie presety, architektura gotowa na większą liczbę;
- nazwa, napięcie, prąd, opcjonalny slew i zachowanie output;
- edycja tą samą klawiaturą numeryczną;
- load/apply, save, duplicate, reset to default;
- trwały zapis z numerem wersji, CRC32 i dwiema kopiami/recovery;
- nigdy nie zapisuj stanu OUTPUT ON jako automatycznie odtwarzanego po starcie.

### Sequencer

- od 1 do 12 kroków, domyślnie 1;
- płynnie przewijana lista, bez stronicowania po 4 elementy;
- wybrany krok pozostaje zaznaczony;
- edytor 2×2: Voltage, Current, Time, Slew Rate;
- klawiatura numeryczna i przechodzenie PREV/NEXT bez wychodzenia;
- `+ STEP` dodaje/kopiuje krok, `- STEP` usuwa aktualny krok i przesuwa kolejne;
- każdy krok: 0–27 V, 0–5 A, 0.1–3600 s, slew 0 = immediate albo 0.001–100 V/s, enabled/skipped;
- tryb once, N loops i infinite;
- run, pause/resume, stop i abort;
- live progress: aktualny krok, czas kroku, czas całej sekwencji, pętla, żądane i potwierdzone wartości;
- podczas pracy główny ekran i WWW muszą pokazywać wartości narzucone przez sekwencer;
- edycja konfiguracji podczas pracy ma być zablokowana lub jawnie odroczona;
- po wyjściu z ekranu sekwencer nadal działa poprawnie.

Sekwencer ma sterować rzeczywistym G4 przez kolejkę `G4AsciiProtocol`, a nie tylko zmieniać liczby w modelu:

- przy przejściu do kroku wyślij docelowy `ILIM`, a następnie `SET` zgodnie z bezpieczną kolejnością ustaloną dla toru mocy;
- dla slew rampę wylicza H7 w stałym kroku czasu, ale wysyła skonsolidowane `SET` maksymalnie co około 120 ms;
- nie kumuluj setek pośrednich SET w kolejce — zawsze zastępuj niewysłaną wartość najnowszą;
- krok może opcjonalnie mieć zachowanie output `KEEP`, `ON` albo `OFF`; domyślnie `KEEP`;
- start sekwencji nie może samoczynnie włączać wyjścia, jeśli użytkownik tego jawnie nie skonfigurował i nie są spełnione warunki safety/permit;
- czas kroku licz monotonicznym zegarem, niezależnie od liczby klatek GUI;
- przejście kroku może wymagać potwierdzenia readback nastaw; timeout powoduje `ABORTED / CONTROLLER TIMEOUT`;
- po stop/abort wykonaj skonfigurowaną bezpieczną akcję: HOLD, RETURN TO START albo OUTPUT OFF, przy czym domyślna to OUTPUT OFF;
- wszystkie polecenia i potwierdzenia pokazuj w live progress oraz diagnostyce;
- WWW i LCD używają tej samej instancji sekwencera, a nie dwóch schedulerów.

### Measurements

- bieżące: VIN, VOUT, IOUT, input/output power, energia Wh/Ah, uptime;
- MOS/PCB/ambient oraz wszystkie temperatury dostępne z G0/G4/BMS;
- wykres z buforem pierścieniowym, wybór kanału i zakresu czasu;
- min/max/average;
- wskaźnik jakości danych: valid/stale/offline;
- odświeżaj liczby płynnie, ale nie przerysowuj całego ekranu przy każdej ramce.

## 5. BMS BQ76922 — pełna obsługa pakietu 4S

Zaimplementuj prawdziwy ekran BMS i warstwę danych/komend. Co najmniej:

- napięcia C1–C4, min/max/delta/sum;
- pack voltage, stack voltage, pack current signed, power;
- SOC, SOH, remaining/full capacity, coulomb counter, cycles;
- temperatury wszystkich dostępnych czujników;
- stan CHG/DSG FET, balancing per cell, charger state;
- alarmy i błędy z dekodowaniem bitów na czytelne nazwy;
- status konfiguracji i ostatniego polecenia;
- ręczne `Refresh`, `Configure BMS`, `BMS OFF`, `Clear fault` z potwierdzeniem dla operacji ryzykownych;
- ustawienia telemetrii;
- osobna strona ustawień ochrony: OV/UV, OCD/SCD, charge/discharge overcurrent, temperature limits i delays — wyłącznie w zakresach dozwolonych przez BQ76922 i projekt hardware;
- ręczne balansowanie tylko w bezpiecznym trybie serwisowym z timeoutem;
- brak komunikacji nie może wyświetlać starych danych jako aktualnych.

Jeżeli kontroler BMS jest po stronie innego MCU, zdefiniuj wersjonowany protokół UART obejmujący snapshot, capabilities, config readback, commands, ACK/NACK i fault/event frame. GUI ma pokazywać wartości odczytane/potwierdzone, nie tylko ostatnio wysłane.

## 6. USB-C Power Delivery i ładowarka

Zaimplementuj ekran i protokół dla pełnego toru USB-C PD:

- attach/detach, orientation, rola portu, PD revision i stan maszyny PD;
- tryby `AUTO/DRP`, `SINK ONLY`, `SOURCE ONLY`;
- lista PDO partnera z typem, zakresem V, maksymalnym I/P i flagami;
- wybrane RDO: numer PDO, requested/operating/max current, contract V/I/P;
- obsługa fixed PDO oraz PPS/APDO, jeśli capabilities to potwierdzają;
- dla PPS: napięcie/prąd, zakresy z APDO, walidacja kroku i przycisk request/apply;
- cable/partner information, błędy negocjacji, hard/soft reset tylko jako jawna akcja serwisowa;
- charger telemetry: VBUS, input current, charge current, battery voltage, charger status i faults;
- tryb source ma być blokowany, gdy stan baterii/BMS/temperatury nie pozwala bezpiecznie źródłować energii;
- wszystkie polecenia wymagają ACK/readback, a UI pokazuje `requested`, `applied`, `contract` osobno.

Nie zakładaj z góry liczby PDO. Użyj listy o rozmiarze ograniczonym przez protokół i możliwości pamięci.

Sterowanie tego ekranu ma korzystać dokładnie z komend G4 `USB ...` i `PPS ...`. Nie wysyłaj PPS z suwaka częściej niż około 120 ms i zawsze koaleskuj oczekujące żądania. Wartości dostępne na suwaku mają wynikać z aktualnie wybranego APDO, a nie ze stałych wpisanych w GUI.

## 6A. Dedykowany tryb ładowania zewnętrznego akumulatora

Dodaj osobny, pełny ekran `BATTERY CHARGER` przeznaczony do kontrolowanego ładowania akumulatora podłączonego do wyjścia zasilacza. To nie jest ten sam ekran co wewnętrzny BMS pakietu urządzenia.

Ta funkcja jest potencjalnie niebezpieczna. Nie wolno automatycznie wykrywać chemii ani liczby ogniw wyłącznie na podstawie napięcia. Użytkownik musi jawnie wybrać profil, liczbę ogniw i potwierdzić parametry. Ładowanie ma zostać zablokowane, jeżeli brakuje wymaganych pomiarów, power permit, poprawnej komunikacji albo występuje fault.

### Obsługiwane profile

Przygotuj wersjonowaną tabelę profili z możliwością dodawania kolejnych:

- Li-ion / LiPo — precharge, CC, CV, termination;
- Li-ion HV — tylko po świadomym wyborze odpowiedniego napięcia końcowego;
- LiFePO4 — precharge, CC, CV z właściwym napięciem na ogniwo;
- LTO — profil z odpowiednim napięciem końcowym;
- Lead-acid — bulk, absorption, opcjonalny float;
- NiMH/NiCd — wyłącznie jeżeli sprzęt dostarcza wystarczająco szybki i dokładny pomiar napięcia oraz temperatury do -ΔV/dT/dt; w przeciwnym razie profil pokaż jako unsupported i nie pozwalaj go uruchomić;
- Custom laboratory profile — z ostrzeżeniem i pełną ręczną konfiguracją.

Wartości domyślne mają być konserwatywne, widoczne i edytowalne w bezpiecznych zakresach. Nie traktuj wartości przykładowych jako uniwersalnie bezpiecznych dla każdego ogniwa.

### Parametry sesji ładowania

- chemistry/profile;
- series cell count;
- nominal capacity w Ah;
- maksymalny C-rate oraz wynikowy current limit;
- precharge threshold na ogniwo;
- precharge current;
- CC current;
- CV target per cell i automatycznie wyliczone napięcie pakietu;
- termination current w A oraz jako C-rate;
- maksymalny czas precharge, CC, CV i całej sesji;
- opcjonalny float voltage dla lead-acid;
- minimalna/maksymalna temperatura, jeżeli pomiar temperatury zewnętrznego pakietu jest dostępny;
- maksymalna dostarczona pojemność Ah i energia Wh;
- remote sense on/off tylko po potwierdzeniu poprawnego podłączenia przewodów sense;
- zachowanie po zakończeniu: zawsze bezpieczne OUTPUT OFF, chyba że profil lead-acid jawnie przewiduje float.

### Maszyna stanów

Zaimplementuj deterministyczną maszynę stanów działającą poza warstwą widoku:

```text
IDLE
VALIDATE
WAIT_FOR_CONNECTION
PRECHARGE
CONSTANT_CURRENT
CONSTANT_VOLTAGE
ABSORPTION
FLOAT
TERMINATING
COMPLETE
PAUSED
ABORTED
FAULT
```

Każdy stan ma mieć jawne warunki wejścia, wyjścia, timeout i akcję awaryjną. Algorytm korzysta z `SET`, `ILIM`, `ON`, `OFF`, `PERMIT` i opcjonalnie `REMOTE`, ale wyłącznie przez centralny broker komend.

Przykładowo dla chemii litowej:

1. VALIDATE — sprawdź profil, liczbę ogniw, napięcie pakietu, komunikację, temperaturę i limity hardware;
2. PRECHARGE — niski prąd do osiągnięcia progu; timeout = fault;
3. CC — zadany prąd, obserwacja napięcia i temperatury;
4. CV — utrzymanie napięcia końcowego, obserwacja spadku prądu;
5. TERMINATING — prąd poniżej progu przez określony czas, nie pojedyncza próbka;
6. COMPLETE — `OFF`, zapis wyniku sesji.

Nie przechodź do kolejnego stanu na podstawie jednej zaszumionej próbki. Stosuj hysteresis i warunek utrzymany przez określony czas. Surowe limity bezpieczeństwa sprawdzaj niezależnie od filtracji prezentacyjnej.

### Ekran ładowarki

Podziel go na logiczne widoki:

- `PROFILE` — chemia, liczba ogniw, pojemność;
- `LIMITS` — CC, CV/cell, cutoff current, temperature i timeouty;
- `READY CHECK` — lista warunków z zielonym/czerwonym statusem;
- `CHARGING` — duży stan PRECHARGE/CC/CV/FLOAT, V/I/P, target, czas, Ah, Wh, temperatura i wykres;
- `RESULT` — powód zakończenia, czas, dostarczone Ah/Wh, min/max temperature i faults.

Przed startem pokaż czytelne podsumowanie: typ chemii, S count, końcowe napięcie całego pakietu, maksymalny prąd, cutoff i timeout. Wymagaj świadomego potwierdzenia. Nigdy nie wznawiaj sesji automatycznie po resecie, utracie UART, utracie zasilania ani reconnect.

### Walidacja i ograniczenia ładowarki

- wyliczone napięcie pakietu nie może przekraczać możliwości 27 V ani limitu zgłoszonego przez kontroler;
- prąd nie może przekraczać 5 A, limitu hardware, limitu profilu ani `capacity × C-rate`;
- przy utracie telemetrii, G4, power permit, remote sense lub przekroczeniu limitu natychmiast przejdź do FAULT i wyślij priorytetowe `OFF`;
- napięcie spoczynkowe nie jest wystarczającym dowodem poprawnej liczby ogniw;
- wykrycie odwrotnej polaryzacji musi pochodzić z hardware/capability; jeśli hardware jej nie wykrywa, pokaż obowiązkową kontrolę ręczną;
- brak zewnętrznego czujnika temperatury musi być jawnie widoczny i może ograniczać dostępne profile;
- wewnętrzny BMS urządzenia i zewnętrzna ładowana bateria to dwa oddzielne obiekty w modelu.

### Historia ładowania

Zapisuj niewielki, cykliczny dziennik sesji: timestamp/uptime, profil, S count, ustawienia, czas, Ah, Wh, rezultat i fault. Nie zapisuj próbek wykresu co 100 ms do Flash. Umożliw eksport historii przez WWW.

## 7. Protection Limits i ścieżka mocy

- globalne limity napięcia, prądu, mocy i temperatury;
- OVP, OCP, OTP, UVLO, reverse current oraz limity zależne od capabilities;
- stan toru: battery → system, USB → system, DC/DC, G0/LDO, output;
- requested/applied/actual dla każdego stopnia;
- czytelny fault latch i historia ostatnich zdarzeń;
- fault zawsze wymusza bezpieczny OFF i wymaga świadomego clear;
- brak telemetrii lub timeout nie może pozostawić UI w stanie fałszywego `ON`.

## 8. Diagnostics

Ekran diagnostyczny ma wyświetlać wszystkie pola otrzymywane od G0/G4/BMS/PD, ale pogrupowane i czytelne:

- link state, protocol/telemetry version, period i age;
- RX bytes, valid frames, CRC errors, TX, ACK, NACK, timeouts, retries, last NACK;
- status_flags i fault_flags z dekodowaniem;
- raw oraz filtered measurements;
- requested/applied limits, DAC readback, preregulator, startup/mode;
- wszystkie temperatury, capabilities i maksymalne limity;
- BMS raw status/faults, PD state, selected PDO/RDO;
- ekran raw UART/log z filtrowaniem i możliwością wyczyszczenia;
- licznik błędów i timestamp ostatniej poprawnej ramki.

Diagnostyka nie może blokować parsera ani GUI.

## 9. Ethernet i strona WWW

Ethernet/LwIP musi działać w osobnym zadaniu i nigdy nie blokować TouchGFX.

- obsłuż link up/down, wielokrotne odpięcie/podpięcie kabla, DHCP renew/rebind i timeout;
- pokaż na GUI `LINK DOWN`, `DHCP`, przydzielony IP, maskę, gateway i MAC;
- opcjonalny statyczny fallback po timeout, ale nigdy fałszywy adres typu `1.50`;
- nie używaj blokujących pętli ani długich callbacków w LwIP raw API;
- pliki WWW trzymaj jako skompresowane zasoby w Flash, nie kopiuj całej strony do RAM;
- odpowiedzi HTTP wysyłaj porcjami zgodnie z oknem TCP; obsłuż partial send i close;
- strona ma działać na Safari/Chrome/Edge i być responsywna.

WWW ma wizualnie nawiązywać do załączonego zielonego `Lab PSU Console`: profesjonalne cards, czytelne statusy, wykresy i sekcje, ale bez kopiowania błędnych danych demo.

Wymagane sekcje WWW:

- Overview/power path;
- Output control i setpoints;
- Presets;
- Sequencer z pełną tabelą i live progress;
- Measurements i wykresy;
- BMS 4S;
- USB-C PD/PDO/RDO/PPS;
- Protections/faults;
- Diagnostics/raw telemetry;
- System/network/firmware.

API ma używać tych samych limitów i formatowania co LCD. Zmiana z WWW ma natychmiast zwiększyć rewizję modelu i pojawić się na LCD; zmiana na LCD/sekwencerze ma pojawić się w WWW. Nie twórz osobnej kopii stanu w JavaScript jako źródła prawdy.

Preferuj lekkie odpytywanie z numerem rewizji lub SSE tylko wtedy, gdy implementacja jest stabilna. Zapewnij odpowiedzi JSON z jednoznacznymi jednostkami (`*_mv`, `*_ma`, `*_ua`, `*_centi_c`) i wynikiem komendy. Waliduj każdy parametr również po stronie firmware.

Dodaj control lease/remote control status, aby dwóch klientów nie wysyłało sprzecznych poleceń. Operacje ryzykowne wymagają potwierdzenia. Po restarcie output zawsze OFF.

## 10. Trwała konfiguracja

Zapisuj presety, sekwencje, limity, ustawienia BMS/PD oraz ustawienia GUI w wydzielonym, udokumentowanym obszarze Flash.

- nie zgaduj adresu — wyznacz go na podstawie linker script i faktycznej mapy zewnętrznego NOR;
- nie nakładaj konfiguracji na assety TouchGFX;
- schema version, length, CRC32, generation counter;
- dwie kopie A/B i bezpieczny commit;
- wear limiting/debounce;
- migracja lub powrót do defaults po niezgodnej wersji;
- nigdy nie przywracaj automatycznie OUTPUT ON.

## 11. Wydajność i bezpieczeństwo

- TouchGFX ma pozostać responsywny przy UART i Ethernet jednocześnie;
- żadnego `HAL_Delay` w GUI/network/runtime;
- parsowanie UART w małych porcjach, ISR tylko przekazuje dane;
- filtrowanie prezentacji w fixed-point, jeśli to rozsądne;
- brak przepełnień podczas mnożenia mV × µA;
- clamp i walidacja przed konwersją typów;
- watchdog i liczniki health dla głównych zadań;
- stale timeout dla każdego źródła telemetrii;
- bezpieczne zachowanie po CRC burst, timeout, reconnect i resecie kontrolera;
- nie zmieniaj konfiguracji cache/MPU/XSPI/framebuffer bez dowodu i testu, ponieważ wcześniej powodowało to pasy i korupcję obrazu.

## 12. Testy i weryfikacja

Dodaj testy host/simulator dla:

- parsera ramek, resynchronizacji po śmieciach, CRC, ACK/NACK i timeoutów;
- dokładnego dekodowania 68-bajtowej telemetrii, szczególnie signed current z offsetu 20;
- obliczeń mocy i formatowania;
- limitów GUI/WWW;
- sekwencera: add/remove, 1/12 kroków, skip, slew, once/N/infinite, stop/abort;
- dwukierunkowej synchronizacji LCD ↔ model ↔ WWW;
- BMS/PD capabilities i niedostępnych funkcji;
- Ethernet link flap i DHCP timeout;
- persistence CRC i recovery A/B.

Dodaj rozbudowany mock sprzętu w symulatorze: prawidłowe dane, stale link, CRC errors, NACK, overtemperature, BMS fault, PD attach/detach i zmienne PDO.

Wykonaj i zapisz wyniki:

1. TouchGFX generate;
2. build simulatora;
3. build targetu;
4. raport wykorzystania FLASH/RAM/NETRAM/RAM_CMD/EXTRAM;
5. testy jednostkowe;
6. statyczna analiza najważniejszych parserów i callbacków;
7. manualna checklista każdego ekranu.

Nie flashuj płytki automatycznie bez zgody użytkownika. Przygotuj gotowy `target.hex` i podaj jego SHA-256.

### Twarda checklista odbioru

Przed uznaniem zadania za zakończone przejdź każdy punkt i wpisz dowód do `docs/implementation-status.md`:

- [ ] cold boot zawsze zaczyna z output OFF;
- [ ] USER BUTTON 1 przełącza output i LD1 pokazuje stan potwierdzony, nie tylko żądany;
- [ ] SET/ILIM z LCD dochodzi do G4 i readback wraca na LCD;
- [ ] SET/ILIM z WWW dochodzi do G4 i pojawia się na LCD;
- [ ] zmiana na LCD pojawia się w WWW;
- [ ] rozłączenie G0/G4 oznacza stale/offline i nie zamraża GUI;
- [ ] pięć kolejnych odłączeń/podłączeń Ethernetu nie zamraża dotyku ani DHCP;
- [ ] parser poprawnie dekoduje signed current G0 z offsetu 20;
- [ ] output power ma poprawne jednostki i nie przepełnia obliczeń;
- [ ] wszystkie dynamiczne teksty mają glyphy i nigdzie nie ma `?` zamiast znaków;
- [ ] presety zapisują się, odtwarzają po restarcie i nie przywracają output ON;
- [ ] sekwencer obsługuje 1 i 12 kroków, add/remove current step, scroll, skip, slew, once/N/infinite;
- [ ] sekwencer faktycznie emituje rate-limited SET/ILIM do G4 i reaguje na timeout;
- [ ] wyjście z ekranu sekwencera nie zatrzymuje wykonania;
- [ ] ekran główny i WWW pokazują live setpoint sekwencera;
- [ ] BMS pokazuje wszystkie cztery ogniwa, delta, temperatury, FET, balancing i faults;
- [ ] BMS OFF, configure i clear fault mają potwierdzenie oraz feedback;
- [ ] USB AUTO/SINK/SOURCE wysyła poprawną komendę i pokazuje stan odczytany;
- [ ] PPS pozostaje zablokowane bez `pps_ctl=1`, Sink i APDO;
- [ ] lista PDO i wybrane RDO pochodzą z telemetrii, nie z danych demonstracyjnych;
- [ ] external battery charger nie startuje bez jawnego profilu, S count i potwierdzenia;
- [ ] charger przechodzi poprawnie PRECHARGE → CC → CV → COMPLETE na mocku;
- [ ] utrata telemetrii w każdym stanie chargera powoduje priorytetowe OFF i FAULT;
- [ ] limity napięcia/prądu są identyczne w LCD, WWW, sekwencerze i chargerze;
- [ ] Diagnostics pokazuje raw i decoded dane wszystkich kontrolerów;
- [ ] simulator build przechodzi;
- [ ] target build przechodzi i raport pamięci nie ma overflow;
- [ ] finalny commit jest wypchnięty na zdalną gałąź.

## 13. Dokumentacja i wynik końcowy

Utwórz lub uzupełnij:

- `README.md` — budowanie, generowanie, uruchomienie i flashowanie;
- `docs/architecture.md` — zadania, model danych i przepływ komend;
- `docs/uart-protocols.md` — G0 oraz nowy protokół G4/BMS/PD z pełnymi tabelami ramek;
- `docs/memory-map.md` — internal Flash, external NOR, PSRAM, assets i konfiguracja;
- `docs/web-api.md` — endpointy, pola, jednostki, limity i przykłady;
- `docs/test-plan.md` — testy i wyniki;
- `docs/pinout.md` — UART7 PE7/PE8, USER BUTTON 1, LD1 i pozostałe użyte sygnały.

Na końcu przedstaw:

- listę zaimplementowanych funkcji;
- listę funkcji rzeczywiście potwierdzonych na sprzęcie;
- listę funkcji wymagających firmware drugiego MCU lub testu hardware;
- wyniki buildów i testów;
- hash firmware;
- branch, commity i URL push/PR;
- znane ograniczenia bez ich ukrywania.

## 14. Sposób pracy

Nie kończ na analizie ani makiecie. Najpierw przygotuj plan etapów i audyt istniejącego kodu, a następnie realizuj kolejne etapy aż do działającego produktu. Nie pytaj o drobne decyzje wizualne, jeśli można zastosować powyższy spójny design system. Pytaj tylko wtedy, gdy brakuje informacji elektrycznej/protokołu, której błędne założenie mogłoby uszkodzić sprzęt.

Priorytety:

1. odzyskanie i zabezpieczenie ostatniej działającej wersji;
2. stabilność UART/G0, modelu danych i output safety;
3. brak zamrażania TouchGFX przez Ethernet;
4. spójność LCD/WWW;
5. sekwencer i presety;
6. BMS oraz USB-C PD;
7. pomiary, wykresy, diagnostyka i persistence;
8. finalne dopracowanie grafiki, dokumentacja, testy, commit i push.

To ma być finalny firmware przyrządu laboratoryjnego, nie demonstracja. Każdy widoczny przycisk ma mieć działanie, walidację, feedback i potwierdzony stan urządzenia.
