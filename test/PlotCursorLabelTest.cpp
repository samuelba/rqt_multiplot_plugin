#include <gtest/gtest.h>

#include <QColor>
#include <QFont>
#include <QPointF>
#include <QRect>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QVector>

#include "rqt_multiplot/PlotCursorLabel.hpp"

namespace {

using rqt_multiplot::kTrackedReadoutSwatchSize;
using rqt_multiplot::kTrackPointSnapPixels;
using rqt_multiplot::nearestPointByX;
using rqt_multiplot::trackedPointLabel;
using rqt_multiplot::trackedPointLabels;
using rqt_multiplot::trackedPointsReadoutRect;
using rqt_multiplot::trackedReadoutLayout;
using rqt_multiplot::TrackedReadoutLayout;
using rqt_multiplot::TrackedReadoutRow;
using rqt_multiplot::trackedReadoutSize;
using rqt_multiplot::trackedReadoutSwatchExtent;
using rqt_multiplot::trackedReadoutSwatchRect;
using rqt_multiplot::trackPointSnapDistance;

TEST(PlotCursorLabel, snapDistanceScalesUnitsPerPixel) {
  EXPECT_DOUBLE_EQ(trackPointSnapDistance(0.5, kTrackPointSnapPixels), 8.0);
  EXPECT_EQ(kTrackPointSnapPixels, 16);
}

TEST(PlotCursorLabel, nearestPointUsesVerticalLineNotEuclidean) {
  const QVector<QPointF> slope{{0.0, 0.0}, {1.0, 10.0}, {2.0, 20.0}};

  EXPECT_EQ(nearestPointByX(slope, 1.2), QPointF(1.0, 10.0));
}

TEST(PlotCursorLabel, formatsCoordinatesWithoutTitle) {
  EXPECT_EQ(trackedPointLabel(QString(), QStringLiteral("1.25"), QStringLiteral("-0.5")), QStringLiteral("1.25, -0.5"));
}

TEST(PlotCursorLabel, prefixesCurveTitle) {
  EXPECT_EQ(trackedPointLabel(QStringLiteral("Pan"), QStringLiteral("1.25"), QStringLiteral("-0.5")), QStringLiteral("Pan: 1.25, -0.5"));
}

TEST(PlotCursorLabel, joinsOneLinePerCurve) {
  EXPECT_EQ(trackedPointLabels({QStringLiteral("Pan: 1, 2"), QStringLiteral("Tilt: 1, 3")}), QStringLiteral("Pan: 1, 2\nTilt: 1, 3"));
}

TEST(PlotCursorLabel, sizesColumnsFromLongestValueInEachColumn) {
  const QFont font;
  const QVector<TrackedReadoutRow> rows{{Qt::red, QStringLiteral("Pan"), QStringLiteral("1"), QStringLiteral("2")},
                                        {Qt::blue, QStringLiteral("VeryLongName"), QStringLiteral("12.34"), QStringLiteral("-0.5")}};
  const auto layout = trackedReadoutLayout(rows, font);

  EXPECT_EQ(layout.swatchSize, trackedReadoutSwatchExtent(layout.rowHeight));
  EXPECT_GT(layout.titleWidth, 0);
  EXPECT_GT(layout.xWidth, 0);
  EXPECT_GT(layout.yWidth, 0);
  EXPECT_EQ(layout.rowHeight, QFontMetrics(font).height());
  EXPECT_EQ(trackedReadoutSize(layout, rows.size()).width(),
            layout.swatchSize + layout.columnGap + layout.titleWidth + layout.columnGap + layout.xWidth + layout.columnGap + layout.yWidth);
  EXPECT_EQ(trackedReadoutSize(layout, rows.size()).height(), layout.rowHeight * rows.size());
}

TEST(PlotCursorLabel, colorSwatchFitsInsideTheRow) {
  EXPECT_EQ(kTrackedReadoutSwatchSize, 10);
  EXPECT_EQ(trackedReadoutSwatchExtent(16), 10);
  EXPECT_EQ(trackedReadoutSwatchExtent(8), 6);
  EXPECT_EQ(trackedReadoutSwatchExtent(0), 0);

  const TrackedReadoutLayout layout{10, 40, 20, 20, 8, 16};
  const QRect swatch = trackedReadoutSwatchRect(layout, 4, 20);

  EXPECT_EQ(swatch, QRect(4, 23, 10, 10));
  EXPECT_GE(swatch.top(), 20);
  EXPECT_LE(swatch.bottom(), 20 + layout.rowHeight - 1);
}

TEST(PlotCursorLabel, placesReadoutAboveRightOfCursor) {
  const QRect rect = trackedPointsReadoutRect(QPoint(50, 50), QSize(80, 20), QRect(0, 0, 200, 100));

  EXPECT_EQ(rect, QRect(55, 25, 80, 20));
}

TEST(PlotCursorLabel, flipsReadoutWhenItWouldLeaveTheCanvas) {
  const QRect right = trackedPointsReadoutRect(QPoint(180, 50), QSize(80, 20), QRect(0, 0, 200, 100));
  const QRect top = trackedPointsReadoutRect(QPoint(50, 10), QSize(80, 20), QRect(0, 0, 200, 100));
  const QRect bottom = trackedPointsReadoutRect(QPoint(50, 8), QSize(80, 30), QRect(0, 0, 200, 40));
  const QRect wide = trackedPointsReadoutRect(QPoint(150, 50), QSize(160, 20), QRect(0, 0, 200, 100));

  EXPECT_EQ(right, QRect(95, 25, 80, 20));
  EXPECT_EQ(top, QRect(55, 15, 80, 20));
  EXPECT_EQ(bottom, QRect(55, 4, 80, 30));
  EXPECT_EQ(wide, QRect(5, 25, 160, 20));
}

}  // namespace
