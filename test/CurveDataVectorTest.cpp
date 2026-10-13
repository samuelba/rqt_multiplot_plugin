#include <QPointF>
#include <QVector>

#include <gtest/gtest.h>

#include "rqt_multiplot/CurveDataVector.hpp"

namespace {

using rqt_multiplot::CurveDataVector;

TEST(CurveDataVector, replacePointsClearsPreviousSamples) {
  CurveDataVector data;
  data.appendPoint(QPointF(0.0, 1.0));
  data.appendPoint(QPointF(1.0, 2.0));
  data.appendPoint(QPointF(2.0, 3.0));

  QVector<QPointF> replacement;
  replacement.append(QPointF(0.0, 9.0));
  replacement.append(QPointF(1.0, 8.0));

  data.replacePoints(replacement);

  ASSERT_EQ(data.getNumPoints(), 2u);
  EXPECT_DOUBLE_EQ(data.getPoint(0).y(), 9.0);
  EXPECT_DOUBLE_EQ(data.getPoint(1).y(), 8.0);
}

TEST(CurveDataVector, replacePointsCanShrinkSeries) {
  CurveDataVector data;
  data.appendPoint(QPointF(0.0, 1.0));
  data.appendPoint(QPointF(1.0, 2.0));
  data.appendPoint(QPointF(2.0, 3.0));
  data.appendPoint(QPointF(3.0, 4.0));

  data.replacePoints(QVector<QPointF>{QPointF(0.0, 5.0)});

  ASSERT_EQ(data.getNumPoints(), 1u);
  EXPECT_DOUBLE_EQ(data.getPoint(0).y(), 5.0);
}

TEST(CurveDataVector, replacePointsCanClearSeries) {
  CurveDataVector data;
  data.appendPoint(QPointF(0.0, 1.0));

  data.replacePoints({});

  EXPECT_TRUE(data.isEmpty());
}

TEST(CurveDataVector, interpolateYReturnsSampleOnExactHit) {
  CurveDataVector data;
  data.appendPoint(QPointF(0.0, 1.0));
  data.appendPoint(QPointF(1.0, 3.0));

  ASSERT_TRUE(data.interpolateY(1.0).has_value());
  EXPECT_DOUBLE_EQ(*data.interpolateY(1.0), 3.0);
  EXPECT_DOUBLE_EQ(*data.interpolateY(0.0), 1.0);
}

TEST(CurveDataVector, interpolateYIsLinearBetweenSamples) {
  CurveDataVector data;
  data.appendPoint(QPointF(0.0, 0.0));
  data.appendPoint(QPointF(2.0, 10.0));
  data.appendPoint(QPointF(4.0, 0.0));

  EXPECT_DOUBLE_EQ(*data.interpolateY(0.5), 2.5);
  EXPECT_DOUBLE_EQ(*data.interpolateY(3.0), 5.0);
}

TEST(CurveDataVector, interpolateYIsEmptyOutsideRange) {
  CurveDataVector data;
  data.appendPoint(QPointF(1.0, 1.0));
  data.appendPoint(QPointF(2.0, 2.0));

  EXPECT_FALSE(data.interpolateY(0.5).has_value());
  EXPECT_FALSE(data.interpolateY(2.5).has_value());
  EXPECT_FALSE(CurveDataVector().interpolateY(0.0).has_value());
}

TEST(CurveDataVector, interpolateYOnSinglePointOnlyHitsThatPoint) {
  CurveDataVector data;
  data.appendPoint(QPointF(1.0, 7.0));

  EXPECT_DOUBLE_EQ(*data.interpolateY(1.0), 7.0);
  EXPECT_FALSE(data.interpolateY(1.5).has_value());
}

TEST(CurveDataVector, getPointsInDistanceKeepsPointsWithEqualX) {
  CurveDataVector data;
  data.appendPoint(QPointF(0.0, 1.0));
  data.appendPoint(QPointF(1.0, 2.0));
  data.appendPoint(QPointF(1.0, 3.0));
  data.appendPoint(QPointF(2.0, 4.0));

  const QVector<size_t> indexes = data.getPointsInDistance(1.0, 0.1);

  EXPECT_EQ(indexes, (QVector<size_t>{1, 2}));
}

TEST(CurveDataVector, getPointsInDistanceReturnsRangeOnMonotonicX) {
  CurveDataVector data;
  for (int i = 0; i < 10; ++i) {
    data.appendPoint(QPointF(i, i));
  }

  EXPECT_EQ(data.getPointsInDistance(4.5, 1.0), (QVector<size_t>{4, 5}));
  EXPECT_TRUE(data.getPointsInDistance(20.0, 1.0).isEmpty());
}

TEST(CurveDataVector, getPointsInDistanceFindsPointsOnNonMonotonicX) {
  CurveDataVector data;
  data.appendPoint(QPointF(0.0, 0.0));
  data.appendPoint(QPointF(3.0, 1.0));
  data.appendPoint(QPointF(1.0, 2.0));
  data.appendPoint(QPointF(3.0, 3.0));
  data.appendPoint(QPointF(2.0, 4.0));

  EXPECT_EQ(data.getPointsInDistance(3.0, 0.1), (QVector<size_t>{1, 3}));
  EXPECT_EQ(data.getPointsInDistance(1.5, 0.6), (QVector<size_t>{2, 4}));
}

TEST(CurveDataVector, interpolateYUsesSortedXOnNonMonotonicX) {
  CurveDataVector data;
  data.appendPoint(QPointF(4.0, 40.0));
  data.appendPoint(QPointF(0.0, 0.0));
  data.appendPoint(QPointF(2.0, 20.0));

  EXPECT_DOUBLE_EQ(*data.interpolateY(1.0), 10.0);
  EXPECT_DOUBLE_EQ(*data.interpolateY(3.0), 30.0);
  EXPECT_FALSE(data.interpolateY(5.0).has_value());
}

TEST(CurveDataVector, clearPointsResetsNonMonotonicIndex) {
  CurveDataVector data;
  data.appendPoint(QPointF(2.0, 0.0));
  data.appendPoint(QPointF(1.0, 0.0));

  data.clearPoints();
  data.appendPoint(QPointF(0.0, 0.0));
  data.appendPoint(QPointF(2.0, 2.0));

  EXPECT_DOUBLE_EQ(*data.interpolateY(1.0), 1.0);
  EXPECT_EQ(data.getPointsInDistance(2.0, 0.0), (QVector<size_t>{1}));
}

}  // namespace
