# Osobny ekran Appearance

Kafelek Appearance w ScreenSettings otwiera ScreenAppearance. LIGHT/DARK i cztery akcenty przeniesione z menu głównego ustawień. MENU wraca do ScreenSettings. Definicja ekranu i interakcji jest zapisana w pliku .touchgfx; logika znajduje się w gui/include/gui/screenappearance_screen i gui/src/screenappearance_screen. Nie edytowano ręcznie plików generowanych.

Domyślnie aplikacja startuje z Dark / Amber. Karta klawiatury na ekranie głównym zajmuje Y=80..464. Klawisze mają wysokość 64 px. Presety mają 154x60 px i większą czcionkę, równo wypełniają szerokość lewej części. Nagłówek klawiatury zawiera edytowany parametr i zakres; wynik wysłania i ostrzeżenia zastępują nagłówek na 180 klatek. Test symulatora sprawdza domyślne kolory i komunikat po APPLY; test gestów potwierdza wpisanie cyfry 7 przez zmieniony układ klawiszy.

Test rzeczywistych próbek dotyku w symulatorze sprawdza otwarcie kafelka, oba tryby i cztery akcenty, powrót oraz siedem pozostałych stron ustawień bez zmiany parametrów PSU. Test gestów nadal przechodzi. Logi: appearance-test.log, appearance-finger.log. Zajętość: appearance-memory.json i appearance-memory.txt.

Audyt akceleracji (bez zmiany konfiguracji sprzętowej):
- main.c uruchamia MX_GPU2D_Init, MX_DMA2D_Init, MX_LTDC_Init, MX_ICACHE_GPU2D_Init; SCB_EnableICache i SCB_EnableDCache.
- TouchGFXConfiguration.cpp tworzy LCDGPU2D_AXI i STM32DMA, framebuffer RGB565, inicjuje Nema GPU.
- TouchGFXGeneratedHAL.cpp używa HALGPU2D::initialize(16384), dwóch framebufferów, IRQ DMA2D/GPU2D/LTDC oraz HALGPU2D::blockCopy. Synchronizacja LTDC wyznacza granice klatek.
- TouchGFXHAL.cpp udostępnia istniejący bufor animacji, pomiar obciążenia MCU i instrumentation.
- FingerSlideTransition wykorzystuje SnapshotWidget i pozycje wyrównane do dwóch pikseli dla RGB565.

Konfiguracja akceleratorów jest aktywna; nie jest to pomiar FPS ani dowód maksymalnej wydajności. CPU nadal obsługuje logikę, układ i przygotowanie poleceń. JPEG jest zainicjowany do dekodowania wideo; GUI nie zawiera obecnie wideo, więc nie zwiększa płynności zwykłych ekranów. Nie dodawano GFXMMU dla prostokątnego framebufferu RGB565.

Narożniki mają wspólne, symetryczne maski ćwiartki koła z wygładzaniem (64 próbki na piksel, małe tablice w NOR). Obrys i wypełnienie są składane raz na piksel. Tło poza zaokrągleniem odpowiada karcie lub nagłówkowi pod elementem, dzięki czemu zagnieżdżone przyciski nie mają ciemnych narożników. Karty mają promień 12 px, przyciski 8 px, drobne elementy 6 px. Bez nowych buforów PSRAM. Test corner-pixel-test.log porównuje wszystkie cztery rogi renderowanych kart, przycisków i klawiszy w obu motywach oraz sprawdza kolor ich tła. Testy nawigacji i gestów nadal przechodzą.

Główne odczyty mają czcionkę 60 px (wcześniej 48 px), wspólną oś X=168 i identyczne położenie w kartach napięcia/prądu. Siedem znaków odczytu ma szerokość 252 px w obszarze 288 px, więc wartości 27.00 V i 5.000 A mieszczą się z zapasem. Usunięto podpis SETPOINT STATUS; pozostawiono komunikat edycji i wysłania.

USB-C: obok aktualnego kontraktu PD (napięcie, limit prądu, maksymalna moc i rola) są rzeczywiste odczyty z TC: TPS VBUS, wejściowy prąd BQ i moc BQ VBUS × IIN. W trybie SOURCE prąd/moc wejściowa nie udają pomiaru oddawanej energii i pozostają niedostępne. BMS korzysta niezależnie z świeżego TB, bms=1, sample=1: pack_mv × i_pack_ma. Dodatni prąd oznacza ładowanie, ujemny rozładowanie, zgodnie z dotychczasową stroną Battery.

Liniowy wskaźnik BMS ma zero w środku, zakres -100..+100 W, zieloną prawą i bursztynową lewą stronę. Wskaźnik płynnie dochodzi do pomiaru, ogranicza pozycję na końcach skali i zachowuje pełną wartość liczbową powyżej zakresu. Przy braku ważnego pomiaru ukrywa wskaźnik i pokazuje --. Nie dodano bitmap ani buforów PSRAM. Definicja elementów w .touchgfx, logika w ScreenUsbPdView; test UsbSelfTest jest opcjonalny (PSU_USB_SELFTEST), bez wpływu na Run Simulator bez tej zmiennej. Testy: ładowanie, rozładowanie, zero, saturacja, nieważny ADC, przeterminowanie TB, SOURCE i jasny motyw. Nawigacja i wszystkie cztery gesty nadal przechodzą.

Walidacja 2026-10-04: Generate / Run Target w Designer zakończone Download verified successfully i Hard reset. NOR: 3163116 B (suma segmentów LOAD o adresie ładowania 0x70000000..0x77ffffff); rozpiętość z przerwą rezerwacji kodu 4779648 B. PSRAM 2693120 B bez nowych buforów.


Aktualizacja klawiatury: karta pod klawiaturą usunięta, panel powiększony do Y=80..464, klawisze 64 px wysokości (wiersze 116/186/256/326/396). Ostrzeżenia i wynik APPLY są wyświetlane przez 180 klatek w miejscu nagłówka; zwykłe podpowiedzi nie zajmują ekranu. Krótki/anulowany gest zachowuje komunikat i jego pozostały czas. Testy dotyku używają nowych środków klawiszy. Stare bitmapy 48 px zarchiwizowane poza aktywnym projektem.

Run Target po powiększeniu klawiatury: Download verified successfully, Hard reset, Done. Test wygaśnięcia komunikatu APPLY przeszedł. NOR 3178260 B, PSRAM 2693120 B.

