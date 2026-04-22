#ifndef GEOUTILS_H
#define GEOUTILS_H

/**
 * @file GeoUtils.h
 * @brief Funkcje geograficzne - obliczanie odleglosci, geokodowanie.
 */

#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include "models/Station.h"

/**
 * @class GeoUtils
 * @brief Narzedzia geograficzne - odleglosc haversine, wyszukiwanie w promieniu.
 */
class GeoUtils {
public:
    /// Promien Ziemi w kilometrach
    static constexpr double EARTH_RADIUS_KM = 6371.0;

    /**
     * @brief Oblicza odleglosc miedzy dwoma punktami (wzor haversine).
     * @param lat1 Szerokosc geograficzna punktu 1 (stopnie)
     * @param lon1 Dlugosc geograficzna punktu 1 (stopnie)
     * @param lat2 Szerokosc geograficzna punktu 2 (stopnie)
     * @param lon2 Dlugosc geograficzna punktu 2 (stopnie)
     * @return Odleglosc w kilometrach
     */
    static double haversineDistance(double lat1, double lon1, double lat2, double lon2) {
        double dLat = toRadians(lat2 - lat1);
        double dLon = toRadians(lon2 - lon1);
        
        double a = std::sin(dLat / 2) * std::sin(dLat / 2) +
                   std::cos(toRadians(lat1)) * std::cos(toRadians(lat2)) *
                   std::sin(dLon / 2) * std::sin(dLon / 2);
        
        double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
        return EARTH_RADIUS_KM * c;
    }

    /**
     * @brief Oblicza odleglosc od stacji do podanego punktu.
     * @param station Stacja pomiarowa
     * @param lat Szerokosc geograficzna punktu
     * @param lon Dlugosc geograficzna punktu
     * @return Odleglosc w kilometrach
     */
    static double distanceToStation(const Station& station, double lat, double lon) {
        return haversineDistance(lat, lon, station.gegrLat, station.gegrLon);
    }

    /**
     * @brief Struktura przechowujaca stacje z odlegloscia.
     */
    struct StationWithDistance {
        Station station;
        double distanceKm;
    };

    /**
     * @brief Wyszukuje stacje w zadanym promieniu od punktu.
     * @param stations Lista wszystkich stacji
     * @param lat Szerokosc geograficzna punktu centralnego
     * @param lon Dlugosc geograficzna punktu centralnego
     * @param radiusKm Promien wyszukiwania w kilometrach
     * @return Lista stacji z odleglosciami, posortowana od najblizszej
     */
    static std::vector<StationWithDistance> findStationsInRadius(
        const std::vector<Station>& stations,
        double lat, double lon, double radiusKm) 
    {
        std::vector<StationWithDistance> result;
        
        for (const auto& station : stations) {
            double dist = distanceToStation(station, lat, lon);
            if (dist <= radiusKm) {
                result.push_back({station, dist});
            }
        }
        
        // Sortuj od najblizszej
        std::sort(result.begin(), result.end(),
            [](const auto& a, const auto& b) { return a.distanceKm < b.distanceKm; });
        
        return result;
    }

    /**
     * @brief Znajduje najblizsze N stacji.
     * @param stations Lista wszystkich stacji
     * @param lat Szerokosc geograficzna punktu
     * @param lon Dlugosc geograficzna punktu
     * @param count Liczba stacji do zwrocenia
     * @return Lista najblizszych stacji z odleglosciami
     */
    static std::vector<StationWithDistance> findNearestStations(
        const std::vector<Station>& stations,
        double lat, double lon, size_t count)
    {
        std::vector<StationWithDistance> all;
        
        for (const auto& station : stations) {
            double dist = distanceToStation(station, lat, lon);
            all.push_back({station, dist});
        }
        
        std::sort(all.begin(), all.end(),
            [](const auto& a, const auto& b) { return a.distanceKm < b.distanceKm; });
        
        if (all.size() > count) {
            all.resize(count);
        }
        
        return all;
    }

