/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/OffsetScaleDraw.h"

#include <cmath>

#include <rqt_multiplot/AxisTimeFormat.h>

namespace rqt_multiplot {

OffsetScaleDraw::OffsetScaleDraw() : offset_(0.0), useTimeScale_(false) {}

OffsetScaleDraw::~OffsetScaleDraw() = default;

void OffsetScaleDraw::setOffset(double offset) {
  if (offset != offset_) {
    offset_ = offset;
    invalidateCache();
  }
}

double OffsetScaleDraw::offset() const {
  return offset_;
}

void OffsetScaleDraw::setUseTimeScale(bool useTimeScale) {
  if (useTimeScale != useTimeScale_) {
    useTimeScale_ = useTimeScale;
    invalidateCache();
  }
}

bool OffsetScaleDraw::useTimeScale() const {
  return useTimeScale_;
}

QwtText OffsetScaleDraw::label(double value) const {
  if (!useTimeScale_) {
    return QwtScaleDraw::label(value);
  }
  return QwtText(AxisTimeFormat::relative(value, offset_, std::fabs(scaleDiv().range())));
}

}  // namespace rqt_multiplot
