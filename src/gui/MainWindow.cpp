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
#include <QApplication>
#include <QStyleFactory>
#include <QtConcurrent>

#include "utils/GeoUtils.h"
#include "utils/Translator.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), m_db(new DatabaseManager("data"))
{
    setupUI();
    
    // Polaczenie ze zmiana jezyka
    connect(&Translator::instance(), &Translator::languageChanged, 
            this, &MainWindow::onLanguageChanged);
    
    statusBar()->showMessage(TR("Gotowy. Pobierz stacje lub wczytaj z bazy."), 5000);
}

void MainWindow::setupUI() {
    setWindowTitle(TR("Monitor Jakosci Powietrza - GIOS"));
    setMinimumSize(1200, 750);
    resize(1400, 850);

    // ==== MENU ====
    m_fileMenu = menuBar()->addMenu(TR("&Plik"));
    m_actRefresh = m_fileMenu->addAction(TR("&Pobierz stacje z API"));
    connect(m_actRefresh, &QAction::triggered, this, &MainWindow::onRefreshStations);
    m_actLoad = m_fileMenu->addAction(TR("&Wczytaj z bazy"));
    connect(m_actLoad, &QAction::triggered, this, &MainWindow::onLoadFromDatabase);
    m_fileMenu->addSeparator();
    m_actExit = m_fileMenu->addAction(TR("&Zakoncz"));
    connect(m_actExit, &QAction::triggered, this, &QWidget::close);

    // Menu jezyka
    m_langMenu = menuBar()->addMenu(TR("&Jezyk"));
    m_langGroup = new QActionGroup(this);
    m_actPolish = m_langMenu->addAction("Polski");
    m_actPolish->setCheckable(true);
    m_actPolish->setChecked(true);
    m_langGroup->addAction(m_actPolish);
    m_actEnglish = m_langMenu->addAction("English");
    m_actEnglish->setCheckable(true);
    m_langGroup->addAction(m_actEnglish);
    connect(m_actPolish, &QAction::triggered, this, &MainWindow::setLanguagePolish);
    connect(m_actEnglish, &QAction::triggered, this, &MainWindow::setLanguageEnglish);

    m_helpMenu = menuBar()->addMenu(TR("&Pomoc"));
    m_helpMenu->addAction(TR("O programie"), this, [this]() {
        QMessageBox::about(this, TR("O programie"),
            TR("<h3>Monitor Jakosci Powietrza</h3><p>Dane: GIOS | Projekt JPO 2025/2026</p>"));
    });

    // Centralny widget
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* splitter = new QSplitter(Qt::Horizontal, this);

    // ==== LEWY PANEL - Stacje ====
    auto* leftPanel = new QWidget();
    auto* leftLay = new QVBoxLayout(leftPanel);
    
    m_stationGroup = new QGroupBox(TR("Stacje pomiarowe"));
    auto* stLay = new QVBoxLayout(m_stationGroup);

    auto* searchLay = new QHBoxLayout();
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText(TR("Szukaj miejscowosci..."));
    searchLay->addWidget(m_searchEdit);
    m_searchBtn = new QPushButton(TR("Szukaj"));
    searchLay->addWidget(m_searchBtn);
    stLay->addLayout(searchLay);

    auto* btnLay = new QHBoxLayout();
    m_refreshBtn = new QPushButton(TR("Pobierz z API"));
    m_loadDbBtn = new QPushButton(TR("Z bazy"));
    btnLay->addWidget(m_refreshBtn);
    btnLay->addWidget(m_loadDbBtn);
    stLay->addLayout(btnLay);

    // === Wyszukiwanie w promieniu ===
    m_radiusGroup = new QGroupBox(TR("Wyszukiwanie w promieniu"));
    auto* radLay = new QVBoxLayout(m_radiusGroup);
    
    auto* locLay = new QHBoxLayout();
    m_locationLabel = new QLabel(TR("Lokalizacja:"));
    locLay->addWidget(m_locationLabel);
    m_locationEdit = new QLineEdit();
    m_locationEdit->setPlaceholderText(TR("np. Poznan, Warszawa..."));
    locLay->addWidget(m_locationEdit);
    radLay->addLayout(locLay);
    
    auto* radSearchLay = new QHBoxLayout();
    m_radiusLabel = new QLabel(TR("Promien (km):"));
    radSearchLay->addWidget(m_radiusLabel);
    m_radiusSpin = new QDoubleSpinBox();
    m_radiusSpin->setRange(1, 500);
    m_radiusSpin->setValue(50);
    m_radiusSpin->setSuffix(" km");
    radSearchLay->addWidget(m_radiusSpin);
    m_radiusSearchBtn = new QPushButton(TR("Szukaj w promieniu"));
    radSearchLay->addWidget(m_radiusSearchBtn);
    radLay->addLayout(radSearchLay);
    
    stLay->addWidget(m_radiusGroup);

    m_stationList = new QListWidget();
    m_stationList->setAlternatingRowColors(true);
    stLay->addWidget(m_stationList, 1);
    
    leftLay->addWidget(m_stationGroup);
    leftPanel->setMinimumWidth(300);
    leftPanel->setMaximumWidth(420);

    // ==== SRODKOWY PANEL - Czujniki ====
    auto* midPanel = new QWidget();
    auto* midLay = new QVBoxLayout(midPanel);

    m_stationInfoLabel = new QLabel(TR("Wybierz stacje z listy."));
    m_stationInfoLabel->setWordWrap(true);
    m_stationInfoLabel->setStyleSheet("border-radius:6px; padding:10px;");
    midLay->addWidget(m_stationInfoLabel);

    m_sensorGroup = new QGroupBox(TR("Czujniki"));
    auto* sensLay = new QVBoxLayout(m_sensorGroup);

    auto* sensBtnLay = new QHBoxLayout();
    m_indexBtn = new QPushButton(TR("Indeks jakosci"));
    m_indexBtn->setEnabled(false);
    sensBtnLay->addWidget(m_indexBtn);
    sensLay->addLayout(sensBtnLay);

    m_sensorList = new QListWidget();
    m_sensorList->setAlternatingRowColors(true);
    sensLay->addWidget(m_sensorList, 1);
    midLay->addWidget(m_sensorGroup, 1);
    midPanel->setMinimumWidth(250);
    midPanel->setMaximumWidth(350);

    // ==== PRAWY PANEL - Dane ====
    auto* rightPanel = new QWidget();
    auto* rightLay = new QVBoxLayout(rightPanel);

    m_saveBtn = new QPushButton(TR("Zapisz dane do bazy"));
    m_saveBtn->setEnabled(false);
    rightLay->addWidget(m_saveBtn);

    m_tabWidget = new QTabWidget();

    m_chartWidget = new ChartWidget();
    m_tabWidget->addTab(m_chartWidget, TR("Wykres"));

    m_analysisText = new QTextEdit();
    m_analysisText->setReadOnly(true);
    m_tabWidget->addTab(m_analysisText, TR("Analiza"));

    m_indexText = new QTextEdit();
    m_indexText->setReadOnly(true);
    m_tabWidget->addTab(m_indexText, TR("Indeks"));

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

    // Polaczenia
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
}

