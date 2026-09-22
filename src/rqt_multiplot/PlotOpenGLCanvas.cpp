/******************************************************************************
 * Copyright (C) 2026 by Samuel Bachmann                                      *
 *                                                                            *
 * This program is free software; you can redistribute it and/or modify       *
 * it under the terms of the Lesser GNU General Public License as published by*
 * the Free Software Foundation; either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 ******************************************************************************/

#include "rqt_multiplot/PlotOpenGLCanvas.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QSurfaceFormat>

#include <qwt/qwt_plot.h>

#include "rqt_multiplot/PlotCanvasPolicy.hpp"

namespace rqt_multiplot {

PlotOpenGLCanvas::PlotOpenGLCanvas(QwtPlot* plot) : QOpenGLWidget(plot) {
  setFormat(openGLCanvasSurfaceFormat(format()));
  setAttribute(Qt::WA_OpaquePaintEvent, true);
  setAutoFillBackground(false);
#ifndef QT_NO_CURSOR
  setCursor(Qt::CrossCursor);
#endif
}

void PlotOpenGLCanvas::setFrameStyle(int /*style*/) {}

// QwtPlotRenderer invokes borderPath via QMetaObject; keep as instance method.
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
QPainterPath PlotOpenGLCanvas::borderPath(const QRect& /*rect*/) const {
  return QPainterPath();
}

void PlotOpenGLCanvas::replot() {
  update();
}

void PlotOpenGLCanvas::paintEvent(QPaintEvent* event) {
  QPainter painter(this);
  painter.setClipRegion(event->region());
  painter.fillRect(contentsRect(), palette().brush(backgroundRole()));

  auto* plot = qobject_cast<QwtPlot*>(parentWidget());
  if (plot == nullptr) {
    return;
  }

  painter.save();
  painter.setClipRect(contentsRect(), Qt::IntersectClip);
  plot->drawCanvas(&painter);
  painter.restore();
}

}  // namespace rqt_multiplot
