/**
 * @file test_analyzer.cpp
 * @brief Testy jednostkowe analizatora danych.
 */
#include <gtest/gtest.h>
#include "analysis/DataAnalyzer.h"

class AnalyzerTest : public ::testing::Test {
protected:
    DataAnalyzer analyzer;
    MeasurementData makeData(std::vector<std::pair<std::string, double>> vals) {
        MeasurementData d; d.key = "TEST";
        for (auto& [date, val] : vals) {
            MeasurementValue mv; mv.date = date;
            if (val < 0) mv.value = std::nullopt; else mv.value = val;
            d.values.push_back(mv);
        }
        return d;
    }
};

TEST_F(AnalyzerTest, EmptyReturnsNullopt) {
    MeasurementData d; d.key = "X";
    EXPECT_FALSE(analyzer.analyze(d).has_value());
}

TEST_F(AnalyzerTest, AllNullsReturnsNullopt) {
    auto d = makeData({{"a", -1}, {"b", -1}});
    EXPECT_FALSE(analyzer.analyze(d).has_value());
}

TEST_F(AnalyzerTest, MinMax) {
    auto d = makeData({{"08:00", 10}, {"09:00", 25}, {"10:00", 5}, {"11:00", 20}});
    auto r = analyzer.analyze(d);
    ASSERT_TRUE(r.has_value());
    EXPECT_NEAR(r->minValue, 5.0, 0.01);
    EXPECT_NEAR(r->maxValue, 25.0, 0.01);
}

TEST_F(AnalyzerTest, Average) {
    auto d = makeData({{"a", 10}, {"b", 20}, {"c", 30}});
    auto r = analyzer.analyze(d);
    EXPECT_NEAR(r->avgValue, 20.0, 0.01);
}

TEST_F(AnalyzerTest, SkipsNulls) {
    auto d = makeData({{"a", 10}, {"b", -1}, {"c", 30}});
    auto r = analyzer.analyze(d);
    EXPECT_EQ(r->totalCount, 3);
    EXPECT_EQ(r->validCount, 2);
    EXPECT_NEAR(r->avgValue, 20.0, 0.01);
}

TEST_F(AnalyzerTest, TrendIncreasing) {
    auto d = makeData({{"a", 10}, {"b", 20}, {"c", 30}, {"d", 40}});
    auto r = analyzer.analyze(d);
    EXPECT_GT(r->trendSlope, 0);
    EXPECT_NE(r->trendDescription.find("rosnaca"), std::string::npos);
}

TEST_F(AnalyzerTest, TrendDecreasing) {
    auto d = makeData({{"a", 40}, {"b", 30}, {"c", 20}, {"d", 10}});
    auto r = analyzer.analyze(d);
    EXPECT_LT(r->trendSlope, 0);
}

TEST_F(AnalyzerTest, TrendStable) {
    auto d = makeData({{"a", 20}, {"b", 20}, {"c", 20}});
    auto r = analyzer.analyze(d);
    EXPECT_NEAR(r->trendSlope, 0.0, 0.01);
    EXPECT_NE(r->trendDescription.find("stabilny"), std::string::npos);
}

TEST_F(AnalyzerTest, FilterByDate) {
    auto d = makeData({{"2025-01-01 08:00:00", 10}, {"2025-01-01 09:00:00", 20},
                       {"2025-01-01 10:00:00", 30}, {"2025-01-01 11:00:00", 40}});
    auto f = analyzer.filterByDateRange(d, "2025-01-01 09:00:00", "2025-01-01 10:00:00");
    EXPECT_EQ(f.values.size(), 2u);
}

TEST_F(AnalyzerTest, AverageEmpty) {
    std::vector<MeasurementValue> empty;
    EXPECT_NEAR(analyzer.calculateAverage(empty), 0.0, 0.01);
}
