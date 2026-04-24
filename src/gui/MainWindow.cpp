/**
 * @file MainWindow.cpp
 * @brief Implementacja glownego okna aplikacji.
 */
#include "gui/MainWindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QMessageBox>
#include <QMenuBar>
#include <QStatusBar>
#include <QtConcurrent>
#include <optional>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include "utils/GeoUtils.h"

namespace {
QString normalizeForSearch(const QString& text) {
    QString out = text.trimmed().toCaseFolded();
    out.replace(QChar(0x0105), QChar('a')); // ą
    out.replace(QChar(0x0107), QChar('c')); // ć
    out.replace(QChar(0x0119), QChar('e')); // ę
    out.replace(QChar(0x0142), QChar('l')); // ł
    out.replace(QChar(0x0144), QChar('n')); // ń
    out.replace(QChar(0x00F3), QChar('o')); // ó
    out.replace(QChar(0x015B), QChar('s')); // ś
    out.replace(QChar(0x017C), QChar('z')); // ż
    out.replace(QChar(0x017A), QChar('z')); // ź
    return out;
}

std::optional<std::pair<double, double>> geocodeOnlineNominatim(
    const QString& location, QString& errorMessage)
{
    using json = nlohmann::json;

    auto request = [&](bool polishOnly) -> std::optional<std::pair<double, double>> {
        cpr::Parameters params{
            {"format", "jsonv2"},
            {"limit", "1"},
            {"q", location.toUtf8().toStdString()}
        };
        if (polishOnly) {
            params.Add({"countrycodes", "pl"});
        }

        const cpr::Response r = cpr::Get(
            cpr::Url{"https://nominatim.openstreetmap.org/search"},
            params,
            cpr::Header{
                {"User-Agent", "AirQualityMonitor/1.0 (educational project)"},
                {"Accept", "application/json"},
                {"Accept-Language", "pl,en"}
            },
            cpr::Timeout{10000}
        );

        if (r.error) {
            errorMessage = QStringLiteral("Blad geokodowania: %1")
                               .arg(QString::fromStdString(r.error.message));
            return std::nullopt;
        }
        if (r.status_code != 200) {
            QString detail = QString::fromStdString(r.text).simplified();
            if (detail.length() > 200) detail = detail.left(197) + "...";
            if (detail.isEmpty()) {
                detail = QStringLiteral("Brak dodatkowych informacji.");
            }
            errorMessage = QStringLiteral("Blad geokodowania HTTP %1: %2")
                               .arg(r.status_code)
                               .arg(detail);
            return std::nullopt;
        }

        try {
            const json data = json::parse(r.text);
            if (!data.is_array() || data.empty() || !data[0].is_object()) {
                return std::nullopt;
            }

            const auto& first = data[0];
            if (!first.contains("lat") || !first.contains("lon")) {
                return std::nullopt;
            }

            const double lat = std::stod(first["lat"].get<std::string>());
            const double lon = std::stod(first["lon"].get<std::string>());
            return std::make_pair(lat, lon);
        } catch (const std::exception& e) {
            errorMessage = QStringLiteral("Blad odpowiedzi geokodowania: %1")
                               .arg(QString::fromUtf8(e.what()));
            return std::nullopt;
        }
    };

    // Najpierw probujemy wynik ograniczony do Polski, potem globalnie.
    if (auto res = request(true)) return res;
    if (!errorMessage.isEmpty()) return std::nullopt;
    return request(false);
}
} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), m_db(std::make_unique<DatabaseManager>("data"))
{
    setupUI();
    statusBar()->showMessage(QStringLiteral("Gotowy. Pobierz stacje lub wczytaj z bazy."), 5000);
}

