# Słownik diagnostyki G4 → H7

Stan źródeł i płytki: 2026-11-02. Plik JSON obok jest wersją do implementacji.

## Zasady wyświetlania

Kluczem jest źródło i pole. BMS.fault=0x10 oznacza różnicę napięć, BMS.sb=0x10 temperaturę ładowania, METER.psu_fault=0x10 błąd ADC. Nie łączyć ich w jedną tabelę. Dla maski wyświetlać wszystkie ustawione bity; dla enum dopasować pełną wartość. Zero maski oznacza brak flag, nie brak urządzenia. Brak/nieświeże dane wyświetlać jako BRAK DANYCH, nigdy OK. Nieznane bity zachować jako hex. Pole severity jest zaleceniem prezentacji, nie poleceniem sterowania zabezpieczeniami.

Zbiorczy SAFETY rozwinąć przez sa/sb/sc. Alarm jest zatrzaskiem zdarzeń i zawiera również zwykłe informacje — alarm!=0 nie oznacza automatycznie awarii. Stan WARN może wynikać z napięcia wysokiego/niskiego nawet przy fault=0. BQ fault może być historyczny (read-to-clear), więc nie dowodzi, że warunek nadal trwa. Stop_reason pozostaje do CLEAR/nowego ON.

Główne błędy G4/G0 są w ramce METER0x10 (offsety62/58, u32 LE), NACK0x82 ma rejected_type i reason. BMS/PD/AUX/BATT są w FIELDS0x15 według tlm_fields.h. Nowy H7 powinien używać jednego spójnego rekordu FIELDS, nie mieszać go z częściami starej telemetrii0x11/12/13.

## BMS.fault — bitmask
| Wartość | Symbol | Tekst H7 | Poziom |
|---|---|---|---|
| 0x1 | CELL_OV | Za wysokie napięcie ogniwa | fault |
| 0x2 | CELL_UV | Za niskie napięcie ogniwa | fault |
| 0x4 | PACK_OV | Za wysokie napięcie pakietu (bit zarezerwowany przez firmware; obecnie nieustawiany) | reserved |
| 0x8 | PACK_UV | Za niskie napięcie pakietu (bit zarezerwowany przez firmware; obecnie nieustawiany) | reserved |
| 0x10 | IMBALANCE | Różnica napięć ogniw co najmniej 100 mV | warning |
| 0x20 | SAFETY | Aktywne zabezpieczenie BMS — szczegóły w sa/sb/sc | fault |
| 0x40 | ALERT | Zatrzaśnięte zdarzenie BMS — szczegóły w alarm | warning |
| 0x80 | I2C | Błąd komunikacji G4–BMS | fault |
| 0x100 | FET_START | Nieudany rozruch toru akumulatora / predischarge | fault |
| 0x200 | BATT_LOW | Niski poziom baterii (ogniwo < 3,00 V) — podłącz ładowarkę | warning |
| 0x400 | BATT_EMPTY | Bateria rozładowana — wyłączanie, podłącz ładowarkę | fault |

## BMS.sa — bitmask
| Wartość | Symbol | Tekst H7 | Poziom |
|---|---|---|---|
| 0x80 | SCD | Zwarcie podczas rozładowania | fault |
| 0x40 | OCD2 | Nadprąd rozładowania, poziom 2 | fault |
| 0x20 | OCD1 | Nadprąd rozładowania, poziom 1 | fault |
| 0x10 | OCC | Nadprąd ładowania | fault |
| 0x8 | COV | Za wysokie napięcie ogniwa | fault |
| 0x4 | CUV | Za niskie napięcie ogniwa | fault |

## BMS.sb — bitmask
| Wartość | Symbol | Tekst H7 | Poziom |
|---|---|---|---|
| 0x80 | OTF | Przegrzanie czujnika przypisanego do FET | fault |
| 0x40 | OTINT | Przegrzanie wnętrza układu BMS | fault |
| 0x20 | OTD | Za wysoka temperatura dla rozładowania | fault |
| 0x10 | OTC | Za wysoka temperatura dla ładowania | fault |
| 0x4 | UTINT | Za niska temperatura wnętrza BMS | fault |
| 0x2 | UTD | Za niska temperatura dla rozładowania | fault |
| 0x1 | UTC | Za niska temperatura dla ładowania | fault |

## BMS.sc — bitmask
| Wartość | Symbol | Tekst H7 | Poziom |
|---|---|---|---|
| 0x80 | OCD3 | Nadprąd rozładowania, poziom 3 | fault |
| 0x40 | SCDL | Zatrzaśnięta ochrona zwarciowa | fault |
| 0x20 | OCDL | Zatrzaśnięta ochrona nadprądowa | fault |
| 0x10 | COVL | Zatrzaśnięta ochrona nadnapięciowa ogniwa | fault |
| 0x4 | PTO | Przekroczony czas precharge BMS | fault |
| 0x2 | HWDF | Watchdog hosta BMS | fault |

