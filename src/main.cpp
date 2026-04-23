/**
 * @file main.cpp
 * @brief Punkt wejscia aplikacji Monitor Jakosci Powietrza.
 */
#include <QApplication>
#include <QStyleFactory>
#include "gui/MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("AirQualityMonitor");
    app.setApplicationVersion("1.0.0");
    app.setStyle(QStyleFactory::create("Fusion"));

    MainWindow w;
    w.show();
    return app.exec();
}
