#include <string>

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QMouseEvent>
#include <QPointF>
#include <QSignalSpy>
#include <QStringList>

#include <gtest/gtest.h>
#include <qwt/qwt_plot.h>
#include <qwt/qwt_text.h>

#include "OverlayTestBags.hpp"
#include "rqt_multiplot/BagOverlaySession.hpp"
#include "rqt_multiplot/CurveAxisConfig.hpp"
#include "rqt_multiplot/CurveConfig.hpp"
#include "rqt_multiplot/CurveData.hpp"
#include "rqt_multiplot/CurveDataVector.hpp"
#include "rqt_multiplot/MultiplotConfig.hpp"
#include "rqt_multiplot/PlotConfig.hpp"
#include "rqt_multiplot/PlotCurve.hpp"
#include "rqt_multiplot/PlotMarkerPair.hpp"
#include "rqt_multiplot/PlotOverlayCurves.hpp"
#include "rqt_multiplot/PlotTabWidget.hpp"
#include "rqt_multiplot/PlotTableWidget.hpp"
#include "rqt_multiplot/PlotWidget.hpp"

namespace {

using rqt_multiplot::BagOverlaySession;
using rqt_multiplot::CurveAxisConfig;
using rqt_multiplot::CurveConfig;
using rqt_multiplot::MultiplotConfig;
using rqt_multiplot::PlotConfig;
using rqt_multiplot::PlotCurve;
using rqt_multiplot::PlotTabWidget;
using rqt_multiplot::PlotWidget;

constexpr int kSignalTimeoutMs = 10000;

void configureFloatCurve(CurveConfig* config, const QString& title) {
  config->setTitle(title);
  for (const auto axis : {CurveConfig::X, CurveConfig::Y}) {
    config->getAxisConfig(axis)->setTopic(QStringLiteral("/float"));
    config->getAxisConfig(axis)->setType(QStringLiteral("std_msgs/msg/Float64"));
  }
  config->getAxisConfig(CurveConfig::X)->setFieldType(CurveAxisConfig::MessageReceiptTime);
  config->getAxisConfig(CurveConfig::Y)->setField(QStringLiteral("data"));
}

void configureSnapshotCurve(CurveConfig* config) {
  config->getAxisConfig(CurveConfig::X)->setTopic(QStringLiteral("/array"));
  config->getAxisConfig(CurveConfig::Y)->setTopic(QStringLiteral("/array"));
  config->getAxisConfig(CurveConfig::X)->setFieldType(CurveAxisConfig::ArrayIndex);
  config->getAxisConfig(CurveConfig::Y)->setField(QStringLiteral("position/*"));
}

QStringList titles(const QVector<PlotCurve*>& curves) {
  QStringList result;
  for (const PlotCurve* curve : curves) {
    result.append(curve->title().text());
  }
  return result;
}

QVector<QPointF> points(const PlotCurve* curve) {
  QVector<QPointF> result;
  for (size_t index = 0; index < curve->getData()->getNumPoints(); ++index) {
    result.append(curve->getData()->getPoint(index));
  }
  return result;
}

class PlotOverlayCurvesTest : public ::testing::Test {
 protected:
  void SetUp() override { rqt_multiplot::test::ensureOverlayTestApplication(); }

  QStringList writeBags(int count) const {
    QStringList paths;
    for (int index = 0; index < count; ++index) {
      paths.append(bags_.writeFloatBag("run" + std::to_string(index + 1), 100 + (10 * index), 10.0 * index));
    }
    return paths;
  }

