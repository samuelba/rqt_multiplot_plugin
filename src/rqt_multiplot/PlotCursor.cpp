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

#include <cmath>
#include <limits>

#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QPen>
#include <QResizeEvent>
#include <QSize>
#include <QStringList>
#include <QtMath>

#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_curve.h>
#include <qwt/qwt_scale_map.h>
#include <qwt/qwt_scale_widget.h>
#include <qwt/qwt_text.h>

#include "rqt_multiplot/AxisTimeFormat.hpp"
#include "rqt_multiplot/CurveData.hpp"
#include "rqt_multiplot/PlotCursorLabel.hpp"
#include "rqt_multiplot/PlotCursorMachine.hpp"

#include "rqt_multiplot/PlotCursor.hpp"

namespace rqt_multiplot {

PlotCursor::PlotCursor(QWidget* canvas)
    : QwtPlotPicker(canvas),
      trackPoints_(false),
      mouseControl_(false),
      xOffset_(0.0),
      yOffset_(0.0),
      xTimeLabelMode_(AxisTimeFormat::LabelMode::Off),
      yTimeLabelMode_(AxisTimeFormat::LabelMode::Off),
      timeZone_(QTimeZone::utc()) {
  setTrackerMode(QwtPicker::AlwaysOn);
  setStateMachine(new PlotCursorMachine());

  setRubberBand(QwtPicker::CrossRubberBand);
  updateOverlayPens();

  connect(plot()->axisWidget(xAxis()), SIGNAL(scaleDivChanged()), this, SLOT(plotXAxisScaleDivChanged()));
  connect(plot()->axisWidget(yAxis()), SIGNAL(scaleDivChanged()), this, SLOT(plotYAxisScaleDivChanged()));
}

PlotCursor::~PlotCursor() = default;

void PlotCursor::setActive(bool active, const QPointF& position) {
  if (mouseControl_) {
    return;
  }

  if (active && !isActive()) {
    setTrackerMode(QwtPicker::AlwaysOff);

    begin();
    append(transform(position));

    currentPosition_ = position;

    emit currentPositionChanged(position);
  } else if (!active && isActive()) {
    remove();
    end(true);

    setTrackerMode(QwtPicker::AlwaysOn);
  }
}

void PlotCursor::setCurrentPosition(const QPointF& position) {
  if (mouseControl_) {
    return;
  }

  if (isActive() && (position != currentPosition_)) {
    currentPosition_ = position;

    blockSignals(true);
    move(transform(position));
    blockSignals(false);
  }
}

const QPointF& PlotCursor::getCurrentPosition() const {
  return currentPosition_;
}

QColor PlotCursor::trackerTextColor() const {
  if ((plot() != nullptr) && (plot()->canvas() != nullptr)) {
    return plot()->canvas()->palette().color(QPalette::WindowText);
  }
  return Qt::black;
}

QColor PlotCursor::trackerBackgroundColor() const {
  QColor background = Qt::white;
  if ((plot() != nullptr) && (plot()->canvas() != nullptr)) {
    background = plot()->canvas()->palette().color(QPalette::Window);
  }
  background.setAlpha(230);
  return background;
}

void PlotCursor::updateOverlayPens() {
  const QPen pen(trackerTextColor(), 0, Qt::DashLine);
  setRubberBandPen(pen);
  setTrackerPen(QPen(trackerTextColor()));
}

void PlotCursor::setTrackPoints(bool track) {
  if (track != trackPoints_) {
    trackPoints_ = track;

    if (isActive()) {
      updateDisplay();
    }
  }
}

bool PlotCursor::arePointsTracked() const {
  return trackPoints_;
}

bool PlotCursor::hasMouseControl() const {
  return mouseControl_;
}

void PlotCursor::setXOffset(double offset) {
  xOffset_ = offset;
}

double PlotCursor::getXOffset() const {
  return xOffset_;
}

void PlotCursor::setYOffset(double offset) {
  yOffset_ = offset;
}

double PlotCursor::getYOffset() const {
  return yOffset_;
}

void PlotCursor::setXTimeLabelMode(AxisTimeFormat::LabelMode mode) {
  xTimeLabelMode_ = mode;
}

AxisTimeFormat::LabelMode PlotCursor::xTimeLabelMode() const {
  return xTimeLabelMode_;
}

void PlotCursor::setYTimeLabelMode(AxisTimeFormat::LabelMode mode) {
  yTimeLabelMode_ = mode;
}

AxisTimeFormat::LabelMode PlotCursor::yTimeLabelMode() const {
  return yTimeLabelMode_;
}

void PlotCursor::setTimeZone(const QTimeZone& zone) {
  timeZone_ = zone;
}

const QTimeZone& PlotCursor::timeZone() const {
  return timeZone_;
}

QString PlotCursor::formatCoordinate(double value, bool isX) const {
  const int axis = isX ? xAxis() : yAxis();
  const QwtScaleMap map = plot()->canvasMap(axis);
  const double span = fabs(map.invTransform(1.0) - map.invTransform(0.0));
  const double offset = isX ? xOffset_ : yOffset_;
  const AxisTimeFormat::LabelMode mode = isX ? xTimeLabelMode_ : yTimeLabelMode_;
  return AxisTimeFormat::coordinate(value, offset, span, mode, timeZone_);
}

QVector<TrackedReadoutRow> PlotCursor::trackedReadoutRows() const {
  QVector<TrackedReadoutRow> rows;
  for (const auto& tracked : trackedPoints_) {
    TrackedReadoutRow row;
    row.title = tracked.title;
    row.x = formatCoordinate(tracked.position.x(), true);
    row.y = formatCoordinate(tracked.position.y(), false);
    rows.append(row);
  }
  return rows;
}

QRect PlotCursor::trackedReadoutRect(const QFont& font) const {
  const QVector<TrackedReadoutRow> rows = trackedReadoutRows();
  if (rows.isEmpty()) {
    return {};
  }

  constexpr int kPadding = 4;
  const TrackedReadoutLayout layout = trackedReadoutLayout(rows, font);
  const QSize size = trackedReadoutSize(layout, static_cast<int>(rows.size()));
  const QRect canvas(0, 0, plot()->canvas()->width(), plot()->canvas()->height());
  return trackedPointsReadoutRect(transform(currentPosition_), size, canvas).adjusted(-kPadding, -kPadding, kPadding, kPadding);
}

QwtText PlotCursor::trackerTextF(const QPointF& point) const {
  if (trackPoints_ && !trackedPoints_.isEmpty()) {
    return {};
  }

  QwtText text(trackedPointLabel(QString(), formatCoordinate(point.x(), true), formatCoordinate(point.y(), false)));
  text.setColor(trackerTextColor());
  text.setBackgroundBrush(trackerBackgroundColor());
  text.setPaintAttribute(QwtText::PaintBackground, true);
  return text;
}

void PlotCursor::drawRubberBand(QPainter* painter) const {
  QwtPlotPicker::drawRubberBand(painter);

  drawTrackedPoints(painter);
}

QRegion PlotCursor::rubberBandMask() const {
  QRegion mask = QwtPlotPicker::rubberBandMask();
  if (!trackPoints_) {
    return mask;
  }

  for (const auto& tracked : trackedPoints_) {
    const QPoint point = transform(tracked.position);
    const int extent = kTrackPointMarkerRadius + 1;
    mask += QRect(point.x() - extent, point.y() - extent, 2 * extent + 1, 2 * extent + 1);
  }

  const QRect readout = trackedReadoutRect(plot()->canvas()->font());
  if (!readout.isEmpty()) {
    mask += readout;
  }

  return mask;
}

void PlotCursor::begin() {
  bool active = isActive();

  QwtPlotPicker::begin();

  if (!active && isActive()) {
    emit activeChanged(true);
  }
}

void PlotCursor::move(const QPoint& point) {
  QPointF newPosition = invTransform(point);

  if (newPosition != currentPosition_) {
    currentPosition_ = newPosition;

    updateDisplay();

    emit currentPositionChanged(newPosition);
  }

  QwtPlotPicker::move(point);
}

bool PlotCursor::end(bool ok) {
  bool active = isActive();

  bool result = QwtPlotPicker::end(ok);

  if (active && !isActive()) {
    emit activeChanged(false);
  }

  return result;
}

bool PlotCursor::eventFilter(QObject* object, QEvent* event) {
  if (object == plot()->canvas()) {
    if (event->type() == QEvent::Enter) {
      mouseControl_ = true;
    } else if (event->type() == QEvent::Leave) {
      mouseControl_ = false;
    } else if (event->type() == QEvent::MouseButtonRelease) {
      updateDisplay();
    }
  }

  bool result = QwtPlotPicker::eventFilter(object, event);

  if (isActive() && object == plot()->canvas()) {
    if (event->type() == QEvent::Resize) {
      transition(event);
    }
  }

  return result;
}

void PlotCursor::updateDisplay() {
  updateTrackedPoints();

  QwtPlotPicker::updateDisplay();
}

void PlotCursor::updateTrackedPoints() {
  trackedPoints_.clear();

  if (!trackPoints_ || !isActive()) {
    return;
  }

  const QwtScaleMap map = plot()->canvasMap(xAxis());
  const double maxDistance = trackPointSnapDistance(map.invTransform(1.0) - map.invTransform(0.0), kTrackPointSnapPixels);

  for (auto* it : plot()->itemList()) {
    if (it->rtti() == QwtPlotItem::Rtti_PlotCurve) {
      auto* curve = dynamic_cast<QwtPlotCurve*>(it);
      auto* data = dynamic_cast<CurveData*>(curve->data());

      if ((data != nullptr) && curve->isVisible()) {
        QVector<size_t> indexes = data->getPointsInDistance(currentPosition_.x(), maxDistance);

        if (!indexes.isEmpty()) {
          TrackedPoint trackedPoint;
          trackedPoint.color = curve->pen().color();
          trackedPoint.title = curve->title().text();

          QPointF nearest;
          double minDx = std::numeric_limits<double>::infinity();
          for (int index = 0; index < indexes.count(); ++index) {
            const QPointF point = data->getPoint(indexes[index]);
            const double dx = std::fabs(point.x() - currentPosition_.x());
            if (dx < minDx) {
              minDx = dx;
              nearest = point;
            }
          }
          trackedPoint.position = nearest;

          trackedPoints_.append(trackedPoint);
        }
      }
    }
  }
}

void PlotCursor::drawTrackedPoints(QPainter* painter) const {
  if (!trackPoints_) {
    return;
  }

  painter->save();
  painter->setRenderHint(QPainter::Antialiasing, true);
  painter->setPen(Qt::NoPen);
  for (const auto& tracked : trackedPoints_) {
    const QPoint point = transform(tracked.position);
    painter->setBrush(tracked.color);
    painter->drawEllipse(point, kTrackPointMarkerRadius, kTrackPointMarkerRadius);
  }
  painter->restore();

  drawTrackedPointReadout(painter);
}

void PlotCursor::drawTrackedPointReadout(QPainter* painter) const {
  const QVector<TrackedReadoutRow> rows = trackedReadoutRows();
  const QRect background = trackedReadoutRect(painter->font());
  if (background.isEmpty()) {
    return;
  }

  painter->save();
  painter->fillRect(background, trackerBackgroundColor());
  QColor border = trackerTextColor();
  border.setAlpha(180);
  painter->setPen(border);
  painter->drawRect(background.adjusted(0, 0, -1, -1));

  const QRect content = background.adjusted(4, 4, -4, -4);
  const TrackedReadoutLayout layout = trackedReadoutLayout(rows, painter->font());
  const QColor textColor = trackerTextColor();
  const int xColumn = content.left() + layout.titleWidth + layout.columnGap;
  const int yColumn = xColumn + layout.xWidth + layout.columnGap;
  int y = content.top();

  painter->setPen(textColor);
  for (const TrackedReadoutRow& row : rows) {
    painter->drawText(content.left(), y, layout.titleWidth, layout.rowHeight, Qt::AlignLeft | Qt::AlignVCenter, row.title);
    painter->drawText(xColumn, y, layout.xWidth, layout.rowHeight, Qt::AlignRight | Qt::AlignVCenter, row.x);
    painter->drawText(yColumn, y, layout.yWidth, layout.rowHeight, Qt::AlignRight | Qt::AlignVCenter, row.y);
    y += layout.rowHeight;
  }
  painter->restore();
}

void PlotCursor::plotXAxisScaleDivChanged() {
  if (isActive()) {
    if (mouseControl_) {
      QPointF newPosition = currentPosition_;

      newPosition.setX(plot()->canvasMap(xAxis()).invTransform(pickedPoints()[0].x()));

      if (newPosition != currentPosition_) {
        currentPosition_ = newPosition;

        updateDisplay();

        emit currentPositionChanged(newPosition);
      }
    } else {
      QPoint newPosition = pickedPoints()[0];

      newPosition.setX(qRound(plot()->canvasMap(xAxis()).transform(currentPosition_.x())));

      blockSignals(true);
      move(newPosition);
      blockSignals(false);
    }
  }
}

void PlotCursor::plotYAxisScaleDivChanged() {
  if (isActive()) {
    if (mouseControl_) {
      QPointF newPosition = currentPosition_;

      newPosition.setY(plot()->canvasMap(yAxis()).invTransform(pickedPoints()[0].y()));

      if (newPosition != currentPosition_) {
        currentPosition_ = newPosition;

        updateDisplay();

        emit currentPositionChanged(newPosition);
      }
    } else {
      QPoint newPosition = pickedPoints()[0];

      newPosition.setY(qRound(plot()->canvasMap(yAxis()).transform(currentPosition_.y())));

      blockSignals(true);
      move(newPosition);
      blockSignals(false);
    }
  }
}

}  // namespace rqt_multiplot
