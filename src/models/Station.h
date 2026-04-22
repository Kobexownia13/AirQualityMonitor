#ifndef STATION_H
#define STATION_H

/**
 * @file Station.h
 * @brief Model stacji pomiarowej GIOS.
 */

#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/// Informacje o gminie
struct Commune {
    std::string communeName;   ///< Nazwa gminy
    std::string districtName;  ///< Nazwa powiatu
    std::string provinceName;  ///< Nazwa wojewodztwa
};

/// Informacje o miescie
struct City {
    int id = 0;           ///< Identyfikator miasta
    std::string name;     ///< Nazwa miejscowosci
    Commune commune;      ///< Informacje o gminie
};

/// Stacja pomiarowa GIOS
struct Station {
    int id = 0;                ///< Identyfikator stacji
    std::string stationName;   ///< Nazwa stacji
    double gegrLat = 0.0;      ///< Szerokosc geograficzna
    double gegrLon = 0.0;      ///< Dlugosc geograficzna
    City city;                 ///< Dane lokalizacyjne
    std::string addressStreet; ///< Adres ulicy
};

// ---- Serializacja do/z JSON (do zapisu w bazie) ----

inline void from_json(const json& j, Commune& c) {
    c.communeName = j.value("communeName", "");
    c.districtName = j.value("districtName", "");
    c.provinceName = j.value("provinceName", "");
}
inline void to_json(json& j, const Commune& c) {
    j = json{{"communeName", c.communeName}, {"districtName", c.districtName}, {"provinceName", c.provinceName}};
}
inline void from_json(const json& j, City& c) {
    c.id = j.value("id", 0);
    c.name = j.value("name", "");
    if (j.contains("commune") && j["commune"].is_object()) c.commune = j["commune"].get<Commune>();
}
inline void to_json(json& j, const City& c) {
    j = json{{"id", c.id}, {"name", c.name}, {"commune", c.commune}};
}
inline void from_json(const json& j, Station& s) {
    s.id = j.value("id", 0);
    s.stationName = j.value("stationName", "");
    s.gegrLat = j.value("gegrLat", 0.0);
    s.gegrLon = j.value("gegrLon", 0.0);
    s.addressStreet = j.value("addressStreet", "");
    if (j.contains("city") && j["city"].is_object()) s.city = j["city"].get<City>();
}
inline void to_json(json& j, const Station& s) {
    j = json{{"id", s.id}, {"stationName", s.stationName},
             {"gegrLat", s.gegrLat}, {"gegrLon", s.gegrLon},
             {"addressStreet", s.addressStreet}, {"city", s.city}};
}

#endif // STATION_H
