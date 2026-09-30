/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <functional>
#include <optional>

#include <QColor>
#include <QFontMetrics>
#include <QRect>
#include <QSize>
#include <QString>
#include <QVector>

#include <qwt/qwt_plot_item.h>

#include "rqt_multiplot/PlotMouseBindings.hpp"
#include "rqt_multiplot/PlotReadoutTable.hpp"

namespace rqt_multiplot {

using CoordinateFormatter = std::function<QString(double value, bool isX)>;

struct MarkerCurveSample {
  QColor color;
  QString title;
  std::optional<double> yA;
  std::optional<double> yB;
};

struct MarkerReadoutFormat {
  CoordinateFormatter coordinate;
  double xSpan = 0.0;
  double ySpan = 0.0;
  bool xIsTime = false;
};

QVector<ReadoutRow> markerReadoutRows(double a, double b, const QVector<MarkerCurveSample>& curves, const MarkerReadoutFormat& format);
QRect markerReadoutRect(int leftPx, int rightPx, const QSize& size, const QRect& canvas, int margin = 5);

constexpr int kMarkerTagPadding = 3;

QSize markerTagSize(const QFontMetrics& metrics, const QString& text);
QRect markerTagRect(int linePx, const QSize& size, const QRect& canvas, Qt::Edge edge);

class PlotMarkerReadout : public QwtPlotItem {
 public:
  static constexpr int kRtti = QwtPlotItem::Rtti_PlotUserItem + 1;

  PlotMarkerReadout();
  ~PlotMarkerReadout() override;

  int rtti() const override;

  void setPositions(const MarkerPositions& positions);
  void setCoordinateFormatter(CoordinateFormatter formatter);
  void setXIsTime(bool xIsTime);
  void setColors(const QColor& text, const QColor& background);

  QVector<MarkerCurveSample> curveSamples() const;
  QVector<ReadoutRow> rows(const QwtScaleMap& xMap, const QwtScaleMap& yMap) const;

  void draw(QPainter* painter, const QwtScaleMap& xMap, const QwtScaleMap& yMap, const QRectF& canvasRect) const override;

 private:
  MarkerPositions positions_;
  CoordinateFormatter formatter_;
  bool xIsTime_;
  QColor textColor_;
  QColor backgroundColor_;
};

}  // namespace rqt_multiplot