  rqt_multiplot::test::TempBagDirectory bags_;
};

TEST_F(PlotOverlayCurvesTest, createsOneCopyPerOverlayBagAndCurveAndSkipsArrays) {
  PlotConfig config;
  configureFloatCurve(config.addCurve(), QStringLiteral("a"));
  configureFloatCurve(config.addCurve(), QStringLiteral("b"));
  configureSnapshotCurve(config.addCurve());
  PlotWidget widget;
  widget.setConfig(&config);
  BagOverlaySession session;
  ASSERT_TRUE(session.load(writeBags(3)));

  widget.setOverlaySession(&session);

  EXPECT_EQ(widget.getCurves().size(), 3);
  EXPECT_EQ(titles(widget.getOverlayCurves()), (QStringList{"a [run2]", "b [run2]", "a [run3]", "b [run3]"}));
  EXPECT_EQ(widget.getOverlaySession(), &session);
  EXPECT_EQ(widget.getOverlayCurves()[0]->pen().style(), Qt::DashLine);
  EXPECT_EQ(widget.getOverlayCurves()[2]->pen().style(), Qt::DotLine);
  EXPECT_DOUBLE_EQ(widget.getOverlayCurves()[0]->getXOffset(), -10.0);
  EXPECT_DOUBLE_EQ(widget.getOverlayCurves()[2]->getXOffset(), -20.0);
}

TEST_F(PlotOverlayCurvesTest, preferredScaleIncludesCopies) {
  PlotConfig config;
  configureFloatCurve(config.addCurve(), QStringLiteral("a"));
  PlotWidget widget;
  widget.setConfig(&config);
  BagOverlaySession session;
  ASSERT_TRUE(session.load(writeBags(2)));
  widget.setOverlaySession(&session);

  widget.getCurves().front()->getData()->appendPoint(QPointF(1.0, 1.0));
  widget.getOverlayCurves().front()->getData()->appendPoint(QPointF(50.0, -20.0));

  const auto bounds = widget.getPreferredScale();
  EXPECT_DOUBLE_EQ(bounds.getMaximum().x(), 50.0);
  EXPECT_DOUBLE_EQ(bounds.getMinimum().y(), -20.0);
}

TEST_F(PlotOverlayCurvesTest, removingCurveRemovesOnlyItsCopies) {
  PlotConfig config;
  configureFloatCurve(config.addCurve(), QStringLiteral("a"));
  configureFloatCurve(config.addCurve(), QStringLiteral("b"));
  PlotWidget widget;
  widget.setConfig(&config);
  BagOverlaySession session;
  ASSERT_TRUE(session.load(writeBags(2)));
  widget.setOverlaySession(&session);

  config.removeCurve(static_cast<size_t>(1));

  EXPECT_EQ(titles(widget.getOverlayCurves()), (QStringList{"a [run2]"}));
}

TEST_F(PlotOverlayCurvesTest, removingFirstLiveCurveKeepsTheRest) {
  PlotConfig config;
  configureFloatCurve(config.addCurve(), QStringLiteral("a"));
  configureFloatCurve(config.addCurve(), QStringLiteral("b"));
  configureFloatCurve(config.addCurve(), QStringLiteral("c"));
  PlotWidget widget;
  widget.setConfig(&config);

  config.removeCurve(static_cast<size_t>(0));

  ASSERT_EQ(widget.getCurves().size(), 2);
  EXPECT_EQ(titles(widget.getCurves()), (QStringList{"b", "c"}));
  EXPECT_EQ(config.getCurveConfig(0)->getColorConfig()->getAutoColorIndex(), 0);
  EXPECT_EQ(config.getCurveConfig(1)->getColorConfig()->getAutoColorIndex(), 1);
}

TEST_F(PlotOverlayCurvesTest, removingFirstLiveCurveAfterOriginIsBound) {
  PlotConfig config;
  configureFloatCurve(config.addCurve(), QStringLiteral("a"));
  configureFloatCurve(config.addCurve(), QStringLiteral("b"));
  PlotWidget widget;
  widget.setConfig(&config);
  widget.bindAxisOrigin(CurveConfig::X, 1.0);

  config.removeCurve(static_cast<size_t>(0));

  EXPECT_EQ(titles(widget.getCurves()), (QStringList{"b"}));
}

TEST_F(PlotOverlayCurvesTest, deletingFirstCurveConfigNotifiesThePlot) {
  PlotConfig config;
  configureFloatCurve(config.addCurve(), QStringLiteral("a"));
  configureFloatCurve(config.addCurve(), QStringLiteral("b"));
  PlotWidget widget;
  widget.setConfig(&config);

  delete config.getCurveConfig(0);

  EXPECT_EQ(widget.getCurves().size(), 1);
  EXPECT_EQ(titles(widget.getCurves()), (QStringList{"b"}));
  EXPECT_EQ(config.getNumCurves(), 1u);
  EXPECT_EQ(config.getCurveConfig(0)->getColorConfig()->getAutoColorIndex(), 0);
}

TEST_F(PlotOverlayCurvesTest, removingFirstCurveRemovesOnlyItsCopies) {
  PlotConfig config;
  configureFloatCurve(config.addCurve(), QStringLiteral("a"));
  configureFloatCurve(config.addCurve(), QStringLiteral("b"));
  PlotWidget widget;
  widget.setConfig(&config);
  BagOverlaySession session;
  ASSERT_TRUE(session.load(writeBags(2)));
  widget.setOverlaySession(&session);

  config.removeCurve(static_cast<size_t>(0));

  EXPECT_EQ(titles(widget.getCurves()), (QStringList{"b"}));
  EXPECT_EQ(titles(widget.getOverlayCurves()), (QStringList{"b [run2]"}));
}

TEST_F(PlotOverlayCurvesTest, clearingCurvesRemovesAllCopies) {
  PlotConfig config;
  configureFloatCurve(config.addCurve(), QStringLiteral("a"));
  configureFloatCurve(config.addCurve(), QStringLiteral("b"));
  PlotWidget widget;
  widget.setConfig(&config);
  BagOverlaySession session;
  ASSERT_TRUE(session.load(writeBags(2)));
  widget.setOverlaySession(&session);

  config.clearCurves();

  EXPECT_TRUE(widget.getOverlayCurves().isEmpty());
}

TEST_F(PlotOverlayCurvesTest, clearingSessionRemovesCopies) {
  PlotConfig config;
  configureFloatCurve(config.addCurve(), QStringLiteral("a"));
  PlotWidget widget;
  widget.setConfig(&config);
  BagOverlaySession session;
  ASSERT_TRUE(session.load(writeBags(2)));
  widget.setOverlaySession(&session);

  session.clear();

  EXPECT_TRUE(widget.getOverlayCurves().isEmpty());
  EXPECT_EQ(widget.getOverlaySession(), nullptr);
}

TEST_F(PlotOverlayCurvesTest, offsetChangeShiftsOnlyCopiesOfThatBag) {
  PlotConfig config;
  configureFloatCurve(config.addCurve(), QStringLiteral("a"));
  PlotWidget widget;
  widget.setConfig(&config);
  BagOverlaySession session;
  ASSERT_TRUE(session.load(writeBags(3)));
  widget.setOverlaySession(&session);
  PlotCurve* second = widget.getOverlayCurves()[0];
  PlotCurve* third = widget.getOverlayCurves()[1];
  second->getData()->appendPoint(QPointF(100.0, 1.0));
  third->getData()->appendPoint(QPointF(100.0, 1.0));

  session.setOffset(1, session.getOffset(1) + 2.5);

  EXPECT_EQ(points(second), QVector<QPointF>({{102.5, 1.0}}));
  EXPECT_EQ(points(third), QVector<QPointF>({{100.0, 1.0}}));
}

TEST_F(PlotOverlayCurvesTest, exportTitlesNameTheBag) {
  PlotConfig config;
  configureFloatCurve(config.addCurve(), QStringLiteral("a"));
  PlotWidget widget;
  widget.setConfig(&config);
  BagOverlaySession session;
  ASSERT_TRUE(session.load(writeBags(2)));
  widget.setOverlaySession(&session);

  QStringList axisTitles;
  widget.writeFormattedCurveAxisTitles(axisTitles);
  QList<QStringList> data;
  widget.writeFormattedCurveData(data);

  EXPECT_EQ(axisTitles, (QStringList{"/float/receipt_time", "/float/data", "/float/receipt_time [run2]", "/float/data [run2]"}));
  EXPECT_EQ(data.size(), 4);
}

TEST(FirstCrossingX, interpolatesRisingAndFallingCrossings) {
  rqt_multiplot::CurveDataVector rising;
  rising.appendPoint(QPointF(0.0, 0.0));
  rising.appendPoint(QPointF(10.0, 10.0));
  rqt_multiplot::CurveDataVector falling;
  falling.appendPoint(QPointF(0.0, 4.0));
  falling.appendPoint(QPointF(1.0, 2.0));
  falling.appendPoint(QPointF(2.0, 0.0));

  EXPECT_DOUBLE_EQ(rqt_multiplot::firstCrossingX(rising, 2.5).value(), 2.5);
  EXPECT_DOUBLE_EQ(rqt_multiplot::firstCrossingX(falling, 1.0).value(), 1.5);
  EXPECT_DOUBLE_EQ(rqt_multiplot::firstCrossingX(falling, 4.0).value(), 0.0);
}

TEST(FirstCrossingX, isEmptyWhenTheValueIsNeverReached) {
  rqt_multiplot::CurveDataVector data;
  EXPECT_FALSE(rqt_multiplot::firstCrossingX(data, 1.0).has_value());
  data.appendPoint(QPointF(0.0, 0.0));
  data.appendPoint(QPointF(1.0, 0.5));
  EXPECT_FALSE(rqt_multiplot::firstCrossingX(data, 1.0).has_value());
}

class PlotOverlayThresholdTest : public PlotOverlayCurvesTest {
 protected:
  void SetUp() override {
    PlotOverlayCurvesTest::SetUp();
    configureFloatCurve(config_.addCurve(), QStringLiteral("a"));
    reference_.setConfig(config_.getCurveConfig(0));
  }

