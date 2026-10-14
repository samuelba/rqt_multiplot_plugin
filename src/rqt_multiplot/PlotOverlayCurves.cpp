/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/BagReader.hpp"
#include "rqt_multiplot/CurveData.hpp"
#include "rqt_multiplot/CurveDataSequencer.hpp"
#include "rqt_multiplot/PlotCurve.hpp"

#include "rqt_multiplot/PlotOverlayCurves.hpp"

namespace rqt_multiplot {

std::optional<double> firstCrossingX(const CurveData& data, double value) {
  const size_t count = data.getNumPoints();
  if (count == 0) {
    return std::nullopt;
  }
  QPointF previous = data.getPoint(0);
  if (previous.y() == value) {
    return previous.x();
  }
  for (size_t index = 1; index < count; ++index) {
    const QPointF current = data.getPoint(index);
    if (current.y() == value) {
      return current.x();
    }
    if ((previous.y() < value) != (current.y() < value)) {
      const double fraction = (value - previous.y()) / (current.y() - previous.y());
      return previous.x() + (fraction * (current.x() - previous.x()));
    }
    previous = current;
  }
  return std::nullopt;
}

ThresholdAlignment PlotOverlayCurves::alignAtThreshold(const PlotCurve* reference, double value) {
  ThresholdAlignment result;
  const std::optional<double> referenceX = (reference != nullptr) ? firstCrossingX(*reference->getData(), value) : std::nullopt;
  result.referenceCrosses = referenceX.has_value();
  if (!referenceX || (session_ == nullptr)) {
    return result;
  }

  for (const PlotCurve* copy : copiesOf(reference)) {
    const int bag = copy->getOverlayBag();
    const std::optional<double> copyX = firstCrossingX(*copy->getData(), value);
    if (copyX) {
      session_->alignAtMarkers(bag, *referenceX, *copyX);
      result.alignedLabels.append(session_->getLabel(bag));
    } else {
      result.missedLabels.append(session_->getLabel(bag));
    }
  }
  return result;
}

PlotOverlayCurves::PlotOverlayCurves(QObject* curveParent, QwtPlot* plot) : QObject(curveParent), curveParent_(curveParent), plot_(plot) {}

PlotOverlayCurves::~PlotOverlayCurves() {
  for (PlotCurve* curve : curves_) {
    destroyCurve(curve);
  }
}

void PlotOverlayCurves::setSession(BagOverlaySession* session, const QVector<PlotCurve*>& sources, bool running) {
  if (session_ != nullptr) {
    disconnect(session_, nullptr, this, nullptr);
  }
  removeAll();

  session_ = session;
  if ((session == nullptr) || !session->isActive()) {
    return;
  }

  connect(session, &BagOverlaySession::offsetChanged, this, &PlotOverlayCurves::sessionOffsetChanged);
  connect(session, &BagOverlaySession::cleared, this, &PlotOverlayCurves::sessionCleared);
  createCopies(sources, running);
}

void PlotOverlayCurves::createCopies(const QVector<PlotCurve*>& sources, bool running) {
  for (int bag = 1; bag < session_->getNumBags(); ++bag) {
    for (const PlotCurve* source : sources) {
      CurveConfig* config = source->getConfig();
      if ((config == nullptr) || CurveDataSequencer::hasSnapshotHint(*config)) {
        continue;
      }

      auto* copy = new PlotCurve(curveParent_);
      copy->attach(plot_);
      copy->setOverlaySource(bag, session_->getLabel(bag));
      copy->setXOffset(session_->getOffset(bag));
      copy->setConfig(config);
      copy->setPlotTimeWindowLength(plotTimeWindowLength_);
      copy->setBroker(session_->getReader(bag));
      connect(copy, &PlotCurve::replotRequested, this, &PlotOverlayCurves::replotRequested);
      if (running) {
        copy->run();
      }
      curves_.append(copy);
    }
  }
  if (!curves_.isEmpty()) {
    emit curvesChanged();
  }
}

BagOverlaySession* PlotOverlayCurves::getSession() const {
  return session_;
}

const QVector<PlotCurve*>& PlotOverlayCurves::getCurves() const {
  return curves_;
}

QVector<PlotCurve*> PlotOverlayCurves::copiesOf(const PlotCurve* source) const {
  QVector<PlotCurve*> copies;
  if ((source == nullptr) || (source->getConfig() == nullptr)) {
    return copies;
  }
  for (PlotCurve* curve : curves_) {
    if (curve->getConfig() == source->getConfig()) {
      copies.append(curve);
    }
  }
  return copies;
}

bool PlotOverlayCurves::isEmpty() const {
  return curves_.isEmpty();
}

void PlotOverlayCurves::removeCopiesOf(const CurveConfig* config) {
  QVector<PlotCurve*> kept;
  for (PlotCurve* curve : curves_) {
    if (curve->getConfig() == config) {
      destroyCurve(curve);
    } else {
      kept.append(curve);
    }
  }
  if (kept.size() != curves_.size()) {
    curves_ = kept;
    emit curvesChanged();
  }
}

void PlotOverlayCurves::removeAll() {
  if (curves_.isEmpty()) {
    return;
  }
  for (PlotCurve* curve : curves_) {
    destroyCurve(curve);
  }
  curves_.clear();
  emit curvesChanged();
}

void PlotOverlayCurves::run() {
  for (PlotCurve* curve : curves_) {
    curve->run();
  }
}

void PlotOverlayCurves::pause() {
  for (PlotCurve* curve : curves_) {
    curve->pause();
  }
}

void PlotOverlayCurves::clear() {
  for (PlotCurve* curve : curves_) {
    curve->clear();
  }
}

void PlotOverlayCurves::setPlotTimeWindowLength(std::optional<int> length) {
  plotTimeWindowLength_ = length;
  for (PlotCurve* curve : curves_) {
    curve->setPlotTimeWindowLength(length);
  }
}

void PlotOverlayCurves::destroyCurve(PlotCurve* curve) {
  curve->detach();
  delete curve;
}

void PlotOverlayCurves::sessionOffsetChanged(int bag, double /*delta*/) {
  for (PlotCurve* curve : curves_) {
    if (curve->getOverlayBag() == bag) {
      curve->setXOffset(session_->getOffset(bag));
    }
  }
}

void PlotOverlayCurves::sessionCleared() {
  disconnect(session_, nullptr, this, nullptr);
  session_ = nullptr;
  removeAll();
}

}  // namespace rqt_multiplot
