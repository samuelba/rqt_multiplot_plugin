/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner, Samuel Bachmann                       *
 * ralf.kaestner@gmail.com                                                    *
 *                                                                            *
 * This program is free software; you can redistribute it and/or modify       *
 * it under the terms of the Lesser GNU General Public License as published by*
 * the Free Software Foundation; either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 ******************************************************************************/

#include "rqt_multiplot/QwtPlotCustom.hpp"

#include <QApplication>
#include <QEvent>

#include <qwt/qwt_abstract_legend.h>
#include <qwt/qwt_scale_draw.h>
#include <qwt/qwt_text.h>

#include "rqt_multiplot/PlotCanvasPolicy.hpp"

namespace rqt_multiplot {
namespace {

int axisExtent(const QwtPlot& plot, int axisId) {
  if (!plot.axisEnabled(axisId)) {
    return 0;
  }
  const QwtScaleDraw* draw = plot.axisScaleDraw(axisId);
  if (draw == nullptr) {
    return 0;
  }
  return qRound(draw->extent(plot.axisFont(axisId)));
}

}  // namespace

QwtPlotCustom::QwtPlotCustom(QWidget* parent) : QwtPlot(parent), layoutSignatureValid_(false), layoutUpdateCount_(0) {}

QwtPlotCustom::QwtPlotCustom(const QwtText& title, QWidget* p) : QwtPlot(title, p), layoutSignatureValid_(false), layoutUpdateCount_(0) {}

QSize QwtPlotCustom::sizeHint() const {
  return QSize(50, 50);
}

QSize QwtPlotCustom::minimumSizeHint() const {
  return QSize(50, 50);
}

PlotLayoutSignature QwtPlotCustom::currentLayoutSignature() const {
  PlotLayoutSignature signature;
  signature.plotSize = size();
  signature.xTitle = axisTitle(QwtPlot::xBottom).text();
  signature.yTitle = axisTitle(QwtPlot::yLeft).text();
  signature.xTopTitle = axisTitle(QwtPlot::xTop).text();
  signature.yRightTitle = axisTitle(QwtPlot::yRight).text();
  signature.xExtent = axisExtent(*this, QwtPlot::xBottom);
  signature.yExtent = axisExtent(*this, QwtPlot::yLeft);
  signature.xTopExtent = axisExtent(*this, QwtPlot::xTop);
  signature.yRightExtent = axisExtent(*this, QwtPlot::yRight);
  if (legend() != nullptr) {
    signature.legendSize = legend()->sizeHint();
  }
  return signature;
}

void QwtPlotCustom::replot() {
  const bool doAutoReplot = autoReplot();
  setAutoReplot(false);

  updateAxes();
  QApplication::sendPostedEvents(this, QEvent::LayoutRequest);

  updateLayout();
  replotPlotCanvas(canvas());

  setAutoReplot(doAutoReplot);
}

void QwtPlotCustom::updateLayout() {
  const PlotLayoutSignature signature = currentLayoutSignature();
  if (!shouldUpdatePlotLayout(layoutSignatureValid_, lastLayoutSignature_, signature)) {
    return;
  }
  lastLayoutSignature_ = signature;
  layoutSignatureValid_ = true;
  ++layoutUpdateCount_;
  QwtPlot::updateLayout();
}

void QwtPlotCustom::invalidateLayoutCache() {
  layoutSignatureValid_ = false;
}

int QwtPlotCustom::layoutUpdateCount() const {
  return layoutUpdateCount_;
}

}  // namespace rqt_multiplot