    /// Znane lokalizacje w Polsce (dla szybkiego wyszukiwania)
    struct KnownLocation {
        std::string name;
        double lat;
        double lon;
    };

    /**
     * @brief Zwraca wspolrzedne znanej lokalizacji (uproszczone geokodowanie).
     * @param query Nazwa lokalizacji (np. "Poznan", "Warszawa")
     * @param outLat Wyjsciowa szerokosc geograficzna
     * @param outLon Wyjsciowa dlugosc geograficzna
     * @return true jesli znaleziono lokalizacje
     */
    static bool geocodeSimple(const std::string& query, double& outLat, double& outLon) {
        static const std::vector<KnownLocation> locations = {
            {"Warszawa", 52.2297, 21.0122},
            {"Krakow", 50.0647, 19.9450},
            {"Kraków", 50.0647, 19.9450},
            {"Lodz", 51.7592, 19.4560},
            {"Łódź", 51.7592, 19.4560},
            {"Wroclaw", 51.1079, 17.0385},
            {"Wrocław", 51.1079, 17.0385},
            {"Poznan", 52.4064, 16.9252},
            {"Poznań", 52.4064, 16.9252},
            {"Gdansk", 54.3520, 18.6466},
            {"Gdańsk", 54.3520, 18.6466},
            {"Szczecin", 53.4285, 14.5528},
            {"Bydgoszcz", 53.1235, 18.0084},
            {"Lublin", 51.2465, 22.5684},
            {"Bialystok", 53.1325, 23.1688},
            {"Białystok", 53.1325, 23.1688},
            {"Katowice", 50.2649, 19.0238},
            {"Rzeszow", 50.0412, 21.9991},
            {"Rzeszów", 50.0412, 21.9991},
            {"Torun", 53.0138, 18.5984},
            {"Toruń", 53.0138, 18.5984},
            {"Kielce", 50.8661, 20.6286},
            {"Olsztyn", 53.7784, 20.4801},
            {"Opole", 50.6751, 17.9213},
            {"Gorzow", 52.7368, 15.2288},
            {"Gorzów", 52.7368, 15.2288},
            {"Zielona Gora", 51.9356, 15.5062},
            {"Zielona Góra", 51.9356, 15.5062},
            {"Gdynia", 54.5189, 18.5305},
            {"Czestochowa", 50.8118, 19.1203},
            {"Częstochowa", 50.8118, 19.1203},
            {"Radom", 51.4027, 21.1471},
            {"Sosnowiec", 50.2863, 19.1041},
            {"Gliwice", 50.2945, 18.6714},
            {"Zabrze", 50.3249, 18.7857},
            {"Bytom", 50.3484, 18.9156},
            {"Ruda Slaska", 50.2558, 18.8556},
            {"Rybnik", 50.1022, 18.5463},
            {"Tychy", 50.1308, 18.9984},
            {"Dabrowa Gornicza", 50.3217, 19.1947},
            {"Elblag", 54.1561, 19.4045},
            {"Płock", 52.5463, 19.7065},
            {"Walbrzych", 50.7714, 16.2843},
            {"Wałbrzych", 50.7714, 16.2843},
            {"Legnica", 51.2070, 16.1619},
            {"Zakopane", 49.2992, 19.9496},
            {"Sopot", 54.4418, 18.5601},
            {"Polanka", 52.4064, 16.9252},  // Poznań - Polanka
            {"Politechnika", 52.2208, 21.0106},  // Warszawa Politechnika
        };
        
        std::string queryLower = query;
        std::transform(queryLower.begin(), queryLower.end(), queryLower.begin(), ::tolower);
        
        for (const auto& loc : locations) {
            std::string nameLower = loc.name;
            std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
            
            if (queryLower.find(nameLower) != std::string::npos ||
                nameLower.find(queryLower) != std::string::npos) {
                outLat = loc.lat;
                outLon = loc.lon;
                return true;
            }
        }
        
        return false;
    }

private:
    static double toRadians(double degrees) {
        return degrees * M_PI / 180.0;
    }
};

#endif // GEOUTILS_H
