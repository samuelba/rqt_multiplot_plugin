#include <cstdlib>

#include <QApplication>
#include <QColor>
#include <QLabel>
#include <QLayout>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <gtest/gtest.h>

#include <rqt_multiplot/AxisTimeFormat.h>
#include <rqt_multiplot/BoundingRectangle.h>
#include <rqt_multiplot/CurveAxisConfig.h>
#include <rqt_multiplot/CurveColorConfig.h>
#include <rqt_multiplot/CurveConfig.h>
#include <rqt_multiplot/CurveData.h>
#include <rqt_multiplot/CurveValuesWidget.h>
#include <rqt_multiplot/PlotConfig.h>
#include <rqt_multiplot/PlotCurve.h>
#include <rqt_multiplot/PlotTableConfig.h>
#include <rqt_multiplot/PlotTableWidget.h>
#include <rqt_multiplot/PlotWidget.h>

namespace {

using rqt_multiplot::AxisTimeFormat;
using rqt_multiplot::BoundingRectangle;
using rqt_multiplot::CurveAxisConfig;
using rqt_multiplot::CurveColorConfig;
using rqt_multiplot::CurveConfig;
using rqt_multiplot::CurveValuesWidget;
using rqt_multiplot::PlotConfig;
using rqt_multiplot::PlotCurve;
using rqt_multiplot::PlotTableConfig;
using rqt_multiplot::PlotTableWidget;
using rqt_multiplot::PlotWidget;

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

QTreeWidget* curveValuesTree(QWidget* values) {
  if (values == nullptr) {
    return nullptr;
  }
  return values->findChild<QTreeWidget*>();
}

QTreeWidgetItem* findItem(QTreeWidget* tree, const QString& text) {
  if (tree == nullptr) {
    return nullptr;
  }
  for (int top = 0; top < tree->topLevelItemCount(); ++top) {
    QTreeWidgetItem* plotItem = tree->topLevelItem(top);
    if ((plotItem != nullptr) && (plotItem->text(0) == text)) {
      return plotItem;
    }
    if (plotItem == nullptr) {
      continue;
    }
    for (int child = 0; child < plotItem->childCount(); ++child) {
      QTreeWidgetItem* curveItem = plotItem->child(child);
      if ((curveItem != nullptr) && (curveItem->text(0) == text)) {
        return curveItem;
      }
    }
  }
  return nullptr;
}

TEST(CurveValuesWidget, groupsIndentedCurvesUnderPlotTitles) {
  ensureApplication();

  PlotTableConfig config(nullptr);
  config.getPlotConfig(0, 0)->setTitle("Payload");
  CurveConfig* pan = config.getPlotConfig(0, 0)->addCurve();
  pan->setTitle("Pan");
  CurveConfig* tilt = config.getPlotConfig(0, 0)->addCurve();
  tilt->setTitle("Tilt");
  PlotConfig* second = config.splitPlot(config.getPlotConfig(0, 0), Qt::Horizontal);
  ASSERT_NE(second, nullptr);
  second->setTitle("Plot 2");
  CurveConfig* curve1 = second->addCurve();
  curve1->setTitle("Curve 1");

  PlotTableWidget table;
  table.setConfig(&config);

  CurveValuesWidget* values = table.getCurveValuesWidget();
  ASSERT_NE(values, nullptr);
  values->refresh();

  QTreeWidget* tree = curveValuesTree(values);
  ASSERT_NE(tree, nullptr);
  EXPECT_EQ(tree->topLevelItemCount(), 2);
  QTreeWidgetItem* payload = tree->topLevelItem(0);
  ASSERT_NE(payload, nullptr);
  EXPECT_EQ(payload->text(0), QString("Payload"));
  EXPECT_TRUE(payload->icon(0).isNull());
  EXPECT_EQ(payload->childCount(), 3);
  EXPECT_EQ(payload->child(0)->text(0), QString("Pan"));
  EXPECT_EQ(payload->child(1)->text(0), QString("Tilt"));
  EXPECT_FALSE(payload->child(0)->icon(0).isNull());

  QTreeWidgetItem* plot2 = tree->topLevelItem(1);
  ASSERT_NE(plot2, nullptr);
  EXPECT_EQ(plot2->text(0), QString("Plot 2"));
  EXPECT_EQ(plot2->childCount(), 2);
  EXPECT_EQ(plot2->child(0)->text(0), QString("Curve 1"));
  EXPECT_TRUE(payload->isExpanded());
  EXPECT_TRUE(plot2->isExpanded());
}

TEST(CurveValuesWidget, addsGapAfterEachPlotCurveBlock) {
  ensureApplication();

  PlotTableConfig config(nullptr);
  config.getPlotConfig(0, 0)->setTitle("Payload");
  config.getPlotConfig(0, 0)->addCurve()->setTitle("Pan");
  config.getPlotConfig(0, 0)->addCurve()->setTitle("Tilt");
  PlotConfig* second = config.splitPlot(config.getPlotConfig(0, 0), Qt::Horizontal);
  ASSERT_NE(second, nullptr);
  second->setTitle("Plot 2");
  second->addCurve()->setTitle("Curve 1");

  PlotTableWidget table;
  table.setConfig(&config);

  CurveValuesWidget* values = table.getCurveValuesWidget();
  ASSERT_NE(values, nullptr);
  values->refresh();

  QTreeWidget* tree = curveValuesTree(values);
  ASSERT_NE(tree, nullptr);
  QTreeWidgetItem* payload = tree->topLevelItem(0);
  ASSERT_NE(payload, nullptr);
  ASSERT_EQ(payload->childCount(), 3);
  EXPECT_EQ(payload->child(0)->text(0), QString("Pan"));
  EXPECT_EQ(payload->child(1)->text(0), QString("Tilt"));
  EXPECT_TRUE(payload->child(2)->text(0).isEmpty());
  EXPECT_TRUE(payload->child(2)->icon(0).isNull());
  EXPECT_GT(payload->child(2)->sizeHint(0).height(), 0);

  QTreeWidgetItem* plot2 = tree->topLevelItem(1);
  ASSERT_NE(plot2, nullptr);
  ASSERT_EQ(plot2->childCount(), 2);
  EXPECT_EQ(plot2->child(0)->text(0), QString("Curve 1"));
  EXPECT_TRUE(plot2->child(1)->text(0).isEmpty());
}

TEST(CurveValuesWidget, expandsCurveRowsWhenSidebarIsShown) {
  ensureApplication();

  PlotTableConfig config(nullptr);
  config.getPlotConfig(0, 0)->setTitle("Payload");
  config.getPlotConfig(0, 0)->addCurve()->setTitle("Pan");

  PlotTableWidget table;
  table.setConfig(&config);

  CurveValuesWidget* values = table.getCurveValuesWidget();
  ASSERT_NE(values, nullptr);
  EXPECT_FALSE(values->isVisibleTo(&table));
  values->refresh();

  config.setSidebarVisible(true);
  table.show();
  QApplication::processEvents();

  QTreeWidget* tree = curveValuesTree(values);
  ASSERT_NE(tree, nullptr);
  ASSERT_EQ(tree->topLevelItemCount(), 1);
  QTreeWidgetItem* payload = tree->topLevelItem(0);
  ASSERT_NE(payload, nullptr);
  EXPECT_TRUE(payload->isExpanded());
  ASSERT_EQ(payload->childCount(), 2);
  EXPECT_EQ(payload->child(0)->text(0), QString("Pan"));
}

TEST(CurveValuesWidget, showsHeadingAboveFirstPlotGroup) {
  ensureApplication();

  PlotTableConfig config(nullptr);
  config.getPlotConfig(0, 0)->setTitle("Payload");
  config.getPlotConfig(0, 0)->addCurve()->setTitle("Pan");

  PlotTableWidget table;
  table.setConfig(&config);

  CurveValuesWidget* values = table.getCurveValuesWidget();
  ASSERT_NE(values, nullptr);

  auto* heading = values->findChild<QLabel*>(QStringLiteral("curveValuesHeading"));
  ASSERT_NE(heading, nullptr);
  EXPECT_EQ(heading->text(), QString("Curve values"));
  ASSERT_NE(values->layout(), nullptr);
  EXPECT_GE(values->layout()->contentsMargins().top(), 8);
  EXPECT_GE(values->layout()->contentsMargins().left(), 8);
  EXPECT_GE(values->layout()->spacing(), 4);
}

TEST(CurveValuesWidget, rightAlignsTimeAndValueColumns) {
  ensureApplication();

  PlotTableConfig config(nullptr);
  config.getPlotConfig(0, 0)->addCurve()->setTitle("Pan");

  PlotTableWidget table;
  table.setConfig(&config);
  CurveValuesWidget* values = table.getCurveValuesWidget();
  ASSERT_NE(values, nullptr);
  values->refresh();

  QTreeWidgetItem* curve = findItem(curveValuesTree(values), QStringLiteral("Pan"));
  ASSERT_NE(curve, nullptr);
  EXPECT_TRUE(curve->textAlignment(1) & Qt::AlignRight);
  EXPECT_TRUE(curve->textAlignment(2) & Qt::AlignRight);
}

TEST(CurveValuesWidget, leavesValuesBlankUntilPointsExist) {
  ensureApplication();

  PlotTableConfig config(nullptr);
  config.getPlotConfig(0, 0)->addCurve()->setTitle("Pan");

  PlotTableWidget table;
  table.setConfig(&config);
  CurveValuesWidget* values = table.getCurveValuesWidget();
  ASSERT_NE(values, nullptr);
  values->refresh();

  QTreeWidgetItem* curve = findItem(curveValuesTree(values), QStringLiteral("Pan"));
  ASSERT_NE(curve, nullptr);
  EXPECT_TRUE(curve->text(1).isEmpty());
  EXPECT_TRUE(curve->text(2).isEmpty());
}

TEST(CurveValuesWidget, showsLatestPointAfterAppend) {
  ensureApplication();

  PlotTableConfig config(nullptr);
  config.getPlotConfig(0, 0)->addCurve()->setTitle("Pan");

  PlotTableWidget table;
  table.setConfig(&config);
  PlotWidget* plot = table.getPlotWidgets().front();
  ASSERT_NE(plot, nullptr);
  ASSERT_EQ(plot->getCurves().count(), 1);
  plot->getCurves().front()->getData()->appendPoint(QPointF(1.5, -0.25));

  CurveValuesWidget* values = table.getCurveValuesWidget();
  ASSERT_NE(values, nullptr);
  values->refresh();

  QTreeWidgetItem* curve = findItem(curveValuesTree(values), QStringLiteral("Pan"));
  ASSERT_NE(curve, nullptr);
  EXPECT_FALSE(curve->text(1).isEmpty());
  EXPECT_FALSE(curve->text(2).isEmpty());
  EXPECT_TRUE(curve->text(2).contains(QLatin1Char('-')) || curve->text(2).contains(QStringLiteral("0.25")) ||
              curve->text(2).contains(QStringLiteral("-0.25")));
}

TEST(CurveValuesWidget, formatsDateTimeAndStartFromZeroX) {
  ensureApplication();

  PlotTableConfig config(nullptr);
  CurveConfig* curveConfig = config.getPlotConfig(0, 0)->addCurve();
  curveConfig->setTitle("Pan");
  curveConfig->getAxisConfig(CurveConfig::X)->setFieldType(CurveAxisConfig::MessageReceiptTime);

  PlotTableWidget table;
  table.setConfig(&config);
  PlotWidget* plot = table.getPlotWidgets().front();
  ASSERT_NE(plot, nullptr);
  plot->getCurves().front()->getData()->appendPoint(QPointF(1789028000.0, 1.0));
  plot->getCurves().front()->getData()->appendPoint(QPointF(1789028001.0, 2.0));

  config.setTimeAxisFormat(PlotTableConfig::DateTime);
  CurveValuesWidget* values = table.getCurveValuesWidget();
  ASSERT_NE(values, nullptr);
  values->refresh();

  QTreeWidgetItem* curve = findItem(curveValuesTree(values), QStringLiteral("Pan"));
  ASSERT_NE(curve, nullptr);
  EXPECT_EQ(curve->text(1), AxisTimeFormat::coordinate(1789028001.0, 0.0, 1.0, AxisTimeFormat::LabelMode::DateTime, plot->getTimeZone()));

  config.setTimeAxisFormat(PlotTableConfig::StartFromZero);
  plot->forceReplot();
  values->refresh();
  curve = findItem(curveValuesTree(values), QStringLiteral("Pan"));
  ASSERT_NE(curve, nullptr);
  EXPECT_EQ(curve->text(1),
            AxisTimeFormat::coordinate(1789028001.0, 1789028000.0, 1.0, AxisTimeFormat::LabelMode::Relative, plot->getTimeZone()));
}

TEST(CurveValuesWidget, keepsFractionalRelativeTimeOnWideWindow) {
  ensureApplication();

  PlotTableConfig config(nullptr);
  CurveConfig* curveConfig = config.getPlotConfig(0, 0)->addCurve();
  curveConfig->setTitle("Pan");
  curveConfig->getAxisConfig(CurveConfig::X)->setFieldType(CurveAxisConfig::MessageReceiptTime);

  PlotTableWidget table;
  table.setConfig(&config);
  PlotWidget* plot = table.getPlotWidgets().front();
  ASSERT_NE(plot, nullptr);

  const double t0 = 1000.0;
  plot->getCurves().front()->getData()->appendPoint(QPointF(t0, 1.0));
  plot->getCurves().front()->getData()->appendPoint(QPointF(t0 + 29.3, 2.0));
  plot->setCurrentScale(BoundingRectangle(QPointF(t0, 0.0), QPointF(t0 + 30.0, 3.0)));
  config.setTimeAxisFormat(PlotTableConfig::Timestamp);
  config.setTimeAxisFormat(PlotTableConfig::StartFromZero);
  plot->forceReplot();

  CurveValuesWidget* values = table.getCurveValuesWidget();
  ASSERT_NE(values, nullptr);
  values->refresh();

  QTreeWidgetItem* curve = findItem(curveValuesTree(values), QStringLiteral("Pan"));
  ASSERT_NE(curve, nullptr);
  EXPECT_EQ(curve->text(1), QStringLiteral("29.3"));
}

TEST(CurveValuesWidget, updatesColorSwatchOnCurrentColorChanged) {
  ensureApplication();

  PlotTableConfig config(nullptr);
  CurveConfig* curveConfig = config.getPlotConfig(0, 0)->addCurve();
  curveConfig->setTitle("Pan");
  curveConfig->getColorConfig()->setType(CurveColorConfig::Custom);
  curveConfig->getColorConfig()->setCustomColor(QColor(255, 0, 0));

  PlotTableWidget table;
  table.setConfig(&config);
  CurveValuesWidget* values = table.getCurveValuesWidget();
  ASSERT_NE(values, nullptr);
  values->refresh();

  QTreeWidgetItem* curve = findItem(curveValuesTree(values), QStringLiteral("Pan"));
  ASSERT_NE(curve, nullptr);
  const QRgb before = curve->icon(0).pixmap(10, 10).toImage().pixel(5, 5);

  curveConfig->getColorConfig()->setCustomColor(QColor(0, 0, 255));
  curve = findItem(curveValuesTree(values), QStringLiteral("Pan"));
  ASSERT_NE(curve, nullptr);
  const QRgb after = curve->icon(0).pixmap(10, 10).toImage().pixel(5, 5);
  EXPECT_NE(before, after);
}

TEST(CurveValuesWidget, hiddenSidebarDoesNotRunRefreshTimer) {
  ensureApplication();

  PlotTableConfig config(nullptr);
  PlotTableWidget table;
  table.setConfig(&config);

  CurveValuesWidget* values = table.getCurveValuesWidget();
  ASSERT_NE(values, nullptr);
  EXPECT_FALSE(values->hasLiveUpdates());

  values->setLiveUpdates(true);
  EXPECT_TRUE(values->hasLiveUpdates());
  values->setLiveUpdates(false);
  EXPECT_FALSE(values->hasLiveUpdates());
}

}  // namespace
