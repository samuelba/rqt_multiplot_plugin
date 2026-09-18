/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#ifndef RQT_MULTIPLOT_TIME_ZONE_UTIL_H
#define RQT_MULTIPLOT_TIME_ZONE_UTIL_H

#include <QString>
#include <QTimeZone>

namespace rqt_multiplot {

class TimeZoneUtil {
 public:
  static QTimeZone localTimeZone();
  static QString localTimeZoneId();
  static QString timeZoneIdFromLocaltimePath(const QString& localtimePath);
};

}  // namespace rqt_multiplot

#endif  // RQT_MULTIPLOT_TIME_ZONE_UTIL_H
