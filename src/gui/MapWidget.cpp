/**
 * @file MapWidget.cpp
 * @brief Implementacja mapy kafelkowej ze stacjami.
 */

#include "gui/MapWidget.h"

#include <QDir>
#include <QFile>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPainterPath>
#include <QStandardPaths>
#include <QToolTip>
#include <QUrl>
#include <QWheelEvent>
#include <QtMath>
#include <algorithm>
#include <cmath>

namespace {
constexpr int TILE_SIZE = 256;
constexpr int MIN_ZOOM = 5;
constexpr int MAX_ZOOM = 13;
constexpr double MIN_MERCATOR_LAT = -85.05112878;
constexpr double MAX_MERCATOR_LAT = 85.05112878;

bool hasCoordinates(const Station& station) {
    return std::abs(station.gegrLat) > 0.0001 && std::abs(station.gegrLon) > 0.0001;
}

double clampLatitude(double lat) {
    return std::clamp(lat, MIN_MERCATOR_LAT, MAX_MERCATOR_LAT);
}

double wrapLongitude(double lon) {
    while (lon < -180.0) lon += 360.0;
    while (lon > 180.0) lon -= 360.0;
    return lon;
}
} // namespace

MapWidget::MapWidget(QWidget* parent)
    : QWidget(parent),
      m_network(new QNetworkAccessManager(this))
{
    setMinimumSize(520, 380);
    setMouseTracking(true);
    setCursor(Qt::OpenHandCursor);

    m_cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (m_cacheDir.isEmpty()) {
        m_cacheDir = QDir::currentPath() + "/data/cache";
    }
    m_cacheDir += "/osm-tiles";
    QDir().mkpath(m_cacheDir);
}

void MapWidget::setStations(const std::vector<Station>& stations) {
    m_stations = stations;
    m_userMovedMap = false;
    fitStations();
    update();
}

void MapWidget::setSelectedStationId(int stationId) {
    m_selectedStationId = stationId;
    update();
}

QPointF MapWidget::latLonToWorld(double lat, double lon, int zoom) const {
    lat = clampLatitude(lat);
    lon = wrapLongitude(lon);

    const double scale = static_cast<double>(TILE_SIZE) * (1 << zoom);
    const double x = (lon + 180.0) / 360.0 * scale;

    const double latRad = qDegreesToRadians(lat);
    const double y = (1.0 - std::log(std::tan(latRad) + 1.0 / std::cos(latRad)) / M_PI) / 2.0 * scale;
    return {x, y};
}

QPointF MapWidget::worldToLatLon(const QPointF& world, int zoom) const {
    const double scale = static_cast<double>(TILE_SIZE) * (1 << zoom);
    const double lon = world.x() / scale * 360.0 - 180.0;
    const double n = M_PI - 2.0 * M_PI * world.y() / scale;
    const double lat = qRadiansToDegrees(std::atan(std::sinh(n)));
    return {clampLatitude(lat), wrapLongitude(lon)};
}

QPointF MapWidget::stationToScreen(const Station& station) const {
    const QPointF center = latLonToWorld(m_centerLat, m_centerLon, m_zoom);
    const QPointF point = latLonToWorld(station.gegrLat, station.gegrLon, m_zoom);
    return {
        width() / 2.0 + point.x() - center.x(),
        height() / 2.0 + point.y() - center.y()
    };
}

const Station* MapWidget::stationAt(const QPoint& pos) const {
    const Station* best = nullptr;
    double bestDistance = 13.0;

    for (const auto& station : m_stations) {
        if (!hasCoordinates(station)) continue;

        const QPointF p = stationToScreen(station);
        const double dx = p.x() - pos.x();
        const double dy = p.y() - pos.y();
        const double distance = std::sqrt(dx * dx + dy * dy);
        if (distance < bestDistance) {
            bestDistance = distance;
            best = &station;
        }
    }
    return best;
}

QString MapWidget::stationLabel(const Station& station) const {
    QString label = QString("[%1] %2")
        .arg(station.id)
        .arg(QString::fromStdString(station.stationName));

    if (!station.city.name.empty()) {
        label += "\n" + QString::fromStdString(station.city.name);
    }
    return label;
}

QString MapWidget::tileKey(int zoom, int x, int y) const {
    return QString("%1_%2_%3").arg(zoom).arg(x).arg(y);
}

QString MapWidget::tilePath(int zoom, int x, int y) const {
    return m_cacheDir + "/" + tileKey(zoom, x, y) + ".png";
}

