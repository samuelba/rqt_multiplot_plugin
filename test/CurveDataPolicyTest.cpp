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