  void load(int numBags) {
    ASSERT_TRUE(session_.load(writeBags(numBags)));
    overlay_.setSession(&session_, {&reference_}, false);
  }

  PlotConfig config_;
  QObject parent_;
  QwtPlot plot_;
  PlotCurve reference_;
  rqt_multiplot::PlotOverlayCurves overlay_{&parent_, &plot_};
  BagOverlaySession session_;
};

TEST_F(PlotOverlayThresholdTest, shiftsEachCrossingCopyOntoTheReference) {
  load(3);
  reference_.getData()->appendPoint(QPointF(100.0, 0.0));
  reference_.getData()->appendPoint(QPointF(110.0, 10.0));
  overlay_.getCurves()[0]->getData()->appendPoint(QPointF(100.0, 0.0));
  overlay_.getCurves()[0]->getData()->appendPoint(QPointF(102.0, 10.0));
  overlay_.getCurves()[1]->getData()->appendPoint(QPointF(100.0, 0.0));

  const auto result = overlay_.alignAtThreshold(&reference_, 5.0);

  EXPECT_TRUE(result.referenceCrosses);
  EXPECT_EQ(result.alignedLabels, QStringList{"run2"});
  EXPECT_EQ(result.missedLabels, QStringList{"run3"});
  EXPECT_DOUBLE_EQ(session_.getOffset(1), -10.0 + 4.0);
  EXPECT_DOUBLE_EQ(session_.getOffset(2), -20.0);
}

TEST_F(PlotOverlayThresholdTest, keepsOffsetsWhenReferenceNeverCrosses) {
  load(2);
  overlay_.getCurves()[0]->getData()->appendPoint(QPointF(100.0, 0.0));
  overlay_.getCurves()[0]->getData()->appendPoint(QPointF(102.0, 10.0));

  const auto result = overlay_.alignAtThreshold(&reference_, 5.0);

  EXPECT_FALSE(result.referenceCrosses);
  EXPECT_DOUBLE_EQ(session_.getOffset(1), -10.0);
}

void openContextMenu(PlotWidget& widget) {
  QWidget* canvas = widget.findChild<QwtPlot*>()->canvas();
  const QPoint position(50, 50);
  QMouseEvent press(QEvent::MouseButtonPress, QPointF(position), QPointF(canvas->mapToGlobal(position)), Qt::RightButton, Qt::RightButton,
                    Qt::NoModifier);
  QMouseEvent release(QEvent::MouseButtonRelease, QPointF(position), QPointF(canvas->mapToGlobal(position)), Qt::RightButton,
                      Qt::RightButton, Qt::NoModifier);
  QApplication::sendEvent(canvas, &press);
  QApplication::sendEvent(canvas, &release);
  widget.findChild<QMenu*>(QStringLiteral("plotContextMenu"))->hide();
}

class PlotOverlayAlignTest : public PlotOverlayCurvesTest {
 protected:
  void SetUp() override {
    PlotOverlayCurvesTest::SetUp();
    configureFloatCurve(config_.addCurve(), QStringLiteral("a"));
    widget_.setConfig(&config_);
    widget_.resize(400, 300);
    widget_.show();
    QApplication::processEvents();
  }

