/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 *                                                                            *
 * This program is free software; you can redistribute it and/or modify       *
 * it under the terms of the Lesser GNU General Public License as published by*
 * the Free Software Foundation; either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the               *
 * Lesser GNU General Public License for more details.                        *
 *                                                                            *
 * You should have received a copy of the Lesser GNU General Public License   *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.       *
 ******************************************************************************/

#pragma once

#include <QColor>
#include <QFont>
#include <QPoint>
#include <QRect>
#include <QRegion>
#include <QString>
#include <QStringList>
#include <QTimeZone>
#include <QVector>

#include <qwt/qwt_plot_picker.h>

#include "rqt_multiplot/AxisTimeFormat.hpp"
#include "rqt_multiplot/PlotCursorLabel.hpp"

class QWidget;

namespace rqt_multiplot {

class PlotCursor : public QwtPlotPicker {
  Q_OBJECT
 public:
  static constexpr int kTrackPointSnapPixels = rqt_multiplot::kTrackPointSnapPixels;

  explicit PlotCursor(QWidget* canvas);
  ~PlotCursor() override;

  void setActive(bool active, const QPointF& position = QPointF(0.0, 0.0));
  using QwtPlotPicker::isActive;
  void setCurrentPosition(const QPointF& position);
  const QPointF& getCurrentPosition() const;
  void setTrackPoints(bool track);
  bool arePointsTracked() const;
  bool hasMouseControl() const;
  void setXOffset(double offset);
  double getXOffset() const;
  void setYOffset(double offset);
  double getYOffset() const;
  void setXTimeLabelMode(AxisTimeFormat::LabelMode mode);
  AxisTimeFormat::LabelMode xTimeLabelMode() const;
  void setYTimeLabelMode(AxisTimeFormat::LabelMode mode);
  AxisTimeFormat::LabelMode yTimeLabelMode() const;
  void setTimeZone(const QTimeZone& zone);
  const QTimeZone& timeZone() const;

  QColor trackerTextColor() const;
  QColor trackerBackgroundColor() const;
  void updateOverlayPens();

  void drawRubberBand(QPainter* painter) const override;
  QRegion rubberBandMask() const override;

 signals:
  void activeChanged(bool active);
  void currentPositionChanged(const QPointF& position);

 protected:
  QwtText trackerTextF(const QPointF& point) const override;

  void begin() override;
  void move(const QPoint& point) override;
  bool end(bool ok) override;

  bool eventFilter(QObject* object, QEvent* event) override;

  void updateDisplay() override;
  void updateTrackedPoints();

  void drawTrackedPoints(QPainter* painter) const;
  void drawTrackedPointReadout(QPainter* painter) const;
  QString formatCoordinate(double value, bool isX) const;
  QVector<TrackedReadoutRow> trackedReadoutRows() const;
  QRect trackedReadoutRect(const QFont& font) const;

 private:
  struct TrackedPoint {
    QPointF position;
    QColor color;
    QString title;
  };

  QPointF currentPosition_;
  QVector<TrackedPoint> trackedPoints_;

  bool trackPoints_;
  bool mouseControl_;
  double xOffset_;
  double yOffset_;
  AxisTimeFormat::LabelMode xTimeLabelMode_;
  AxisTimeFormat::LabelMode yTimeLabelMode_;
  QTimeZone timeZone_;

 private slots:
  void plotXAxisScaleDivChanged();
  void plotYAxisScaleDivChanged();
};

}  // namespace rqt_multiplot