void MainWindow::setLoading(bool on, const QString& msg) {
    m_progressBar->setVisible(on);
    if (on) { m_progressBar->setRange(0, 0); statusBar()->showMessage(msg); }
    m_refreshBtn->setEnabled(!on);
}

// ---- Jezyk ----

void MainWindow::setLanguagePolish() {
    Translator::instance().setLanguage(Translator::Polish);
}

void MainWindow::setLanguageEnglish() {
    Translator::instance().setLanguage(Translator::English);
}

void MainWindow::onLanguageChanged() {
    retranslateUI();
}

void MainWindow::retranslateUI() {
    setWindowTitle(TR("Monitor Jakosci Powietrza - GIOS"));
    
    // Menu
    m_fileMenu->setTitle(TR("&Plik"));
    m_actRefresh->setText(TR("&Pobierz stacje z API"));
    m_actLoad->setText(TR("&Wczytaj z bazy"));
    m_actExit->setText(TR("&Zakoncz"));
    m_langMenu->setTitle(TR("&Jezyk"));
    m_helpMenu->setTitle(TR("&Pomoc"));
    
    // Grupy
    m_stationGroup->setTitle(TR("Stacje pomiarowe"));
    m_sensorGroup->setTitle(TR("Czujniki"));
    m_radiusGroup->setTitle(TR("Wyszukiwanie w promieniu"));
    
    // Przyciski i etykiety
    m_searchEdit->setPlaceholderText(TR("Szukaj miejscowosci..."));
    m_searchBtn->setText(TR("Szukaj"));
    m_refreshBtn->setText(TR("Pobierz z API"));
    m_loadDbBtn->setText(TR("Z bazy"));
    m_locationLabel->setText(TR("Lokalizacja:"));
    m_locationEdit->setPlaceholderText(TR("np. Poznan, Warszawa..."));
    m_radiusLabel->setText(TR("Promien (km):"));
    m_radiusSearchBtn->setText(TR("Szukaj w promieniu"));
    m_indexBtn->setText(TR("Indeks jakosci"));
    m_saveBtn->setText(TR("Zapisz dane do bazy"));
    m_stationInfoLabel->setText(TR("Wybierz stacje z listy."));
    
    // Zakladki
    m_tabWidget->setTabText(0, TR("Wykres"));
    m_tabWidget->setTabText(1, TR("Analiza"));
    m_tabWidget->setTabText(2, TR("Indeks"));
}

