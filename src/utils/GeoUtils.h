#ifndef GEOUTILS_H
#define GEOUTILS_H

/**
 * @file GeoUtils.h
 * @brief Funkcje geograficzne - obliczanie odleglosci, geokodowanie.
 */

#include <cmath>
#include <cctype>
#include <string>
#include <vector>
#include <algorithm>
#include "models/Station.h"

// Czasem M_PI nie jest dostepne, wiec mamy awaryjna definicje.
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
 * @class GeoUtils
 * @brief Proste narzedzia geograficzne dla stacji.
 */
class GeoUtils {
public:
    /// Sredni promien Ziemi w kilometrach.
    static constexpr double EARTH_RADIUS_KM = 6371.0;

    /**
     * @brief Odleglosc miedzy dwoma punktami liczona wzorem haversine.
     * @param lat1 Szerokosc geograficzna punktu 1 (stopnie)
     * @param lon1 Dlugosc geograficzna punktu 1 (stopnie)
     * @param lat2 Szerokosc geograficzna punktu 2 (stopnie)
     * @param lon2 Dlugosc geograficzna punktu 2 (stopnie)
     * @return Odleglosc w kilometrach.
     */
    static double haversineDistance(double lat1, double lon1, double lat2, double lon2) {
        const double dLat = toRadians(lat2 - lat1);
        const double dLon = toRadians(lon2 - lon1);

        const double a = std::sin(dLat / 2) * std::sin(dLat / 2) +
                         std::cos(toRadians(lat1)) * std::cos(toRadians(lat2)) *
                         std::sin(dLon / 2) * std::sin(dLon / 2);

        const double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
        return EARTH_RADIUS_KM * c;
    }

    /**
     * @brief Odleglosc od stacji do punktu (lat/lon).
     */
    static double distanceToStation(const Station& station, double lat, double lon) {
        return haversineDistance(lat, lon, station.gegrLat, station.gegrLon);
    }

    /**
     * @brief Stacja wraz z policzona odlegloscia.
     */
    struct StationWithDistance {
        Station station;
        double distanceKm;
    };

    /**
     * @brief Wyszukuje stacje w promieniu od punktu.
     * @return Lista od najblizszej do najdalszej.
     */
    static std::vector<StationWithDistance> findStationsInRadius(
        const std::vector<Station>& stations,
        double lat, double lon, double radiusKm)
    {
        std::vector<StationWithDistance> result;
        for (const auto& station : stations) {
            const double dist = distanceToStation(station, lat, lon);
            if (dist <= radiusKm) {
                result.push_back({station, dist});
            }
        }
        std::sort(result.begin(), result.end(),
            [](const auto& a, const auto& b) { return a.distanceKm < b.distanceKm; });
        return result;
    }

    /**
     * @brief Zwraca N najblizszych stacji.
     */
    static std::vector<StationWithDistance> findNearestStations(
        const std::vector<Station>& stations,
        double lat, double lon, size_t count)
    {
        std::vector<StationWithDistance> all;
        all.reserve(stations.size());
        for (const auto& station : stations) {
            all.push_back({station, distanceToStation(station, lat, lon)});
        }
        std::sort(all.begin(), all.end(),
            [](const auto& a, const auto& b) { return a.distanceKm < b.distanceKm; });
        if (all.size() > count) all.resize(count);
        return all;
    }

    /// Prosta lokalizacja znana lokalnie (offline).
    struct KnownLocation {
        std::string name;
        double lat;
        double lon;
    };

