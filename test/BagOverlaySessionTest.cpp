#include <string>

#include <QApplication>
#include <QSignalSpy>
#include <QStringList>

#include <gtest/gtest.h>

#include "BagMessageCollector.hpp"
#include "OverlayTestBags.hpp"
#include "rqt_multiplot/BagOverlaySession.hpp"
#include "rqt_multiplot/BagReader.hpp"

namespace {

using rqt_multiplot::BagMessageCollector;
using rqt_multiplot::BagOverlaySession;

constexpr int kSignalTimeoutMs = 10000;

class BagOverlaySessionTest : public ::testing::Test {
 protected:
  void SetUp() override { rqt_multiplot::test::ensureOverlayTestApplication(); }

  QString writeBag(const std::string& name, int64_t startSec, double value) const { return bags_.writeFloatBag(name, startSec, value); }

  rqt_multiplot::test::TempBagDirectory bags_;
};

TEST_F(BagOverlaySessionTest, loadSortsByStartTimeAndAlignsStartsToReference) {
  const QString late = writeBag("late", 110, 10.0);
  const QString early = writeBag("early", 100, 1.0);

  BagOverlaySession session;
  ASSERT_TRUE(session.load(QStringList{late, early}));

  ASSERT_EQ(session.getNumBags(), 2);
  EXPECT_EQ(session.getPath(0), early);
  EXPECT_EQ(session.getLabel(0), QStringLiteral("early"));
  EXPECT_EQ(session.getLabel(1), QStringLiteral("late"));
  EXPECT_DOUBLE_EQ(session.getOffset(0), 0.0);
  EXPECT_DOUBLE_EQ(session.getOffset(1), -10.0);
  EXPECT_NE(session.getReader(0), session.getReader(1));
}

TEST_F(BagOverlaySessionTest, loadRejectsTooFewOrTooManyBags) {
  const QString a = writeBag("a", 1, 0.0);
  const QString b = writeBag("b", 2, 0.0);
  const QString c = writeBag("c", 3, 0.0);
  const QString d = writeBag("d", 4, 0.0);

  BagOverlaySession session;
  EXPECT_FALSE(session.load(QStringList{a}));
  EXPECT_FALSE(session.isActive());
  EXPECT_FALSE(session.getError().isEmpty());

  EXPECT_FALSE(session.load(QStringList{a, a}));
  EXPECT_FALSE(session.load(QStringList{a, b, c, d}));
  EXPECT_FALSE(session.isActive());

  EXPECT_TRUE(session.load(QStringList{a, b, c}));
  EXPECT_TRUE(session.getError().isEmpty());
  EXPECT_EQ(session.getNumBags(), 3);
}

TEST_F(BagOverlaySessionTest, loadRejectsBagThatCannotBeOpened) {
  const QString good = writeBag("good", 1, 0.0);
  const QString missing = QStringLiteral("/this/path/does/not/exist.mcap");

  BagOverlaySession session;
  EXPECT_FALSE(session.load(QStringList{good, missing}));
  EXPECT_TRUE(session.getError().contains(missing));
  EXPECT_FALSE(session.isActive());
}

TEST_F(BagOverlaySessionTest, eachReaderDeliversOnlyItsOwnBag) {
  const QString early = writeBag("early", 100, 1.0);
  const QString late = writeBag("late", 200, 10.0);

  BagOverlaySession session;
  ASSERT_TRUE(session.load(QStringList{late, early}));

  BagMessageCollector reference;
  BagMessageCollector overlay;
  ASSERT_TRUE(session.getReader(0)->subscribe(QStringLiteral("/float"), &reference, SLOT(onMessage(const QString&, const Message&)), {},
                                              Qt::AutoConnection));
  ASSERT_TRUE(session.getReader(1)->subscribe(QStringLiteral("/float"), &overlay, SLOT(onMessage(const QString&, const Message&)), {},
                                              Qt::AutoConnection));
  QSignalSpy started(&session, &BagOverlaySession::readingStarted);
  QSignalSpy progress(&session, &BagOverlaySession::readingProgressChanged);
  QSignalSpy finished(&session, &BagOverlaySession::readingFinished);

  session.read();
  EXPECT_TRUE(session.isReading());
  ASSERT_TRUE(finished.wait(kSignalTimeoutMs));
  QApplication::processEvents();

  EXPECT_EQ(started.count(), 1);
  EXPECT_FALSE(session.isReading());
  ASSERT_EQ(reference.samples.size(), 2);
  ASSERT_EQ(overlay.samples.size(), 2);
  EXPECT_DOUBLE_EQ(reference.samples[0].value, 1.0);
  EXPECT_DOUBLE_EQ(overlay.samples[0].value, 10.0);
  ASSERT_FALSE(progress.isEmpty());
  EXPECT_DOUBLE_EQ(progress.last().at(0).toDouble(), 1.0);
}

TEST_F(BagOverlaySessionTest, setOffsetEmitsDeltaAndKeepsReferenceFixed) {
  BagOverlaySession session;
  ASSERT_TRUE(session.load(QStringList{writeBag("a", 10, 0.0), writeBag("b", 20, 0.0)}));
  QSignalSpy changed(&session, &BagOverlaySession::offsetChanged);

  session.setOffset(1, -7.5);
  session.setOffset(1, -7.5);
  session.setOffset(0, 3.0);

  ASSERT_EQ(changed.count(), 1);
  EXPECT_EQ(changed[0].at(0).toInt(), 1);
  EXPECT_DOUBLE_EQ(changed[0].at(1).toDouble(), 2.5);
  EXPECT_DOUBLE_EQ(session.getOffset(1), -7.5);
  EXPECT_DOUBLE_EQ(session.getOffset(0), 0.0);
}

TEST_F(BagOverlaySessionTest, alignAtMarkersMovesMarkerBOntoMarkerA) {
  BagOverlaySession session;
  ASSERT_TRUE(session.load(QStringList{writeBag("a", 10, 0.0), writeBag("b", 20, 0.0)}));

  session.alignAtMarkers(1, 4.0, 6.5);
  session.alignAtMarkers(0, 4.0, 6.5);

  EXPECT_DOUBLE_EQ(session.getOffset(1), -10.0 - 2.5);
  EXPECT_DOUBLE_EQ(session.getOffset(0), 0.0);
}

TEST_F(BagOverlaySessionTest, resetOffsetsRestoresStartAlignment) {
  BagOverlaySession session;
  ASSERT_TRUE(session.load(QStringList{writeBag("a", 10, 0.0), writeBag("b", 20, 0.0), writeBag("c", 25, 0.0)}));
  session.alignAtMarkers(1, 4.0, 6.5);
  QSignalSpy changed(&session, &BagOverlaySession::offsetChanged);

  session.resetOffsets();

  ASSERT_EQ(changed.count(), 1);
  EXPECT_EQ(changed[0].at(0).toInt(), 1);
  EXPECT_DOUBLE_EQ(session.getOffset(1), -10.0);
  EXPECT_DOUBLE_EQ(session.getOffset(2), -15.0);
}

TEST_F(BagOverlaySessionTest, clearEmitsClearedOnceAndDeactivates) {
  BagOverlaySession session;
  ASSERT_TRUE(session.load(QStringList{writeBag("a", 10, 0.0), writeBag("b", 20, 0.0)}));
  QSignalSpy cleared(&session, &BagOverlaySession::cleared);

  session.clear();
  session.clear();

  EXPECT_EQ(cleared.count(), 1);
  EXPECT_FALSE(session.isActive());
  EXPECT_EQ(session.getReader(1), nullptr);
}

TEST_F(BagOverlaySessionTest, loadingNewBagsClearsPreviousOverlay) {
  BagOverlaySession session;
  ASSERT_TRUE(session.load(QStringList{writeBag("a", 10, 0.0), writeBag("b", 20, 0.0)}));
  QSignalSpy cleared(&session, &BagOverlaySession::cleared);

  ASSERT_TRUE(session.load(QStringList{writeBag("c", 10, 0.0), writeBag("d", 20, 0.0)}));

  EXPECT_EQ(cleared.count(), 1);
  EXPECT_EQ(session.getLabel(0), QStringLiteral("c"));
}

TEST(BagOverlaySessionLabels, usesFileOrDirectoryNameAndParentOnClash) {
  EXPECT_EQ(BagOverlaySession::labelsForPaths(QStringList{QStringLiteral("/data/run1.mcap"), QStringLiteral("/data/run2/")}),
            (QStringList{QStringLiteral("run1"), QStringLiteral("run2")}));
  EXPECT_EQ(BagOverlaySession::labelsForPaths(QStringList{QStringLiteral("/a/bag_0.mcap"), QStringLiteral("/b/bag_0.mcap")}),
            (QStringList{QStringLiteral("a/bag_0"), QStringLiteral("b/bag_0")}));
}

}  // namespace
