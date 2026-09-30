/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include <utility>

#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QWidget>

#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_marker.h>
#include <qwt/qwt_scale_map.h>
#include <qwt/qwt_text.h>

#include "rqt_multiplot/PlotMarkerPair.hpp"

namespace rqt_multiplot {

namespace {

class PlotMarkerLine : public QwtPlotMarker {
 public:
  using QwtPlotMarker::QwtPlotMarker;

 protected:
  void drawLabel(QPainter* painter, const QRectF& canvasRect, const QPointF& pos) const override {
    const QwtText text = label();
    if (text.isEmpty()) {
      return;
    }
    painter->save();
    painter->setFont(text.font());
    const QSize size = markerTagSize(painter->fontMetrics(), text.text());
    for (const Qt::Edge edge : {Qt::TopEdge, Qt::BottomEdge}) {
      const QRect rect = markerTagRect(qRound(pos.x()), size, canvasRect.toAlignedRect(), edge);
      painter->setPen(Qt::NoPen);
      painter->setBrush(text.backgroundBrush());
      painter->drawRect(rect);
      painter->setPen(text.color());
      painter->drawText(rect, Qt::AlignCenter, text.text());
    }
    painter->restore();
  }
};

QwtPlotMarker* createMarker(const QString& label, QwtPlot* plot) {
  auto* marker = new PlotMarkerLine(label);
  marker->setLabel(QwtText(label));
  marker->setLineStyle(QwtPlotMarker::VLine);
  marker->setZ(30.0);
  marker->setVisible(false);
  marker->attach(plot);
  return marker;
}

}  // namespace

PlotMarkerPair::PlotMarkerPair(QwtPlot* plot, QObject* parent)
    : QObject(parent),
      plot_(plot),
      markerA_(createMarker(QStringLiteral("A"), plot)),
      markerB_(createMarker(QStringLiteral("B"), plot)),
      readout_(new PlotMarkerReadout()),
      hoverCursorSet_(false) {
  readout_->attach(plot);
  setColors(Qt::black, Qt::white);
}

PlotMarkerPair::~PlotMarkerPair() {
  delete readout_;
  delete markerB_;
  delete markerA_;
}

void PlotMarkerPair::setCanvas(QWidget* canvas) {
  if (canvas_ != nullptr) {
    canvas_->removeEventFilter(this);
  }
  canvas_ = canvas;
  dragged_.reset();
  hoverCursorSet_ = false;
  if (canvas_ != nullptr) {
    canvas_->setMouseTracking(true);
    canvas_->installEventFilter(this);
  }
}

const MarkerPositions& PlotMarkerPair::positions() const {
  return positions_;
}

bool PlotMarkerPair::hasAnyMarker() const {
  return positions_.a.has_value() || positions_.b.has_value();
}

void PlotMarkerPair::setPositions(const MarkerPositions& positions) {
  if (positions != positions_) {
    positions_ = positions;
    applyPositions();
  }
}

void PlotMarkerPair::setMarker(MarkerId marker, double x) {
  MarkerPositions positions = positions_;
  (marker == MarkerId::A ? positions.a : positions.b) = x;
  setPositions(positions);
}

void PlotMarkerPair::clearMarkers() {
  dragged_.reset();
  setPositions(MarkerPositions());
}

void PlotMarkerPair::placeMarker(MarkerId marker, double x) {
  setMarker(marker, x);
  userChanged();
}

void PlotMarkerPair::removeMarkers() {
  clearMarkers();
  userChanged();
}

void PlotMarkerPair::setColors(const QColor& foreground, const QColor& background) {
  QColor lineColor = foreground;
  lineColor.setAlpha(200);
  for (QwtPlotMarker* marker : {markerA_, markerB_}) {
    marker->setLinePen(QPen(lineColor, 0.0, Qt::DashLine));
    QwtText label = marker->label();
    label.setColor(background);
    label.setBackgroundBrush(foreground);
    marker->setLabel(label);
  }

  QColor readoutBackground = background;
  readoutBackground.setAlpha(230);
  readout_->setColors(foreground, readoutBackground);
}

void PlotMarkerPair::setCoordinateFormatter(CoordinateFormatter formatter) {
  readout_->setCoordinateFormatter(std::move(formatter));
}

void PlotMarkerPair::setXIsTime(bool xIsTime) {
  readout_->setXIsTime(xIsTime);
}

QwtPlotMarker* PlotMarkerPair::marker(MarkerId marker) const {
  return (marker == MarkerId::A) ? markerA_ : markerB_;
}

PlotMarkerReadout* PlotMarkerPair::readout() const {
  return readout_;
}

bool PlotMarkerPair::eventFilter(QObject* object, QEvent* event) {
  if ((object != canvas_) || (canvas_ == nullptr)) {
    return false;
  }

  if ((event->type() == QEvent::Leave) && !dragged_) {
    setHoverCursor(false);
    return false;
  }
  const auto* mouseEvent = dynamic_cast<QMouseEvent*>(event);
  return (mouseEvent != nullptr) && handleMouseEvent(*mouseEvent);
}

bool PlotMarkerPair::handleMouseEvent(const QMouseEvent& event) {
  const int px = mouseEventPosition(event).x();
  switch (event.type()) {
    case QEvent::MouseButtonPress:
      if (isMarkerPlaceMouse(event.button(), event.modifiers())) {
        const double x = canvasToX(px);
        dragged_ = markerToPlace(positions_, x);
        placeMarker(*dragged_, x);
        return true;
      }
      if (isMarkerDragMouse(event.button(), event.modifiers())) {
        dragged_ = hitTest(px);
        return dragged_.has_value();
      }
      return false;
    case QEvent::MouseMove:
      if (dragged_) {
        placeMarker(*dragged_, canvasToX(px));
      } else if (event.buttons() == Qt::NoButton) {
        updateHoverCursor(px);
      }
      return false;
    case QEvent::MouseButtonRelease:
      if (dragged_ && (event.button() == Qt::LeftButton)) {
        dragged_.reset();
        return true;
      }
      return false;
    default:
      return false;
  }
}

void PlotMarkerPair::applyPositions() {
  markerA_->setVisible(positions_.a.has_value());
  markerB_->setVisible(positions_.b.has_value());
  if (positions_.a) {
    markerA_->setXValue(*positions_.a);
  }
  if (positions_.b) {
    markerB_->setXValue(*positions_.b);
  }
  readout_->setPositions(positions_);
  plot_->replot();
}

void PlotMarkerPair::userChanged() {
  emit markersChanged(positions_);
}

std::optional<MarkerId> PlotMarkerPair::hitTest(int px) const {
  const QwtScaleMap map = plot_->canvasMap(QwtPlot::xBottom);
  const auto toPx = [&map](const std::optional<double>& x) -> std::optional<double> {
    return x ? std::optional<double>(map.transform(*x)) : std::nullopt;
  };
  return markerHit(toPx(positions_.a), toPx(positions_.b), px);
}

double PlotMarkerPair::canvasToX(int px) const {
  return plot_->canvasMap(QwtPlot::xBottom).invTransform(px);
}

void PlotMarkerPair::updateHoverCursor(int px) {
  setHoverCursor(hitTest(px).has_value());
}

void PlotMarkerPair::setHoverCursor(bool overMarker) {
  if (overMarker && !hoverCursorSet_) {
    canvasCursor_ = canvas_->cursor();
    canvas_->setCursor(Qt::SizeHorCursor);
    hoverCursorSet_ = true;
  } else if (!overMarker && hoverCursorSet_) {
    canvas_->setCursor(canvasCursor_);
    hoverCursorSet_ = false;
  }
}

}  // namespace rqt_multiplot