void MapWidget::fitStations() {
    if (m_stations.empty()) {
        m_centerLat = 52.1;
        m_centerLon = 19.2;
        m_zoom = 6;
        return;
    }

    double minLat = 90.0;
    double maxLat = -90.0;
    double minLon = 180.0;
    double maxLon = -180.0;
    int count = 0;

    for (const auto& station : m_stations) {
        if (!hasCoordinates(station)) continue;
        minLat = std::min(minLat, station.gegrLat);
        maxLat = std::max(maxLat, station.gegrLat);
        minLon = std::min(minLon, station.gegrLon);
        maxLon = std::max(maxLon, station.gegrLon);
        count++;
    }

    if (count == 0) {
        m_centerLat = 52.1;
        m_centerLon = 19.2;
        m_zoom = 6;
        return;
    }

    m_centerLat = (minLat + maxLat) / 2.0;
    m_centerLon = (minLon + maxLon) / 2.0;

    if (count == 1) {
        m_zoom = 11;
        return;
    }

    const int viewWidth = std::max(width(), 520) - 90;
    const int viewHeight = std::max(height(), 380) - 90;
    int bestZoom = MIN_ZOOM;

    for (int zoom = MAX_ZOOM; zoom >= MIN_ZOOM; --zoom) {
        const QPointF topLeft = latLonToWorld(maxLat, minLon, zoom);
        const QPointF bottomRight = latLonToWorld(minLat, maxLon, zoom);
        const double dx = std::abs(bottomRight.x() - topLeft.x());
        const double dy = std::abs(bottomRight.y() - topLeft.y());
        if (dx <= viewWidth && dy <= viewHeight) {
            bestZoom = zoom;
            break;
        }
    }

    m_zoom = bestZoom;
}

void MapWidget::requestTile(int zoom, int x, int y) {
    const QString key = tileKey(zoom, x, y);
    if (m_tiles.contains(key) || m_pendingTiles.contains(key)) return;

    const QString path = tilePath(zoom, x, y);
    QPixmap cached;
    if (cached.load(path)) {
        m_tiles.insert(key, cached);
        return;
    }

    m_pendingTiles.insert(key);

    const QUrl url(QString("https://tile.openstreetmap.org/%1/%2/%3.png")
                       .arg(zoom).arg(x).arg(y));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("AirQualityMonitor/1.0 educational Qt project"));

    QNetworkReply* reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, key, path]() {
        m_pendingTiles.remove(key);

        if (reply->error() == QNetworkReply::NoError) {
            const QByteArray data = reply->readAll();
            QPixmap pixmap;
            if (pixmap.loadFromData(data)) {
                m_tiles.insert(key, pixmap);
                QFile file(path);
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(data);
                }
            }
        }

        reply->deleteLater();
        update();
    });
}

void MapWidget::drawTiles(QPainter& painter) {
    painter.fillRect(rect(), QColor(232, 235, 238));

    const QPointF center = latLonToWorld(m_centerLat, m_centerLon, m_zoom);
    const double left = center.x() - width() / 2.0;
    const double top = center.y() - height() / 2.0;
    const double right = left + width();
    const double bottom = top + height();

    const int minTileX = static_cast<int>(std::floor(left / TILE_SIZE));
    const int maxTileX = static_cast<int>(std::floor(right / TILE_SIZE));
    const int minTileY = static_cast<int>(std::floor(top / TILE_SIZE));
    const int maxTileY = static_cast<int>(std::floor(bottom / TILE_SIZE));
    const int tileCount = 1 << m_zoom;

    for (int tx = minTileX; tx <= maxTileX; ++tx) {
        int wrappedX = tx % tileCount;
        if (wrappedX < 0) wrappedX += tileCount;

        for (int ty = minTileY; ty <= maxTileY; ++ty) {
            if (ty < 0 || ty >= tileCount) continue;

            const QRectF target(
                tx * TILE_SIZE - left,
                ty * TILE_SIZE - top,
                TILE_SIZE,
                TILE_SIZE
            );

            const QString key = tileKey(m_zoom, wrappedX, ty);
            if (m_tiles.contains(key)) {
                painter.drawPixmap(target.toRect(), m_tiles.value(key));
            } else {
                painter.fillRect(target, QColor(226, 230, 234));
                painter.setPen(QPen(QColor(205, 212, 220), 1));
                painter.drawRect(target);
                requestTile(m_zoom, wrappedX, ty);
            }
        }
    }
}

