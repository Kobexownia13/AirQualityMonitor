/**
 * @file ApiClient.cpp
 * @brief Implementacja klienta REST API GIOS v1.
 */

#include "api/ApiClient.h"
#include <cpr/cpr.h>
#include <stdexcept>
#include <iostream>
#include <cctype>
#include <algorithm>

// Najwazniejsze klucze zwracane przez API.
static const std::string KEY_STATION_LIST    = "Lista stacji pomiarowych";
static const std::string KEY_STATION_ID      = "Identyfikator stacji";
static const std::string KEY_STATION_NAME    = "Nazwa stacji";
static const std::string KEY_LAT             = "WGS84 \xCF\x86 N";
static const std::string KEY_LON             = "WGS84 \xCE\xBB E";
static const std::string KEY_CITY_ID         = "Identyfikator miasta";
static const std::string KEY_CITY_NAME       = "Nazwa miasta";
static const std::string KEY_COMMUNE         = "Gmina";
static const std::string KEY_DISTRICT        = "Powiat";
static const std::string KEY_PROVINCE        = "Wojew\xC3\xB3" "dztwo";
static const std::string KEY_STREET          = "Ulica";
static const std::string KEY_SENSOR_ID       = "Identyfikator stanowiska";
static const std::string KEY_PARAM_NAME      = "Wska\xC5\xBA" "nik - nazwa";
static const std::string KEY_PARAM_FORMULA   = "Wska\xC5\xBA" "nik - wz\xC3\xB3r";
static const std::string KEY_PARAM_CODE      = "Wska\xC5\xBA" "nik - kod";
static const std::string KEY_PARAM_ID        = "Wska\xC5\xBA" "nik - identyfikator";
static const std::string KEY_DATE            = "Data";
static const std::string KEY_VALUE           = "Warto\xC5\x9B\xC4\x87";

ApiClient& ApiClient::instance() {
    static ApiClient inst;
    return inst;
}

std::string ApiClient::safeStr(const json& j, const std::string& key) {
    if (j.contains(key) && j[key].is_string()) return j[key].get<std::string>();
    return "";
}

static int safeInt(const json& j, const std::string& key, int defaultValue = -1) {
    if (!j.contains(key) || j[key].is_null()) {
        return defaultValue;
    }

    const auto& v = j[key];
    try {
        if (v.is_number_integer()) return v.get<int>();
        if (v.is_number_unsigned()) return static_cast<int>(v.get<unsigned int>());
        if (v.is_number_float()) return static_cast<int>(v.get<double>());
        if (v.is_string()) return std::stoi(v.get<std::string>());
    } catch (...) {
        return defaultValue;
    }
    return defaultValue;
}

json ApiClient::makeGetRequest(const std::string& endpoint) {
    std::string url = std::string(BASE_URL) + endpoint;

    cpr::Response r = cpr::Get(cpr::Url{url}, cpr::Timeout{15000});

    if (r.error)
        throw std::runtime_error("Blad sieci dla " + endpoint + ": " + r.error.message);
    if (r.status_code != 200) {
        auto compactWhitespace = [](std::string s) {
            std::string out;
            out.reserve(s.size());
            bool prevSpace = false;
            for (unsigned char c : s) {
                const bool isSpace = std::isspace(c) != 0;
                if (isSpace) {
                    if (!prevSpace) out.push_back(' ');
                    prevSpace = true;
                } else {
                    out.push_back(static_cast<char>(c));
                    prevSpace = false;
                }
            }
            // Proste przyciecie spacji z poczatku i konca.
            while (!out.empty() && out.front() == ' ') out.erase(out.begin());
            while (!out.empty() && out.back() == ' ') out.pop_back();
            return out;
        };

        auto shorten = [](std::string s, size_t maxLen) {
            if (s.size() <= maxLen) return s;
            if (maxLen < 4) return s.substr(0, maxLen);
            return s.substr(0, maxLen - 3) + "...";
        };

        std::string detail;
        if (!r.text.empty()) {
            try {
                auto err = json::parse(r.text);
                auto pick = [&](const std::string& key) -> std::string {
                    return (err.contains(key) && err[key].is_string()) ? err[key].get<std::string>() : "";
                };

                detail = pick("error_reason");
                if (detail.empty()) detail = pick("error_result");
                if (detail.empty()) detail = pick("message");
                if (detail.empty()) detail = pick("error");
                if (detail.empty()) detail = pick("komunikat");
                if (detail.empty()) detail = pick("detail");
                if (detail.empty() && err.contains("errors") && err["errors"].is_array() &&
                    !err["errors"].empty() && err["errors"][0].is_string()) {
                    detail = err["errors"][0].get<std::string>();
                }
                if (detail.empty() && err.is_string()) {
                    detail = err.get<std::string>();
                }
            } catch (...) {
                detail = r.text;
            }
        }

        detail = shorten(compactWhitespace(detail), 260);
        if (detail.empty()) detail = "Brak dodatkowych informacji z serwera.";

        throw std::runtime_error(
            "HTTP " + std::to_string(r.status_code) + " dla " + endpoint + ": " + detail);
    }

    try { return json::parse(r.text); }
    catch (const json::parse_error& e) {
        throw std::runtime_error(std::string("Blad JSON: ") + e.what());
    }
}

