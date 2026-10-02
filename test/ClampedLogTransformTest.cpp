#include <cmath>

#include <gtest/gtest.h>
#include <qwt/qwt_transform.h>

#include "rqt_multiplot/ClampedLogTransform.hpp"

namespace {

using rqt_multiplot::ClampedLogTransform;

TEST(ClampedLogTransform, nonPositiveValuesStayFiniteBelowLogMin) {
  ClampedLogTransform transform;
  const double floor = QwtLogTransform().transform(QwtLogTransform::LogMin);

  EXPECT_TRUE(std::isfinite(transform.transform(0.0)));
  EXPECT_LT(transform.transform(0.0), floor);
  EXPECT_TRUE(std::isfinite(transform.transform(-2.0)));
  EXPECT_LT(transform.transform(-2.0), floor);
  EXPECT_DOUBLE_EQ(transform.transform(10.0), QwtLogTransform().transform(10.0));
}

}  // namespace
