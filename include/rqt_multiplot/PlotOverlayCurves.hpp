/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <optional>

#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QVector>

#include "rqt_multiplot/BagOverlaySession.hpp"

class QwtPlot;

namespace rqt_multiplot {

class CurveConfig;
class CurveData;
class PlotCurve;

// Points are scanned in arrival order; the crossing X is linearly interpolated.
std::optional<double> firstCrossingX(const CurveData& data, double value);

struct ThresholdAlignment {
  bool referenceCrosses = false;
  QStringList alignedLabels;
  QStringList missedLabels;
};

// Runtime-only curve copies, one per overlay bag and source curve; they share the source's CurveConfig.
class PlotOverlayCurves : public QObject {
  Q_OBJECT
 public:
  PlotOverlayCurves(QObject* curveParent, QwtPlot* plot);
  ~PlotOverlayCurves() override;

  void setSession(BagOverlaySession* session, const QVector<PlotCurve*>& sources, bool running);
  BagOverlaySession* getSession() const;
  const QVector<PlotCurve*>& getCurves() const;
  QVector<PlotCurve*> copiesOf(const PlotCurve* source) const;
  bool isEmpty() const;

  void removeCopiesOf(const CurveConfig* config);
  void removeAll();

  ThresholdAlignment alignAtThreshold(const PlotCurve* reference, double value);

  void run();
  void pause();
  void clear();
  void setPlotTimeWindowLength(std::optional<int> length);

 signals:
  void replotRequested();
  void curvesChanged();

 private:
  QObject* curveParent_;
  QwtPlot* plot_;
  QPointer<BagOverlaySession> session_;
  QVector<PlotCurve*> curves_;
  std::optional<int> plotTimeWindowLength_;

  void createCopies(const QVector<PlotCurve*>& sources, bool running);
  static void destroyCurve(PlotCurve* curve);

 private slots:
  void sessionOffsetChanged(int bag, double delta);
  void sessionCleared();
};

}  // namespace rqt_multiplot