void MainWindow::setupUI() {
    setWindowTitle(QStringLiteral("Monitor Jakosci Powietrza - GIOS"));
    setMinimumSize(1200, 750);
    resize(1400, 850);

    // Menu
    m_fileMenu = menuBar()->addMenu(QStringLiteral("&Plik"));
    m_actRefresh = m_fileMenu->addAction(QStringLiteral("&Pobierz stacje z API"));
    connect(m_actRefresh, &QAction::triggered, this, &MainWindow::onRefreshStations);
    m_actLoad = m_fileMenu->addAction(QStringLiteral("&Wczytaj z bazy"));
    connect(m_actLoad, &QAction::triggered, this, &MainWindow::onLoadFromDatabase);
    m_fileMenu->addSeparator();
    m_actExit = m_fileMenu->addAction(QStringLiteral("&Zakoncz"));
    connect(m_actExit, &QAction::triggered, this, &QWidget::close);

    m_helpMenu = menuBar()->addMenu(QStringLiteral("&Pomoc"));
    m_helpMenu->addAction(QStringLiteral("O programie"), this, [this]() {
        QMessageBox::about(this, QStringLiteral("O programie"),
            QStringLiteral("<h3>Monitor Jakosci Powietrza</h3><p>Dane: GIOS | Projekt JPO 2025/2026</p>"));
    });

    // Glowny uklad okna
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* splitter = new QSplitter(Qt::Horizontal, this);

    // Lewy panel: stacje
    auto* leftPanel = new QWidget();
    auto* leftLay = new QVBoxLayout(leftPanel);
    
    m_stationGroup = new QGroupBox(QStringLiteral("Stacje pomiarowe"));
    auto* stLay = new QVBoxLayout(m_stationGroup);

    auto* searchLay = new QHBoxLayout();
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText(QStringLiteral("Szukaj miejscowosci..."));
    searchLay->addWidget(m_searchEdit);
    m_searchBtn = new QPushButton(QStringLiteral("Szukaj"));
    searchLay->addWidget(m_searchBtn);
    stLay->addLayout(searchLay);

    auto* btnLay = new QHBoxLayout();
    m_refreshBtn = new QPushButton(QStringLiteral("Pobierz z API"));
    m_loadDbBtn = new QPushButton(QStringLiteral("Z bazy"));
    btnLay->addWidget(m_refreshBtn);
    btnLay->addWidget(m_loadDbBtn);
    stLay->addLayout(btnLay);

    // Wyszukiwanie stacji po promieniu
    m_radiusGroup = new QGroupBox(QStringLiteral("Wyszukiwanie w promieniu"));
    auto* radLay = new QVBoxLayout(m_radiusGroup);
    
    auto* locLay = new QHBoxLayout();
    m_locationLabel = new QLabel(QStringLiteral("Lokalizacja:"));
    locLay->addWidget(m_locationLabel);
    m_locationEdit = new QLineEdit();
    m_locationEdit->setPlaceholderText(QStringLiteral("np. Poznan, Warszawa..."));
    locLay->addWidget(m_locationEdit);
    radLay->addLayout(locLay);
    
    auto* radSearchLay = new QHBoxLayout();
    m_radiusLabel = new QLabel(QStringLiteral("Promien (km):"));
    radSearchLay->addWidget(m_radiusLabel);
    m_radiusSpin = new QDoubleSpinBox();
    m_radiusSpin->setRange(1, 500);
    m_radiusSpin->setValue(50);
    m_radiusSpin->setSuffix(" km");
    radSearchLay->addWidget(m_radiusSpin);
    radSearchLay->addStretch();
    radLay->addLayout(radSearchLay);

    m_radiusSearchBtn = new QPushButton(QStringLiteral("Szukaj w promieniu"));
    m_radiusSearchBtn->setMinimumHeight(30);
    radLay->addWidget(m_radiusSearchBtn);
    
    stLay->addWidget(m_radiusGroup);

    m_stationList = new QListWidget();
    m_stationList->setAlternatingRowColors(true);
    stLay->addWidget(m_stationList, 1);
    
    leftLay->addWidget(m_stationGroup);
    leftPanel->setMinimumWidth(300);
    leftPanel->setMaximumWidth(420);

    // Srodkowy panel: czujniki
    auto* midPanel = new QWidget();
    auto* midLay = new QVBoxLayout(midPanel);

    m_stationInfoLabel = new QLabel(QStringLiteral("Wybierz stacje z listy."));
    m_stationInfoLabel->setWordWrap(true);
    m_stationInfoLabel->setStyleSheet("border-radius:6px; padding:10px;");
    midLay->addWidget(m_stationInfoLabel);

    m_sensorGroup = new QGroupBox(QStringLiteral("Czujniki"));
    auto* sensLay = new QVBoxLayout(m_sensorGroup);

    auto* sensBtnLay = new QHBoxLayout();
    m_indexBtn = new QPushButton(QStringLiteral("Indeks jakosci"));
    m_indexBtn->setEnabled(false);
    sensBtnLay->addWidget(m_indexBtn);
    sensLay->addLayout(sensBtnLay);

    m_sensorList = new QListWidget();
    m_sensorList->setAlternatingRowColors(true);
    sensLay->addWidget(m_sensorList, 1);
    midLay->addWidget(m_sensorGroup, 1);
    midPanel->setMinimumWidth(250);
    midPanel->setMaximumWidth(350);

    // Prawy panel: wykres, analiza i indeks
    auto* rightPanel = new QWidget();
    auto* rightLay = new QVBoxLayout(rightPanel);

    m_saveBtn = new QPushButton(QStringLiteral("Zapisz dane do bazy"));
    m_saveBtn->setEnabled(false);
    rightLay->addWidget(m_saveBtn);

    m_tabWidget = new QTabWidget();

    m_chartWidget = new ChartWidget();
    m_tabWidget->addTab(m_chartWidget, QStringLiteral("Wykres"));

    m_mapWidget = new MapWidget();
    m_tabWidget->addTab(m_mapWidget, QStringLiteral("Mapa"));

    m_analysisText = new QTextEdit();
    m_analysisText->setReadOnly(true);
    m_tabWidget->addTab(m_analysisText, QStringLiteral("Analiza"));

    m_indexText = new QTextEdit();
    m_indexText->setReadOnly(true);
    m_tabWidget->addTab(m_indexText, QStringLiteral("Indeks"));

    rightLay->addWidget(m_tabWidget, 1);

    splitter->addWidget(leftPanel);
    splitter->addWidget(midPanel);
    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 2);
    splitter->setStretchFactor(2, 5);

    auto* mainLay = new QHBoxLayout(central);
    mainLay->addWidget(splitter);

    m_progressBar = new QProgressBar();
    m_progressBar->setMaximumWidth(200);
    m_progressBar->setVisible(false);
    statusBar()->addPermanentWidget(m_progressBar);

    // Sygnaly i sloty
    connect(m_refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefreshStations);
    connect(m_loadDbBtn, &QPushButton::clicked, this, &MainWindow::onLoadFromDatabase);
    connect(m_searchBtn, &QPushButton::clicked, this, &MainWindow::onFilterStations);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &MainWindow::onFilterStations);
    connect(m_stationList, &QListWidget::itemClicked, this, &MainWindow::onStationSelected);
    connect(m_sensorList, &QListWidget::itemClicked, this, &MainWindow::onSensorSelected);
    connect(m_saveBtn, &QPushButton::clicked, this, &MainWindow::onSaveData);
    connect(m_indexBtn, &QPushButton::clicked, this, &MainWindow::onShowIndex);
    connect(m_radiusSearchBtn, &QPushButton::clicked, this, &MainWindow::onSearchRadius);
    connect(m_locationEdit, &QLineEdit::returnPressed, this, &MainWindow::onSearchRadius);
    connect(m_mapWidget, &MapWidget::stationClicked, this, &MainWindow::selectStationById);

    // Wyszukiwanie dziala dopiero po zaladowaniu stacji.
    setSearchControlsEnabled(false);
}

