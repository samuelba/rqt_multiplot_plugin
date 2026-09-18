/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#ifndef RQT_MULTIPLOT_AXIS_TIME_FORMAT_H
#define RQT_MULTIPLOT_AXIS_TIME_FORMAT_H

#include <QString>

class QTimeZone;

namespace rqt_multiplot {

class AxisTimeFormat {
 public:
  enum class LabelMode { Off, Timestamp, Relative, DateTime };

  static int decimalPlaces(double span);
  static QString fixed(double value, double span);
  static QString relative(double value, double t0, double span);
  static QString dateTime(double epochSeconds, const QTimeZone& zone);
  static QString coordinate(double value, double offset, double span, bool timeScale, const QTimeZone& zone);
  static QString coordinate(double value, double offset, double span, LabelMode mode, const QTimeZone& zone);
};

}  // namespace rqt_multiplot

#endif
