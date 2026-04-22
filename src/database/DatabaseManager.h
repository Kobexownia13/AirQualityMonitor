#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

/**
 * @file DatabaseManager.h
 * @brief Menedzer lokalnej bazy danych JSON.
 */

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "models/Station.h"
#include "models/Sensor.h"
#include "models/MeasurementData.h"

using json = nlohmann::json;

/**
 * @class DatabaseManager
 * @brief Zapis/odczyt danych do plikow JSON w katalogu data/.
 */
class DatabaseManager {
public:
    explicit DatabaseManager(const std::string& basePath = "data");

    bool saveStations(const std::vector<Station>& stations);
    std::vector<Station> loadStations();
    bool hasStations() const;

    bool saveSensors(int stationId, const std::vector<Sensor>& sensors);
    std::vector<Sensor> loadSensors(int stationId);
    bool hasSensors(int stationId) const;

    bool saveMeasurementData(int sensorId, const MeasurementData& data);
    MeasurementData loadMeasurementData(int sensorId);
    bool hasMeasurementData(int sensorId) const;

private:
    std::string m_basePath;
    bool saveJsonToFile(const std::string& filename, const json& data);
    json loadJsonFromFile(const std::string& filename);
    bool fileExists(const std::string& filename) const;
    std::string filePath(const std::string& filename) const;
};

#endif // DATABASEMANAGER_H