// ---- Pobieranie stacji (w osobnym watku) ----

void MainWindow::onRefreshStations() {
    setLoading(true, TR("Pobieranie stacji z API GIOS..."));

    auto* future = new QFutureWatcher<std::vector<Station>>(this);
    connect(future, &QFutureWatcher<std::vector<Station>>::finished, this, [this, future]() {
        try {
            m_stations = future->result();
            m_db->saveStations(m_stations);
            updateStationList();
            setLoading(false);
            statusBar()->showMessage(TR("Pobrano %1 stacji.").arg(m_stations.size()), 5000);
        } catch (const std::exception& e) {
            setLoading(false);
            QString msg = QString(TR("Blad") + ": %1").arg(e.what());
            if (m_db->hasStations()) {
                auto reply = QMessageBox::warning(this, TR("Blad polaczenia"),
                    msg + "\n\n" + TR("Wczytac dane z bazy?"), QMessageBox::Yes | QMessageBox::No);
                if (reply == QMessageBox::Yes) onLoadFromDatabase();
            } else {
                QMessageBox::warning(this, TR("Blad"), msg);
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
        QMessageBox::information(this, TR("Baza"), TR("Brak zapisanych stacji. Pobierz z API."));
        return;
    }
    m_stations = m_db->loadStations();
    updateStationList();
    statusBar()->showMessage(TR("Wczytano %1 stacji z bazy.").arg(m_stations.size()), 5000);
}

void MainWindow::onFilterStations() {
    updateStationList(m_searchEdit->text().trimmed());
}

void MainWindow::updateStationList(const QString& filter) {
    m_stationList->clear();
    int count = 0;
    for (const auto& s : m_stations) {
        QString cityName = QString::fromStdString(s.city.name);
        QString stName = QString::fromStdString(s.stationName);
        if (!filter.isEmpty() &&
            !cityName.contains(filter, Qt::CaseInsensitive) &&
            !stName.contains(filter, Qt::CaseInsensitive)) continue;

        auto* item = new QListWidgetItem(
            QString("[%1] %2\n     %3").arg(s.id).arg(stName, cityName));
        item->setData(Qt::UserRole, s.id);
        m_stationList->addItem(item);
        count++;
    }
    statusBar()->showMessage(TR("Wyswietlono %1 z %2 stacji.").arg(count).arg(m_stations.size()), 3000);
}

// ---- Wyszukiwanie w promieniu ----

void MainWindow::onSearchRadius() {
    QString location = m_locationEdit->text().trimmed();
    if (location.isEmpty()) return;
    
    double lat, lon;
    if (!GeoUtils::geocodeSimple(location.toStdString(), lat, lon)) {
        QMessageBox::warning(this, TR("Blad"), 
            TR("Nie znaleziono lokalizacji. Sprobuj: Warszawa, Krakow, Poznan..."));
        return;
    }
    
    double radiusKm = m_radiusSpin->value();
    auto results = GeoUtils::findStationsInRadius(m_stations, lat, lon, radiusKm);
    
    if (results.empty()) {
        QMessageBox::information(this, TR("Wyszukiwanie w promieniu"),
            TR("Brak stacji w promieniu %1 km.").arg(radiusKm));
        return;
    }
    
    // Pokaz wyniki na liscie
    m_stationList->clear();
    for (const auto& r : results) {
        QString text = QString("[%1] %2\n     %3 (%.1f km)")
            .arg(r.station.id)
            .arg(QString::fromStdString(r.station.stationName))
            .arg(QString::fromStdString(r.station.city.name))
            .arg(r.distanceKm);
        auto* item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, r.station.id);
        m_stationList->addItem(item);
    }
    
    statusBar()->showMessage(TR("Znaleziono %1 stacji w promieniu %2 km.")
        .arg(results.size()).arg(radiusKm), 5000);
}

// ---- Wybor stacji z listy ----

void MainWindow::selectStationById(int stationId) {
    // Znajdz i zaznacz stacje na liscie
    for (int i = 0; i < m_stationList->count(); ++i) {
        auto* item = m_stationList->item(i);
        if (item->data(Qt::UserRole).toInt() == stationId) {
            m_stationList->setCurrentItem(item);
            m_stationList->scrollToItem(item);
            onStationSelected();
            return;
        }
    }
    
    // Jesli nie znaleziono na liscie (bo filtrowana), dodaj tymczasowo
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

// ---- Wybor stacji -> pobierz czujniki ----

void MainWindow::onStationSelected() {
    auto* item = m_stationList->currentItem();
    if (!item) return;
    m_selectedStationId = item->data(Qt::UserRole).toInt();

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
             TR("Miasto:"), TR("Adres:"), TR("Gmina:"), TR("Powiat:"), TR("Woj.:")));

    m_indexBtn->setEnabled(true);
    setLoading(true, TR("Pobieranie czujnikow..."));

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
            statusBar()->showMessage(TR("Pobrano %1 czujnikow.").arg(m_sensors.size()), 5000);
        } catch (const std::exception& e) {
            setLoading(false);
            QMessageBox::warning(this, TR("Blad"), e.what());
        }
        future->deleteLater();
    });
    future->setFuture(QtConcurrent::run([stId]() {
        return ApiClient::instance().fetchSensors(stId);
    }));
}

