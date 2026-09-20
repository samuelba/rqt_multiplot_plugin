/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/SnapshotHistory.h"

#include <cmath>

namespace rqt_multiplot {

int snapshotFadeAlpha(int baseAlpha, size_t age, size_t count) {
  if (age == 0) {
    return baseAlpha;
  }
  if (count == 0 || age > count) {
    return 0;
  }
  return static_cast<int>(
      std::lround(static_cast<double>(baseAlpha) * static_cast<double>(count + 1 - age) / static_cast<double>(count + 2)));
}

void SnapshotHistory::setCapacity(size_t capacity) {
  capacity_ = capacity;
  trim();
}

size_t SnapshotHistory::getCapacity() const {
  return capacity_;
}

void SnapshotHistory::push(const QVector<QPointF>& snapshot) {
  if (capacity_ == 0) {
    return;
  }

  frames_.prepend(snapshot);
  trim();
}

void SnapshotHistory::clear() {
  frames_.clear();
}

void SnapshotHistory::rescaleAxis(int axis, double factor) {
  for (auto& frame : frames_) {
    for (auto& point : frame) {
      if (axis == 0) {
        point.setX(point.x() * factor);
      } else {
        point.setY(point.y() * factor);
      }
    }
  }
}

const QVector<QVector<QPointF>>& SnapshotHistory::frames() const {
  return frames_;
}

void SnapshotHistory::trim() {
  if (static_cast<size_t>(frames_.size()) > capacity_) {
    frames_.resize(static_cast<int>(capacity_));
  }
}

}  // namespace rqt_multiplot