  void placeMarkers(double a, double b) {
    widget_.getMarkers()->placeMarker(rqt_multiplot::MarkerId::A, a);
    widget_.getMarkers()->placeMarker(rqt_multiplot::MarkerId::B, b);
  }

  PlotConfig config_;
  PlotWidget widget_;
  BagOverlaySession session_;
};

TEST_F(PlotOverlayAlignTest, alignActionNeedsOverlayAndBothMarkers) {
  auto* align = widget_.findChild<QAction*>(QStringLiteral("actionContextAlignOverlay"));
  auto* threshold = widget_.findChild<QAction*>(QStringLiteral("actionContextAlignThreshold"));
  ASSERT_NE(align, nullptr);
  ASSERT_NE(threshold, nullptr);

  placeMarkers(4.0, 6.5);
  openContextMenu(widget_);
  EXPECT_FALSE(align->isEnabled());
  EXPECT_FALSE(threshold->isEnabled());

  ASSERT_TRUE(session_.load(writeBags(2)));
  widget_.setOverlaySession(&session_);
  widget_.getMarkers()->clearMarkers();
  openContextMenu(widget_);
  EXPECT_FALSE(align->isEnabled());
  EXPECT_TRUE(threshold->isEnabled());

  placeMarkers(4.0, 6.5);
  openContextMenu(widget_);
  ASSERT_TRUE(align->isEnabled());
  EXPECT_TRUE(align->isVisible());
  align->trigger();

  EXPECT_DOUBLE_EQ(session_.getOffset(1), -12.5);
  EXPECT_DOUBLE_EQ(widget_.getOverlayCurves().front()->getXOffset(), -12.5);
  EXPECT_FALSE(widget_.getMarkers()->hasAnyMarker());
}

TEST_F(PlotOverlayAlignTest, resetAlignmentRestoresStartAlignedOffsets) {
  auto* reset = widget_.findChild<QAction*>(QStringLiteral("actionContextResetOverlayAlignment"));
  ASSERT_NE(reset, nullptr);
  openContextMenu(widget_);
  EXPECT_FALSE(reset->isEnabled());

  ASSERT_TRUE(session_.load(writeBags(3)));
  widget_.setOverlaySession(&session_);
  session_.setOffset(1, 3.0);
  session_.setOffset(2, 4.0);
  openContextMenu(widget_);
  ASSERT_TRUE(reset->isEnabled());

  reset->trigger();

  EXPECT_DOUBLE_EQ(session_.getOffset(1), -10.0);
  EXPECT_DOUBLE_EQ(session_.getOffset(2), -20.0);
  EXPECT_DOUBLE_EQ(widget_.getOverlayCurves()[1]->getXOffset(), -20.0);
}

TEST_F(PlotOverlayAlignTest, threeBagsOfferOneAlignEntryPerOverlayBag) {
  ASSERT_TRUE(session_.load(writeBags(3)));
  widget_.setOverlaySession(&session_);
  placeMarkers(4.0, 6.5);

  openContextMenu(widget_);

  auto* submenu = widget_.findChild<QMenu*>(QStringLiteral("menuContextAlignOverlay"));
  ASSERT_NE(submenu, nullptr);
  EXPECT_TRUE(submenu->menuAction()->isVisible());
  EXPECT_FALSE(widget_.findChild<QAction*>(QStringLiteral("actionContextAlignOverlay"))->isVisible());
  auto* third = submenu->findChild<QAction*>(QStringLiteral("actionContextAlignOverlay2"));
  ASSERT_NE(third, nullptr);
  EXPECT_EQ(third->text(), QStringLiteral("run3"));

  third->trigger();

  EXPECT_DOUBLE_EQ(session_.getOffset(1), -10.0);
  EXPECT_DOUBLE_EQ(session_.getOffset(2), -22.5);
}

class PlotTabOverlayTest : public PlotOverlayCurvesTest {
 protected:
  void SetUp() override {
    PlotOverlayCurvesTest::SetUp();
    tabs_.setConfig(&config_);
    plot_ = tabs_.getPlotTable(0)->getPlotWidgets().front();
    configureFloatCurve(plot_->getConfig()->addCurve(), QStringLiteral("value"));
  }