// ---- Wybor czujnika -> pobierz pomiary ----

void MainWindow::onSensorSelected() {
    auto* item = m_sensorList->currentItem();
    if (!item) return;
    m_selectedSensorId = item->data(Qt::UserRole).toInt();

    setLoading(true, TR("Pobieranie danych pomiarowych..."));

    int sId = m_selectedSensorId;
    auto* future = new QFutureWatcher<MeasurementData>(this);
    connect(future, &QFutureWatcher<MeasurementData>::finished, this, [this, future, sId]() {
        try {
            m_currentData = future->result();
            QString paramName = QString::fromStdString(m_currentData.key);
            for (const auto& s : m_sensors)
                if (s.id == sId) {
                    paramName = QString::fromStdString(s.param.paramName + " [" + s.param.paramFormula + "]");
                    break;
                }

            m_chartWidget->setData(m_currentData, paramName);
            m_tabWidget->setCurrentIndex(0);
            showAnalysis(m_currentData);
            m_saveBtn->setEnabled(true);
            setLoading(false);

            auto valid = m_currentData.validValues();
            statusBar()->showMessage(TR("Pobrano %1 pomiarow (%2 prawidlowych).")
                .arg(m_currentData.values.size()).arg(valid.size()), 5000);
        } catch (const std::exception& e) {
            setLoading(false);
            // probuj z bazy
            if (m_db->hasMeasurementData(sId)) {
                m_currentData = m_db->loadMeasurementData(sId);
                m_chartWidget->setData(m_currentData, TR("Z bazy"));
                showAnalysis(m_currentData);
                statusBar()->showMessage(TR("Wczytano dane z bazy."), 5000);
            } else {
                QMessageBox::warning(this, TR("Blad"), e.what());
            }
        }
        future->deleteLater();
    });
    future->setFuture(QtConcurrent::run([sId]() {
        return ApiClient::instance().fetchMeasurementData(sId);
    }));
}

// ---- Zapis ----

void MainWindow::onSaveData() {
    if (m_currentData.values.empty()) return;
    if (!m_stations.empty()) m_db->saveStations(m_stations);
    if (!m_sensors.empty() && m_selectedStationId > 0) m_db->saveSensors(m_selectedStationId, m_sensors);
    m_db->saveMeasurementData(m_selectedSensorId, m_currentData);
    QMessageBox::information(this, TR("Zapis"), TR("Dane zapisane do bazy."));
}

