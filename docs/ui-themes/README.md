# Osobny ekran Appearance

Kafelek Appearance w ScreenSettings otwiera ScreenAppearance. LIGHT/DARK i cztery akcenty przeniesione z menu głównego ustawień. MENU wraca do ScreenSettings. Definicja ekranu i interakcji jest zapisana w pliku .touchgfx; logika znajduje się w gui/include/gui/screenappearance_screen i gui/src/screenappearance_screen. Nie edytowano ręcznie plików generowanych.

Test rzeczywistych próbek dotyku w symulatorze sprawdza otwarcie kafelka, oba tryby i cztery akcenty, powrót oraz siedem pozostałych stron ustawień bez zmiany parametrów PSU. Test gestów nadal przechodzi. Logi: appearance-test.log, appearance-finger.log. Zajętość: appearance-memory.json i appearance-memory.txt.

Audyt akceleracji (bez zmiany konfiguracji sprzętowej):
- main.c uruchamia MX_GPU2D_Init, MX_DMA2D_Init, MX_LTDC_Init, MX_ICACHE_GPU2D_Init; SCB_EnableICache i SCB_EnableDCache.
- TouchGFXConfiguration.cpp tworzy LCDGPU2D_AXI i STM32DMA, framebuffer RGB565, inicjuje Nema GPU.
- TouchGFXGeneratedHAL.cpp używa HALGPU2D::initialize(16384), dwóch framebufferów, IRQ DMA2D/GPU2D/LTDC oraz HALGPU2D::blockCopy. Synchronizacja LTDC wyznacza granice klatek.
- TouchGFXHAL.cpp udostępnia istniejący bufor animacji, pomiar obciążenia MCU i instrumentation.
- FingerSlideTransition wykorzystuje SnapshotWidget i pozycje wyrównane do dwóch pikseli dla RGB565.

Konfiguracja akceleratorów jest aktywna; nie jest to pomiar FPS ani dowód maksymalnej wydajności. CPU nadal obsługuje logikę, układ i przygotowanie poleceń. JPEG jest zainicjowany do dekodowania wideo; GUI nie zawiera obecnie wideo, więc nie zwiększa płynności zwykłych ekranów. Nie dodawano GFXMMU dla prostokątnego framebufferu RGB565.

Narożniki mają wspólne, symetryczne maski ćwiartki koła z wygładzaniem (64 próbki na piksel, małe tablice w NOR). Obrys i wypełnienie są składane raz na piksel. Tło poza zaokrągleniem odpowiada karcie lub nagłówkowi pod elementem, dzięki czemu zagnieżdżone przyciski nie mają ciemnych narożników. Karty mają promień 12 px, przyciski 8 px, drobne elementy 6 px. Bez nowych buforów PSRAM. Test corner-pixel-test.log porównuje wszystkie cztery rogi renderowanych kart, przycisków i klawiszy w obu motywach oraz sprawdza kolor ich tła. Testy nawigacji i gestów nadal przechodzą.