    /**
     * @brief Proste geokodowanie offline po nazwie miejscowosci.
     *
     * Kolejnosc dopasowania:
     * 1. Dokladna nazwa.
     * 2. Dopasowanie po poczatku.
     * 3. Dopasowanie po podciagu (od min. 3 znakow).
     */
    static bool geocodeSimple(const std::string& query, double& outLat, double& outLon) {
        static const std::vector<KnownLocation> locations = {
            {"Warszawa", 52.2297, 21.0122},
            {"Krakow", 50.0647, 19.9450},
            {"Lodz", 51.7592, 19.4560},
            {"Wroclaw", 51.1079, 17.0385},
            {"Poznan", 52.4064, 16.9252},
            {"Gdansk", 54.3520, 18.6466},
            {"Szczecin", 53.4285, 14.5528},
            {"Bydgoszcz", 53.1235, 18.0084},
            {"Lublin", 51.2465, 22.5684},
            {"Bialystok", 53.1325, 23.1688},
            {"Katowice", 50.2649, 19.0238},
            {"Rzeszow", 50.0412, 21.9991},
            {"Torun", 53.0138, 18.5984},
            {"Kielce", 50.8661, 20.6286},
            {"Olsztyn", 53.7784, 20.4801},
            {"Opole", 50.6751, 17.9213},
            {"Gorzow", 52.7368, 15.2288},
            {"Zielona Gora", 51.9356, 15.5062},
            {"Gdynia", 54.5189, 18.5305},
            {"Czestochowa", 50.8118, 19.1203},
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
            {"Plock", 52.5463, 19.7065},
            {"Walbrzych", 50.7714, 16.2843},
            {"Legnica", 51.2070, 16.1619},
            {"Zakopane", 49.2992, 19.9496},
            {"Sopot", 54.4418, 18.5601},
            {"Polanka", 52.4064, 16.9252},
            {"Politechnika", 52.2208, 21.0106},
        };

        const std::string q = normalize(query);
        if (q.empty()) return false;

        // 1) Dokladna nazwa po normalizacji.
        for (const auto& loc : locations) {
            if (normalize(loc.name) == q) {
                outLat = loc.lat;
                outLon = loc.lon;
                return true;
            }
        }
        // 2) Nazwa zaczyna sie od zapytania albo zapytanie od nazwy.
        for (const auto& loc : locations) {
            const std::string n = normalize(loc.name);
            if (q.rfind(n, 0) == 0 || n.rfind(q, 0) == 0) {
                outLat = loc.lat;
                outLon = loc.lon;
                return true;
            }
        }
        // 3) Podciag, ale od 3 znakow, zeby unikac przypadkowych trafien.
        if (q.size() >= 3) {
            for (const auto& loc : locations) {
                const std::string n = normalize(loc.name);
                if (n.find(q) != std::string::npos || q.find(n) != std::string::npos) {
                    outLat = loc.lat;
                    outLon = loc.lon;
                    return true;
                }
            }
        }
        return false;
    }

private:
    static double toRadians(double degrees) {
        return degrees * M_PI / 180.0;
    }

    // Normalizacja tekstu do porownan bez roznicy wielkosci liter i polskich znakow.
    static std::string normalize(const std::string& s) {
        std::string out;
        out.reserve(s.size());
        for (size_t i = 0; i < s.size(); ) {
            const unsigned char c = static_cast<unsigned char>(s[i]);

            // ASCII: zwykle male litery.
            if (c < 0x80) {
                out.push_back(static_cast<char>(std::tolower(c)));
                ++i;
                continue;
            }

            // Obsluga najczestszych jednobajtowych kodowan (ANSI).
            if (const char* rep = polishSingleByteFold(c)) {
                out.append(rep);
                ++i;
                continue;
            }

            // Dwubajtowe znaki UTF-8.
            if ((c & 0xE0) == 0xC0 && i + 1 < s.size()) {
                const unsigned char c2 = static_cast<unsigned char>(s[i + 1]);
                const char* rep = polishFold(c, c2);
                if (rep) {
                    out.append(rep);
                    i += 2;
                    continue;
                }
                // Nieznany znak dwubajtowy pomijamy.
                i += 2;
                continue;
            }

            // Trojbajtowe i dluzsze znaki pomijamy.
            ++i;
        }
        return out;
    }

    // Zamiana jednobajtowych polskich znakow na litery ASCII.
    static const char* polishSingleByteFold(unsigned char c) {
        switch (c) {
        case 0xA5: case 0xB9: case 0xA1: case 0xB1: return "a";
        case 0xC6: case 0xE6: return "c";
        case 0xCA: case 0xEA: return "e";
        case 0xA3: case 0xB3: return "l";
        case 0xD1: case 0xF1: return "n";
        case 0xD3: case 0xF3: return "o";
        case 0x8C: case 0x9C: case 0xA6: case 0xB6: return "s";
        case 0x8F: case 0x9F: case 0xAC: case 0xBC: case 0xAF: case 0xBF: return "z";
        default: return nullptr;
        }
    }

    // Zamiana popularnych polskich znakow UTF-8 na litery ASCII.
    static const char* polishFold(unsigned char c1, unsigned char c2) {
        if (c1 == 0xC4) {
            if (c2 == 0x84 || c2 == 0x85) return "a";
            if (c2 == 0x86 || c2 == 0x87) return "c";
            if (c2 == 0x98 || c2 == 0x99) return "e";
        }
        if (c1 == 0xC5) {
            if (c2 == 0x81 || c2 == 0x82) return "l";
            if (c2 == 0x83 || c2 == 0x84) return "n";
            if (c2 == 0x9A || c2 == 0x9B) return "s";
            if (c2 == 0xB9 || c2 == 0xBA) return "z";
            if (c2 == 0xBB || c2 == 0xBC) return "z";
        }
        if (c1 == 0xC3) {
            if (c2 == 0x93 || c2 == 0xB3) return "o";
        }
        return nullptr;
    }
};

#endif // GEOUTILS_H

