/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#ifndef RQT_MULTIPLOT_PLOT_CURSOR_LABEL_H
#define RQT_MULTIPLOT_PLOT_CURSOR_LABEL_H

#include <cmath>
#include <limits>

#include <QFont>
#include <QFontMetrics>
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

struct TrackedReadoutRow {
  QString title;
  QString x;
  QString y;
};

struct TrackedReadoutLayout {
  int titleWidth = 0;
  int xWidth = 0;
  int yWidth = 0;
  int columnGap = 8;
  int rowHeight = 0;
};

inline TrackedReadoutLayout trackedReadoutLayout(const QVector<TrackedReadoutRow>& rows, const QFont& font) {
  const QFontMetrics metrics(font);
  TrackedReadoutLayout layout;
  layout.rowHeight = metrics.height();
  for (const TrackedReadoutRow& row : rows) {
    layout.titleWidth = std::max(layout.titleWidth, metrics.horizontalAdvance(row.title));
    layout.xWidth = std::max(layout.xWidth, metrics.horizontalAdvance(row.x));
    layout.yWidth = std::max(layout.yWidth, metrics.horizontalAdvance(row.y));
  }
  return layout;
}

inline QSize trackedReadoutSize(const TrackedReadoutLayout& layout, int rowCount) {
  if (rowCount <= 0) {
    return {};
  }
  const int width = layout.titleWidth + layout.columnGap + layout.xWidth + layout.columnGap + layout.yWidth;
  const int height = layout.rowHeight * rowCount;
  return {width, height};
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
