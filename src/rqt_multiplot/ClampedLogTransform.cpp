/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/ClampedLogTransform.hpp"

namespace rqt_multiplot {

namespace {

constexpr double kBelowLogMin = 1.0;

}  // namespace

ClampedLogTransform::ClampedLogTransform() = default;

ClampedLogTransform::~ClampedLogTransform() = default;

double ClampedLogTransform::transform(double value) const {
  if (!(value > 0.0)) {
    return QwtLogTransform::transform(QwtLogTransform::LogMin) - kBelowLogMin;
  }
  return QwtLogTransform::transform(value);
}

QwtTransform* ClampedLogTransform::copy() const {
  return new ClampedLogTransform();
}

}  // namespace rqt_multiplot
