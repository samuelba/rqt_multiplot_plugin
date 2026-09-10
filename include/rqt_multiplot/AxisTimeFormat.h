/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#ifndef RQT_MULTIPLOT_AXIS_TIME_FORMAT_H
#define RQT_MULTIPLOT_AXIS_TIME_FORMAT_H

#include <QString>

namespace rqt_multiplot {

class AxisTimeFormat {
 public:
  static int decimalPlaces(double span);
  static QString fixed(double value, double span);
  static QString relative(double value, double t0, double span);
};

}  // namespace rqt_multiplot

#endif
