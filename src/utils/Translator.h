#ifndef TRANSLATOR_H
#define TRANSLATOR_H

/**
 * @file Translator.h
 * @brief System tlumaczen aplikacji (PL/EN).
 */

#include <QString>
#include <QMap>
#include <QObject>

/**
 * @class Translator
 * @brief Singleton do zarzadzania tlumaczeniami.
 */
class Translator : public QObject {
    Q_OBJECT
public:
    /// Dostepne jezyki
    enum Language { Polish, English };

    /// Zwraca instancje singletona
    static Translator& instance() {
        static Translator inst;
        return inst;
    }

    /// Ustawia aktywny jezyk
    void setLanguage(Language lang) {
        if (m_currentLang != lang) {
            m_currentLang = lang;
            emit languageChanged();
        }
    }

    /// Zwraca aktywny jezyk
    Language currentLanguage() const { return m_currentLang; }

    /// Tlumaczenie tekstu
    QString tr(const QString& key) const {
        if (m_currentLang == English && m_translations.contains(key)) {
            return m_translations[key];
        }
        return key; // Polski jest domyslny
    }

    /// Skrot do tlumaczenia
    QString operator()(const QString& key) const { return tr(key); }

signals:
    void languageChanged();

private:
    Translator() : m_currentLang(Polish) {
        initTranslations();
    }
    Translator(const Translator&) = delete;
    Translator& operator=(const Translator&) = delete;

    Language m_currentLang;
    QMap<QString, QString> m_translations;

