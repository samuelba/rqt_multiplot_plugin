/******************************************************************************
 * Copyright (C) 2026 by Samuel Bachmann                                      *
 *                                                                            *
 * This program is free software; you can redistribute it and/or modify       *
 * it under the terms of the Lesser GNU General Public License as published by*
 * the Free Software Foundation; either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 ******************************************************************************/

#ifndef RQT_MULTIPLOT_PLOT_CANVAS_POLICY_H
#define RQT_MULTIPLOT_PLOT_CANVAS_POLICY_H

#include <QString>
#include <QSurfaceFormat>

class QWidget;
class QwtPlot;

namespace rqt_multiplot {

inline QString openGLCanvasFallbackWarning() {
  return QStringLiteral(
      "rqt_multiplot: OpenGL plot canvas was requested, but no OpenGL context could be created. Using the software canvas.");
}

inline bool shouldWarnOpenGLCanvasFallback(bool requested, bool available) {
  return requested && !available;
}

constexpr int kOpenGLCanvasSamples = 4;

inline QSurfaceFormat openGLCanvasSurfaceFormat(const QSurfaceFormat& current) {
  QSurfaceFormat format = current;
  if (format.samples() < kOpenGLCanvasSamples) {
    format.setSamples(kOpenGLCanvasSamples);
  }
  return format;
}

bool openGLPlotCanvasAvailable();
void warnIfOpenGLCanvasFallback(bool requested, bool available);
QWidget* createPlotCanvas(QwtPlot* plot, bool wantOpenGL);
bool isOpenGLPlotCanvas(const QWidget* canvas);
void configurePlotCanvas(QWidget* canvas);
void replotPlotCanvas(QWidget* canvas);

}  // namespace rqt_multiplot

#endif