// Pobieranie stacji ze stronicowania API.

std::vector<Station> ApiClient::fetchAllStations() {
    std::vector<Station> all;
    int page = 0, totalPages = 1;

    while (page < totalPages) {
        auto data = makeGetRequest("/station/findAll?page=" + std::to_string(page) + "&size=500");

        if (data.contains("totalPages"))
            totalPages = data["totalPages"].get<int>();

        json arr;
        if (data.contains(KEY_STATION_LIST))   arr = data[KEY_STATION_LIST];
        else if (data.is_array())              arr = data;
        else throw std::runtime_error("Nieprawidlowy format danych stacji.");

        for (const auto& item : arr) {
            Station s;
            s.id = item.value(KEY_STATION_ID, 0);
            s.stationName = safeStr(item, KEY_STATION_NAME);
            std::string lat = safeStr(item, KEY_LAT);
            std::string lon = safeStr(item, KEY_LON);
            s.gegrLat = lat.empty() ? 0.0 : std::stod(lat);
            s.gegrLon = lon.empty() ? 0.0 : std::stod(lon);
            s.addressStreet = safeStr(item, KEY_STREET);
            s.city.id = item.value(KEY_CITY_ID, 0);
            s.city.name = safeStr(item, KEY_CITY_NAME);
            s.city.commune.communeName = safeStr(item, KEY_COMMUNE);
            s.city.commune.districtName = safeStr(item, KEY_DISTRICT);
            s.city.commune.provinceName = safeStr(item, KEY_PROVINCE);
            all.push_back(s);
        }
        page++;
    }
    return all;
}

// Pobieranie czujnikow dla jednej stacji.

std::vector<Sensor> ApiClient::fetchSensors(int stationId) {
    auto data = makeGetRequest("/station/sensors/" + std::to_string(stationId));

    json arr;
    if (data.is_array()) {
        arr = data;
    } else if (data.is_object()) {
        for (auto& [key, val] : data.items())
            if (val.is_array() && key.find("@") == std::string::npos) { arr = val; break; }
    }
    if (!arr.is_array())
        throw std::runtime_error("Nieprawidlowy format danych czujnikow.");

    std::vector<Sensor> sensors;
    for (const auto& item : arr) {
        Sensor s;
        s.id = item.value(KEY_SENSOR_ID, item.value("id", 0));
        s.stationId = stationId;
        s.param.paramName = safeStr(item, KEY_PARAM_NAME);
        s.param.paramFormula = safeStr(item, KEY_PARAM_FORMULA);
        s.param.paramCode = safeStr(item, KEY_PARAM_CODE);
        s.param.idParam = item.value(KEY_PARAM_ID, 0);
        // Fallback dla starszego formatu odpowiedzi.
        if (s.param.paramName.empty() && item.contains("param")) {
            auto p = item["param"];
            s.param.paramName = p.value("paramName", "");
            s.param.paramFormula = p.value("paramFormula", "");
            s.param.paramCode = p.value("paramCode", "");
            s.param.idParam = p.value("idParam", 0);
        }
        sensors.push_back(s);
    }
    return sensors;
}

// Pobieranie danych pomiarowych jednego czujnika.

