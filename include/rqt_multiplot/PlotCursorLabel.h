/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#ifndef RQT_MULTIPLOT_PLOT_CURSOR_LABEL_H
#define RQT_MULTIPLOT_PLOT_CURSOR_LABEL_H

#include <cmath>
#include <limits>

#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QVector>

namespace rqt_multiplot {

constexpr int kTrackPointSnapPixels = 16;
constexpr int kTrackPointMarkerRadius = 3;

inline double trackPointSnapDistance(double unitsPerPixel, int pixels = kTrackPointSnapPixels) {
  return std::fabs(unitsPerPixel) * static_cast<double>(pixels);
}

inline QPointF nearestPointByX(const QVector<QPointF>& points, double cursorX) {
  QPointF nearest;
  double minDx = std::numeric_limits<double>::infinity();
  for (const QPointF& point : points) {
    const double dx = std::fabs(point.x() - cursorX);
    if (dx < minDx) {
      minDx = dx;
      nearest = point;
    }
  }
  return nearest;
}

inline QString trackedPointLabel(const QString& title, const QString& x, const QString& y) {
  const QString coordinates = x + QStringLiteral(", ") + y;
  if (title.isEmpty()) {
    return coordinates;
  }
  return title + QStringLiteral(": ") + coordinates;
}

inline QString trackedPointLabels(const QStringList& lines) {
  return lines.join(QLatin1Char('\n'));
}

inline QRect trackedPointsReadoutRect(const QPoint& cursor, const QSize& size, const QRect& canvas, int margin = 5) {
  int x = cursor.x() + margin;
  int y = cursor.y() - size.height() - margin;

  if (x + size.width() > canvas.right() - margin) {
    x = cursor.x() - size.width() - margin;
  }
  if (y < canvas.top() + margin) {
    y = cursor.y() + margin;
  }
  if (y + size.height() > canvas.bottom() - margin) {
    y = canvas.bottom() - margin - size.height();
  }
  if (x < canvas.left() + margin) {
    x = canvas.left() + margin;
  }

  return {QPoint(x, y), size};
}

}  // namespace rqt_multiplot

#endif
