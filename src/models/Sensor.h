#ifndef SENSOR_H
#define SENSOR_H

/**
 * @file Sensor.h
 * @brief Model stanowiska pomiarowego (czujnika).
 */

#include <string>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

/// Informacje o mierzonym parametrze
struct Param {
    std::string paramName;     ///< Pelna nazwa parametru
    std::string paramFormula;  ///< Symbol (np. "PM10")
    std::string paramCode;     ///< Kod parametru
    int idParam = 0;           ///< Identyfikator parametru
};

/// Stanowisko pomiarowe (czujnik)
struct Sensor {
    int id = 0;          ///< Identyfikator stanowiska
    int stationId = 0;   ///< Identyfikator stacji
    Param param;         ///< Mierzony parametr
};

inline void from_json(const json& j, Param& p) {
    p.paramName = j.value("paramName", "");
    p.paramFormula = j.value("paramFormula", "");
    p.paramCode = j.value("paramCode", "");
    p.idParam = j.value("idParam", 0);
}
inline void to_json(json& j, const Param& p) {
    j = json{{"paramName", p.paramName}, {"paramFormula", p.paramFormula},
             {"paramCode", p.paramCode}, {"idParam", p.idParam}};
}
inline void from_json(const json& j, Sensor& s) {
    s.id = j.value("id", 0);
    s.stationId = j.value("stationId", 0);
    if (j.contains("param") && j["param"].is_object()) s.param = j["param"].get<Param>();
}
inline void to_json(json& j, const Sensor& s) {
    j = json{{"id", s.id}, {"stationId", s.stationId}, {"param", s.param}};
}

#endif // SENSOR_H