## BMS.alarm — bitmask
| Wartość | Symbol | Tekst H7 | Poziom |
|---|---|---|---|
| 0x8000 | SSBC | Zdarzenie zabezpieczenia w sb/sc | warning |
| 0x4000 | SSA | Zdarzenie zabezpieczenia w sa | warning |
| 0x2000 | PF | Permanent Fail — wymaga szczegółowej diagnostyki | fault |
| 0x1000 | MSK_SFALERT | Zamaskowane zbiorczo zdarzenie Safety Alert | warning |
| 0x800 | MSK_PFALERT | Zamaskowane zbiorczo zdarzenie PF Alert | warning |
| 0x400 | INITSTART | Rozpoczęto inicjalizację | info |
| 0x200 | INITCOMP | Zakończono inicjalizację | info |
| 0x80 | FULLSCAN | Zakończono skan pomiarów | info |
| 0x40 | XCHG | Ładowanie zablokowane | warning |
| 0x20 | XDSG | Rozładowanie zablokowane | warning |
| 0x10 | SHUTV | Warunek niskiego napięcia wyłączenia | warning |
| 0x8 | FUSE | Zdarzenie bezpiecznika | fault |
| 0x4 | CB | Zdarzenie balansowania | info |
| 0x2 | ADSCAN | Zakończono skan ADC | info |
| 0x1 | WAKE | Wybudzenie | info |

## PD.bq_fault — bitmask
| Wartość | Symbol | Tekst H7 | Poziom |
|---|---|---|---|
| 0x80 | ACOV | Za wysokie napięcie wejścia ładowarki | fault |
| 0x40 | BATOC | Nadprąd baterii; w PTM możliwy też BATOVP | fault |
| 0x20 | ACOC | Nadprąd wejścia ładowarki | fault |
| 0x10 | SYSOVP | Za wysokie napięcie SYS | fault |
| 0x8 | VSYS_UVP | Za niskie napięcie SYS | fault |
| 0x4 | FORCE_CONVERTER_OFF | Przetwornica wymuszenie wyłączona | warning |
| 0x2 | OTG_OVP | Za wysokie napięcie wyjścia OTG | fault |
| 0x1 | OTG_UVP | Za niskie napięcie wyjścia OTG | fault |

## METER.psu_fault — bitmask
| Wartość | Symbol | Tekst H7 | Poziom |
|---|---|---|---|
| 0x1 | DRIVER | Błąd sterownika głównej przetwornicy | fault |
| 0x2 | OVP | Nadnapięcie głównego wyjścia | fault |
| 0x4 | OCP | Nadprąd głównego wyjścia | fault |
| 0x8 | UVIN | Za niskie napięcie wejścia głównej przetwornicy | fault |
| 0x10 | ADC | Błąd pomiarów ADC | fault |
| 0x20 | BMS | Wyjście zatrzymane przez BMS — odczytaj BMS.fault/sa/sb/sc | fault |

## METER.g0_fault — bitmask
| Wartość | Symbol | Tekst H7 | Poziom |
|---|---|---|---|
| 0x1 | HW_INIT | G0: błąd inicjalizacji sprzętu | fault |
| 0x2 | PGOOD_LOST | G0: utrata PGOOD | fault |
| 0x4 | POWER_KILL | G0: aktywny sygnał zatrzymania | fault |
| 0x8 | VIN_LOW | G0: za niskie napięcie wejścia | fault |
| 0x10 | VOUT_HARD | G0: twarde przekroczenie napięcia wyjścia | fault |
| 0x20 | VOUT_HIGH | G0: przekroczone napięcie wyjścia | fault |
| 0x40 | TEMP_HIGH | G0: przegrzanie | fault |
| 0x80 | IOUT_HARD | G0: twarde przekroczenie prądu | fault |

## NACK.reason — enum
| Wartość | Symbol | Tekst H7 | Poziom |
|---|---|---|---|
| 0x1 | UNKNOWN | Nieznany typ polecenia | warning |
| 0x2 | BAD_PAYLOAD | Nieprawidłowa długość lub format polecenia | warning |
| 0x3 | RANGE | Parametr poza zakresem | warning |
| 0x4 | UNSAFE | Polecenie niedozwolone w obecnym stanie | warning |
| 0x5 | BUSY | Urządzenie zajęte | info |
| 0x6 | TIMEOUT | Przekroczono czas odpowiedzi | fault |
| 0x7 | LINK | Brak wymaganego połączenia | fault |

## AUX.stop_reason — enum
| Wartość | Symbol | Tekst H7 | Poziom |
|---|---|---|---|
| 0x0 | NONE | Brak zapisanej przyczyny zatrzymania | info |
| 0x1 | HOST_LINK | Utrata połączenia H7–G4 | fault |
| 0x2 | G0_LINK | Utrata połączenia G4–G0 | fault |
| 0x3 | G0_KILL | G0 zatrzymał wyjście sygnałem KILL | fault |
| 0x4 | G0_FAULT | Błąd G0 zatrzymał wyjście | fault |
| 0x5 | START_FAILURE | Nieudany rozruch wyjścia | fault |

