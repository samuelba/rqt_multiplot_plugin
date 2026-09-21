/******************************************************************************
 * Copyright (C) 2026 by Samuel Bachmann                                      *
 *                                                                            *
 * This program is free software; you can redistribute it and/or modify       *
 * it under the terms of the Lesser GNU General Public License as published by*
 * the Free Software Foundation; either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 ******************************************************************************/

#include "rqt_multiplot/PlotCanvasPolicy.hpp"

#include <QCoreApplication>
#include <QFrame>
#include <QMetaObject>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QWidget>
#include <QtGlobal>

#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_canvas.h>

#include "rqt_multiplot/PlotOpenGLCanvas.hpp"

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0) && !defined(QWT_NO_OPENGL)
#if __has_include(<qwt/qwt_plot_opengl_canvas.h>)
#include <qwt/qwt_plot_opengl_canvas.h>
#define RQT_MULTIPLOT_QWT_OPENGL_CANVAS QwtPlotOpenGLCanvas
#endif
#endif

namespace rqt_multiplot {

bool openGLPlotCanvasAvailable() {
  if (QCoreApplication::instance() == nullptr) {
    return false;
  }

  static const bool available = []() {
    QOffscreenSurface surface;
    surface.create();
    if (!surface.isValid()) {
      return false;
    }
    QOpenGLContext context;
    return context.create();
  }();
  return available;
}

void warnIfOpenGLCanvasFallback(bool requested, bool available) {
  if (!shouldWarnOpenGLCanvasFallback(requested, available)) {
    return;
  }
  qWarning("%s", qUtf8Printable(openGLCanvasFallbackWarning()));
}

QWidget* createPlotCanvas(QwtPlot* plot, bool wantOpenGL) {
  if (wantOpenGL) {
#ifdef RQT_MULTIPLOT_QWT_OPENGL_CANVAS
    return new RQT_MULTIPLOT_QWT_OPENGL_CANVAS(kOpenGLCanvasSamples, plot);
#else
    return new PlotOpenGLCanvas(plot);
#endif
  }
  return new QwtPlotCanvas(plot);
}

bool isOpenGLPlotCanvas(const QWidget* canvas) {
#ifdef RQT_MULTIPLOT_QWT_OPENGL_CANVAS
  if (qobject_cast<const RQT_MULTIPLOT_QWT_OPENGL_CANVAS*>(canvas) != nullptr) {
    return true;
  }
#endif
  return qobject_cast<const PlotOpenGLCanvas*>(canvas) != nullptr;
}

void configurePlotCanvas(QWidget* canvas) {
  if (canvas == nullptr) {
    return;
  }

  canvas->setContextMenuPolicy(Qt::NoContextMenu);
  if (auto* frame = qobject_cast<QFrame*>(canvas)) {
    frame->setFrameStyle(QFrame::NoFrame);
    return;
  }

  const QMetaObject* meta = canvas->metaObject();
  if ((meta != nullptr) && (meta->indexOfMethod("setFrameStyle(int)") >= 0)) {
    QMetaObject::invokeMethod(canvas, "setFrameStyle", Qt::DirectConnection, Q_ARG(int, static_cast<int>(QFrame::NoFrame)));
  }
}

void replotPlotCanvas(QWidget* canvas) {
  if (canvas == nullptr) {
    return;
  }

  if (auto* plotCanvas = qobject_cast<QwtPlotCanvas*>(canvas)) {
    plotCanvas->invalidateBackingStore();
  }

  const bool ok = QMetaObject::invokeMethod(canvas, "replot", Qt::DirectConnection);
  if (!ok) {
    canvas->update();
  }
}

}  // namespace rqt_multiplot