void MainWindow::setLoading(bool on, const QString& msg) {
    m_progressBar->setVisible(on);
    if (on) { m_progressBar->setRange(0, 0); statusBar()->showMessage(msg); }
    m_refreshBtn->setEnabled(!on);
}

void MainWindow::setSearchControlsEnabled(bool enabled) {
    m_searchEdit->setEnabled(enabled);
    m_searchBtn->setEnabled(enabled);
    m_radiusGroup->setEnabled(enabled);
}

// Pobieranie stacji

void MainWindow::onRefreshStations() {
    setLoading(true, QStringLiteral("Pobieranie stacji z API GIOS..."));

    auto* future = new QFutureWatcher<std::vector<Station>>(this);
    connect(future, &QFutureWatcher<std::vector<Station>>::finished, this, [this, future]() {
        try {
            m_stations = future->result();
            m_db->saveStations(m_stations);
            setSearchControlsEnabled(!m_stations.empty());
            updateStationList();
            setLoading(false);
            statusBar()->showMessage(QStringLiteral("Pobrano %1 stacji.").arg(m_stations.size()), 5000);
        } catch (const std::exception& e) {
            setLoading(false);
            // Pokazujemy konkretny powod bledu, zeby bylo jasne co sie stalo.
            const QString detail = QString::fromUtf8(e.what());
            QString msg = QStringLiteral("Brak polaczenia z serwerem GIOS.\nSprawdz polaczenie internetowe.");
            if (!detail.isEmpty()) msg += "\n\n(" + detail + ")";
            if (m_db->hasStations()) {
                auto reply = QMessageBox::warning(this, QStringLiteral("Blad polaczenia"),
                    msg + "\n\n" + QStringLiteral("Wczytac dane z bazy?"), QMessageBox::Yes | QMessageBox::No);
                if (reply == QMessageBox::Yes) onLoadFromDatabase();
            } else {
                QMessageBox::warning(this, QStringLiteral("Blad"), msg);
            }
        }
        future->deleteLater();
    });
    future->setFuture(QtConcurrent::run([]() {
        return ApiClient::instance().fetchAllStations();
    }));
}

