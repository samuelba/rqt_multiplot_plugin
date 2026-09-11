/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/AxisTimeFormat.h"

#include <algorithm>
#include <cmath>

namespace rqt_multiplot {

int AxisTimeFormat::decimalPlaces(double span) {
  const double absSpan = std::fabs(span);
  if (!std::isfinite(absSpan) || absSpan <= 0.0) {
    return 3;
  }

  const int places = static_cast<int>(std::ceil(-std::log10(absSpan))) + 1;
  return std::clamp(places, 0, 9);
}

QString AxisTimeFormat::fixed(double value, double span) {
  if (!std::isfinite(value)) {
    return QStringLiteral("nan");
  }

  QString text = QString::number(value, 'f', decimalPlaces(span));
  if (text.contains(QLatin1Char('.'))) {
    while (text.endsWith(QLatin1Char('0'))) {
      text.chop(1);
    }
    if (text.endsWith(QLatin1Char('.'))) {
      text.chop(1);
    }
  }
  return text;
}

QString AxisTimeFormat::relative(double value, double t0, double span) {
  return fixed(value - t0, span);
}

QString AxisTimeFormat::coordinate(double value, double offset, double span, bool timeScale) {
  if (timeScale) {
    return relative(value, offset, span);
  }

  const double precision = std::log10(std::fabs(span));
  if ((precision < 0.0) && (std::fabs(value) >= 1.0)) {
    return QString::asprintf("%.*f", static_cast<int>(std::ceil(std::fabs(precision))), value);
  }
  return QString::asprintf("%g", value);
}

}  // namespace rqt_multiplot
