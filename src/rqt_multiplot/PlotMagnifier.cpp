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

#include <QMouseEvent>

#include <qwt/qwt_plot.h>
#include <qwt/qwt_scale_div.h>
#include <qwt/qwt_scale_map.h>

#include "rqt_multiplot/PlotMouseBindings.hpp"

#include "rqt_multiplot/PlotMagnifier.hpp"

namespace rqt_multiplot {

namespace {

void rescaleAxis(QwtPlot* plot, int axisId, double factor) {
  if (factor == 1.0) {
    return;
  }

  const auto axis = static_cast<QwtPlot::Axis>(axisId);
#if QWT_VERSION >= 0x060100
  const QwtScaleDiv& scaleDiv = plot->axisScaleDiv(axis);
#else
  const QwtScaleDiv& scaleDiv = *plot->axisScaleDiv(axis);
#endif

#if QWT_VERSION < 0x060100
  if (!scaleDiv.isValid()) {
    return;
  }
#endif

  const QwtScaleMap map = plot->canvasMap(axis);
  const double transformedLower = map.transform(scaleDiv.lowerBound());
  const double transformedUpper = map.transform(scaleDiv.upperBound());
  const double center = 0.5 * (transformedLower + transformedUpper);
  const double halfWidth = 0.5 * std::fabs(transformedUpper - transformedLower) * factor;
  const double first = map.invTransform(center - halfWidth);
  const double second = map.invTransform(center + halfWidth);
  if (first < second) {
    plot->setAxisScale(axis, first, second);
  } else if (second < first) {
    plot->setAxisScale(axis, second, first);
  }
}

}  // namespace

PlotMagnifier::PlotMagnifier(QWidget* canvas) : QwtPlotMagnifier(canvas), magnifying_(false), dragStarted_(false) {}

PlotMagnifier::~PlotMagnifier() = default;

void PlotMagnifier::rescale(double xFactor, double yFactor) {
  double fx = std::fabs(xFactor);
  double fy = std::fabs(yFactor);

  if ((fx == 1.0) && (fy == 1.0)) {
    return;
  }

  bool autoReplot = plot()->autoReplot();

  plot()->setAutoReplot(false);

  rescaleAxis(plot(), QwtPlot::xBottom, fx);
  rescaleAxis(plot(), QwtPlot::yLeft, fy);

  plot()->setAutoReplot(autoReplot);
  plot()->replot();
}

void PlotMagnifier::widgetMousePressEvent(QMouseEvent* event) {
  QwtPlotMagnifier::widgetMousePressEvent(event);

#if QWT_VERSION >= 0x060100
  Qt::MouseButton button{};
  Qt::KeyboardModifiers buttonState{};
#else
  int button = 0;
  int buttonState = 0;
#endif

  getMouseButton(button, buttonState);

  if (event->button() != button || (parentWidget() == nullptr)) {
    return;
  }

  if (static_cast<int>(event->modifiers() & Qt::KeyboardModifierMask) != static_cast<int>(buttonState & Qt::KeyboardModifierMask)) {
    return;
  }

  magnifying_ = true;
  dragStarted_ = false;
  position_ = mouseEventPosition(*event);
}

void PlotMagnifier::widgetMouseMoveEvent(QMouseEvent* event) {
  if (!magnifying_) {
    return;
  }

  const QPoint eventPosition = mouseEventPosition(*event);
  if (!dragStarted_) {
    if (isStationaryClick(position_, eventPosition)) {
      return;
    }
    dragStarted_ = true;
  }
  int dx = eventPosition.x() - position_.x();
  int dy = eventPosition.y() - position_.y();

  double fx = 1.0;
  double fy = 1.0;

  if (dx != 0) {
    fx = mouseFactor();

    if (dx < 0) {
      fx = 1.0 / fx;
    }
  }

  if (dy != 0) {
    fy = mouseFactor();

    if (dy < 0) {
      fy = 1.0 / fy;
    }
  }

  rescale(fx, fy);

  position_ = eventPosition;
}

void PlotMagnifier::widgetMouseReleaseEvent(QMouseEvent* event) {
  QwtPlotMagnifier::widgetMouseReleaseEvent(event);

  magnifying_ = false;
  dragStarted_ = false;
}

}  // namespace rqt_multiplot
