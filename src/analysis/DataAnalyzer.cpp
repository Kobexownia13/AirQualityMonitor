/**
 * @file DataAnalyzer.cpp
 * @brief Implementacja analizy danych pomiarowych.
 */
#include "analysis/DataAnalyzer.h"
#include <algorithm>
#include <cmath>
#include <limits>

// Glowna analiza jednego zestawu pomiarow.
// Zwracamy nullopt, jesli nie ma ani jednego poprawnego punktu.
// Przed liczeniem trendu ustawiamy dane chronologicznie (od najstarszego).
std::optional<AnalysisResult> DataAnalyzer::analyze(const MeasurementData& data) const {
    auto valid = data.validValues();
    if (valid.empty()) return std::nullopt;

    // Daty maja format "YYYY-MM-DD HH:MM:SS", wiec zwykle porownanie stringow wystarcza.
    std::sort(valid.begin(), valid.end(),
        [](const MeasurementValue& a, const MeasurementValue& b) {
            return a.date < b.date;
        });

    AnalysisResult r;
    r.totalCount = static_cast<int>(data.values.size());
    r.validCount = static_cast<int>(valid.size());

    auto [minV, minD] = findMin(valid);
    r.minValue = minV;
    r.minDate = minD;

    auto [maxV, maxD] = findMax(valid);
    r.maxValue = maxV;
    r.maxDate = maxD;

    r.avgValue = calculateAverage(valid);
    r.trendSlope = calculateTrend(valid);
    r.trendDescription = describeTrend(r.trendSlope);
    return r;
}

// Minimalna wartosc + data jej wystapienia.
std::pair<double, std::string> DataAnalyzer::findMin(const std::vector<MeasurementValue>& v) const {
    if (v.empty()) return {0, ""};
    auto it = std::min_element(v.begin(), v.end(),
        [](const auto& a, const auto& b) { return a.value.value() < b.value.value(); });
    return {it->value.value(), it->date};
}

// Maksymalna wartosc + data jej wystapienia.
std::pair<double, std::string> DataAnalyzer::findMax(const std::vector<MeasurementValue>& v) const {
    if (v.empty()) return {0, ""};
    auto it = std::max_element(v.begin(), v.end(),
        [](const auto& a, const auto& b) { return a.value.value() < b.value.value(); });
    return {it->value.value(), it->date};
}

// Klasyczna srednia arytmetyczna.
double DataAnalyzer::calculateAverage(const std::vector<MeasurementValue>& v) const {
    if (v.empty()) return 0;
    double sum = 0;
    for (const auto& m : v) sum += m.value.value();
    return sum / static_cast<double>(v.size());
}

// Trend liczony jako nachylenie prostej regresji liniowej.
// Zakladamy, ze v jest juz posortowane po dacie rosnaco.
double DataAnalyzer::calculateTrend(const std::vector<MeasurementValue>& v) const {
    if (v.size() < 2) return 0;
    const int n = static_cast<int>(v.size());

    double sx = 0, sy = 0, sxy = 0, sx2 = 0;
    for (int i = 0; i < n; i++) {
        const double x = i;
        const double y = v[i].value.value();
        sx += x;
        sy += y;
        sxy += x * y;
        sx2 += x * x;
    }

    const double denom = n * sx2 - sx * sx;
    // Dla praktycznie zerowego mianownika nie ma sensownego trendu.
    if (std::abs(denom) < 1e-12) return 0;
    return (n * sxy - sx * sy) / denom;
}

// Prosty opis trendu na bazie nachylenia.
std::string DataAnalyzer::describeTrend(double slope) const {
    if (slope > 0.01) return "Tendencja rosnaca";
    if (slope < -0.01) return "Tendencja malejaca";
    return "Trend stabilny";
}

// Zwraca pomiary z zakresu [from, to].
MeasurementData DataAnalyzer::filterByDateRange(
    const MeasurementData& data, const std::string& from, const std::string& to) const {
    MeasurementData filtered;
    filtered.key = data.key;
    for (const auto& v : data.values) {
        if (v.date >= from && v.date <= to) {
            filtered.values.push_back(v);
        }
    }
    return filtered;
}
