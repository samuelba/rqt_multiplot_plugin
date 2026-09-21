/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/OffsetScaleEngine.hpp"

#include <QList>

namespace rqt_multiplot {
namespace {

QList<double> shiftTicks(const QList<double>& ticks, double offset) {
  QList<double> shifted;
  shifted.reserve(ticks.size());
  for (double tick : ticks) {
    shifted.append(tick + offset);
  }
  return shifted;
}

}  // namespace

OffsetScaleEngine::OffsetScaleEngine() : offset_(0.0) {}

OffsetScaleEngine::~OffsetScaleEngine() = default;

void OffsetScaleEngine::setOffset(double offset) {
  offset_ = offset;
}

double OffsetScaleEngine::offset() const {
  return offset_;
}

void OffsetScaleEngine::autoScale(int maxNumSteps, double& x1, double& x2, double& stepSize) const {
  x1 -= offset_;
  x2 -= offset_;
  QwtLinearScaleEngine::autoScale(maxNumSteps, x1, x2, stepSize);
  x1 += offset_;
  x2 += offset_;
}

QwtScaleDiv OffsetScaleEngine::divideScale(double x1, double x2, int maxMajorSteps, int maxMinorSteps, double stepSize) const {
  const QwtScaleDiv relativeDiv = QwtLinearScaleEngine::divideScale(x1 - offset_, x2 - offset_, maxMajorSteps, maxMinorSteps, stepSize);

  QwtScaleDiv shifted(relativeDiv.lowerBound() + offset_, relativeDiv.upperBound() + offset_);
  shifted.setTicks(QwtScaleDiv::MinorTick, shiftTicks(relativeDiv.ticks(QwtScaleDiv::MinorTick), offset_));
  shifted.setTicks(QwtScaleDiv::MediumTick, shiftTicks(relativeDiv.ticks(QwtScaleDiv::MediumTick), offset_));
  shifted.setTicks(QwtScaleDiv::MajorTick, shiftTicks(relativeDiv.ticks(QwtScaleDiv::MajorTick), offset_));
  return shifted;
}

}  // namespace rqt_multiplot
