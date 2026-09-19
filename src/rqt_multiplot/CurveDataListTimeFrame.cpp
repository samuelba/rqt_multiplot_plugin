/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner, Samuel Bachmann                       *
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

#include "rqt_multiplot/CurveDataListTimeFrame.h"

namespace rqt_multiplot {

/*****************************************************************************/
/* Constructors and Destructor                                               */
/*****************************************************************************/

CurveDataListTimeFrame::CurveDataListTimeFrame(double length) : timeFrameLength_(length) {}

CurveDataListTimeFrame::~CurveDataListTimeFrame() = default;

/*****************************************************************************/
/* Accessors                                                                 */
/*****************************************************************************/

size_t CurveDataListTimeFrame::getNumPoints() const {
  return points_.size();
}

QPointF CurveDataListTimeFrame::getPoint(size_t index) const {
  return points_[index];
}

BoundingRectangle CurveDataListTimeFrame::getBounds() const {
  return bounds_;
}

/*****************************************************************************/
/* Methods                                                                   */
/*****************************************************************************/

void CurveDataListTimeFrame::dropExpiredPoints(double timeCutoff) {
  while (!points_.empty() && points_.front().x() < timeCutoff) {
    const double y = points_.front().y();
    if (std::isfinite(y)) {
      const auto yIt = yValues_.find(y);
      if (yIt != yValues_.end()) {
        yValues_.erase(yIt);
      }
    }
    points_.pop_front();
  }
}

void CurveDataListTimeFrame::syncBounds() {
  if (points_.empty()) {
    bounds_.clear();
    return;
  }
  const double yMin = yValues_.empty() ? points_.front().y() : *yValues_.begin();
  const double yMax = yValues_.empty() ? yMin : *yValues_.rbegin();
  bounds_.setMinimum(QPointF(points_.front().x(), yMin));
  bounds_.setMaximum(QPointF(points_.back().x(), yMax));
}

void CurveDataListTimeFrame::appendPoint(const QPointF& point) {
  dropExpiredPoints(point.x() - timeFrameLength_);
  points_.push_back(point);
  if (std::isfinite(point.y())) {
    yValues_.insert(point.y());
  }
  syncBounds();
}

void CurveDataListTimeFrame::replacePoints(const QVector<QPointF>& points) {
  points_.assign(points.begin(), points.end());
  yValues_.clear();
  for (const auto& point : points_) {
    if (std::isfinite(point.y())) {
      yValues_.insert(point.y());
    }
  }
  syncBounds();
}

void CurveDataListTimeFrame::clearPoints() {
  points_.clear();
  yValues_.clear();
  bounds_.clear();
}

}  // namespace rqt_multiplot
