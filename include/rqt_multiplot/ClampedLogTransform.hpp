/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <qwt/qwt_transform.h>

namespace rqt_multiplot {

class ClampedLogTransform : public QwtLogTransform {
 public:
  ClampedLogTransform();
  ~ClampedLogTransform() override;

  double transform(double value) const override;
  QwtTransform* copy() const override;
};

}  // namespace rqt_multiplot
