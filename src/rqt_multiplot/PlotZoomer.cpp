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

#include <QColor>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QPen>

#include <qwt/qwt_plot.h>

#include "rqt_multiplot/PlotMouseBindings.hpp"

#include "rqt_multiplot/PlotZoomer.hpp"

namespace rqt_multiplot {

PlotZoomer::PlotZoomer(QWidget* canvas, bool doReplot) : QwtPlotZoomer(canvas, doReplot) {
  setMousePattern(MouseSelect1, Qt::LeftButton, Qt::ControlModifier);
  setRubberBand(RectRubberBand);
  updateOverlayPens();
}

PlotZoomer::~PlotZoomer() = default;

void PlotZoomer::updateOverlayPens() {
  QColor color = Qt::black;
  if ((plot() != nullptr) && (plot()->canvas() != nullptr)) {
    color = plot()->canvas()->palette().color(QPalette::WindowText);
  }
  setRubberBandPen(QPen(color, 0, Qt::DashLine));
}

QRect PlotZoomer::selectionRect() const {
  if (pickedPoints().count() < 2) {
    return {};
  }

  return QRect(pickedPoints().first(), pickedPoints().last()).normalized();
}

void PlotZoomer::drawRubberBand(QPainter* painter) const {
  const QRect rect = selectionRect();
  if (!isActive() || rect.isNull()) {
    return;
  }

  painter->save();
  painter->setClipping(false);
  painter->setPen(rubberBandPen());
  painter->drawRect(rect);
  painter->restore();
}

QRegion PlotZoomer::rubberBandMask() const {
  const QRect rect = selectionRect();
  if (rect.isNull()) {
    return {};
  }

  return QRegion(rect.adjusted(-2, -2, 2, 2));
}

void PlotZoomer::widgetMousePressEvent(QMouseEvent* event) {
  if (isZoomResetMouse(event->button())) {
    position_ = mouseEventPosition(*event);
    pressRecorded_ = true;
  }

  QwtPlotZoomer::widgetMousePressEvent(event);
}

void PlotZoomer::widgetMouseReleaseEvent(QMouseEvent* event) {
  if (isZoomResetMouse(event->button())) {
    if (pressRecorded_ && isZoomResetClick(event->button(), position_, mouseEventPosition(*event))) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
      emit contextMenuRequested(event->globalPosition().toPoint());
#else
      emit contextMenuRequested(event->globalPos());
#endif
    }
    pressRecorded_ = false;
    return;
  }

  QwtPlotZoomer::widgetMouseReleaseEvent(event);
}

}  // namespace rqt_multiplot