// ---- Indeks jakosci ----

void MainWindow::onShowIndex() {
    if (m_selectedStationId <= 0) return;
    setLoading(true, TR("Pobieranie indeksu..."));

    int stId = m_selectedStationId;
    auto* future = new QFutureWatcher<AirQualityIndex>(this);
    connect(future, &QFutureWatcher<AirQualityIndex>::finished, this, [this, future]() {
        try {
            auto idx = future->result();

            // Budujemy tabele HTML z kolorami dla kazdego parametru
            QString html = "<h3>" + TR("Indeks jakosci powietrza") + "</h3>";
            html += "<table style='border-collapse:collapse; width:100%;'>";

            // Funkcja pomocnicza - dodaje wiersz do tabeli
            // Kazdy wiersz ma nazwe parametru i kolorowy poziom indeksu
            auto dodajWiersz = [&](const QString& nazwa, const IndexLevel& poziom) {
                QString kolor = QString::fromStdString(poziom.color());
                QString etykieta = QString::fromStdString(poziom.name);
                if (etykieta.isEmpty()) etykieta = TR("Brak danych");

                html += QString(
                    "<tr>"
                    "<td style='padding:8px; border:1px solid #555;'>%1</td>"
                    "<td style='padding:8px; border:1px solid #555; background:%2; "
                    "color:white; text-align:center;'><b>%3</b></td>"
                    "</tr>"
                ).arg(nazwa, kolor, etykieta);
            };

            // Dodajemy wiersz dla kazdego zanieczyszczenia
            dodajWiersz(TR("Ogolny"),  idx.overall);
            dodajWiersz("NO2",     idx.no2);
            dodajWiersz("SO2",     idx.so2);
            dodajWiersz("PM10",    idx.pm10);
            dodajWiersz("PM2.5",   idx.pm25);
            dodajWiersz("O3",      idx.o3);
            html += "</table>";

            // Data obliczenia indeksu
            if (!idx.calcDate.empty())
                html += QString("<p><small>%1 %2</small></p>")
                    .arg(TR("Data obliczenia:"),
                         QString::fromStdString(idx.calcDate));

            m_indexText->setHtml(html);
            m_tabWidget->setCurrentIndex(2);
            setLoading(false);
        } catch (const std::exception& e) {
            setLoading(false);
            QMessageBox::warning(this, TR("Blad"), e.what());
        }
        future->deleteLater();
    });
    future->setFuture(QtConcurrent::run([stId]() {
        return ApiClient::instance().fetchAirQualityIndex(stId);
    }));
}

// ---- Analiza ----

void MainWindow::showAnalysis(const MeasurementData& data) {
    auto result = m_analyzer.analyze(data);
    if (!result) {
        m_analysisText->setHtml("<p>" + TR("Brak danych do analizy.") + "</p>");
        return;
    }
    const auto& r = *result;
    QString html = "<h3>" + TR("Analiza danych") + "</h3>";
    html += QString("<p><b>%1</b> %2</p>").arg(TR("Parametr:"), QString::fromStdString(data.key));
    html += QString("<p><b>%1</b> %2 (%3 %4)</p>")
        .arg(TR("Pomiary:")).arg(r.totalCount).arg(TR("prawidlowych:")).arg(r.validCount);
    html += "<table style='border-collapse:collapse; width:100%;'>";
    auto row = [&](const QString& label, const QString& val, const QString& extra = "") {
        html += QString("<tr><td style='padding:6px; border:1px solid #ddd;'><b>%1</b></td>"
                       "<td style='padding:6px; border:1px solid #ddd;'>%2</td>"
                       "<td style='padding:6px; border:1px solid #ddd;'>%3</td></tr>")
                .arg(label, val, extra);
    };
    row(TR("Minimum"), QString::number(r.minValue, 'f', 2), QString::fromStdString(r.minDate));
    row(TR("Maksimum"), QString::number(r.maxValue, 'f', 2), QString::fromStdString(r.maxDate));
    row(TR("Srednia"), QString::number(r.avgValue, 'f', 2));
    row(TR("Trend"), TR(QString::fromStdString(r.trendDescription)),
        QString("wsp: %1").arg(r.trendSlope, 0, 'f', 4));
    html += "</table>";
    m_analysisText->setHtml(html);
}
