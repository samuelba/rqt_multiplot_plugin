/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/OffsetScaleDraw.h"

#include <cmath>

namespace rqt_multiplot {

OffsetScaleDraw::OffsetScaleDraw() : offset_(0.0), timeLabelMode_(AxisTimeFormat::LabelMode::Off) {}

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

void OffsetScaleDraw::setTimeLabelMode(AxisTimeFormat::LabelMode mode) {
  if (mode != timeLabelMode_) {
    timeLabelMode_ = mode;
    invalidateCache();
  }
}

AxisTimeFormat::LabelMode OffsetScaleDraw::timeLabelMode() const {
  return timeLabelMode_;
}

void OffsetScaleDraw::setUseTimeScale(bool useTimeScale) {
  setTimeLabelMode(useTimeScale ? AxisTimeFormat::LabelMode::Relative : AxisTimeFormat::LabelMode::Off);
}

bool OffsetScaleDraw::useTimeScale() const {
  return timeLabelMode_ != AxisTimeFormat::LabelMode::Off;
}

QwtText OffsetScaleDraw::label(double value) const {
  const double span = std::fabs(scaleDiv().range());
  switch (timeLabelMode_) {
    case AxisTimeFormat::LabelMode::Timestamp:
      return QwtText(AxisTimeFormat::fixed(value, span));
    case AxisTimeFormat::LabelMode::Relative:
      return QwtText(AxisTimeFormat::relative(value, offset_, span));
    case AxisTimeFormat::LabelMode::DateTime:
      return QwtText(AxisTimeFormat::dateTime(value));
    case AxisTimeFormat::LabelMode::Off:
    default:
      return QwtScaleDraw::label(value);
  }
}

}  // namespace rqt_multiplot
