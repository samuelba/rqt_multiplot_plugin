/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#ifndef RQT_MULTIPLOT_OFFSET_SCALE_DRAW_H
#define RQT_MULTIPLOT_OFFSET_SCALE_DRAW_H

#include <qwt/qwt_scale_draw.h>
#include <qwt/qwt_text.h>

#include <rqt_multiplot/AxisTimeFormat.h>

namespace rqt_multiplot {

class OffsetScaleDraw : public QwtScaleDraw {
 public:
  OffsetScaleDraw();
  ~OffsetScaleDraw() override;

  void setOffset(double offset);
  double offset() const;
  void setTimeLabelMode(AxisTimeFormat::LabelMode mode);
  AxisTimeFormat::LabelMode timeLabelMode() const;
  void setUseTimeScale(bool useTimeScale);
  bool useTimeScale() const;

  QwtText label(double value) const override;

 private:
  double offset_;
  AxisTimeFormat::LabelMode timeLabelMode_;
};

}  // namespace rqt_multiplot

#endif
