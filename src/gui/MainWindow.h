#ifndef MAINWINDOW_H
#define MAINWINDOW_H

/**
 * @file MainWindow.h
 * @brief Glowne okno aplikacji.
 */

#include <QMainWindow>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QProgressBar>
#include <QTabWidget>
#include <QDoubleSpinBox>
#include <QMenu>
#include <QGroupBox>
#include <memory>
#include <vector>

#include "models/Station.h"
#include "models/Sensor.h"
#include "models/MeasurementData.h"
#include "api/ApiClient.h"
#include "database/DatabaseManager.h"
#include "analysis/DataAnalyzer.h"
#include "gui/ChartWidget.h"

/**
 * @class MainWindow
 * @brief Glowne okno - laczy API, baze, wykres, mape i analize.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onRefreshStations();
    void onLoadFromDatabase();
    void onFilterStations();
    void onStationSelected();
    void onSensorSelected();
    void onSaveData();
    void onShowIndex();
    void onSearchRadius();

private:
    std::unique_ptr<DatabaseManager> m_db;
    DataAnalyzer m_analyzer;

    std::vector<Station> m_stations;
    std::vector<Sensor> m_sensors;
    MeasurementData m_currentData;
    int m_selectedStationId = -1;
    int m_selectedSensorId = -1;

    // Widoki i kontrolki GUI
    QLineEdit* m_searchEdit;
    QPushButton* m_refreshBtn;
    QPushButton* m_loadDbBtn;
    QListWidget* m_stationList;
    QListWidget* m_sensorList;
    QLabel* m_stationInfoLabel;
    QTabWidget* m_tabWidget;
    ChartWidget* m_chartWidget;
    QTextEdit* m_analysisText;
    QTextEdit* m_indexText;
    QPushButton* m_saveBtn;
    QPushButton* m_indexBtn;
    QProgressBar* m_progressBar;

    // Kontrolki wyszukiwania w promieniu
    QLineEdit* m_locationEdit;
    QDoubleSpinBox* m_radiusSpin;
    QPushButton* m_radiusSearchBtn;

    // Pozostale elementy interfejsu
    QGroupBox* m_stationGroup;
    QGroupBox* m_sensorGroup;
    QGroupBox* m_radiusGroup;
    QAction* m_actRefresh;
    QAction* m_actLoad;
    QAction* m_actExit;
    QMenu* m_fileMenu;
    QMenu* m_helpMenu;
    QPushButton* m_searchBtn;
    QLabel* m_radiusLabel;
    QLabel* m_locationLabel;

    void setupUI();
    void updateStationList(const QString& filter = "");
    void showAnalysis(const MeasurementData& data);
    void setLoading(bool on, const QString& msg = "");
    void setSearchControlsEnabled(bool enabled);
    void selectStationById(int stationId);
    void fetchAndDisplayIndex(bool switchToIndexTab, bool useGlobalLoading);
};

#endif // MAINWINDOW_H
