# Monitor Jakości Powietrza

Aplikacja desktopowa do monitorowania jakości powietrza w Polsce.
Dane z API GIOŚ (v1): https://powietrze.gios.gov.pl/pjp/content/api

**Projekt JPO 2025/2026**

## Funkcjonalności

- Pobieranie danych z REST API GIOŚ (stacje, czujniki, pomiary, indeks jakości)
- Lokalna baza danych JSON (zapis/odczyt danych historycznych)
- Wizualizacja danych na wykresie liniowym (Qt Charts) z filtrowaniem po dacie
- Analiza statystyczna: minimum, maksimum, średnia, trend (regresja liniowa)
- Indeks jakości powietrza z kolorową skalą
- Wyszukiwanie stacji po nazwie miejscowości
- Wyszukiwanie stacji w zadanym promieniu od lokalizacji (haversine)
- Interaktywna mapa stacji pomiarowych z wyborem stacji przez kliknięcie punktu
- Interfejs aplikacji w języku polskim
- Obsługa błędów sieciowych z propozycją danych z bazy
- Wielowątkowość (QtConcurrent) — pobieranie danych nie blokuje GUI
- Dokumentacja kodu (Doxygen)
- Testy jednostkowe modułu analizy (GoogleTest)

## Wymagania

- C++17, CMake 3.16+
- Qt6 (Widgets, Network, Charts, Concurrent)
- Biblioteki pobierane automatycznie przez CMake FetchContent: nlohmann/json, CPR, GoogleTest

## Kompilacja

### Qt Creator (najprościej)

1. Otwórz Qt Creator → File → Open File or Project → wybierz `CMakeLists.txt`
2. Wybierz kit (np. Desktop Qt 6.x MSVC) → Configure Project
3. Kliknij zielony trójkąt ▶ (Ctrl+R)

### Wiersz poleceń (Windows)

```
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=C:\Qt\6.x.x\msvc2022_64
cmake --build . --config Release
Release\AirQualityMonitor.exe
```

### Linux / macOS

```
mkdir build && cd build
cmake ..
make -j$(nproc)
./AirQualityMonitor
```

## Testy

```
cd build
ctest --output-on-failure
```

Obecnie pokrywają one moduł `DataAnalyzer` (min, max, średnia, trend,
filtrowanie po dacie) — plik `tests/test_analyzer.cpp`.

## Dokumentacja Doxygen

```
doxygen Doxyfile
```

Wynik: `docs/html/index.html`

## Struktura projektu

```
src/
├── main.cpp                    - punkt wejścia
├── api/
│   ├── ApiClient.h
│   └── ApiClient.cpp           - klient REST API GIOŚ v1 (JSON-LD)
├── models/
│   ├── Station.h               - model stacji pomiarowej
│   ├── Sensor.h                - model czujnika
│   ├── MeasurementData.h       - model danych pomiarowych
│   └── AirQualityIndex.h       - model indeksu jakości
├── database/
│   ├── DatabaseManager.h
│   └── DatabaseManager.cpp     - zapis/odczyt JSON
├── analysis/
│   ├── DataAnalyzer.h
│   └── DataAnalyzer.cpp        - analiza statystyczna
├── utils/
│   ├── GeoUtils.h              - haversine, geokodowanie, wyszukiwanie w promieniu
└── gui/
    ├── MainWindow.h
    ├── MainWindow.cpp          - główne okno aplikacji
    ├── ChartWidget.h
    ├── ChartWidget.cpp         - widget wykresu z filtrowaniem po dacie
    ├── MapWidget.h
    └── MapWidget.cpp           - uproszczona mapa stacji pomiarowych
tests/
└── test_analyzer.cpp           - testy modułu DataAnalyzer (GoogleTest)
```

## Wzorce projektowe

- **Singleton** - `ApiClient` (jedna globalna instancja klienta API)
- **Observer** - sygnaly/sloty Qt (zdarzenia GUI i aktualizacja widoku)
- **MVC** — separacja modeli (`models/`), logiki (`analysis/`, `api/`, `database/`) i widoków (`gui/`)

## Funkcja wyszukiwania w promieniu

Aplikacja umożliwia wyszukiwanie stacji pomiarowych w zadanym promieniu (km) od podanej lokalizacji:

1. Wpisz nazwę miejscowości (np. "Poznań", "Warszawa", "Katowice")
2. Ustaw promień wyszukiwania (1–500 km)
3. Kliknij "Szukaj w promieniu"
4. Wyniki pojawią się na liście

Obsługiwane są główne miasta Polski oraz popularne lokalizacje — lista w `GeoUtils::geocodeSimple`.
Wyszukiwanie jest odporne na polskie znaki (np. "Łódź", "Poznań") i wielkość liter.

## API GIOŚ v1

Nowe API (od 30.06.2025) zwraca dane w formacie JSON-LD z polskimi nazwami pól.
Dokumentacja: https://powietrze.gios.gov.pl/pjp/content/api

W `ApiClient.cpp` klucze pól API są zdefiniowane jako stałe z komentarzami o kodowaniu UTF-8
(niektóre znaki są zapisane w postaci sekwencji `\xHH` podzielonych przez konkatenację
stringów — jest to zabezpieczenie pod MSVC, który inaczej mógłby zinterpretować ciąg hex
jako zbyt długi).

## Autor

*_Mateusz Kwasek_*

