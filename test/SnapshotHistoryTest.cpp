#include <QPointF>
#include <QVector>

#include <gtest/gtest.h>

#include <rqt_multiplot/SnapshotHistory.h>

namespace {

using rqt_multiplot::SnapshotHistory;
using rqt_multiplot::snapshotFadeAlpha;

TEST(SnapshotHistory, fadeAlphaIsFullForCurrentAndDecaysWithAge) {
  EXPECT_EQ(snapshotFadeAlpha(255, 0, 5), 255);
  EXPECT_EQ(snapshotFadeAlpha(255, 1, 5), 182);
  EXPECT_EQ(snapshotFadeAlpha(255, 5, 5), 36);
  EXPECT_EQ(snapshotFadeAlpha(255, 1, 0), 0);
  EXPECT_EQ(snapshotFadeAlpha(255, 6, 5), 0);
}

TEST(SnapshotHistory, keepsNewestFramesUpToCapacity) {
  SnapshotHistory history;
  history.setCapacity(2);

  history.push(QVector<QPointF>{QPointF(0.0, 1.0)});
  history.push(QVector<QPointF>{QPointF(0.0, 2.0)});
  history.push(QVector<QPointF>{QPointF(0.0, 3.0)});

  ASSERT_EQ(history.frames().size(), 2);
  EXPECT_DOUBLE_EQ(history.frames()[0][0].y(), 3.0);
  EXPECT_DOUBLE_EQ(history.frames()[1][0].y(), 2.0);
}

TEST(SnapshotHistory, clearingCapacityDropsFrames) {
  SnapshotHistory history;
  history.setCapacity(3);
  history.push(QVector<QPointF>{QPointF(0.0, 1.0)});

  history.setCapacity(0);

  EXPECT_TRUE(history.frames().isEmpty());
}

TEST(SnapshotHistory, clearRemovesFrames) {
  SnapshotHistory history;
  history.setCapacity(2);
  history.push(QVector<QPointF>{QPointF(0.0, 1.0)});

  history.clear();

  EXPECT_TRUE(history.frames().isEmpty());
  EXPECT_EQ(history.getCapacity(), 2u);
}

}  // namespace
