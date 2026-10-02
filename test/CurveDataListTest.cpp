#include <cmath>
#include <limits>

#include <QPointF>
#include <QStringList>

#include <gtest/gtest.h>

#include "rqt_multiplot/CurveConfig.hpp"
#include "rqt_multiplot/CurveDataCircularBuffer.hpp"
#include "rqt_multiplot/CurveDataList.hpp"

namespace {

using rqt_multiplot::CurveConfig;
using rqt_multiplot::CurveDataCircularBuffer;
using rqt_multiplot::CurveDataList;

TEST(CurveDataList, appendPointTracksBoundsAndClearResetsThem) {
  CurveDataList data;
  EXPECT_TRUE(data.isEmpty());
  EXPECT_EQ(data.size(), 0u);

  rqt_multiplot::CurveData& base = data;
  base.appendPoint(1.0, 4.0);
  data.appendPoint(QPointF(3.0, -2.0));

  ASSERT_EQ(data.getNumPoints(), 2u);
  EXPECT_EQ(data.sample(0), QPointF(1.0, 4.0));
  EXPECT_DOUBLE_EQ(data.getValue(1, CurveConfig::X), 3.0);
  EXPECT_DOUBLE_EQ(data.getValue(1, CurveConfig::Y), -2.0);
  EXPECT_TRUE(std::isnan(data.getValue(0, static_cast<CurveConfig::Axis>(99))));

  const auto bounds = data.getBounds();
  EXPECT_DOUBLE_EQ(bounds.getMinimum().x(), 1.0);
  EXPECT_DOUBLE_EQ(bounds.getMaximum().x(), 3.0);
  EXPECT_DOUBLE_EQ(bounds.getMinimum().y(), -2.0);
  EXPECT_DOUBLE_EQ(bounds.getMaximum().y(), 4.0);
  EXPECT_EQ(data.boundingRect(), bounds.getRectangle());

  const auto xBounds = data.getAxisBounds(CurveConfig::X);
  EXPECT_DOUBLE_EQ(xBounds.first, 1.0);
  EXPECT_DOUBLE_EQ(xBounds.second, 3.0);
  const auto yBounds = data.getAxisBounds(CurveConfig::Y);
  EXPECT_DOUBLE_EQ(yBounds.first, -2.0);
  EXPECT_DOUBLE_EQ(yBounds.second, 4.0);
  EXPECT_EQ(data.getAxisBounds(static_cast<CurveConfig::Axis>(99)), (QPair<double, double>()));

  const auto near = data.getPointsInDistance(2.0, 1.1);
  ASSERT_EQ(near.size(), 2);
  EXPECT_TRUE(data.getPointsInDistance(10.0, 0.1).isEmpty());

  QStringList formattedX{QStringLiteral("stale")};
  QStringList formattedY{QStringLiteral("stale")};
  data.writeFormatted(formattedX, formattedY);
  ASSERT_EQ(formattedX.size(), 2);
  EXPECT_EQ(formattedX.at(0), QString::number(1.0, 'g', 20));
  EXPECT_EQ(formattedY.at(1), QString::number(-2.0, 'g', 20));

  const auto interpolated = data.interpolateY(2.0);
  ASSERT_TRUE(interpolated.has_value());
  EXPECT_DOUBLE_EQ(*interpolated, 1.0);
  EXPECT_FALSE(data.interpolateY(-1.0).has_value());

  data.clearPoints();
  EXPECT_TRUE(data.isEmpty());
  EXPECT_TRUE(data.getPointsInDistance(1.0, 1.0).isEmpty());
  EXPECT_FALSE(data.getBounds().isValid());
}

TEST(CurveDataCircularBuffer, pointsInDistanceFollowWrappedIndexes) {
  CurveDataCircularBuffer data(2);
  EXPECT_EQ(data.getCapacity(), 2u);

  data.appendPoint(QPointF(0.0, 1.0));
  data.appendPoint(QPointF(1.0, 5.0));
  data.appendPoint(QPointF(2.0, 3.0));

  ASSERT_EQ(data.getNumPoints(), 2u);
  EXPECT_DOUBLE_EQ(data.getPoint(0).x(), 1.0);
  EXPECT_DOUBLE_EQ(data.getPoint(1).x(), 2.0);

  const auto indexes = data.getPointsInDistance(1.5, 0.6);
  ASSERT_EQ(indexes.size(), 2);
  EXPECT_DOUBLE_EQ(data.getPoint(indexes.at(0)).x(), 1.0);
  EXPECT_DOUBLE_EQ(data.getPoint(indexes.at(1)).x(), 2.0);
  EXPECT_TRUE(data.getPointsInDistance(10.0, 0.1).isEmpty());

  const auto bounds = data.getBounds();
  EXPECT_DOUBLE_EQ(bounds.getMinimum().y(), 3.0);
  EXPECT_DOUBLE_EQ(bounds.getMaximum().y(), 5.0);

  data.clearPoints();
  EXPECT_TRUE(data.isEmpty());
  EXPECT_FALSE(data.getBounds().isValid());
}

}  // namespace