void MainWindow::onLoadFromDatabase() {
    if (!m_db->hasStations()) {
        QMessageBox::information(this, QStringLiteral("Baza"), QStringLiteral("Brak zapisanych stacji. Pobierz z API."));
        return;
    }
    m_stations = m_db->loadStations();
    setSearchControlsEnabled(!m_stations.empty());
    updateStationList();
    statusBar()->showMessage(QStringLiteral("Wczytano %1 stacji z bazy.").arg(m_stations.size()), 5000);
}

void MainWindow::onFilterStations() {
    if (m_stations.empty()) {
        statusBar()->showMessage(QStringLiteral("Najpierw wczytaj stacje z API lub z bazy."), 3000);
        return;
    }
    updateStationList(m_searchEdit->text().trimmed());
}

void MainWindow::updateStationList(const QString& filter) {
    m_stationList->clear();
    int count = 0;
    const QString normalizedFilter = normalizeForSearch(filter);
    std::vector<Station> displayedStations;

    for (const auto& s : m_stations) {
        const QString cityName = QString::fromStdString(s.city.name);
        const QString stName = QString::fromStdString(s.stationName);

        const QString normalizedCity = normalizeForSearch(cityName);
        const QString normalizedStation = normalizeForSearch(stName);
        if (!normalizedFilter.isEmpty() &&
            !normalizedCity.contains(normalizedFilter) &&
            !normalizedStation.contains(normalizedFilter)) continue;

        auto* item = new QListWidgetItem(
            QString("[%1] %2\n     %3").arg(s.id).arg(stName, cityName));
        item->setData(Qt::UserRole, s.id);
        m_stationList->addItem(item);
        displayedStations.push_back(s);
        count++;
    }
    if (m_mapWidget) {
        m_mapWidget->setStations(displayedStations);
        m_mapWidget->setSelectedStationId(m_selectedStationId);
    }
    statusBar()->showMessage(QStringLiteral("Wyswietlono %1 z %2 stacji.").arg(count).arg(m_stations.size()), 3000);
}

// Wyszukiwanie w promieniu

