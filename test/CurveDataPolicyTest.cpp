#include <limits>

#include <QPointF>
#include <QVector>

#include <gtest/gtest.h>

#include <rqt_multiplot/CurveDataCircularBuffer.h>
#include <rqt_multiplot/CurveDataListTimeFrame.h>

namespace {

using rqt_multiplot::CurveDataCircularBuffer;
using rqt_multiplot::CurveDataListTimeFrame;

QVector<QPointF> indexSeries(int count) {
  QVector<QPointF> points;
  points.reserve(count);
  for (int i = 0; i < count; ++i) {
    points.append(QPointF(static_cast<double>(i), static_cast<double>(i) * 0.5));
  }
  return points;
}

TEST(CurveDataListTimeFrame, appendPointDropsPointsOutsideWindow) {
  CurveDataListTimeFrame data(10.0);
  data.appendPoint(QPointF(0.0, 1.0));
  data.appendPoint(QPointF(5.0, 2.0));
  data.appendPoint(QPointF(12.0, 3.0));

  ASSERT_EQ(data.getNumPoints(), 2u);
  EXPECT_DOUBLE_EQ(data.getPoint(0).x(), 5.0);
  EXPECT_DOUBLE_EQ(data.getPoint(1).x(), 12.0);
}

TEST(CurveDataListTimeFrame, appendPointXBoundsAreOldestAndNewest) {
  CurveDataListTimeFrame data(10.0);
  data.appendPoint(QPointF(0.0, 1.0));
  data.appendPoint(QPointF(5.0, 2.0));
  data.appendPoint(QPointF(12.0, 3.0));

  const auto bounds = data.getBounds();
  EXPECT_DOUBLE_EQ(bounds.getMinimum().x(), 5.0);
  EXPECT_DOUBLE_EQ(bounds.getMaximum().x(), 12.0);
}

TEST(CurveDataListTimeFrame, appendPointRescansYWhenDroppedPointWasMaximum) {
  CurveDataListTimeFrame data(10.0);
  data.appendPoint(QPointF(0.0, 10.0));
  data.appendPoint(QPointF(5.0, 2.0));
  data.appendPoint(QPointF(6.0, 3.0));
  data.appendPoint(QPointF(12.0, 4.0));

  const auto bounds = data.getBounds();
  EXPECT_DOUBLE_EQ(bounds.getMinimum().y(), 2.0);
  EXPECT_DOUBLE_EQ(bounds.getMaximum().y(), 4.0);
}

TEST(CurveDataListTimeFrame, appendPointRescansYWhenDroppedPointWasMinimum) {
  CurveDataListTimeFrame data(10.0);
  data.appendPoint(QPointF(0.0, -4.0));
  data.appendPoint(QPointF(5.0, 2.0));
  data.appendPoint(QPointF(6.0, 3.0));
  data.appendPoint(QPointF(12.0, 1.0));

  const auto bounds = data.getBounds();
  EXPECT_DOUBLE_EQ(bounds.getMinimum().y(), 1.0);
  EXPECT_DOUBLE_EQ(bounds.getMaximum().y(), 3.0);
}

TEST(CurveDataListTimeFrame, appendPointKeepsYBoundsWhenDroppedPointIsInterior) {
  CurveDataListTimeFrame data(10.0);
  data.appendPoint(QPointF(0.0, 5.0));
  data.appendPoint(QPointF(5.0, 1.0));
  data.appendPoint(QPointF(6.0, 9.0));
  data.appendPoint(QPointF(12.0, 4.0));

  const auto bounds = data.getBounds();
  EXPECT_DOUBLE_EQ(bounds.getMinimum().y(), 1.0);
  EXPECT_DOUBLE_EQ(bounds.getMaximum().y(), 9.0);
}

TEST(CurveDataListTimeFrame, appendPointRescansYWhenMultipleExtremaExpire) {
  CurveDataListTimeFrame data(10.0);
  data.appendPoint(QPointF(0.0, 1.0));
  data.appendPoint(QPointF(1.0, 8.0));
  data.appendPoint(QPointF(2.0, 4.0));
  data.appendPoint(QPointF(20.0, 3.0));

  const auto bounds = data.getBounds();
  EXPECT_DOUBLE_EQ(bounds.getMinimum().x(), 20.0);
  EXPECT_DOUBLE_EQ(bounds.getMaximum().x(), 20.0);
  EXPECT_DOUBLE_EQ(bounds.getMinimum().y(), 3.0);
  EXPECT_DOUBLE_EQ(bounds.getMaximum().y(), 3.0);
}

TEST(CurveDataListTimeFrame, appendPointBoundsTrackMonotonicRamp) {
  CurveDataListTimeFrame data(10.0);
  for (int i = 0; i < 20; ++i) {
    data.appendPoint(QPointF(static_cast<double>(i), static_cast<double>(i)));
  }

  ASSERT_EQ(data.getNumPoints(), 11u);
  const auto bounds = data.getBounds();
  EXPECT_DOUBLE_EQ(bounds.getMinimum().x(), 9.0);
  EXPECT_DOUBLE_EQ(bounds.getMaximum().x(), 19.0);
  EXPECT_DOUBLE_EQ(bounds.getMinimum().y(), 9.0);
  EXPECT_DOUBLE_EQ(bounds.getMaximum().y(), 19.0);
}

TEST(CurveDataListTimeFrame, appendPointIgnoresNanYForBounds) {
  CurveDataListTimeFrame data(10.0);
  data.appendPoint(QPointF(0.0, 1.0));
  data.appendPoint(QPointF(1.0, std::numeric_limits<double>::quiet_NaN()));
  data.appendPoint(QPointF(2.0, 3.0));

  ASSERT_EQ(data.getNumPoints(), 3u);
  const auto bounds = data.getBounds();
  EXPECT_DOUBLE_EQ(bounds.getMinimum().y(), 1.0);
  EXPECT_DOUBLE_EQ(bounds.getMaximum().y(), 3.0);
}

TEST(CurveDataListTimeFrame, clearPointsInvalidatesBounds) {
  CurveDataListTimeFrame data(10.0);
  data.appendPoint(QPointF(1.0, 2.0));
  data.clearPoints();

  EXPECT_TRUE(data.isEmpty());
  EXPECT_FALSE(data.getBounds().isValid());
}

TEST(CurveDataListTimeFrame, replacePointsKeepsFullIndexSnapshot) {
  CurveDataListTimeFrame data(10.0);
  data.appendPoint(QPointF(100.0, 1.0));

  const auto snapshot = indexSeries(21);
  data.replacePoints(snapshot);

  ASSERT_EQ(data.getNumPoints(), 21u);
  EXPECT_DOUBLE_EQ(data.getPoint(0).x(), 0.0);
  EXPECT_DOUBLE_EQ(data.getPoint(20).x(), 20.0);
  EXPECT_DOUBLE_EQ(data.getPoint(20).y(), 10.0);
}

TEST(CurveDataCircularBuffer, replacePointsKeepsConfiguredCapacity) {
  CurveDataCircularBuffer data(5);
  data.appendPoint(QPointF(0.0, 1.0));

  data.replacePoints(indexSeries(12));

  EXPECT_EQ(data.getCapacity(), 5u);
  ASSERT_EQ(data.getNumPoints(), 5u);
  EXPECT_DOUBLE_EQ(data.getPoint(0).x(), 7.0);
  EXPECT_DOUBLE_EQ(data.getPoint(4).x(), 11.0);
}

TEST(CurveDataCircularBuffer, replacePointsCanClearSeries) {
  CurveDataCircularBuffer data(4);
  data.appendPoint(QPointF(1.0, 2.0));

  data.replacePoints({});

  EXPECT_TRUE(data.isEmpty());
}

}  // namespace
