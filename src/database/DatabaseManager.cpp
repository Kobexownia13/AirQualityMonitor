/**
 * @file DatabaseManager.cpp
 * @brief Implementacja menedzera bazy danych JSON.
 */
#include "database/DatabaseManager.h"
#include <fstream>
#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;

DatabaseManager::DatabaseManager(const std::string& basePath) : m_basePath(basePath) {
    if (!fs::exists(m_basePath)) fs::create_directories(m_basePath);
}

std::string DatabaseManager::filePath(const std::string& fn) const { return m_basePath + "/" + fn; }
bool DatabaseManager::fileExists(const std::string& fn) const { return fs::exists(filePath(fn)); }

bool DatabaseManager::saveJsonToFile(const std::string& fn, const json& data) {
    std::ofstream f(filePath(fn));
    if (!f.is_open()) return false;
    f << data.dump(4);
    return true;
}

json DatabaseManager::loadJsonFromFile(const std::string& fn) {
    std::ifstream f(filePath(fn));
    if (!f.is_open()) return json();
    try { json d; f >> d; return d; }
    catch (...) { return json(); }
}

bool DatabaseManager::saveStations(const std::vector<Station>& s) { return saveJsonToFile("stations.json", json(s)); }
std::vector<Station> DatabaseManager::loadStations() {
    auto d = loadJsonFromFile("stations.json");
    return d.is_array() ? d.get<std::vector<Station>>() : std::vector<Station>{};
}
bool DatabaseManager::hasStations() const { return fileExists("stations.json"); }

bool DatabaseManager::saveSensors(int id, const std::vector<Sensor>& s) {
    return saveJsonToFile("sensors_" + std::to_string(id) + ".json", json(s));
}
std::vector<Sensor> DatabaseManager::loadSensors(int id) {
    auto d = loadJsonFromFile("sensors_" + std::to_string(id) + ".json");
    return d.is_array() ? d.get<std::vector<Sensor>>() : std::vector<Sensor>{};
}
bool DatabaseManager::hasSensors(int id) const { return fileExists("sensors_" + std::to_string(id) + ".json"); }

bool DatabaseManager::saveMeasurementData(int id, const MeasurementData& data) {
    return saveJsonToFile("measurements_" + std::to_string(id) + ".json", json(data));
}
MeasurementData DatabaseManager::loadMeasurementData(int id) {
    auto d = loadJsonFromFile("measurements_" + std::to_string(id) + ".json");
    return d.is_object() ? d.get<MeasurementData>() : MeasurementData{};
}
bool DatabaseManager::hasMeasurementData(int id) const { return fileExists("measurements_" + std::to_string(id) + ".json"); }
