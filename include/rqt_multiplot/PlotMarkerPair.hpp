/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <optional>

#include <QColor>
#include <QCursor>
#include <QObject>
#include <QPointer>

#include "rqt_multiplot/PlotMarkerReadout.hpp"
#include "rqt_multiplot/PlotMouseBindings.hpp"

class QwtPlot;
class QMouseEvent;
class QwtPlotMarker;
class QWidget;

namespace rqt_multiplot {

class PlotMarkerPair : public QObject {
  Q_OBJECT
 public:
  explicit PlotMarkerPair(QwtPlot* plot, QObject* parent = nullptr);
  ~PlotMarkerPair() override;

  void setCanvas(QWidget* canvas);

  const MarkerPositions& positions() const;
  bool hasAnyMarker() const;
  void setPositions(const MarkerPositions& positions);
  void setMarker(MarkerId marker, double x);
  void clearMarkers();

  void placeMarker(MarkerId marker, double x);
  void removeMarkers();

  void setColors(const QColor& foreground, const QColor& background);
  void setCoordinateFormatter(CoordinateFormatter formatter);
  void setXIsTime(bool xIsTime);

  QwtPlotMarker* marker(MarkerId marker) const;
  PlotMarkerReadout* readout() const;

 signals:
  void markersChanged(const rqt_multiplot::MarkerPositions& positions);

 protected:
  bool eventFilter(QObject* object, QEvent* event) override;

 private:
  QwtPlot* plot_;
  QPointer<QWidget> canvas_;
  QwtPlotMarker* markerA_;
  QwtPlotMarker* markerB_;
  PlotMarkerReadout* readout_;
  MarkerPositions positions_;
  std::optional<MarkerId> dragged_;
  bool hoverCursorSet_;
  QCursor canvasCursor_;

  bool handleMouseEvent(const QMouseEvent& event);
  void applyPositions();
  void userChanged();
  std::optional<MarkerId> hitTest(int px) const;
  double canvasToX(int px) const;
  void updateHoverCursor(int px);
  void setHoverCursor(bool overMarker);
};

}  // namespace rqt_multiplot