void MainWindow::onSearchRadius() {
    if (m_stations.empty()) {
        statusBar()->showMessage(QStringLiteral("Najpierw wczytaj stacje z API lub z bazy."), 3000);
        return;
    }

    QString location = m_locationEdit->text().trimmed();
    if (location.isEmpty()) return;
    
    double lat, lon;
    const std::string locationUtf8 = location.toUtf8().toStdString();
    if (!GeoUtils::geocodeSimple(locationUtf8, lat, lon)) {
        setLoading(true, QStringLiteral("Geokodowanie lokalizacji..."));
        QString geocodeError;
        const auto online = geocodeOnlineNominatim(location, geocodeError);
        setLoading(false);

        if (!online) {
            if (geocodeError.isEmpty()) {
                QMessageBox::warning(this, QStringLiteral("Blad"),
                    QStringLiteral("Nie znaleziono lokalizacji \"%1\".").arg(location));
            } else {
                QMessageBox::warning(this, QStringLiteral("Blad"), geocodeError);
            }
            return;
        }

        lat = online->first;
        lon = online->second;
    }
    
    double radiusKm = m_radiusSpin->value();
    auto results = GeoUtils::findStationsInRadius(m_stations, lat, lon, radiusKm);
    
    if (results.empty()) {
        QMessageBox::information(this, QStringLiteral("Wyszukiwanie w promieniu"),
            QStringLiteral("Brak stacji w promieniu %1 km.").arg(radiusKm));
        return;
    }
    
    // Pokazujemy wyniki na tej samej liscie stacji.
    m_stationList->clear();
    std::vector<Station> displayedStations;
    for (const auto& r : results) {
        QString text = QString("[%1] %2\n     %3 (%4 km)")
            .arg(r.station.id)
            .arg(QString::fromStdString(r.station.stationName))
            .arg(QString::fromStdString(r.station.city.name))
            .arg(r.distanceKm, 0, 'f', 1);
        auto* item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, r.station.id);
        m_stationList->addItem(item);
        displayedStations.push_back(r.station);
    }
    if (m_mapWidget) {
        m_mapWidget->setStations(displayedStations);
        m_mapWidget->setSelectedStationId(m_selectedStationId);
    }
    
    statusBar()->showMessage(QStringLiteral("Znaleziono %1 stacji w promieniu %2 km.")
        .arg(results.size()).arg(radiusKm), 5000);
}

// Wybor stacji z listy

void MainWindow::selectStationById(int stationId) {
    // Szukamy stacji na aktualnie widocznej liscie.
    for (int i = 0; i < m_stationList->count(); ++i) {
        auto* item = m_stationList->item(i);
        if (item->data(Qt::UserRole).toInt() == stationId) {
            m_stationList->setCurrentItem(item);
            m_stationList->scrollToItem(item);
            onStationSelected();
            return;
        }
    }
    
    // Jesli lista jest przefiltrowana, dodajemy stacje tymczasowo.
    for (const auto& s : m_stations) {
        if (s.id == stationId) {
            auto* item = new QListWidgetItem(
                QString("[%1] %2\n     %3").arg(s.id)
                    .arg(QString::fromStdString(s.stationName))
                    .arg(QString::fromStdString(s.city.name)));
            item->setData(Qt::UserRole, s.id);
            m_stationList->insertItem(0, item);
            m_stationList->setCurrentItem(item);
            onStationSelected();
            return;
        }
    }
}

// Wybor stacji i pobieranie czujnikow

