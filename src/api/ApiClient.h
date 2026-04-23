#ifndef APICLIENT_H
#define APICLIENT_H

/**
 * @file ApiClient.h
 * @brief Klient REST API GIOS (wersja v1, JSON-LD).
 */

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include "models/Station.h"
#include "models/Sensor.h"
#include "models/MeasurementData.h"
#include "models/AirQualityIndex.h"

using json = nlohmann::json;

/**
 * @class ApiClient
 * @brief Singleton - klient API GIOS.
 */
class ApiClient {
public:
    static ApiClient& instance();

    /// Pobiera wszystkie stacje (obsluguje stronicowanie)
    std::vector<Station> fetchAllStations();

    /// Pobiera czujniki stacji
    std::vector<Sensor> fetchSensors(int stationId);

    /// Pobiera dane pomiarowe czujnika
    MeasurementData fetchMeasurementData(int sensorId);

    /// Pobiera indeks jakosci powietrza
    AirQualityIndex fetchAirQualityIndex(int stationId);

private:
    ApiClient() = default;
    ApiClient(const ApiClient&) = delete;
    ApiClient& operator=(const ApiClient&) = delete;

    static constexpr const char* BASE_URL = "https://api.gios.gov.pl/pjp-api/v1/rest";

    json makeGetRequest(const std::string& endpoint);

    /// Bezpieczne wyciaganie stringa z JSON (obsluguje null)
    static std::string safeStr(const json& j, const std::string& key);
};

#endif // APICLIENT_H
