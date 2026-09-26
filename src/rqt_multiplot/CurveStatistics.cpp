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

#include "rqt_multiplot/CurveStatistics.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include <QPointF>

#include "rqt_multiplot/CurveData.hpp"

namespace rqt_multiplot {

namespace {

std::vector<double> collectAxisValues(const CurveData& data, CurveConfig::Axis axis, const BoundingRectangle* viewport) {
  std::vector<double> values;
  values.reserve(data.getNumPoints());

  for (size_t index = 0; index < data.getNumPoints(); ++index) {
    const QPointF point = data.getPoint(index);
    if (!std::isfinite(point.x()) || !std::isfinite(point.y())) {
      continue;
    }
    if ((viewport != nullptr) && !viewport->contains(point)) {
      continue;
    }
    values.push_back(data.getValue(index, axis));
  }

  return values;
}

double percentileLinear(const std::vector<double>& sorted, double fraction) {
  const double position = fraction * static_cast<double>(sorted.size() - 1);
  const auto lower = static_cast<size_t>(std::floor(position));
  const auto upper = static_cast<size_t>(std::ceil(position));
  if (lower == upper) {
    return sorted[lower];
  }
  const double weight = position - static_cast<double>(lower);
  return sorted[lower] + (weight * (sorted[upper] - sorted[lower]));
}

std::optional<double> repeatedMode(const std::vector<double>& sorted) {
  std::optional<double> mode;
  size_t bestCount = 1;
  size_t run = 1;

  for (size_t index = 1; index < sorted.size(); ++index) {
    if (sorted[index] == sorted[index - 1]) {
      ++run;
      continue;
    }
    if (run > bestCount) {
      bestCount = run;
      mode = sorted[index - 1];
    }
    run = 1;
  }

  if (run > bestCount) {
    mode = sorted.back();
  }
  return mode;
}

CurveStatistics summarizeValues(std::vector<double> values) {
  CurveStatistics stats;
  stats.count = values.size();
  if (stats.count == 0) {
    return stats;
  }

  std::sort(values.begin(), values.end());

  double sum = 0.0;
  double sumOfSquares = 0.0;
  for (const double value : values) {
    sum += value;
    sumOfSquares += value * value;
  }

  const double mean = sum / static_cast<double>(stats.count);
  stats.mean = mean;
  stats.minimum = values.front();
  stats.maximum = values.back();
  stats.range = values.back() - values.front();
  stats.median = percentileLinear(values, 0.5);
  stats.percentile25 = percentileLinear(values, 0.25);
  stats.percentile75 = percentileLinear(values, 0.75);
  stats.rms = std::sqrt(sumOfSquares / static_cast<double>(stats.count));
  stats.sum = sum;
  stats.mode = repeatedMode(values);

  if (stats.count >= 2) {
    double squaredDeviation = 0.0;
    for (const double value : values) {
      const double delta = value - mean;
      squaredDeviation += delta * delta;
    }
    stats.standardDeviation = std::sqrt(squaredDeviation / static_cast<double>(stats.count));
  }

  return stats;
}

}  // namespace

CurveStatistics computeCurveStatistics(const CurveData& data, CurveConfig::Axis axis, const BoundingRectangle* viewport) {
  return summarizeValues(collectAxisValues(data, axis, viewport));
}

}  // namespace rqt_multiplot