void MainWindow::onStationSelected() {
    auto* item = m_stationList->currentItem();
    if (!item) return;
    m_selectedStationId = item->data(Qt::UserRole).toInt();
    if (m_mapWidget) {
        m_mapWidget->setSelectedStationId(m_selectedStationId);
    }

    const Station* sel = nullptr;
    for (const auto& s : m_stations)
        if (s.id == m_selectedStationId) { sel = &s; break; }
    if (!sel) return;

    m_stationInfoLabel->setText(QString(
        "<b>%1</b><br>%7 %2<br>%8 %3<br>%9 %4, %10 %5<br>%11 %6")
        .arg(QString::fromStdString(sel->stationName),
             QString::fromStdString(sel->city.name),
             QString::fromStdString(sel->addressStreet.empty() ? "-" : sel->addressStreet),
             QString::fromStdString(sel->city.commune.communeName),
             QString::fromStdString(sel->city.commune.districtName),
             QString::fromStdString(sel->city.commune.provinceName),
             QStringLiteral("Miasto:"), QStringLiteral("Adres:"), QStringLiteral("Gmina:"), QStringLiteral("Powiat:"), QStringLiteral("Woj.:")));

    m_indexText->setHtml(QStringLiteral("<p>Trwa pobieranie indeksu dla wybranej stacji...</p>"));
    m_indexBtn->setEnabled(true);
    fetchAndDisplayIndex(false, false);
    setLoading(true, QStringLiteral("Pobieranie czujnikow..."));

    int stId = m_selectedStationId;
    auto* future = new QFutureWatcher<std::vector<Sensor>>(this);
    connect(future, &QFutureWatcher<std::vector<Sensor>>::finished, this, [this, future, stId]() {
        try {
            m_sensors = future->result();
            m_sensorList->clear();
            for (const auto& s : m_sensors) {
                QString text = QString("[%1] %2 (%3)")
                    .arg(s.id)
                    .arg(QString::fromStdString(s.param.paramName),
                         QString::fromStdString(s.param.paramFormula));
                auto* item = new QListWidgetItem(text);
                item->setData(Qt::UserRole, s.id);
                m_sensorList->addItem(item);
            }
            m_db->saveSensors(stId, m_sensors);
            setLoading(false);
            statusBar()->showMessage(QStringLiteral("Pobrano %1 czujnikow.").arg(m_sensors.size()), 5000);
        } catch (const std::exception& e) {
            setLoading(false);
            const QString detail = QString::fromUtf8(e.what());
            if (m_db->hasSensors(stId)) {
                m_sensors = m_db->loadSensors(stId);
                m_sensorList->clear();
                for (const auto& s : m_sensors) {
                    QString text = QString("[%1] %2 (%3)")
                    .arg(s.id)
                        .arg(QString::fromStdString(s.param.paramName),
                             QString::fromStdString(s.param.paramFormula));
                    auto* item = new QListWidgetItem(text);
                    item->setData(Qt::UserRole, s.id);
                    m_sensorList->addItem(item);
                }
                QString msg = QStringLiteral("Blad API - czujniki wczytane z bazy.");
                if (!detail.isEmpty()) msg += QStringLiteral(" (%1)").arg(detail);
                statusBar()->showMessage(msg, 8000);
            } else {
                QMessageBox::warning(this, QStringLiteral("Blad polaczenia"),
                                     QStringLiteral("Nie udalo sie pobrac czujnikow dla tej stacji.\n\n%1")
                                         .arg(detail.isEmpty()
                                              ? QStringLiteral("Brak dodatkowych szczegolow bledu.")
                                              : detail));
            }
        } catch (...) {
            setLoading(false);
            if (m_db->hasSensors(stId)) {
                m_sensors = m_db->loadSensors(stId);
                m_sensorList->clear();
                for (const auto& s : m_sensors) {
                    QString text = QString("[%1] %2 (%3)")
                    .arg(s.id)
                        .arg(QString::fromStdString(s.param.paramName),
                             QString::fromStdString(s.param.paramFormula));
                    auto* item = new QListWidgetItem(text);
                    item->setData(Qt::UserRole, s.id);
                    m_sensorList->addItem(item);
                }
                statusBar()->showMessage(QStringLiteral("Blad API - czujniki wczytane z bazy."), 8000);
            } else {
                QMessageBox::warning(this, QStringLiteral("Blad polaczenia"),
                                     QStringLiteral("Nie udalo sie pobrac czujnikow dla tej stacji."));
            }
        }
        future->deleteLater();
    });
    future->setFuture(QtConcurrent::run([stId]() {
        return ApiClient::instance().fetchSensors(stId);
    }));
}

// Wybor czujnika i pobieranie pomiarow.
// Zamiast rzucac wyjatek przez granice watku, zwracamy wynik z polem ok/errorMessage.

struct FetchMeasurementResult {
    MeasurementData data;
    bool ok = false;
    QString errorMessage;
};

struct FetchIndexResult {
    AirQualityIndex data;
    bool ok = false;
    QString errorMessage;
};

