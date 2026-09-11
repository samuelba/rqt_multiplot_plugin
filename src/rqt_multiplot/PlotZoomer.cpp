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

#include <QMouseEvent>
#include <QPainter>
#include <QPen>

#include <qwt/qwt_plot_canvas.h>

#include "rqt_multiplot/PlotZoomer.h"

namespace rqt_multiplot {

/*****************************************************************************/
/* Constructors and Destructor                                               */
/*****************************************************************************/

PlotZoomer::PlotZoomer(QwtPlotCanvas* canvas, bool doReplot) : QwtPlotZoomer(canvas, doReplot) {
  setMousePattern(MouseSelect1, Qt::LeftButton, Qt::ControlModifier);
  setRubberBand(RectRubberBand);
  setRubberBandPen(QPen(Qt::DashLine));
}

PlotZoomer::~PlotZoomer() = default;

/*****************************************************************************/
/* Methods                                                                   */
/*****************************************************************************/

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
  if (mouseMatch(MouseSelect2, event)) {
    position_ = event->pos();
  }

  QwtPlotZoomer::widgetMousePressEvent(event);
}

void PlotZoomer::widgetMouseReleaseEvent(QMouseEvent* event) {
  if (mouseMatch(MouseSelect2, event)) {
    if (position_ == event->pos()) {
      zoom(0);
      emit zoomResetRequested();
    }
  } else {
    QwtPlotZoomer::widgetMouseReleaseEvent(event);
  }
}

}  // namespace rqt_multiplot
