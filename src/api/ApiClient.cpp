/**
 * @file ApiClient.cpp
 * @brief Implementacja klienta REST API GIOS v1.
 */

#include "api/ApiClient.h"
#include <cpr/cpr.h>
#include <stdexcept>
#include <iostream>

// Polskie nazwy pol w API v1 (UTF-8, rozbite dla MSVC)
static const std::string KEY_STATION_LIST    = "Lista stacji pomiarowych";
static const std::string KEY_STATION_ID      = "Identyfikator stacji";
static const std::string KEY_STATION_NAME    = "Nazwa stacji";
static const std::string KEY_LAT             = "WGS84 \xCF\x86 N";                  // φ
static const std::string KEY_LON             = "WGS84 \xCE\xBB E";                  // λ
static const std::string KEY_CITY_ID         = "Identyfikator miasta";
static const std::string KEY_CITY_NAME       = "Nazwa miasta";
static const std::string KEY_COMMUNE         = "Gmina";
static const std::string KEY_DISTRICT        = "Powiat";
static const std::string KEY_PROVINCE        = "Wojew\xC3\xB3" "dztwo";             // Województwo
static const std::string KEY_STREET          = "Ulica";
static const std::string KEY_SENSOR_ID       = "Identyfikator stanowiska";
static const std::string KEY_PARAM_NAME      = "Wska\xC5\xBA" "nik - nazwa";        // Wskaźnik
static const std::string KEY_PARAM_FORMULA   = "Wska\xC5\xBA" "nik - wz\xC3\xB3r";
static const std::string KEY_PARAM_CODE      = "Wska\xC5\xBA" "nik - kod";
static const std::string KEY_PARAM_ID        = "Wska\xC5\xBA" "nik - identyfikator";
static const std::string KEY_DATE            = "Data";
static const std::string KEY_VALUE           = "Warto\xC5\x9B\xC4\x87";             // Wartość

ApiClient& ApiClient::instance() {
    static ApiClient inst;
    return inst;
}

std::string ApiClient::safeStr(const json& j, const std::string& key) {
    if (j.contains(key) && j[key].is_string()) return j[key].get<std::string>();
    return "";
}

json ApiClient::makeGetRequest(const std::string& endpoint) {
    std::string url = std::string(BASE_URL) + endpoint;

    cpr::Response r = cpr::Get(cpr::Url{url}, cpr::Timeout{15000});

    if (r.error)
        throw std::runtime_error("Blad sieci: " + r.error.message);
    if (r.status_code != 200)
        throw std::runtime_error("HTTP " + std::to_string(r.status_code));

    try { return json::parse(r.text); }
    catch (const json::parse_error& e) {
        throw std::runtime_error(std::string("Blad JSON: ") + e.what());
    }
}

// ---- Stacje (stronicowanie) ----

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

// ---- Czujniki ----

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

        // fallback stare API
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

// ---- Dane pomiarowe ----

MeasurementData ApiClient::fetchMeasurementData(int sensorId) {
    // Prosimy o wiecej pomiarow niz domyslne 20 (API jest stronicowane)
    auto data = makeGetRequest("/data/getData/" + std::to_string(sensorId) + "?size=500");

    MeasurementData md;
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
            // try/catch - jesli jeden pomiar jest uszkodzony,
            // pomijamy go zamiast wywalac caly program
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
                // Pomijamy uszkodzony pomiar
                continue;
            }
        }
    }
    return md;
}

// =============================================
// Pobieranie indeksu jakosci powietrza
// =============================================
// API v1 zwraca dane w obiekcie "AqIndex" z polskimi nazwami pol.
// Przyklad odpowiedzi:
// {
//   "AqIndex": {
//     "Wartość indeksu": 1,
//     "Nazwa kategorii indeksu": "Dobry",
//     "Wartość indeksu dla wskaźnika NO2": 0,
//     "Nazwa kategorii indeksu dla wskażnika NO2": "Bardzo dobry",
//     ...
//   }
// }
// UWAGA: W API jest literowka - raz "wskaźnika" (poprawnie), raz "wskażnika" (blad)
// Pole z wartoscia numeryczna uzywa "wskaźnika", a pole z nazwa uzywa "wskażnika"

AirQualityIndex ApiClient::fetchAirQualityIndex(int stationId) {
    auto data = makeGetRequest("/aqindex/getIndex/" + std::to_string(stationId));

    AirQualityIndex aqi;
    aqi.stationId = stationId;

    // Dane indeksu sa w obiekcie pod kluczem "AqIndex"
    if (!data.contains("AqIndex") || !data["AqIndex"].is_object()) {
        return aqi;  // zwracamy pusty indeks jesli brak danych
    }

    json idx = data["AqIndex"];

    // Data obliczenia indeksu
    // UTF-8: "oblicze\xC5\x84" = "obliczeń"
    aqi.calcDate = safeStr(idx, "Data wykonania oblicze\xC5\x84 indeksu");

    // Indeks ogolny (najgorszy z czastkowych)
    // UTF-8: "Warto\xC5\x9B\xC4\x87" = "Wartość"
    aqi.overall.id   = idx.value("Warto\xC5\x9B\xC4\x87 indeksu", -1);
    aqi.overall.name = safeStr(idx, "Nazwa kategorii indeksu");

    // Funkcja pomocnicza do parsowania indeksow czastkowych
    // Kazdy parametr (NO2, SO2, PM10...) ma dwa pola:
    //   "Wartość indeksu dla wskaźnika X" - numer (0-5)
    //   "Nazwa kategorii indeksu dla wskażnika X" - tekst ("Dobry", "Zly"...)
    // UWAGA na literowke: "wskaźnika" vs "wskażnika" w API!
    auto pobierzIndeksCzastkowy = [&](const std::string& parametr) -> IndexLevel {
        IndexLevel poziom;

        // Pole numeryczne (uzywa poprawnego "wskaźnika")
        // UTF-8: "wska\xC5\xBA" + "nika" = "wskaźnika"
        std::string kluczWartosci = "Warto\xC5\x9B\xC4\x87 indeksu dla wska\xC5\xBA" "nika " + parametr;
        if (idx.contains(kluczWartosci) && !idx[kluczWartosci].is_null()) {
            poziom.id = idx[kluczWartosci].get<int>();
        }

        // Pole tekstowe (uzywa blednego "wskażnika" - literowka w API GIOS)
        // UTF-8: "wska\xC5\xBC" + "nika" = "wskażnika"
        std::string kluczNazwy = "Nazwa kategorii indeksu dla wska\xC5\xBC" "nika " + parametr;
        poziom.name = safeStr(idx, kluczNazwy);

        return poziom;
    };

    // Parsujemy indeksy czastkowe dla kazdego zanieczyszczenia
    aqi.no2  = pobierzIndeksCzastkowy("NO2");
    aqi.so2  = pobierzIndeksCzastkowy("SO2");
    aqi.pm10 = pobierzIndeksCzastkowy("PM10");
    aqi.pm25 = pobierzIndeksCzastkowy("PM2.5");
    aqi.o3   = pobierzIndeksCzastkowy("O3");

    return aqi;
}
