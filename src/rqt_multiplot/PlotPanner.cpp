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

#include <QCursor>
#include <QEvent>
#include <QMouseEvent>
#include <QPalette>
#include <QWidget>

#include <qwt/qwt_plot.h>
#include <qwt/qwt_scale_div.h>

#include <rqt_multiplot/PackageResource.h>
#include <rqt_multiplot/PlotMouseBindings.h>

#include "rqt_multiplot/PlotPanner.h"

namespace rqt_multiplot {

/*****************************************************************************/
/* Constructors and Destructor                                               */
/*****************************************************************************/

PlotPanner::PlotPanner(QWidget* canvas) : QObject(canvas), canvas_(canvas), panning_(false) {
  refreshCursor();

  if (canvas != nullptr) {
    canvas->installEventFilter(this);
  }
}

PlotPanner::~PlotPanner() = default;

QwtPlot* PlotPanner::plot() const {
  if (canvas_ == nullptr) {
    return nullptr;
  }
  return qobject_cast<QwtPlot*>(canvas_->parent());
}

void PlotPanner::refreshCursor() {
  cursor_ = QCursor(packagePixmap(QStringLiteral("resource/move.svg"), QSize(23, 23)), 11, 11);
}

/*****************************************************************************/
/* Methods                                                                   */
/*****************************************************************************/

bool PlotPanner::eventFilter(QObject* object, QEvent* event) {
  if (object == canvas_) {
    QwtPlot* plotWidget = plot();
    if (event->type() == QEvent::PaletteChange) {
      refreshCursor();
    }
    if ((plotWidget != nullptr) && !panning_ && (event->type() == QEvent::MouseButtonPress)) {
      auto* mouseEvent = dynamic_cast<QMouseEvent*>(event);

      if (isPanMouse(mouseEvent->button(), mouseEvent->modifiers())) {
        position_ = mouseEventPosition(*mouseEvent);

        xMap_ = plotWidget->canvasMap(QwtPlot::xBottom);
        yMap_ = plotWidget->canvasMap(QwtPlot::yLeft);

#if QWT_VERSION >= 0x060100
        QPointF minimum(plotWidget->axisScaleDiv(QwtPlot::xBottom).lowerBound(), plotWidget->axisScaleDiv(QwtPlot::yLeft).lowerBound());
        QPointF maximum(plotWidget->axisScaleDiv(QwtPlot::xBottom).upperBound(), plotWidget->axisScaleDiv(QwtPlot::yLeft).upperBound());
#else
        QPointF minimum(plotWidget->axisScaleDiv(QwtPlot::xBottom)->lowerBound(), plotWidget->axisScaleDiv(QwtPlot::yLeft)->lowerBound());
        QPointF maximum(plotWidget->axisScaleDiv(QwtPlot::xBottom)->upperBound(), plotWidget->axisScaleDiv(QwtPlot::yLeft)->upperBound());
#endif

        bounds_.setMinimum(minimum);
        bounds_.setMaximum(maximum);

        canvasCursor_ = canvas_->cursor();
        canvas_->setCursor(cursor_);

        panning_ = true;
      }
    } else if ((plotWidget != nullptr) && panning_ && (event->type() == QEvent::MouseMove)) {
      auto* mouseEvent = dynamic_cast<QMouseEvent*>(event);

      const QPoint eventPosition = mouseEventPosition(*mouseEvent);
      double dx = eventPosition.x() - position_.x();
      double dy = eventPosition.y() - position_.y();

      QPointF minimum(xMap_.invTransform(xMap_.transform(bounds_.getMinimum().x()) - dx),
                      yMap_.invTransform(yMap_.transform(bounds_.getMinimum().y()) - dy));
      QPointF maximum(xMap_.invTransform(xMap_.transform(bounds_.getMaximum().x()) - dx),
                      yMap_.invTransform(yMap_.transform(bounds_.getMaximum().y()) - dy));

      bool autoReplot = plotWidget->autoReplot();
      plotWidget->setAutoReplot(false);

      plotWidget->setAxisScale(QwtPlot::xBottom, minimum.x(), maximum.x());
      plotWidget->setAxisScale(QwtPlot::yLeft, minimum.y(), maximum.y());

      plotWidget->setAutoReplot(autoReplot);
      plotWidget->replot();
    } else if (panning_ && (event->type() == QEvent::MouseButtonRelease)) {
      canvas_->setCursor(canvasCursor_);

      panning_ = false;
    }
  }

  return false;
}

}  // namespace rqt_multiplot
