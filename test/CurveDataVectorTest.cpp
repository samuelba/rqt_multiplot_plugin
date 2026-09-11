#include <QPointF>
#include <QVector>

#include <gtest/gtest.h>

#include <rqt_multiplot/CurveDataVector.h>

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

}  // namespace
