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
- **Wyszukiwanie stacji w zadanym promieniu od lokalizacji** (haversine)
- **Interaktywna mapa ze stacjami** (Leaflet + OpenStreetMap)
- **Wybór języka interfejsu: Polski / English**
- Obsługa błędów sieciowych z propozycją danych z bazy
- Wielowątkowość (QtConcurrent) — pobieranie danych nie blokuje GUI
- Dokumentacja kodu (Doxygen)
- Testy jednostkowe (GoogleTest)

## Wymagania

- C++17, CMake 3.16+
- Qt6 (Widgets, Network, Charts, Concurrent, **WebEngineWidgets, WebChannel**)
- Biblioteki pobierane automatycznie: nlohmann/json, CPR, GoogleTest

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

## Dokumentacja Doxygen

```
doxygen Doxyfile
```

Wynik: `docs/output/html/index.html`

## Struktura projektu

```
src/
├── main.cpp               - punkt wejścia
├── api/
│   └── ApiClient.h/.cpp   - klient REST API GIOŚ v1 (JSON-LD)
├── models/
│   ├── Station.h           - model stacji pomiarowej
│   ├── Sensor.h            - model czujnika
│   ├── MeasurementData.h   - model danych pomiarowych
│   └── AirQualityIndex.h   - model indeksu jakości
├── database/
│   └── DatabaseManager.h/.cpp - zapis/odczyt JSON
├── analysis/
│   └── DataAnalyzer.h/.cpp - analiza statystyczna
├── utils/
│   ├── GeoUtils.h          - funkcje geograficzne (haversine, geokodowanie)
│   └── Translator.h        - system tłumaczeń PL/EN
└── gui/
    ├── MainWindow.h/.cpp   - główne okno aplikacji
    ├── ChartWidget.h/.cpp  - widget wykresu
    └── MapWidget.h/.cpp    - widget mapy (Leaflet)
tests/
├── test_models.cpp         - testy modeli
├── test_analyzer.cpp       - testy analizatora
├── test_database.cpp       - testy bazy danych
└── test_geoutils.cpp       - testy funkcji geograficznych
```

## Wzorce projektowe

- **Singleton** — ApiClient, Translator (jedna instancja)
- **Observer** — sygnały/sloty Qt (powiadamianie GUI o zdarzeniach)
- **MVC** — separacja modeli, logiki i widoków
- **Bridge** — komunikacja C++ ↔ JavaScript w MapWidget

## Funkcja wyszukiwania w promieniu

Aplikacja umożliwia wyszukiwanie stacji pomiarowych w zadanym promieniu (km) od podanej lokalizacji:

1. Wpisz nazwę miejscowości (np. "Poznań", "Warszawa", "Wydział Informatyki")
2. Ustaw promień wyszukiwania (1-500 km)
3. Kliknij "Szukaj w promieniu"
4. Wyniki pojawią się na liście i na mapie z zaznaczonym promieniem

Obsługiwane są główne miasta Polski oraz popularne lokalizacje.

## Wybór języka

Menu → Język → Polski / English

Cały interfejs (menu, etykiety, komunikaty) zmienia się dynamicznie.

## API GIOŚ v1

Nowe API (od 30.06.2025) zwraca dane w formacie JSON-LD z polskimi nazwami pól.
Dokumentacja: https://powietrze.gios.gov.pl/pjp/content/api

## Autor
**_Mateusz Kwasek_**