MeasurementData ApiClient::fetchMeasurementData(int sensorId) {
    // Bierzemy wiekszy rozmiar strony, zeby dostac wiecej niz domyslne 20 pomiarow.
    const std::string endpoint = "/data/getData/" + std::to_string(sensorId) + "?size=500";
    json data;
    try {
        data = makeGetRequest(endpoint);
    } catch (const std::runtime_error& e) {
        // Dla stacji manualnych API zwraca czesto HTTP 400.
        // Zamieniamy to na krotszy i bardziej zrozumialy komunikat.
        auto toLower = [](std::string s) {
            std::transform(s.begin(), s.end(), s.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return s;
        };
        const std::string msg = e.what();
        const std::string lowerMsg = toLower(msg);
        const bool isManualStationMessage =
            lowerMsg.find("/data/getdata/") != std::string::npos &&
            lowerMsg.find("http 400") != std::string::npos &&
            (lowerMsg.find("manual") != std::string::npos ||
             lowerMsg.find("4-8 tyg") != std::string::npos ||
             lowerMsg.find("archiwalne dane pomiarowe") != std::string::npos);

        if (isManualStationMessage) {
            throw std::runtime_error(
                "To stanowisko jest typu manualnego. "
                "Biezace pomiary nie sa publikowane na zywo; "
                "dane pojawiaja sie po okolo 4-8 tygodniach "
                "w usludze API \"Archiwalne dane pomiarowe\".");
        }
        throw;
    }

    MeasurementData md;

    // Czesc odpowiedzi z API przychodzi jako obiekt bledu zamiast danych.
    auto isErrorObject = [](const json& d) {
        return d.is_object() && (
               d.contains("error_result") || d.contains("error_reason") ||
               d.contains("error") || d.contains("message") ||
               d.contains("komunikat"));
    };

    if (isErrorObject(data)) {
        std::string reason = safeStr(data, "error_reason");
        if (reason.empty()) reason = safeStr(data, "error_result");
        if (reason.empty()) reason = safeStr(data, "message");
        if (reason.empty()) reason = safeStr(data, "error");
        if (reason.empty()) reason = safeStr(data, "komunikat");
        if (reason.empty()) {
            reason = "Brak biezacych danych dla tego czujnika. "
                     "To prawdopodobnie stacja manualna - dane pojawiaja sie "
                     "z opoznieniem 4-8 tygodni po analizie laboratoryjnej.";
        }
        throw std::runtime_error(reason);
    }

    if (!data.is_object())
        throw std::runtime_error("Nieprawidlowy format danych pomiarowych.");

    md.key = safeStr(data, "key");
    if (md.key.empty()) md.key = safeStr(data, "Klucz");

    json vals;
    if (data.contains("values"))               vals = data["values"];
    else {
        for (auto& [key, val] : data.items())
            if (val.is_array() && key.find("@") == std::string::npos) {
                vals = val;
                if (md.key.empty()) md.key = key;
                break;
            }
    }

    if (vals.is_array()) {
        for (const auto& v : vals) {
            // Jeden uszkodzony rekord nie powinien wylozyc calego wczytywania.
            try {
                MeasurementValue mv;
                mv.date = safeStr(v, KEY_DATE);
                if (mv.date.empty()) mv.date = safeStr(v, "date");

                bool found = false;
                if (v.contains(KEY_VALUE) && !v[KEY_VALUE].is_null()) {
                    mv.value = v[KEY_VALUE].get<double>(); found = true;
                }
                if (!found && v.contains("value") && !v["value"].is_null()) {
                    mv.value = v["value"].get<double>(); found = true;
                }
                if (!found) mv.value = std::nullopt;

                md.values.push_back(mv);
            } catch (...) {
                // Pomijamy pojedynczy zly rekord.
                continue;
            }
        }
    }
    // Jasny komunikat zamiast pustej tabeli bez wyjasnienia.
    if (md.values.empty()) {
        throw std::runtime_error(
            "Brak dostepnych pomiarow dla tego czujnika. "
            "Jezeli to stacja manualna (PM10 z filtra, WWA, metale ciezkie), "
            "dane pojawiaja sie z opoznieniem 4-8 tygodni.");
    }

    return md;
}

// Pobieranie indeksu jakosci dla stacji.
// API ma drobna niespojnosc w nazwach kluczy, wiec czytamy oba warianty.

AirQualityIndex ApiClient::fetchAirQualityIndex(int stationId) {
    auto data = makeGetRequest("/aqindex/getIndex/" + std::to_string(stationId));

    AirQualityIndex aqi;
    aqi.stationId = stationId;
    // Gios zwraca indeks w obiekcie "AqIndex".
    if (!data.contains("AqIndex") || !data["AqIndex"].is_object()) {
        return aqi;
    }

    json idx = data["AqIndex"];
    // Data przeliczenia indeksu.
    aqi.calcDate = safeStr(idx, "Data wykonania oblicze\xC5\x84 indeksu");
    // Indeks ogolny.
    aqi.overall.id   = safeInt(idx, "Warto\xC5\x9B\xC4\x87 indeksu", -1);
    aqi.overall.name = safeStr(idx, "Nazwa kategorii indeksu");
    // Kazdy parametr ma pole z liczba i osobne pole z opisem slownym.
    auto pobierzIndeksCzastkowy = [&](const std::string& parametr) -> IndexLevel {
        IndexLevel poziom;
        // Klucz liczbowy.
        std::string kluczWartosci = "Warto\xC5\x9B\xC4\x87 indeksu dla wska\xC5\xBA" "nika " + parametr;
        poziom.id = safeInt(idx, kluczWartosci, -1);
        // Klucz opisowy (z literowka po stronie API).
        std::string kluczNazwy = "Nazwa kategorii indeksu dla wska\xC5\xBC" "nika " + parametr;
        poziom.name = safeStr(idx, kluczNazwy);

        return poziom;
    };
    // Indeksy czastkowe.
    aqi.no2  = pobierzIndeksCzastkowy("NO2");
    aqi.so2  = pobierzIndeksCzastkowy("SO2");
    aqi.pm10 = pobierzIndeksCzastkowy("PM10");
    aqi.pm25 = pobierzIndeksCzastkowy("PM2.5");
    aqi.o3   = pobierzIndeksCzastkowy("O3");

    return aqi;
}

