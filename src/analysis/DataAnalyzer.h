#ifndef DATAANALYZER_H
#define DATAANALYZER_H

/**
 * @file DataAnalyzer.h
 * @brief Modul analizy danych pomiarowych.
 */

#include <string>
#include <vector>
#include <optional>
#include "models/MeasurementData.h"

/// Wynik analizy
struct AnalysisResult {
    double minValue = 0, maxValue = 0, avgValue = 0;
    std::string minDate, maxDate;
    int validCount = 0, totalCount = 0;
    double trendSlope = 0;
    std::string trendDescription;
};

/**
 * @class DataAnalyzer
 * @brief Oblicza statystyki danych pomiarowych.
 */
class DataAnalyzer {
public:
    std::optional<AnalysisResult> analyze(const MeasurementData& data) const;
    std::pair<double, std::string> findMin(const std::vector<MeasurementValue>& v) const;
    std::pair<double, std::string> findMax(const std::vector<MeasurementValue>& v) const;
    double calculateAverage(const std::vector<MeasurementValue>& v) const;
    double calculateTrend(const std::vector<MeasurementValue>& v) const;
    std::string describeTrend(double slope) const;
    MeasurementData filterByDateRange(const MeasurementData& data,
                                      const std::string& from, const std::string& to) const;
};

#endif // DATAANALYZER_H
