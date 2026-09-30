/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include <algorithm>
#include <cmath>
#include <utility>

#include <QPainter>

#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_curve.h>
#include <qwt/qwt_scale_map.h>
#include <qwt/qwt_text.h>

#include "rqt_multiplot/AxisTimeFormat.hpp"
#include "rqt_multiplot/CurveData.hpp"

#include "rqt_multiplot/PlotMarkerReadout.hpp"

namespace rqt_multiplot {

namespace {

QString delta() {
  return QString::fromUtf8("\u0394");
}

QString optionalValue(const std::optional<double>& value, const CoordinateFormatter& formatter) {
  return value ? formatter(*value, false) : QString();
}

double unitsPerPixel(const QwtScaleMap& map) {
  return std::fabs(map.invTransform(1.0) - map.invTransform(0.0));
}

}  // namespace

QVector<ReadoutRow> markerReadoutRows(double a, double b, const QVector<MarkerCurveSample>& curves, const MarkerReadoutFormat& format) {
  const QString xName = format.xIsTime ? QStringLiteral("t") : QStringLiteral("x");
  const double dx = b - a;

  QVector<ReadoutRow> rows;
  rows.append({QColor(),
               QString(),
               {QStringLiteral("A"), QStringLiteral("B"), delta(), delta() + QStringLiteral("/") + delta() + xName},
               ReadoutMark::None});
  rows.append({QColor(),
               xName,
               {format.coordinate(a, true), format.coordinate(b, true), AxisTimeFormat::fixed(dx, format.xSpan)},
               ReadoutMark::None});

  for (const MarkerCurveSample& curve : curves) {
    ReadoutRow row{curve.color, curve.title, {optionalValue(curve.yA, format.coordinate), optionalValue(curve.yB, format.coordinate)}};
    if (curve.yA && curve.yB) {
      const double dy = *curve.yB - *curve.yA;
      row.values.append(AxisTimeFormat::fixed(dy, format.ySpan));
      row.values.append((dx == 0.0) ? QString::fromUtf8("\u2013") : AxisTimeFormat::fixed(dy / dx, format.ySpan / std::fabs(dx)));
    }
    rows.append(row);
  }
  return rows;
}

QRect markerReadoutRect(int leftPx, int rightPx, const QSize& size, const QRect& canvas, int margin) {
  int x = rightPx + margin;
  if (x + size.width() > canvas.right() - margin) {
    x = leftPx - margin - size.width();
  }
  if (x < canvas.left() + margin) {
    x = std::max(canvas.left() + margin, canvas.right() - margin - size.width());
  }
  return {QPoint(x, canvas.top() + margin), size};
}

QSize markerTagSize(const QFontMetrics& metrics, const QString& text) {
  return metrics.size(Qt::TextSingleLine, text) + QSize(2 * kMarkerTagPadding, 2 * kMarkerTagPadding);
}

QRect markerTagRect(int linePx, const QSize& size, const QRect& canvas, Qt::Edge edge) {
  const int maxLeft = std::max(canvas.left(), canvas.right() + 1 - size.width());
  const int left = std::clamp(linePx - size.width() / 2, canvas.left(), maxLeft);
  const int top = (edge == Qt::BottomEdge) ? canvas.bottom() + 1 - size.height() : canvas.top();
  return {QPoint(left, top), size};
}

PlotMarkerReadout::PlotMarkerReadout()
    : formatter_([](double value, bool /*isX*/) { return QString::number(value, 'g', 6); }),
      xIsTime_(false),
      textColor_(Qt::black),
      backgroundColor_(Qt::white) {
  setZ(100.0);
  setItemAttribute(QwtPlotItem::Legend, false);
  setItemAttribute(QwtPlotItem::AutoScale, false);
}

PlotMarkerReadout::~PlotMarkerReadout() = default;

int PlotMarkerReadout::rtti() const {
  return kRtti;
}

void PlotMarkerReadout::setPositions(const MarkerPositions& positions) {
  positions_ = positions;
}

void PlotMarkerReadout::setCoordinateFormatter(CoordinateFormatter formatter) {
  formatter_ = std::move(formatter);
}

void PlotMarkerReadout::setXIsTime(bool xIsTime) {
  xIsTime_ = xIsTime;
}

void PlotMarkerReadout::setColors(const QColor& text, const QColor& background) {
  textColor_ = text;
  backgroundColor_ = background;
}

QVector<MarkerCurveSample> PlotMarkerReadout::curveSamples() const {
  QVector<MarkerCurveSample> samples;
  if ((plot() == nullptr) || !positions_.a || !positions_.b) {
    return samples;
  }

  for (QwtPlotItem* item : plot()->itemList(QwtPlotItem::Rtti_PlotCurve)) {
    auto* curve = dynamic_cast<QwtPlotCurve*>(item);
    auto* data = (curve != nullptr) ? dynamic_cast<CurveData*>(curve->data()) : nullptr;
    if ((data == nullptr) || !curve->isVisible()) {
      continue;
    }
    samples.append({curve->pen().color(), curve->title().text(), data->interpolateY(*positions_.a), data->interpolateY(*positions_.b)});
  }
  return samples;
}

QVector<ReadoutRow> PlotMarkerReadout::rows(const QwtScaleMap& xMap, const QwtScaleMap& yMap) const {
  if (!positions_.a || !positions_.b) {
    return {};
  }
  const MarkerReadoutFormat format{formatter_, unitsPerPixel(xMap), unitsPerPixel(yMap), xIsTime_};
  return markerReadoutRows(*positions_.a, *positions_.b, curveSamples(), format);
}

void PlotMarkerReadout::draw(QPainter* painter, const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QRectF& canvasRect) const {
  const QVector<ReadoutRow> tableRows = rows(xMap, yMap);
  if (tableRows.isEmpty()) {
    return;
  }

  const QSize size = readoutSize(readoutLayout(tableRows, painter->font()), static_cast<int>(tableRows.size()));
  const QSize padded = size + QSize(2 * kReadoutPadding, 2 * kReadoutPadding);
  const int aPx = qRound(xMap.transform(*positions_.a));
  const int bPx = qRound(xMap.transform(*positions_.b));
  const int tagHeight = markerTagSize(painter->fontMetrics(), QStringLiteral("A")).height();
  const QRect belowTags = canvasRect.toAlignedRect().adjusted(0, tagHeight, 0, 0);
  const QRect background = markerReadoutRect(std::min(aPx, bPx), std::max(aPx, bPx), padded, belowTags);
  drawReadoutTable(*painter, background, tableRows, textColor_, backgroundColor_);
}

}  // namespace rqt_multiplot
