/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <QTimeZone>

#include <qwt/qwt_scale_draw.h>
#include <qwt/qwt_text.h>

#include "rqt_multiplot/AxisTimeFormat.hpp"

namespace rqt_multiplot {

class OffsetScaleDraw : public QwtScaleDraw {
 public:
  OffsetScaleDraw();
  ~OffsetScaleDraw() override;

  void setOffset(double offset);
  double offset() const;
  void setTimeLabelMode(AxisTimeFormat::LabelMode mode);
  AxisTimeFormat::LabelMode timeLabelMode() const;
  void setTimeZone(const QTimeZone& zone);
  const QTimeZone& timeZone() const;
  void setUseTimeScale(bool useTimeScale);
  bool useTimeScale() const;

  QwtText label(double value) const override;

 private:
  double offset_;
  AxisTimeFormat::LabelMode timeLabelMode_;
  QTimeZone timeZone_;
};

}  // namespace rqt_multiplot
