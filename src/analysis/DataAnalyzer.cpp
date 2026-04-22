/**
 * @file DataAnalyzer.cpp
 * @brief Implementacja analizy danych pomiarowych.
 */
#include "analysis/DataAnalyzer.h"
#include <algorithm>
#include <cmath>
#include <limits>

std::optional<AnalysisResult> DataAnalyzer::analyze(const MeasurementData& data) const {
    auto valid = data.validValues();
    if (valid.empty()) return std::nullopt;

    AnalysisResult r;
    r.totalCount = (int)data.values.size();
    r.validCount = (int)valid.size();
    auto [minV, minD] = findMin(valid); r.minValue = minV; r.minDate = minD;
    auto [maxV, maxD] = findMax(valid); r.maxValue = maxV; r.maxDate = maxD;
    r.avgValue = calculateAverage(valid);
    r.trendSlope = calculateTrend(valid);
    r.trendDescription = describeTrend(r.trendSlope);
    return r;
}

std::pair<double, std::string> DataAnalyzer::findMin(const std::vector<MeasurementValue>& v) const {
    if (v.empty()) return {0, ""};
    auto it = std::min_element(v.begin(), v.end(),
        [](const auto& a, const auto& b) { return a.value.value() < b.value.value(); });
    return {it->value.value(), it->date};
}

std::pair<double, std::string> DataAnalyzer::findMax(const std::vector<MeasurementValue>& v) const {
    if (v.empty()) return {0, ""};
    auto it = std::max_element(v.begin(), v.end(),
        [](const auto& a, const auto& b) { return a.value.value() < b.value.value(); });
    return {it->value.value(), it->date};
}

double DataAnalyzer::calculateAverage(const std::vector<MeasurementValue>& v) const {
    if (v.empty()) return 0;
    double sum = 0;
    for (const auto& m : v) sum += m.value.value();
    return sum / v.size();
}

double DataAnalyzer::calculateTrend(const std::vector<MeasurementValue>& v) const {
    if (v.size() < 2) return 0;
    int n = (int)v.size();
    double sx = 0, sy = 0, sxy = 0, sx2 = 0;
    for (int i = 0; i < n; i++) {
        double x = i, y = v[i].value.value();
        sx += x; sy += y; sxy += x * y; sx2 += x * x;
    }
    double denom = n * sx2 - sx * sx;
    if (std::abs(denom) < 1e-12) return 0;
    return (n * sxy - sx * sy) / denom;
}

std::string DataAnalyzer::describeTrend(double slope) const {
    if (slope > 0.01) return "Tendencja rosnaca";
    if (slope < -0.01) return "Tendencja malejaca";
    return "Trend stabilny";
}

MeasurementData DataAnalyzer::filterByDateRange(
    const MeasurementData& data, const std::string& from, const std::string& to) const {
    MeasurementData filtered;
    filtered.key = data.key;
    for (const auto& v : data.values)
        if (v.date >= from && v.date <= to) filtered.values.push_back(v);
    return filtered;
}
