/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <qwt/qwt_scale_div.h>
#include <qwt/qwt_scale_engine.h>

namespace rqt_multiplot {

class OffsetScaleEngine : public QwtLinearScaleEngine {
 public:
  OffsetScaleEngine();
  ~OffsetScaleEngine() override;

  void setOffset(double offset);
  double offset() const;

  void autoScale(int maxNumSteps, double& x1, double& x2, double& stepSize) const override;
  QwtScaleDiv divideScale(double x1, double x2, int maxMajorSteps, int maxMinorSteps, double stepSize) const override;

 private:
  double offset_;
};

}  // namespace rqt_multiplot
