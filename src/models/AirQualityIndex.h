#ifndef AIRQUALITYINDEX_H
#define AIRQUALITYINDEX_H

/**
 * @file AirQualityIndex.h
 * @brief Model indeksu jakosci powietrza.
 */

#include <string>

/// Poziom indeksu jakosci
struct IndexLevel {
    int id = -1;
    std::string name;

    /// Kolor odpowiadajacy poziomowi
    std::string color() const {
        switch (id) {
            case 0: return "#00b050";
            case 1: return "#92d050";
            case 2: return "#ffc000";
            case 3: return "#ff6600";
            case 4: return "#ff0000";
            case 5: return "#990000";
            default: return "#808080";
        }
    }
    bool isValid() const { return id >= 0 && id <= 5; }
};

/// Indeks jakosci powietrza stacji
struct AirQualityIndex {
    int stationId = 0;
    std::string calcDate;
    IndexLevel overall;
    // W API v1 mamy indeksy dla: NO2, SO2, O3, PM10 i PM2.5.
    // CO tutaj celowo pomijamy, bo API go nie zwraca.
    IndexLevel pm10, pm25, no2, so2, o3;
};

#endif // AIRQUALITYINDEX_H
