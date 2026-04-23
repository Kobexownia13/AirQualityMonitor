=======================================
  MONITOR JAKOSCI POWIETRZA
  Projekt JPO 2025/2026
=======================================

OPIS
  Aplikacja desktopowa do monitorowania
  jakosci powietrza w Polsce.
  Dane z API GIOS (v1, JSON-LD).

FUNKCJONALNOSCI
  - Pobieranie danych z API GIOS
  - Lokalna baza danych JSON
  - Wykresy (Qt Charts)
  - Analiza statystyczna
  - Wyszukiwanie w promieniu (km)
  - Interfejs w jezyku polskim

WYMAGANIA
  - C++17, CMake 3.16+
  - Qt6 (Widgets, Network, Charts, Concurrent)
  - Biblioteki: nlohmann/json, CPR
    (pobierane automatycznie przez CMake)

KOMPILACJA (Qt Creator)
  1. File > Open > CMakeLists.txt
  2. Configure Project
  3. Ctrl+R (Run)

KOMPILACJA (terminal)
  mkdir build && cd build
  cmake .. -DCMAKE_PREFIX_PATH=<sciezka_do_Qt>
  cmake --build . --config Release

TESTY
  cd build
  ctest --output-on-failure

DOKUMENTACJA
  doxygen Doxyfile

STATUS: kompletny (GUI, wykresy, analiza,
  wyszukiwanie w promieniu, testy,
  Doxygen, wielowatkowosc)
=======================================

