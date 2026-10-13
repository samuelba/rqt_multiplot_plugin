#include <cmath>
#include <cstdlib>
#include <optional>

#include <QApplication>
#include <QMetaObject>
#include <QPalette>
#include <QStringList>
#include <QVector>
#include <QWidget>

#include <gtest/gtest.h>
#include <qwt/qwt_legend_data.h>
#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_item.h>
#include <qwt/qwt_text.h>

#include "rqt_multiplot/CurveAxisConfig.hpp"
#include "rqt_multiplot/CurveConfig.hpp"
#include "rqt_multiplot/CurveData.hpp"
#include "rqt_multiplot/CurveDataCircularBuffer.hpp"
#include "rqt_multiplot/CurveDataConfig.hpp"
#include "rqt_multiplot/CurveDataListTimeFrame.hpp"
#include "rqt_multiplot/PlotConfig.hpp"
#include "rqt_multiplot/PlotCurve.hpp"
#include "rqt_multiplot/PlotWidget.hpp"

namespace {

using rqt_multiplot::CurveAxisConfig;
using rqt_multiplot::CurveColorConfig;
using rqt_multiplot::CurveConfig;
using rqt_multiplot::CurveDataCircularBuffer;
using rqt_multiplot::CurveDataConfig;
using rqt_multiplot::CurveDataListTimeFrame;
using rqt_multiplot::PlotConfig;
using rqt_multiplot::PlotCurve;
using rqt_multiplot::PlotWidget;
using rqt_multiplot::snapshotFadeAlpha;

QApplication* ensureApplication() {
  if (QApplication::instance() != nullptr) {
    return qobject_cast<QApplication*>(QApplication::instance());
  }
  qputenv("QT_QPA_PLATFORM", "offscreen");
  static int argc = 1;
  static char arg0[] = "test_rqt_multiplot";
  static char* argv[] = {arg0, nullptr};
  return new QApplication(argc, argv);
}

QStringList* gQtWarnings = nullptr;

void captureQtWarnings(QtMsgType type, const QMessageLogContext& /*context*/, const QString& message) {
  if ((gQtWarnings != nullptr) && (type == QtWarningMsg)) {
    gQtWarnings->append(message);
  }
}

void configureSnapshotCurve(CurveConfig* config) {
  config->getAxisConfig(CurveConfig::X)->setTopic("/array");
  config->getAxisConfig(CurveConfig::Y)->setTopic("/array");
  config->getAxisConfig(CurveConfig::X)->setFieldType(CurveAxisConfig::ArrayIndex);
  config->getAxisConfig(CurveConfig::Y)->setField("position/*");
  config->getStyleConfig()->setFadeHistory(3);
}

TEST(PlotCurve, destroyingParentAfterSamplesDoesNotCrash) {
  ensureApplication();

  auto* parent = new QWidget();
  auto* plot = new QwtPlot(parent);
  auto* curve = new PlotCurve(parent);
  curve->attach(plot);
  curve->getData()->appendPoint(QPointF(1.0, 2.0));
  curve->getData()->appendPoint(QPointF(2.0, 3.0));

  delete parent;
}

TEST(PlotCurve, destroyingParentAfterSnapshotGhostsDoesNotCrash) {
  ensureApplication();

  CurveConfig config;
  configureSnapshotCurve(&config);

  auto* parent = new QWidget();
  auto* plot = new QwtPlot(parent);
  auto* curve = new PlotCurve(parent);
  curve->attach(plot);
  curve->setConfig(&config);
  curve->run();

  const QVector<QPointF> first{QPointF(0.0, 1.0), QPointF(1.0, 2.0)};
  const QVector<QPointF> second{QPointF(0.0, 3.0), QPointF(1.0, 4.0)};
  ASSERT_TRUE(QMetaObject::invokeMethod(curve, "dataSequencerSeriesReceived", Qt::DirectConnection, Q_ARG(QVector<QPointF>, first)));
  ASSERT_TRUE(QMetaObject::invokeMethod(curve, "dataSequencerSeriesReceived", Qt::DirectConnection, Q_ARG(QVector<QPointF>, second)));
  ASSERT_EQ(plot->itemList().count(), 3);

  delete parent;
}

TEST(PlotCurve, clearingConfigDoesNotWarnAboutMissingChangedSignal) {
  ensureApplication();

  QStringList warnings;
  gQtWarnings = &warnings;
  const QtMessageHandler previous = qInstallMessageHandler(captureQtWarnings);

  CurveConfig config;
  PlotCurve curve;
  curve.setConfig(&config);
  curve.setConfig(nullptr);

  qInstallMessageHandler(previous);
  gQtWarnings = nullptr;

  for (const QString& warning : warnings) {
    EXPECT_FALSE(warning.contains(QStringLiteral("No such signal"))) << warning.toStdString();
  }
}

TEST(PlotCurve, hidingCurveHidesSnapshotGhosts) {
  ensureApplication();

  CurveConfig config;
  configureSnapshotCurve(&config);

  auto* parent = new QWidget();
  auto* plot = new QwtPlot(parent);
  auto* curve = new PlotCurve(parent);
  curve->attach(plot);
  curve->setConfig(&config);
  curve->run();

  const QVector<QPointF> first{QPointF(0.0, 1.0), QPointF(1.0, 2.0)};
  const QVector<QPointF> second{QPointF(0.0, 3.0), QPointF(1.0, 4.0)};
  ASSERT_TRUE(QMetaObject::invokeMethod(curve, "dataSequencerSeriesReceived", Qt::DirectConnection, Q_ARG(QVector<QPointF>, first)));
  ASSERT_TRUE(QMetaObject::invokeMethod(curve, "dataSequencerSeriesReceived", Qt::DirectConnection, Q_ARG(QVector<QPointF>, second)));
  ASSERT_EQ(plot->itemList().count(), 3);

  curve->setVisible(false);

  EXPECT_FALSE(curve->isVisible());
  for (QwtPlotItem* item : plot->itemList()) {
    EXPECT_FALSE(item->isVisible());
  }

  curve->setVisible(true);

  EXPECT_TRUE(curve->isVisible());
  for (QwtPlotItem* item : plot->itemList()) {
    EXPECT_TRUE(item->isVisible());
  }

  delete parent;
}

TEST(PlotCurve, hiddenLegendDataUsesStrikeoutAndDisabledColor) {
  ensureApplication();

  PlotCurve curve;
  curve.setTitle(QStringLiteral("alpha"));

  const auto visibleData = curve.legendData();
  ASSERT_FALSE(visibleData.isEmpty());
  EXPECT_FALSE(visibleData.front().title().font().strikeOut());

  curve.setVisible(false);

  const auto hiddenData = curve.legendData();
  ASSERT_FALSE(hiddenData.isEmpty());
  EXPECT_TRUE(hiddenData.front().title().font().strikeOut());
  EXPECT_EQ(hiddenData.front().title().color(), QApplication::palette().color(QPalette::Disabled, QPalette::WindowText));
}

TEST(PlotCurve, togglingUnitConversionRescalesStoredYValues) {
  ensureApplication();

  CurveConfig config;
  config.getAxisConfig(CurveConfig::Y)->setField("linear/x");

  PlotCurve curve;
  curve.setConfig(&config);
  curve.getData()->appendPoint(QPointF(1.0, M_PI));
  curve.getData()->appendPoint(QPointF(2.0, 2.0 * M_PI));

  config.getAxisConfig(CurveConfig::Y)->setUnitConversion(CurveAxisConfig::RadiansToDegrees);

  EXPECT_DOUBLE_EQ(curve.getData()->getPoint(0).x(), 1.0);
  EXPECT_DOUBLE_EQ(curve.getData()->getPoint(1).x(), 2.0);
  EXPECT_NEAR(curve.getData()->getPoint(0).y(), 180.0, 1e-9);
  EXPECT_NEAR(curve.getData()->getPoint(1).y(), 360.0, 1e-9);
}

TEST(PlotCurve, hiddenCurveReportsEmptyPreferredScale) {
  ensureApplication();

  CurveConfig config;
  PlotCurve curve;
  curve.setConfig(&config);
  curve.getData()->appendPoint(QPointF(1.0, 2.0));
  curve.getData()->appendPoint(QPointF(3.0, 4.0));

  EXPECT_TRUE(curve.getPreferredScale().isValid());

  curve.setVisible(false);

  EXPECT_FALSE(curve.getPreferredScale().isValid());
}

void configureReceiptTimeCurve(CurveConfig* config) {
  config->getAxisConfig(CurveConfig::X)->setFieldType(CurveAxisConfig::MessageReceiptTime);
  config->getAxisConfig(CurveConfig::Y)->setField("linear/x");
  config->getDataConfig()->setType(CurveDataConfig::CircularBuffer);
  config->getDataConfig()->setCircularBufferCapacity(100);
}

TEST(PlotCurve, plotTimeWindowOverridePrunesToLastSeconds) {
  ensureApplication();

  CurveConfig config;
  configureReceiptTimeCurve(&config);

  PlotCurve curve;
  curve.setConfig(&config);
  curve.setPlotTimeWindowLength(10);

  ASSERT_TRUE(dynamic_cast<CurveDataListTimeFrame*>(curve.getData()) != nullptr);

  curve.getData()->appendPoint(QPointF(0.0, 1.0));
  curve.getData()->appendPoint(QPointF(5.0, 2.0));
  curve.getData()->appendPoint(QPointF(12.0, 3.0));

  ASSERT_EQ(curve.getData()->getNumPoints(), 2u);
  EXPECT_DOUBLE_EQ(curve.getData()->getPoint(0).x(), 5.0);
  EXPECT_DOUBLE_EQ(curve.getData()->getPoint(1).x(), 12.0);
}

TEST(PlotCurve, clearingPlotTimeWindowRestoresCurveDataBackend) {
  ensureApplication();

  CurveConfig config;
  configureReceiptTimeCurve(&config);

  PlotCurve curve;
  curve.setConfig(&config);
  curve.setPlotTimeWindowLength(10);
  ASSERT_TRUE(dynamic_cast<CurveDataListTimeFrame*>(curve.getData()) != nullptr);

  curve.setPlotTimeWindowLength(std::nullopt);
  ASSERT_TRUE(dynamic_cast<CurveDataCircularBuffer*>(curve.getData()) != nullptr);
}

TEST(PlotWidget, plotTimeWindowAppliesToAllCurvesWhenEligible) {
  ensureApplication();

  PlotConfig config;
  configureReceiptTimeCurve(config.addCurve());
  configureReceiptTimeCurve(config.addCurve());
  config.setTimeWindowEnabled(true);
  config.setTimeWindowLength(10);

  PlotWidget widget;
  widget.setConfig(&config);

  const auto curves = widget.getCurves();
  ASSERT_EQ(curves.size(), 2);
  for (PlotCurve* curve : curves) {
    EXPECT_TRUE(dynamic_cast<CurveDataListTimeFrame*>(curve->getData()) != nullptr);
  }
}

TEST(PlotWidget, plotTimeWindowDoesNotApplyWhenIneligible) {
  ensureApplication();

  PlotConfig config;
  configureReceiptTimeCurve(config.addCurve());
  config.addCurve()->getAxisConfig(CurveConfig::X)->setField("linear/x");
  config.setTimeWindowEnabled(true);
  config.setTimeWindowLength(10);

  PlotWidget widget;
  widget.setConfig(&config);

  const auto curves = widget.getCurves();
  ASSERT_EQ(curves.size(), 2);
  for (PlotCurve* curve : curves) {
    EXPECT_EQ(dynamic_cast<CurveDataListTimeFrame*>(curve->getData()), nullptr);
  }
}

TEST(PlotWidget, destroyingAfterPlottedSamplesDoesNotCrash) {
  ensureApplication();

  PlotConfig config;
  config.addCurve();

  auto* widget = new PlotWidget();
  widget->setConfig(&config);
  const auto curves = widget->findChildren<PlotCurve*>();
  ASSERT_EQ(curves.size(), 1);
  curves.front()->getData()->appendPoint(QPointF(1.0, 2.0));
  widget->forceReplot();

  delete widget;
}

void feedPoints(PlotCurve& curve, const QVector<QPointF>& points) {
  for (const auto& point : points) {
    ASSERT_TRUE(QMetaObject::invokeMethod(&curve, "dataSequencerPointReceived", Qt::DirectConnection, Q_ARG(QPointF, point)));
  }
}

QVector<QPointF> curvePoints(const rqt_multiplot::CurveData& data) {
  QVector<QPointF> points;
  for (size_t index = 0; index < data.getNumPoints(); ++index) {
    points.append(data.getPoint(index));
  }
  return points;
}

rqt_multiplot::CurveFilterSpec movingAverageSpec(int windowSize) {
  auto spec = rqt_multiplot::defaultCurveFilterSpec(rqt_multiplot::CurveFilterType::MovingAverage);
  spec.windowSize = windowSize;
  return spec;
}

const QVector<QPointF> kFilterInput{{0.0, 1.0}, {1.0, 3.0}, {2.0, -2.0}, {3.0, 6.0}, {4.0, 0.5}};

TEST(PlotCurve, chainChangeReplaysSameAsLiveFiltering) {
  ensureApplication();

  CurveConfig liveConfig;
  configureReceiptTimeCurve(&liveConfig);
  liveConfig.getFilterChainConfig()->setFilters({movingAverageSpec(2)});
  PlotCurve live;
  live.setConfig(&liveConfig);
  live.run();
  feedPoints(live, kFilterInput);

  CurveConfig replayConfig;
  configureReceiptTimeCurve(&replayConfig);
  PlotCurve replay;
  replay.setConfig(&replayConfig);
  replay.run();
  feedPoints(replay, kFilterInput);
  replayConfig.getFilterChainConfig()->setFilters({movingAverageSpec(2)});

  EXPECT_EQ(curvePoints(*replay.getData()), curvePoints(*live.getData()));
  EXPECT_EQ(curvePoints(*replay.getRawData()), kFilterInput);
}

TEST(PlotCurve, removingChainRestoresRawPoints) {
  ensureApplication();

  CurveConfig config;
  configureReceiptTimeCurve(&config);
  config.getFilterChainConfig()->setFilters({rqt_multiplot::defaultCurveFilterSpec(rqt_multiplot::CurveFilterType::Derivative)});
  PlotCurve curve;
  curve.setConfig(&config);
  curve.run();
  feedPoints(curve, kFilterInput);
  ASSERT_EQ(curve.getData()->getNumPoints(), 4U);

  config.getFilterChainConfig()->setFilters({});

  EXPECT_EQ(curvePoints(*curve.getData()), kFilterInput);
}

TEST(PlotCurve, unitChangeReplaysNonLinearFilter) {
  ensureApplication();

  auto threshold = rqt_multiplot::defaultCurveFilterSpec(rqt_multiplot::CurveFilterType::Threshold);
  threshold.comparison = rqt_multiplot::CurveFilterComparison::Greater;
  threshold.thresholdA = 1.0;
  CurveConfig config;
  configureReceiptTimeCurve(&config);
  config.getFilterChainConfig()->setFilters({threshold});
  PlotCurve curve;
  curve.setConfig(&config);
  curve.run();
  feedPoints(curve, {{0.0, 0.5}, {1.0, 2.0}});

  config.getAxisConfig(CurveConfig::Y)->setUnitConversion(CurveAxisConfig::RadiansToDegrees);

  EXPECT_EQ(curvePoints(*curve.getData()), QVector<QPointF>({{0.0, 1.0}, {1.0, 1.0}}));
}

TEST(PlotCurve, clearResetsFilterState) {
  ensureApplication();

  CurveConfig config;
  configureReceiptTimeCurve(&config);
  config.getFilterChainConfig()->setFilters({rqt_multiplot::defaultCurveFilterSpec(rqt_multiplot::CurveFilterType::Integral)});
  PlotCurve curve;
  curve.setConfig(&config);
  curve.run();
  feedPoints(curve, {{0.0, 1.0}, {1.0, 1.0}});

  curve.clear();
  feedPoints(curve, {{5.0, 2.0}, {6.0, 2.0}});

  EXPECT_EQ(curvePoints(*curve.getData()), QVector<QPointF>({{5.0, 0.0}, {6.0, 2.0}}));
  EXPECT_EQ(curve.getRawData()->getNumPoints(), 2U);
}

TEST(PlotCurve, snapshotCurveIgnoresFilters) {
  ensureApplication();

  CurveConfig config;
  configureSnapshotCurve(&config);
  config.getFilterChainConfig()->setFilters({movingAverageSpec(2)});
  PlotCurve curve;
  curve.setConfig(&config);

  EXPECT_EQ(curve.getRawData(), curve.getData());
}

TEST(PlotCurve, overlaySourceAddsBagLabelToTitleAndFollowsRename) {
  ensureApplication();

  CurveConfig config;
  config.setTitle(QStringLiteral("speed"));
  PlotCurve curve;
  curve.setOverlaySource(1, QStringLiteral("run2"));
  curve.setConfig(&config);

  EXPECT_EQ(curve.getOverlayBag(), 1);
  EXPECT_EQ(curve.title().text(), QStringLiteral("speed [run2]"));

  config.setTitle(QStringLiteral("velocity"));
  EXPECT_EQ(curve.title().text(), QStringLiteral("velocity [run2]"));
}

TEST(PlotCurve, overlayPenStyleDependsOnBagAndUsesWeakerColor) {
  ensureApplication();

  CurveConfig config;
  PlotCurve reference;
  reference.setConfig(&config);
  PlotCurve second;
  second.setOverlaySource(1, QStringLiteral("b"));
  second.setConfig(&config);
  PlotCurve third;
  third.setOverlaySource(2, QStringLiteral("c"));
  third.setConfig(&config);

  const QColor referenceColor = reference.pen().color();
  const int fadedAlpha = snapshotFadeAlpha(referenceColor.alpha(), 1, 2);

  EXPECT_EQ(reference.pen().style(), Qt::SolidLine);
  EXPECT_EQ(second.pen().style(), Qt::DashLine);
  EXPECT_EQ(third.pen().style(), Qt::DotLine);
  EXPECT_EQ(second.pen().color().rgb(), referenceColor.rgb());
  EXPECT_EQ(third.pen().color().rgb(), referenceColor.rgb());
  EXPECT_EQ(second.pen().color().alpha(), fadedAlpha);
  EXPECT_EQ(third.pen().color().alpha(), fadedAlpha);

  config.getStyleConfig()->setPenWidth(3);
  EXPECT_EQ(second.pen().width(), 3);
  EXPECT_EQ(second.pen().color().alpha(), fadedAlpha);

  config.getColorConfig()->setType(CurveColorConfig::Custom);
  config.getColorConfig()->setCustomColor(QColor(10, 20, 30));
  EXPECT_EQ(reference.pen().color(), QColor(10, 20, 30));
  EXPECT_EQ(second.pen().color(), QColor(10, 20, 30, fadedAlpha));
  EXPECT_EQ(third.pen().color(), QColor(10, 20, 30, fadedAlpha));
}

TEST(PlotCurve, xOffsetShiftsIncomingAndStoredTimePoints) {
  ensureApplication();

  CurveConfig config;
  configureReceiptTimeCurve(&config);
  PlotCurve curve;
  curve.setConfig(&config);
  curve.setXOffset(-10.0);
  curve.run();
  feedPoints(curve, {{100.0, 1.0}, {101.0, 2.0}});

  EXPECT_EQ(curvePoints(*curve.getData()), QVector<QPointF>({{90.0, 1.0}, {91.0, 2.0}}));

  curve.setXOffset(-5.0);

  EXPECT_DOUBLE_EQ(curve.getXOffset(), -5.0);
  EXPECT_EQ(curvePoints(*curve.getData()), QVector<QPointF>({{95.0, 1.0}, {96.0, 2.0}}));
}

TEST(PlotCurve, xOffsetShiftsRawAndFilteredPoints) {
  ensureApplication();

  CurveConfig config;
  configureReceiptTimeCurve(&config);
  config.getFilterChainConfig()->setFilters({rqt_multiplot::defaultCurveFilterSpec(rqt_multiplot::CurveFilterType::Derivative)});
  PlotCurve curve;
  curve.setConfig(&config);
  curve.run();
  feedPoints(curve, {{0.0, 0.0}, {1.0, 2.0}, {2.0, 6.0}});

  curve.setXOffset(10.0);

  EXPECT_EQ(curvePoints(*curve.getRawData()), QVector<QPointF>({{10.0, 0.0}, {11.0, 2.0}, {12.0, 6.0}}));
  EXPECT_EQ(curvePoints(*curve.getData()), QVector<QPointF>({{11.0, 2.0}, {12.0, 4.0}}));
}

TEST(PlotCurve, xOffsetIsIgnoredWhenXIsNotTime) {
  ensureApplication();

  CurveConfig config;
  config.getAxisConfig(CurveConfig::X)->setField("position");
  config.getAxisConfig(CurveConfig::Y)->setField("velocity");
  PlotCurve curve;
  curve.setConfig(&config);
  curve.setXOffset(-10.0);
  curve.run();
  feedPoints(curve, {{1.0, 1.0}});

  EXPECT_EQ(curvePoints(*curve.getData()), QVector<QPointF>({{1.0, 1.0}}));
}

}  // namespace
