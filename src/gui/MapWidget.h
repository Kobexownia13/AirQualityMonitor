#ifndef MAPWIDGET_H
#define MAPWIDGET_H

/**
 * @file MapWidget.h
 * @brief Interaktywna mapa stacji oparta o kafelki OpenStreetMap.
 */

#include <QHash>
#include <QPixmap>
#include <QPointF>
#include <QSet>
#include <QWidget>
#include <vector>

#include "models/Station.h"

class QNetworkAccessManager;

/**
 * @class MapWidget
 * @brief Pokazuje stacje na normalnej mapie kafelkowej.
 */
class MapWidget : public QWidget {
    Q_OBJECT
public:
    explicit MapWidget(QWidget* parent = nullptr);

    /// Ustawia stacje widoczne na mapie i dopasowuje widok do wynikow.
    void setStations(const std::vector<Station>& stations);
    /// Podswietla aktualnie wybrana stacje.
    void setSelectedStationId(int stationId);

signals:
    /// Sygnal wysylany po kliknieciu markera stacji.
    void stationClicked(int stationId);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    std::vector<Station> m_stations;
    int m_selectedStationId = -1;
    int m_hoveredStationId = -1;

    QNetworkAccessManager* m_network = nullptr;
    QHash<QString, QPixmap> m_tiles;
    QSet<QString> m_pendingTiles;
    QString m_cacheDir;

    int m_zoom = 6;
    double m_centerLat = 52.1;
    double m_centerLon = 19.2;
    bool m_userMovedMap = false;
    bool m_dragging = false;
    QPoint m_dragStartPos;
    QPointF m_dragStartCenterWorld;

    QPointF latLonToWorld(double lat, double lon, int zoom) const;
    QPointF worldToLatLon(const QPointF& world, int zoom) const;
    QPointF stationToScreen(const Station& station) const;
    const Station* stationAt(const QPoint& pos) const;
    QString stationLabel(const Station& station) const;
    QString tileKey(int zoom, int x, int y) const;
    QString tilePath(int zoom, int x, int y) const;

    void fitStations();
    void requestTile(int zoom, int x, int y);
    void drawTiles(QPainter& painter);
    void drawMarkers(QPainter& painter);
    void drawMarker(QPainter& painter, const QPointF& pos, bool selected, bool hovered) const;
    void setCenterFromWorld(const QPointF& world, int zoom);
};

#endif // MAPWIDGET_H