    void initTranslations() {
        // === Menu ===
        m_translations["&Plik"] = "&File";
        m_translations["&Pobierz stacje z API"] = "&Fetch stations from API";
        m_translations["&Wczytaj z bazy"] = "&Load from database";
        m_translations["&Zakoncz"] = "&Exit";
        m_translations["&Pomoc"] = "&Help";
        m_translations["O programie"] = "About";
        m_translations["&Jezyk"] = "&Language";
        m_translations["Polski"] = "Polish";
        m_translations["English"] = "English";

        // === Tytuly i naglowki ===
        m_translations["Monitor Jakosci Powietrza - GIOS"] = "Air Quality Monitor - GIOS";
        m_translations["Stacje pomiarowe"] = "Monitoring Stations";
        m_translations["Czujniki"] = "Sensors";
        m_translations["Wykres"] = "Chart";
        m_translations["Analiza"] = "Analysis";
        m_translations["Indeks"] = "Index";
        m_translations["Zakres dat"] = "Date Range";
        m_translations["Mapa"] = "Map";
        m_translations["Wyszukiwanie w promieniu"] = "Search by Radius";

        // === Przyciski ===
        m_translations["Pobierz z API"] = "Fetch from API";
        m_translations["Z bazy"] = "From DB";
        m_translations["Szukaj"] = "Search";
        m_translations["Filtruj"] = "Filter";
        m_translations["Resetuj"] = "Reset";
        m_translations["Zapisz dane do bazy"] = "Save data to database";
        m_translations["Indeks jakosci"] = "Air Quality Index";
        m_translations["Szukaj w promieniu"] = "Search by radius";
        m_translations["Pokaz mape"] = "Show map";

        // === Etykiety ===
        m_translations["Szukaj miejscowosci..."] = "Search city...";
        m_translations["Wybierz stacje z listy."] = "Select a station from the list.";
        m_translations["Od:"] = "From:";
        m_translations["Do:"] = "To:";
        m_translations["Promien (km):"] = "Radius (km):";
        m_translations["Lokalizacja:"] = "Location:";
        m_translations["np. Poznan, Warszawa..."] = "e.g. Poznan, Warsaw...";

        // === Statusy i komunikaty ===
        m_translations["Gotowy. Pobierz stacje lub wczytaj z bazy."] = "Ready. Fetch stations or load from database.";
        m_translations["Pobieranie stacji z API GIOS..."] = "Fetching stations from GIOS API...";
        m_translations["Pobrano %1 stacji."] = "Fetched %1 stations.";
        m_translations["Wczytano %1 stacji z bazy."] = "Loaded %1 stations from database.";
        m_translations["Brak zapisanych stacji. Pobierz z API."] = "No saved stations. Fetch from API.";
        m_translations["Wyswietlono %1 z %2 stacji."] = "Displayed %1 of %2 stations.";
        m_translations["Pobieranie czujnikow..."] = "Fetching sensors...";
        m_translations["Pobrano %1 czujnikow."] = "Fetched %1 sensors.";
        m_translations["Pobieranie danych pomiarowych..."] = "Fetching measurement data...";
        m_translations["Pobrano %1 pomiarow (%2 prawidlowych)."] = "Fetched %1 measurements (%2 valid).";
        m_translations["Wczytano dane z bazy."] = "Loaded data from database.";
        m_translations["Pobieranie indeksu..."] = "Fetching air quality index...";
        m_translations["Dane zapisane do bazy."] = "Data saved to database.";
        m_translations["Zapis"] = "Save";

        // === Bledy ===
        m_translations["Blad"] = "Error";
        m_translations["Blad polaczenia"] = "Connection Error";
        m_translations["Wczytac dane z bazy?"] = "Load data from database?";
        m_translations["Nie znaleziono lokalizacji. Sprobuj: Warszawa, Krakow, Poznan..."] = 
            "Location not found. Try: Warsaw, Krakow, Poznan...";
        m_translations["Brak stacji w promieniu %1 km."] = "No stations within %1 km radius.";
        m_translations["Znaleziono %1 stacji w promieniu %2 km."] = "Found %1 stations within %2 km radius.";

        // === Analiza ===
        m_translations["Analiza danych"] = "Data Analysis";
        m_translations["Parametr:"] = "Parameter:";
        m_translations["Pomiary:"] = "Measurements:";
        m_translations["prawidlowych:"] = "valid:";
        m_translations["Minimum"] = "Minimum";
        m_translations["Maksimum"] = "Maximum";
        m_translations["Srednia"] = "Average";
        m_translations["Trend"] = "Trend";
        m_translations["Tendencja rosnaca"] = "Increasing trend";
        m_translations["Tendencja malejaca"] = "Decreasing trend";
        m_translations["Trend stabilny"] = "Stable trend";
        m_translations["Brak danych do analizy."] = "No data to analyze.";

        // === Indeks jakosci ===
        m_translations["Indeks jakosci powietrza"] = "Air Quality Index";
        m_translations["Brak danych"] = "No data";
        m_translations["Data obliczenia:"] = "Calculation date:";

        // === Wykres ===
        m_translations["Brak danych"] = "No data";
        m_translations["Brak prawidlowych pomiarow."] = "No valid measurements.";
        m_translations["Wybierz stacje i czujnik."] = "Select a station and sensor.";
        m_translations["Wyswietlono %1 z %2 pomiarow."] = "Displayed %1 of %2 measurements.";
        m_translations["Pomiary:"] = "Measurements:";
        m_translations["Data i czas"] = "Date and time";

        // === Mapa ===
        m_translations["Ladowanie mapy..."] = "Loading map...";
        m_translations["Stacja:"] = "Station:";
        m_translations["Odleglosc:"] = "Distance:";

        // === O programie ===
        m_translations["<h3>Monitor Jakosci Powietrza</h3><p>Dane: GIOS | Projekt JPO 2025/2026</p>"] = 
            "<h3>Air Quality Monitor</h3><p>Data: GIOS | JPO Project 2025/2026</p>";

        // === Info o stacji ===
        m_translations["Miasto:"] = "City:";
        m_translations["Adres:"] = "Address:";
        m_translations["Gmina:"] = "Commune:";
        m_translations["Powiat:"] = "District:";
        m_translations["Woj.:"] = "Province:";
    }
};

/// Makro ulatwiajace tlumaczenie
#define TR(key) Translator::instance().tr(key)

#endif // TRANSLATOR_H
