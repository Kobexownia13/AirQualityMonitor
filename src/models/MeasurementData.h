#ifndef MEASUREMENTDATA_H
#define MEASUREMENTDATA_H

/**
 * @file MeasurementData.h
 * @brief Model danych pomiarowych.
 */

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

/// Pojedynczy pomiar
struct MeasurementValue {
    std::string date;               ///< Data i czas pomiaru
    std::optional<double> value;    ///< Wartosc (nullopt = brak)
};

/// Dane pomiarowe z jednego czujnika
struct MeasurementData {
    std::string key;                          ///< Kod parametru
    std::vector<MeasurementValue> values;     ///< Lista pomiarow

    /// Zwraca tylko pomiary z wartosciami
    std::vector<MeasurementValue> validValues() const {
        std::vector<MeasurementValue> r;
        for (const auto& v : values)
            if (v.value.has_value()) r.push_back(v);
        return r;
    }
};

inline void from_json(const json& j, MeasurementValue& mv) {
    mv.date = j.value("date", "");
    if (j.contains("value") && !j["value"].is_null()) mv.value = j["value"].get<double>();
    else mv.value = std::nullopt;
}
inline void to_json(json& j, const MeasurementValue& mv) {
    j["date"] = mv.date;
    if (mv.value.has_value()) j["value"] = mv.value.value();
    else j["value"] = nullptr;
}
inline void from_json(const json& j, MeasurementData& md) {
    md.key = j.value("key", "");
    if (j.contains("values") && j["values"].is_array())
        md.values = j["values"].get<std::vector<MeasurementValue>>();
}
inline void to_json(json& j, const MeasurementData& md) {
    j = json{{"key", md.key}, {"values", md.values}};
}

#endif // MEASUREMENTDATA_H
