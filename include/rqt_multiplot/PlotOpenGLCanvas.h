/******************************************************************************
 * Copyright (C) 2026 by Samuel Bachmann                                      *
 *                                                                            *
 * This program is free software; you can redistribute it and/or modify       *
 * it under the terms of the Lesser GNU General Public License as published by*
 * the Free Software Foundation; either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 ******************************************************************************/

#ifndef RQT_MULTIPLOT_PLOT_OPENGL_CANVAS_H
#define RQT_MULTIPLOT_PLOT_OPENGL_CANVAS_H

#include <QOpenGLWidget>
#include <QPaintEvent>

class QwtPlot;

namespace rqt_multiplot {

class PlotOpenGLCanvas : public QOpenGLWidget {
  Q_OBJECT
 public:
  explicit PlotOpenGLCanvas(QwtPlot* plot = nullptr);
  void setFrameStyle(int style);

 public slots:
  void replot();

 protected:
  void paintEvent(QPaintEvent* event) override;
};

}  // namespace rqt_multiplot

#endif
