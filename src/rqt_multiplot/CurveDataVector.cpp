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

#include <algorithm>
#include <cmath>
#include <iterator>

#include "rqt_multiplot/CurveDataVector.hpp"

namespace rqt_multiplot {

CurveDataVector::CurveDataVector() = default;

CurveDataVector::~CurveDataVector() = default;

size_t CurveDataVector::getNumPoints() const {
  return points_.count();
}

QPointF CurveDataVector::getPoint(size_t index) const {
  return points_[static_cast<int>(index)];
}

int CurveDataVector::lowerBoundIndex(double x) const {
  const auto lessX = [](const QPointF& point, double value) { return point.x() < value; };
  return static_cast<int>(std::distance(points_.cbegin(), std::lower_bound(points_.cbegin(), points_.cend(), x, lessX)));
}

QVector<size_t> CurveDataVector::getPointsInDistance(double x, double maxDistance) const {
  QVector<size_t> indexes;

  if (isMonotonic_) {
    for (int index = lowerBoundIndex(x - maxDistance); index < points_.count(); ++index) {
      if (fabs(x - points_[index].x()) > maxDistance) {
        break;
      }
      indexes.push_back(static_cast<size_t>(index));
    }
    return indexes;
  }

  for (auto it = x_.lower_bound(XCoordinateRef(x - maxDistance)); it != x_.end(); ++it) {
    if (fabs(x - it->x_) > maxDistance) {
      break;
    }
    indexes.push_back(it->index_);
  }

  return indexes;
}

std::optional<double> CurveDataVector::interpolateY(double x) const {
  if (isMonotonic_) {
    const int right = lowerBoundIndex(x);
    if (right == points_.count()) {
      return std::nullopt;
    }
    if (points_[right].x() == x) {
      return points_[right].y();
    }
    if (right == 0) {
      return std::nullopt;
    }
    return interpolateLinear(points_[right - 1], points_[right], x);
  }

  const auto right = x_.lower_bound(XCoordinateRef(x));
  if (right == x_.end()) {
    return std::nullopt;
  }
  const QPointF rightPoint = points_[static_cast<int>(right->index_)];
  if (right->x_ == x) {
    return rightPoint.y();
  }
  if (right == x_.begin()) {
    return std::nullopt;
  }
  return interpolateLinear(points_[static_cast<int>(std::prev(right)->index_)], rightPoint, x);
}

BoundingRectangle CurveDataVector::getBounds() const {
  return bounds_;
}

void CurveDataVector::appendPoint(const QPointF& point) {
  bounds_ += point;

  if (points_.capacity() < points_.count() + 1) {
    points_.reserve((points_.capacity() != 0) ? 2 * points_.capacity() : 1);
  }

  points_.append(point);

  if (isMonotonic_) {
    if ((points_.count() > 1) && (point.x() < points_[points_.count() - 2].x())) {
      isMonotonic_ = false;
      buildIndex();
    }
    return;
  }

  if (x_.capacity() < x_.size() + 1) {
    x_.reserve((x_.capacity() != 0u) ? 2 * x_.capacity() : 1);
  }

  x_.insert(XCoordinateRef(point.x(), points_.size() - 1));
}

void CurveDataVector::buildIndex() {
  x_.clear();
  x_.reserve(static_cast<size_t>(points_.count()));
  for (int index = 0; index < points_.count(); ++index) {
    x_.insert(XCoordinateRef(points_[index].x(), static_cast<size_t>(index)));
  }
}

void CurveDataVector::clearPoints() {
  points_.clear();
  x_.clear();
  isMonotonic_ = true;

  bounds_.clear();
}

}  // namespace rqt_multiplot
