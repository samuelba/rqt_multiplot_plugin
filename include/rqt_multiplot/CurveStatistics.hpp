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

#pragma once

#include <cstddef>
#include <optional>

#include "rqt_multiplot/BoundingRectangle.hpp"
#include "rqt_multiplot/CurveConfig.hpp"

namespace rqt_multiplot {

class CurveData;

struct CurveStatistics {
  size_t count = 0;
  std::optional<double> mean;
  std::optional<double> standardDeviation;
  std::optional<double> minimum;
  std::optional<double> maximum;
  std::optional<double> range;
  std::optional<double> median;
  std::optional<double> percentile25;
  std::optional<double> percentile75;
  std::optional<double> mode;
  std::optional<double> rms;
  std::optional<double> sum;
};

// viewport == nullptr keeps every finite point. A point is kept only when it lies inside the rectangle.
CurveStatistics computeCurveStatistics(const CurveData& data, CurveConfig::Axis axis, const BoundingRectangle* viewport);

}  // namespace rqt_multiplot
