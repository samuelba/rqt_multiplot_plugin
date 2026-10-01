/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <memory>
#include <optional>
#include <vector>

#include <QPointF>
#include <QString>
#include <QVector>

namespace rqt_multiplot {

enum class CurveFilterType {
  Derivative,
  Integral,
  MovingAverage,
  MovingRms,
  MovingVariance,
  LowPass,
  Absolute,
  ScaleOffset,
  Threshold,
  OutlierRemoval,
  TimeSincePrevious,
  SamplesCount,
};

enum class CurveFilterComparison { Equal, Less, LessEqual, Greater, GreaterEqual, Range };

struct CurveFilterSpec {
  CurveFilterType type = CurveFilterType::Derivative;
  bool useFixedStep = false;
  double fixedStep = 0.01;
  int windowSize = 10;
  bool compensateLag = false;
  bool standardDeviation = false;
  double timeConstant = 0.1;
  double scale = 1.0;
  double offset = 0.0;
  CurveFilterComparison comparison = CurveFilterComparison::Greater;
  double thresholdA = 0.0;
  double thresholdB = 1.0;
  double outlierFactor = 100.0;
  int windowMilliseconds = 1000;

  bool operator==(const CurveFilterSpec& other) const;
  bool operator!=(const CurveFilterSpec& other) const { return !(*this == other); }
};

QVector<CurveFilterType> allCurveFilterTypes();
QString curveFilterTypeName(CurveFilterType type);
QString curveFilterTypeDescription(CurveFilterType type);
QString curveFilterTypeKey(CurveFilterType type);
std::optional<CurveFilterType> curveFilterTypeFromKey(const QString& key);
bool curveFilterHasParameters(CurveFilterType type);
QString curveFilterSummary(const CurveFilterSpec& spec);
CurveFilterSpec defaultCurveFilterSpec(CurveFilterType type);

class CurveFilter {
 public:
  virtual ~CurveFilter() = default;

  virtual std::optional<QPointF> process(const QPointF& point) = 0;
  virtual void reset() = 0;
};

std::unique_ptr<CurveFilter> createCurveFilter(const CurveFilterSpec& spec);

class CurveFilterChain {
 public:
  CurveFilterChain() = default;
  explicit CurveFilterChain(const QVector<CurveFilterSpec>& specs);

  bool isEmpty() const;
  std::optional<QPointF> process(const QPointF& point);
  void reset();

 private:
  std::vector<std::unique_ptr<CurveFilter>> filters_;
};

}  // namespace rqt_multiplot
