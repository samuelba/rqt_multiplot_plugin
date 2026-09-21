/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

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