## AUX.sense_code — enum
| Wartość | Symbol | Tekst H7 | Poziom |
|---|---|---|---|
| 0x0 | OK | Remote sense poprawny | info |
| 0x1 | NOT_READY | Napięcie za niskie do testu sense | info |
| 0x2 | DROP_P | Za duży spadek przewodu dodatniego | fault |
| 0x3 | DROP_SUM | Za duża suma spadków przewodów | fault |
| 0x4 | REVERSED | Odwrócone przewody sense | fault |
| 0x5 | N_ON_POUT | Oba przewody sense przy dodatnim wyjściu | fault |
| 0x6 | SHORT | Podejrzenie zwarcia przewodów sense | fault |
| 0x7 | DROP_N | Za duży spadek przewodu ujemnego | fault |
| 0x8 | NO_SAMPLE | Brak próbki ADC sense | fault |
| 0x9 | LOCAL_HIGH | Nasycenie/przekroczenie zakresu pomiaru lokalnego | fault |
| 0xa | CV_VD | Remote sense: brak zgodności napięcia z zadaniem w CV | fault |

## BATT.soc_flags — bitmask
| Wartość | Symbol | Tekst H7 | Poziom |
|---|---|---|---|
| 0x1 | VALID | SOC dostępny | info |
| 0x2 | UNCERTAIN | SOC niepotwierdzony wiarygodną kotwicą | warning |
| 0x4 | RESTING | Prąd w paśmie spoczynku estymatora | info |
| 0x8 | RELAXED | Upłynął czas relaksacji estymatora | info |
| 0x10 | FCC_LEARNED | Co najmniej jedna zaakceptowana aktualizacja FCC | info |
| 0x20 | CHARGING | Ładowanie według estymatora | info |
| 0x40 | DISCHARGING | Rozładowanie według estymatora | info |
| 0x80 | FULL | Potwierdzona kotwica pełnego naładowania | info |
| 0x100 | LOW | Niski poziom ładunku | warning |
| 0x200 | EMPTY | Kotwica dolna/pusty pakiet | warning |
| 0x400 | RESTORED | SOC odtworzony z pamięci | info |
| 0x800 | CLEAN_RESTORE | Odtworzono zapis z kontrolowanego shutdown | info |
| 0x1000 | DIRTY | Stan oczekuje zapisu do pamięci | info |

## BMS.st — enum
| Wartość | Symbol | Tekst H7 | Poziom |
|---|---|---|---|
| 0x0 | ABSENT | BMS nieobecny | fault |
| 0x1 | INIT | Inicjalizacja BMS | info |
| 0x2 | READY | BMS gotowy | info |
| 0x3 | DISCHARGING | Rozładowanie | info |
| 0x4 | CHARGING | Ładowanie | info |
| 0x5 | WARN | Ostrzeżenie BMS; sprawdź napięcia, temperatury i fault | warning |
| 0x6 | FAULT | Błąd BMS | fault |

## Dodatkowe warunki diagnostyczne

- bms=0: BRAK BMS; cfg=0: KONFIGURACJA NIEGOTOWA; sample=0 albo stare dane: BRAK POMIARU. cfg_fail/i2c_err/alerts to liczniki, nie maski — pokazywać wartość i przyrost.

- golden bit0: zgodność sprawdzanej konfiguracji, bit1: zgodność fingerprintu przy starcie; bit1 NIE jest dowodem fizycznego wypalenia OTP.

- balance_mask bit0..4: odczyt aktywnych kanałów1..5. CHG/DSG i fet są stanami, nie błędami.

- bq_iindpm i bq_vindpm: ograniczenie wejścia (informacja), nie błąd ani dowód pełnej baterii. bq_fast nie rozróżnia CC od CV. bq_pre oznacza fazę niskiego napięcia.

- store_ok=0: BŁĄD ZAPISU STANU BATERII; soc_flags UNCERTAIN lub learn_n=0: nie pokazywać SOH jako pomiaru potwierdzonego.

- Temperaturę ts1_dK oznaczać PCB, a temp_dC w BATT pochodzi z tego samego czujnika.

- Szczegółowych PF Status A/B/C/D ani wszystkich zdarzeń TPS obecna telemetria FIELDS nie przesyła. H7 nie może wymyślać przyczyny: PF bez szczegółów lub BŁĄD PD — WYMAGANA DIAGNOSTYKA.

## Źródła

Core/Inc/bq76922.h, app.h, h7_link_proto.h, sense_check.h, ldo_link.h, bms_gauge.h, tlm_fields.h; G0 Core/Inc/uart_protocol.h.

TI BQ76922 TRM SLUUCG7 §§12.2.3–7,12.2.17: https://www.ti.com/lit/ug/sluucg7/sluucg7.pdf

TI BQ25731 SLUSE66A §§9.6.8: https://www.ti.com/lit/ds/symlink/bq25731.pdf
