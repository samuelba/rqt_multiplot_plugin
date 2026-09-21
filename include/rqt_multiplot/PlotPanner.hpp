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

#include <QCursor>
#include <QObject>
#include <QPointF>

#include <qwt/qwt_scale_map.h>

#include "rqt_multiplot/BoundingRectangle.hpp"

class QWidget;
class QwtPlot;

namespace rqt_multiplot {

class PlotPanner : public QObject {
  Q_OBJECT
 public:
  explicit PlotPanner(QWidget* canvas);
  ~PlotPanner() override;

 protected:
  bool eventFilter(QObject* object, QEvent* event) override;

 private:
  QWidget* canvas_;
  QwtPlot* plot() const;

  bool panning_;

  QPoint position_;
  QCursor cursor_;
  QCursor canvasCursor_;

  QwtScaleMap xMap_;
  QwtScaleMap yMap_;
  BoundingRectangle bounds_;

  void refreshCursor();
};

}  // namespace rqt_multiplot