void MainWindow::onSensorSelected() {
    auto* item = m_sensorList->currentItem();
    if (!item) return;
    m_selectedSensorId = item->data(Qt::UserRole).toInt();

    setLoading(true, QStringLiteral("Pobieranie danych pomiarowych..."));

    const int sId = m_selectedSensorId;
    auto* future = new QFutureWatcher<FetchMeasurementResult>(this);

    connect(future, &QFutureWatcher<FetchMeasurementResult>::finished, this, [this, future, sId]() {
        // Uzytkownik mogl juz wybrac inny czujnik, wiec ignorujemy stary wynik.
        if (sId != m_selectedSensorId) {
            future->deleteLater();
            return;
        }

        const FetchMeasurementResult r = future->result();
        setLoading(false);
        // API zwrocilo dane.
        if (r.ok) {
            m_currentData = r.data;

            // Nazwa parametru do opisu wykresu.
            QString paramName = QString::fromStdString(m_currentData.key);
            for (const auto& s : m_sensors) {
                if (s.id == sId) {
                    paramName = QString::fromStdString(
                        s.param.paramName + " [" + s.param.paramFormula + "]");
                    break;
                }
            }

            m_chartWidget->setData(m_currentData, paramName);
            m_tabWidget->setCurrentIndex(0);
            showAnalysis(m_currentData);
            m_saveBtn->setEnabled(true);

            const auto valid = m_currentData.validValues();
            statusBar()->showMessage(QStringLiteral("Pobrano %1 pomiarow (%2 prawidlowych).")
                .arg(m_currentData.values.size()).arg(valid.size()), 5000);

            future->deleteLater();
            return;
        }

        // API nie zadzialalo, probujemy dane lokalne.
        if (m_db->hasMeasurementData(sId)) {
            m_currentData = m_db->loadMeasurementData(sId);

            QString paramName = QString::fromStdString(m_currentData.key);
            for (const auto& s : m_sensors) {
                if (s.id == sId) {
                    paramName = QString::fromStdString(
                        s.param.paramName + " [" + s.param.paramFormula + "]");
                    break;
                }
            }

            m_chartWidget->setData(m_currentData, paramName);
            showAnalysis(m_currentData);
            m_saveBtn->setEnabled(true);
            statusBar()->showMessage(QStringLiteral("Blad API - wczytano dane z bazy."), 5000);
            future->deleteLater();
            return;
        }

        // Brak danych z API i z bazy.
        QString errMsg = r.errorMessage;
        // Awaryjny tekst, gdy API nie podalo sensownego bledu.
        if (errMsg.isEmpty() || errMsg == "std::exception") {
            errMsg = QStringLiteral("Ten czujnik nie zwrocil danych pomiarowych.");
        }
        QMessageBox::information(this, QStringLiteral("Brak danych"), errMsg);
        future->deleteLater();
    });

    future->setFuture(QtConcurrent::run([sId]() -> FetchMeasurementResult {
        FetchMeasurementResult r;
        try {
            r.data = ApiClient::instance().fetchMeasurementData(sId);
            r.ok = true;
        } catch (const std::exception& e) {
            // Przekazujemy czytelny blad do GUI przez strukture wyniku.
            r.errorMessage = QString::fromUtf8(e.what());
            r.ok = false;
        } catch (...) {
            r.errorMessage = "Nieznany blad podczas pobierania danych.";
            r.ok = false;
        }
        return r;
    }));
}

// Zapis danych do bazy

void MainWindow::onSaveData() {
    if (m_currentData.values.empty()) return;
    if (!m_stations.empty()) m_db->saveStations(m_stations);
    if (!m_sensors.empty() && m_selectedStationId > 0) m_db->saveSensors(m_selectedStationId, m_sensors);
    m_db->saveMeasurementData(m_selectedSensorId, m_currentData);
    QMessageBox::information(this, QStringLiteral("Zapis"), QStringLiteral("Dane zapisane do bazy."));
}

// Indeks jakosci

void MainWindow::onShowIndex() {
    fetchAndDisplayIndex(true, true);
}

