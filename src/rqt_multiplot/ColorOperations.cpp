/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 *                                                                            *
 * This program is free software; you can redistribute it and/or modify       *
 * it under the terms of the Lesser GNU General Public License as published by*
 * the Free Software Foundation; either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the               *
 * Lesser GNU General Public License for more details.                        *
 *                                                                            *
 * You should have received a copy of the Lesser GNU General Public License   *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.       *
 ******************************************************************************/

#include <cmath>
#include <limits>

#include "rqt_multiplot/BitOperations.hpp"

#include "rqt_multiplot/ColorOperations.hpp"

namespace rqt_multiplot {

float ColorOperations::intToHue(unsigned char val) {
  return static_cast<float>(BitOperations::revertByte(val)) / static_cast<float>(std::numeric_limits<unsigned char>::max());
}

unsigned char ColorOperations::hueToInt(float hue) {
  return BitOperations::revertByte(static_cast<unsigned int>(
      std::round(static_cast<double>(hue) / (2.0 * M_PI) * static_cast<double>(std::numeric_limits<unsigned char>::max()))));
}

QColor ColorOperations::hsvToRgb(const QColor& hsv) {
  QColor rgb;

  rgb.setAlphaF(hsv.alphaF());

  if (hsv.blueF() > 0.0) {
    const auto hue = static_cast<float>(hsv.redF() * 2.0 * M_PI / (60.0 * M_PI / 180.0));
    const int i = static_cast<int>(std::floor(hue));
    const float f = hue - static_cast<float>(i);
    const auto p = static_cast<float>(hsv.blueF() * (1.0 - hsv.greenF()));
    const auto q = static_cast<float>(hsv.blueF() * (1.0 - static_cast<double>(hsv.greenF()) * f));
    const auto t = static_cast<float>(hsv.blueF() * (1.0 - hsv.greenF() * (1.0 - static_cast<double>(f))));

    switch (i) {
      case 0:
        rgb.setRedF(hsv.blueF());
        rgb.setGreenF(t);
        rgb.setBlueF(p);
        break;
      case 1:
        rgb.setRedF(q);
        rgb.setGreenF(hsv.blueF());
        rgb.setBlueF(p);
        break;
      case 2:
        rgb.setRedF(p);
        rgb.setGreenF(hsv.blueF());
        rgb.setBlueF(t);
        break;
      case 3:
        rgb.setRedF(p);
        rgb.setGreenF(q);
        rgb.setBlueF(hsv.blueF());
        break;
      case 4:
        rgb.setRedF(t);
        rgb.setGreenF(p);
        rgb.setBlueF(hsv.blueF());
        break;
      default:
        rgb.setRedF(hsv.blueF());
        rgb.setGreenF(p);
        rgb.setBlueF(q);
        break;
    }
  } else {
    rgb.setRedF(hsv.blueF());
    rgb.setGreenF(hsv.blueF());
    rgb.setBlueF(hsv.blueF());
  }

  return rgb;
}

QColor ColorOperations::intToRgb(unsigned char val) {
  QColor hsv;
  hsv.setRgbF(intToHue(val), 1.0, 1.0, 1.0);

  return hsvToRgb(hsv);
}

QColor ColorOperations::invertRgb(const QColor& rgb) {
  return QColor::fromRgbF(static_cast<float>(1.0 - rgb.redF()), static_cast<float>(1.0 - rgb.greenF()),
                          static_cast<float>(1.0 - rgb.blueF()), static_cast<float>(rgb.alphaF()));
}

}  // namespace rqt_multiplot
