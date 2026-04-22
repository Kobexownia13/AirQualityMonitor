/**
 * @file ChartWidget.cpp
 * @brief Implementacja widgetu wykresu.
 */
#include "gui/ChartWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <algorithm>
#include <cmath>

ChartWidget::ChartWidget(QWidget* parent) : QWidget(parent) { setupUI(); }

void ChartWidget::setupUI() {
    auto* lay = new QVBoxLayout(this);

    auto* filterGroup = new QGroupBox("Zakres dat");
    auto* fLay = new QHBoxLayout(filterGroup);
    fLay->addWidget(new QLabel("Od:"));
    m_dateFrom = new QDateTimeEdit(this);
    m_dateFrom->setCalendarPopup(true);
    m_dateFrom->setDisplayFormat("yyyy-MM-dd HH:mm");
    fLay->addWidget(m_dateFrom);
    fLay->addWidget(new QLabel("Do:"));
    m_dateTo = new QDateTimeEdit(this);
    m_dateTo->setCalendarPopup(true);
    m_dateTo->setDisplayFormat("yyyy-MM-dd HH:mm");
    fLay->addWidget(m_dateTo);
    m_filterBtn = new QPushButton("Filtruj", this);
    m_resetBtn = new QPushButton("Caly zakres", this);
    fLay->addWidget(m_filterBtn);
    fLay->addWidget(m_resetBtn);
    fLay->addStretch();
    lay->addWidget(filterGroup);

    m_chart = new QChart();
    m_chart->setAnimationOptions(QChart::SeriesAnimations);
    m_chart->legend()->hide();
    m_chartView = new QChartView(m_chart, this);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumHeight(350);
    lay->addWidget(m_chartView, 1);

    m_infoLabel = new QLabel("Wybierz stacje i czujnik.", this);
    m_infoLabel->setAlignment(Qt::AlignCenter);
    lay->addWidget(m_infoLabel);

    connect(m_filterBtn, &QPushButton::clicked, this, &ChartWidget::onFilterClicked);
    connect(m_resetBtn, &QPushButton::clicked, this, &ChartWidget::onResetClicked);
}

void ChartWidget::setData(const MeasurementData& data, const QString& paramName) {
    m_fullData = data;
    m_paramName = paramName;
    if (!data.values.empty()) {
        auto minmax = std::minmax_element(data.values.begin(), data.values.end(),
            [](const auto& a, const auto& b) { return a.date < b.date; });
        m_dateFrom->setDateTime(QDateTime::fromString(
            QString::fromStdString(minmax.first->date), "yyyy-MM-dd HH:mm:ss"));
        m_dateTo->setDateTime(QDateTime::fromString(
            QString::fromStdString(minmax.second->date), "yyyy-MM-dd HH:mm:ss"));
    }
    updateChart(data);
}

void ChartWidget::clearChart() {
    m_chart->removeAllSeries();
    for (auto* a : m_chart->axes()) m_chart->removeAxis(a);
    m_chart->setTitle("");
    m_infoLabel->setText("Wybierz stacje i czujnik.");
}

void ChartWidget::updateChart(const MeasurementData& data) {
    m_chart->removeAllSeries();
    for (auto* a : m_chart->axes()) m_chart->removeAxis(a);

    auto valid = data.validValues();
    if (valid.empty()) {
        m_chart->setTitle("Brak danych");
        m_infoLabel->setText("Brak prawidlowych pomiarow.");
        return;
    }

    std::sort(valid.begin(), valid.end(),
        [](const auto& a, const auto& b) { return a.date < b.date; });

    auto* series = new QLineSeries();
    series->setColor(QColor("#2196F3"));
    series->setPen(QPen(QColor("#2196F3"), 2));

    double minV = valid[0].value.value(), maxV = minV;
    for (const auto& v : valid) {
        QDateTime dt = QDateTime::fromString(QString::fromStdString(v.date), "yyyy-MM-dd HH:mm:ss");
        double val = v.value.value();
        series->append(dt.toMSecsSinceEpoch(), val);
        minV = std::min(minV, val);
        maxV = std::max(maxV, val);
    }

    m_chart->addSeries(series);

    auto* axisX = new QDateTimeAxis();
    axisX->setFormat("dd.MM\nHH:mm");
    axisX->setTitleText("Data i czas");
    axisX->setTickCount(8);
    m_chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto* axisY = new QValueAxis();
    double margin = std::max(1.0, (maxV - minV) * 0.1);
    axisY->setRange(std::max(0.0, minV - margin), maxV + margin);
    axisY->setTitleText(m_paramName);
    axisY->setLabelFormat("%.1f");
    m_chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    m_chart->setTitle(QString("Pomiary: %1").arg(m_paramName));
    m_infoLabel->setText(QString("Wyswietlono %1 z %2 pomiarow.")
                         .arg(valid.size()).arg(data.values.size()));
}

// Filtruje wykres po zakresie dat wybranym przez uzytkownika
void ChartWidget::onFilterClicked() {
    QDateTime from = m_dateFrom->dateTime();
    QDateTime to = m_dateTo->dateTime();

    // Sprawdzamy czy zakres jest prawidlowy
    if (from >= to) {
        m_infoLabel->setText("Data poczatkowa musi byc wczesniejsza od koncowej.");
        return;
    }

    // Filtrujemy pomiary - porownujemy obiekty QDateTime (nie stringi)
    MeasurementData filtered;
    filtered.key = m_fullData.key;

    for (const auto& measurement : m_fullData.values) {
        QDateTime measurementDate = QDateTime::fromString(
            QString::fromStdString(measurement.date), "yyyy-MM-dd HH:mm:ss");

        if (measurementDate >= from && measurementDate <= to) {
            filtered.values.push_back(measurement);
        }
    }

    updateChart(filtered);
}

// Resetuje filtr - pokazuje wszystkie dane i ustawia daty na rzeczywisty zakres
void ChartWidget::onResetClicked() {
    updateChart(m_fullData);

    // Ustawiamy pola dat na rzeczywisty zakres danych
    if (!m_fullData.values.empty()) {
        auto minmax = std::minmax_element(
            m_fullData.values.begin(), m_fullData.values.end(),
            [](const MeasurementValue& a, const MeasurementValue& b) {
                return a.date < b.date;
            }
        );

        m_dateFrom->setDateTime(QDateTime::fromString(
            QString::fromStdString(minmax.first->date), "yyyy-MM-dd HH:mm:ss"));
        m_dateTo->setDateTime(QDateTime::fromString(
            QString::fromStdString(minmax.second->date), "yyyy-MM-dd HH:mm:ss"));

        m_infoLabel->setText(
            QString("Zakres danych: %1 do %2")
                .arg(QString::fromStdString(minmax.first->date),
                     QString::fromStdString(minmax.second->date))
        );
    }
}