void MainWindow::fetchAndDisplayIndex(bool switchToIndexTab, bool useGlobalLoading) {
    if (m_selectedStationId <= 0) return;
    if (useGlobalLoading) {
        setLoading(true, QStringLiteral("Pobieranie indeksu..."));
    }

    const int stId = m_selectedStationId;
    auto* future = new QFutureWatcher<FetchIndexResult>(this);
    connect(future, &QFutureWatcher<FetchIndexResult>::finished, this, [this, future, stId, switchToIndexTab, useGlobalLoading]() {
        // Jesli w miedzyczasie zmieniono stacje, ignorujemy spozniony wynik.
        if (stId != m_selectedStationId) {
            future->deleteLater();
            return;
        }

        const FetchIndexResult r = future->result();
        if (useGlobalLoading) {
            setLoading(false);
        }

        if (!r.ok) {
            QString err = r.errorMessage.trimmed();
            if (err.isEmpty() || err == QStringLiteral("std::exception")) {
                err = QStringLiteral("Brak aktualnego indeksu dla tej stacji.");
            }
            m_indexText->setHtml(
                QStringLiteral("<h3>Indeks jakosci powietrza</h3><p>%1</p>")
                    .arg(err.toHtmlEscaped()));
            if (switchToIndexTab) {
                m_tabWidget->setCurrentWidget(m_indexText);
            }
            future->deleteLater();
            return;
        }

        const auto& idx = r.data;
        // Budujemy tabele HTML z poziomami indeksu.
        QString html = "<h3>" + QStringLiteral("Indeks jakosci powietrza") + "</h3>";
        html += "<table style='border-collapse:collapse; width:100%;'>";

        // Pomocniczo dodajemy jeden wiersz tabeli.
        auto dodajWiersz = [&](const QString& nazwa, const IndexLevel& poziom) {
            QString kolor = QString::fromStdString(poziom.color());
            QString etykieta = QString::fromStdString(poziom.name);
            if (etykieta.isEmpty()) etykieta = QStringLiteral("Brak danych");

            html += QString(
                "<tr>"
                "<td style='padding:8px; border:1px solid #555;'>%1</td>"
                "<td style='padding:8px; border:1px solid #555; background:%2; "
                "color:white; text-align:center;'><b>%3</b></td>"
                "</tr>"
            ).arg(nazwa, kolor, etykieta);
        };

        // Wiersze dla wszystkich obslugiwanych zanieczyszczen.
        dodajWiersz(QStringLiteral("Ogolny"),  idx.overall);
        dodajWiersz("NO2",     idx.no2);
        dodajWiersz("SO2",     idx.so2);
        dodajWiersz("PM10",    idx.pm10);
        dodajWiersz("PM2.5",   idx.pm25);
        dodajWiersz("O3",      idx.o3);
        html += "</table>";

        // Data ostatniego przeliczenia indeksu.
        if (!idx.calcDate.empty())
            html += QString("<p><small>%1 %2</small></p>")
                .arg(QStringLiteral("Data obliczenia:"),
                     QString::fromStdString(idx.calcDate));

        m_indexText->setHtml(html);
        if (switchToIndexTab) {
            m_tabWidget->setCurrentWidget(m_indexText);
        }
        future->deleteLater();
    });
    future->setFuture(QtConcurrent::run([stId]() -> FetchIndexResult {
        FetchIndexResult r;
        try {
            r.data = ApiClient::instance().fetchAirQualityIndex(stId);
            r.ok = true;
        } catch (const std::exception& e) {
            r.errorMessage = QString::fromUtf8(e.what());
            r.ok = false;
        } catch (...) {
            r.errorMessage = QStringLiteral("Nie udalo sie pobrac indeksu dla tej stacji.");
            r.ok = false;
        }
        return r;
    }));
}

// Analiza danych pomiarowych

void MainWindow::showAnalysis(const MeasurementData& data) {
    auto result = m_analyzer.analyze(data);
    if (!result) {
        m_analysisText->setHtml("<p>" + QStringLiteral("Brak danych do analizy.") + "</p>");
        return;
    }
    const auto& r = *result;
    QString html = "<h3>" + QStringLiteral("Analiza danych") + "</h3>";
    html += QString("<p><b>%1</b> %2</p>").arg(QStringLiteral("Parametr:"), QString::fromStdString(data.key));
    html += QString("<p><b>%1</b> %2 (%3 %4)</p>")
        .arg(QStringLiteral("Pomiary:")).arg(r.totalCount).arg(QStringLiteral("prawidlowych:")).arg(r.validCount);
    html += "<table style='border-collapse:collapse; width:100%;'>";
    auto row = [&](const QString& label, const QString& val, const QString& extra = "") {
        html += QString("<tr><td style='padding:6px; border:1px solid #ddd;'><b>%1</b></td>"
                       "<td style='padding:6px; border:1px solid #ddd;'>%2</td>"
                       "<td style='padding:6px; border:1px solid #ddd;'>%3</td></tr>")
                .arg(label, val, extra);
    };
    row(QStringLiteral("Minimum"), QString::number(r.minValue, 'f', 2), QString::fromStdString(r.minDate));
    row(QStringLiteral("Maksimum"), QString::number(r.maxValue, 'f', 2), QString::fromStdString(r.maxDate));
    row(QStringLiteral("Srednia"), QString::number(r.avgValue, 'f', 2));
    row(QStringLiteral("Trend"), QString::fromStdString(r.trendDescription),
        QString("wsp: %1").arg(r.trendSlope, 0, 'f', 4));
    html += "</table>";
    m_analysisText->setHtml(html);
}