  MultiplotConfig config_{nullptr};
  PlotTabWidget tabs_;
  PlotWidget* plot_ = nullptr;
};

TEST_F(PlotTabOverlayTest, overlayReadsEachBagIntoItsOwnCurveAlignedToReferenceStart) {
  const QString early = bags_.writeFloatBag("early", 100, 1.0);
  const QString late = bags_.writeFloatBag("late", 110, 10.0);
  QSignalSpy finished(&tabs_, &PlotTabWidget::jobFinished);

  ASSERT_TRUE(tabs_.loadOverlayBagFiles(QStringList{late, early}));
  ASSERT_TRUE(finished.wait(kSignalTimeoutMs));
  QApplication::processEvents();

  ASSERT_EQ(plot_->getOverlayCurves().size(), 1);
  EXPECT_EQ(points(plot_->getCurves().front()), QVector<QPointF>({{100.0, 1.0}, {101.0, 2.0}}));
  EXPECT_EQ(points(plot_->getOverlayCurves().front()), QVector<QPointF>({{100.0, 10.0}, {101.0, 11.0}}));
  EXPECT_EQ(plot_->getOverlayCurves().front()->title().text(), QStringLiteral("value [late]"));
  EXPECT_TRUE(plot_->isPaused());
}

TEST_F(PlotTabOverlayTest, openingBagsAfterOverlayRemovesCopies) {
  ASSERT_TRUE(tabs_.loadOverlayBagFiles(writeBags(2)));
  ASSERT_EQ(plot_->getOverlayCurves().size(), 1);

  tabs_.loadFromBagFiles(QStringList{QStringLiteral("/this/path/does/not/exist.mcap")}, true);

  EXPECT_TRUE(plot_->getOverlayCurves().isEmpty());
  EXPECT_FALSE(tabs_.getOverlaySession()->isActive());
}

TEST_F(PlotTabOverlayTest, addingBagsKeepsCopies) {
  ASSERT_TRUE(tabs_.loadOverlayBagFiles(writeBags(2)));

  tabs_.loadFromBagFiles(QStringList{QStringLiteral("/this/path/does/not/exist.mcap")}, false);

  EXPECT_EQ(plot_->getOverlayCurves().size(), 1);
  EXPECT_TRUE(tabs_.getOverlaySession()->isActive());
}

TEST_F(PlotTabOverlayTest, rejectsSingleBagWithoutTouchingPlots) {
  plot_->getCurves().front()->getData()->appendPoint(QPointF(1.0, 1.0));

  EXPECT_FALSE(tabs_.loadOverlayBagFiles(writeBags(1)));

  EXPECT_TRUE(plot_->getOverlayCurves().isEmpty());
  EXPECT_EQ(plot_->getCurves().front()->getData()->getNumPoints(), 1u);
  EXPECT_FALSE(tabs_.getOverlaySession()->getError().isEmpty());
}

}  // namespace
