/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/OffsetScaleDraw.h"

#include <cmath>

#include <rqt_multiplot/AxisTimeFormat.h>

namespace rqt_multiplot {

OffsetScaleDraw::OffsetScaleDraw() : offset_(0.0) {}

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

QwtText OffsetScaleDraw::label(double value) const {
  return QwtText(AxisTimeFormat::relative(value, offset_, std::fabs(scaleDiv().range())));
}

}  // namespace rqt_multiplot
