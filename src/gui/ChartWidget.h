#ifndef CHARTWIDGET_H
#define CHARTWIDGET_H

/**
 * @file ChartWidget.h
 * @brief Widget wykresu danych pomiarowych.
 */

#include <QWidget>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>
#include <QDateTimeEdit>
#include <QPushButton>
#include <QLabel>
#include "models/MeasurementData.h"

/**
 * @class ChartWidget
 * @brief Wykres liniowy z filtrowaniem po dacie.
 */
class ChartWidget : public QWidget {
    Q_OBJECT
public:
    explicit ChartWidget(QWidget* parent = nullptr);
    void setData(const MeasurementData& data, const QString& paramName);
    void clearChart();

private slots:
    void onFilterClicked();
    void onResetClicked();

private:
    QChartView* m_chartView;
    QChart* m_chart;
    QDateTimeEdit* m_dateFrom;
    QDateTimeEdit* m_dateTo;
    QPushButton* m_filterBtn;
    QPushButton* m_resetBtn;
    QLabel* m_infoLabel;
    MeasurementData m_fullData;
    QString m_paramName;

    void updateChart(const MeasurementData& data);
    void setupUI();
};

#endif // CHARTWIDGET_H