void MapWidget::drawMarker(QPainter& painter, const QPointF& pos, bool selected, bool hovered) const {
    const double radius = selected ? 8.0 : (hovered ? 7.0 : 5.5);
    const QColor fill = selected ? QColor(230, 74, 64)
                                 : (hovered ? QColor(255, 180, 55) : QColor(25, 118, 210));

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 55));
    painter.drawEllipse(pos + QPointF(2, 2), radius + 2, radius + 2);

    painter.setPen(QPen(Qt::white, selected ? 3 : 2));
    painter.setBrush(fill);
    painter.drawEllipse(pos, radius, radius);

    painter.setPen(QPen(QColor(38, 50, 56), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(pos, radius, radius);
}

void MapWidget::drawMarkers(QPainter& painter) {
    const Station* selectedStation = nullptr;

    for (const auto& station : m_stations) {
        if (!hasCoordinates(station)) continue;
        if (station.id == m_selectedStationId) {
            selectedStation = &station;
            continue;
        }

        const QPointF p = stationToScreen(station);
        if (!rect().adjusted(-20, -20, 20, 20).contains(p.toPoint())) continue;
        drawMarker(painter, p, false, station.id == m_hoveredStationId);
    }

    if (selectedStation) {
        const QPointF p = stationToScreen(*selectedStation);
        drawMarker(painter, p, true, selectedStation->id == m_hoveredStationId);
    }
}

void MapWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    drawTiles(painter);
    drawMarkers(painter);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255, 215));
    painter.drawRoundedRect(QRectF(12, 12, 270, 38), 8, 8);

    painter.setPen(QColor(32, 43, 54));
    QFont titleFont = font();
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.drawText(QRectF(24, 16, 246, 16),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("Mapa stacji pomiarowych"));

    painter.setFont(font());
    painter.setPen(QColor(74, 86, 99));
    painter.drawText(QRectF(24, 32, 246, 14),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("Przeciagnij mape, kolko myszy = zoom"));

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255, 220));
    painter.drawRoundedRect(QRectF(12, height() - 38, 440, 26), 8, 8);
    painter.setPen(QColor(74, 86, 99));
    painter.drawText(QRectF(24, height() - 34, 420, 18),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("Stacje na mapie: %1. Kliknij marker, aby wybrac stacje.")
                         .arg(m_stations.size()));

    painter.setPen(QColor(74, 86, 99));
    painter.drawText(QRectF(width() - 240, height() - 30, 228, 18),
                     Qt::AlignRight | Qt::AlignVCenter,
                     QStringLiteral("© OpenStreetMap contributors"));

    if (m_stations.empty()) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 255, 230));
        painter.drawRoundedRect(rect().adjusted(width() / 2 - 210, height() / 2 - 45,
                                                -width() / 2 + 210, -height() / 2 + 45),
                                10, 10);
        painter.setPen(QColor(60, 70, 82));
        painter.drawText(rect(), Qt::AlignCenter,
                         QStringLiteral("Pobierz stacje z API albo wczytaj je z bazy,\naby pokazac je na mapie."));
    }
}

void MapWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    m_dragStartPos = event->pos();
    m_dragStartCenterWorld = latLonToWorld(m_centerLat, m_centerLon, m_zoom);
    m_dragging = false;
    setCursor(Qt::ClosedHandCursor);
}

void MapWidget::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        const QPoint delta = event->pos() - m_dragStartPos;
        if (delta.manhattanLength() > 3) {
            m_dragging = true;
            m_userMovedMap = true;
            setCenterFromWorld(m_dragStartCenterWorld - QPointF(delta), m_zoom);
            QToolTip::hideText();
            update();
        }
        return;
    }

    const Station* station = stationAt(event->pos());
    const int newHoveredId = station ? station->id : -1;
    if (newHoveredId != m_hoveredStationId) {
        m_hoveredStationId = newHoveredId;
        update();
    }

    if (station) {
        setCursor(Qt::PointingHandCursor);
        QToolTip::showText(event->globalPosition().toPoint(), stationLabel(*station), this);
    } else {
        setCursor(Qt::OpenHandCursor);
        QToolTip::hideText();
    }
}

void MapWidget::mouseReleaseEvent(QMouseEvent* event) {
    setCursor(Qt::OpenHandCursor);

    if (event->button() == Qt::LeftButton && !m_dragging) {
        if (const Station* station = stationAt(event->pos())) {
            emit stationClicked(station->id);
        }
    }

    m_dragging = false;
}

void MapWidget::leaveEvent(QEvent*) {
    m_hoveredStationId = -1;
    QToolTip::hideText();
    setCursor(Qt::OpenHandCursor);
    update();
}

void MapWidget::wheelEvent(QWheelEvent* event) {
    const int delta = event->angleDelta().y();
    if (delta == 0) return;

    const int oldZoom = m_zoom;
    const int newZoom = std::clamp(m_zoom + (delta > 0 ? 1 : -1), MIN_ZOOM, MAX_ZOOM);
    if (newZoom == oldZoom) return;

    const QPointF cursor = event->position();
    const QPointF oldCenter = latLonToWorld(m_centerLat, m_centerLon, oldZoom);
    const QPointF cursorWorld = oldCenter + (cursor - QPointF(width() / 2.0, height() / 2.0));
    const QPointF cursorLatLon = worldToLatLon(cursorWorld, oldZoom);
    const QPointF newCursorWorld = latLonToWorld(cursorLatLon.x(), cursorLatLon.y(), newZoom);
    const QPointF newCenter = newCursorWorld - (cursor - QPointF(width() / 2.0, height() / 2.0));

    m_zoom = newZoom;
    m_userMovedMap = true;
    setCenterFromWorld(newCenter, newZoom);
    update();
}

void MapWidget::resizeEvent(QResizeEvent*) {
    if (!m_userMovedMap) {
        fitStations();
    }
    update();
}

void MapWidget::setCenterFromWorld(const QPointF& world, int zoom) {
    const QPointF latLon = worldToLatLon(world, zoom);
    m_centerLat = latLon.x();
    m_centerLon = latLon.y();
}
