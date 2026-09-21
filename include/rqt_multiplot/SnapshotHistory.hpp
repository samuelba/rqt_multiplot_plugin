/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <QPointF>
#include <QVector>

namespace rqt_multiplot {

int snapshotFadeAlpha(int baseAlpha, size_t age, size_t count);

class SnapshotHistory {
 public:
  void setCapacity(size_t capacity);
  size_t getCapacity() const;
  void push(const QVector<QPointF>& snapshot);
  void clear();
  void rescaleAxis(int axis, double factor);
  const QVector<QVector<QPointF>>& frames() const;

 private:
  void trim();

  size_t capacity_ = 0;
  QVector<QVector<QPointF>> frames_;
};

}  // namespace rqt_multiplot
