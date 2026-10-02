#include <QApplication>
#include <QCheckBox>

#include <gtest/gtest.h>
#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_grid.h>
#include <qwt/qwt_plot_item.h>
#include <qwt/qwt_scale_div.h>
#include <qwt/qwt_scale_engine.h>

#include "rqt_multiplot/BoundingRectangle.hpp"
#include "rqt_multiplot/CurveAxisConfig.hpp"
#include "rqt_multiplot/CurveData.hpp"
#include "rqt_multiplot/OffsetScaleEngine.hpp"
#include "rqt_multiplot/PlotAxisConfigWidget.hpp"
#include "rqt_multiplot/PlotConfig.hpp"
#include "rqt_multiplot/PlotCurve.hpp"
#include "rqt_multiplot/PlotTableConfig.hpp"
#include "rqt_multiplot/PlotTableWidget.hpp"
#include "rqt_multiplot/PlotWidget.hpp"

namespace {

using rqt_multiplot::BoundingRectangle;
using rqt_multiplot::CurveAxisConfig;
using rqt_multiplot::CurveConfig;
using rqt_multiplot::OffsetScaleEngine;
using rqt_multiplot::PlotAxesConfig;
using rqt_multiplot::PlotAxisConfig;
using rqt_multiplot::PlotAxisConfigWidget;
using rqt_multiplot::PlotConfig;
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

QwtPlot* plotOf(PlotWidget& widget) {
  return widget.findChild<QwtPlot*>();
}

QwtPlotGrid* gridOf(QwtPlot& plot) {
  const QwtPlotItemList items = plot.itemList(QwtPlotItem::Rtti_PlotGrid);
  if (items.isEmpty()) {
    return nullptr;
  }
  return static_cast<QwtPlotGrid*>(items.first());
}

TEST(PlotAxisConfigWidget, logCheckboxUpdatesConfig) {
  ensureApplication();
  PlotAxisConfig config;
  PlotAxisConfigWidget widget;
  widget.setConfig(&config);

  auto* box = widget.findChild<QCheckBox*>("checkBoxLogScale");
  ASSERT_NE(box, nullptr);
  EXPECT_FALSE(box->isChecked());

  box->setChecked(true);
  EXPECT_TRUE(config.isLogScale());

  config.setLogScale(false);
  EXPECT_FALSE(box->isChecked());
}

TEST(PlotWidget, timeAxisIgnoresLogScale) {
  ensureApplication();
  PlotConfig config;
  config.addCurve()->getAxisConfig(CurveConfig::X)->setFieldType(CurveAxisConfig::MessageReceiptTime);
  PlotWidget widget;
  widget.setConfig(&config);
  config.getAxesConfig()->getAxisConfig(PlotAxesConfig::X)->setLogScale(true);
  config.getAxesConfig()->getAxisConfig(PlotAxesConfig::Y)->setLogScale(true);

  QwtPlot* plot = plotOf(widget);
  ASSERT_NE(plot, nullptr);
  EXPECT_NE(dynamic_cast<OffsetScaleEngine*>(plot->axisScaleEngine(QwtPlot::xBottom)), nullptr);
  EXPECT_EQ(dynamic_cast<QwtLogScaleEngine*>(plot->axisScaleEngine(QwtPlot::xBottom)), nullptr);
  EXPECT_NE(dynamic_cast<QwtLogScaleEngine*>(plot->axisScaleEngine(QwtPlot::yLeft)), nullptr);
  EXPECT_FALSE(widget.isXLogScale());
  EXPECT_TRUE(widget.isYLogScale());

  QwtPlotGrid* grid = gridOf(*plot);
  ASSERT_NE(grid, nullptr);
  EXPECT_FALSE(grid->xMinEnabled());
  EXPECT_TRUE(grid->yMinEnabled());
}

TEST(PlotWidget, logYPreferredMinimumUsesSmallestPositiveValue) {
  ensureApplication();
  PlotConfig config;
  config.addCurve();
  config.getAxesConfig()->getAxisConfig(PlotAxesConfig::Y)->setLogScale(true);
  PlotWidget widget;
  widget.setConfig(&config);
  widget.getCurves().front()->getData()->appendPoint(QPointF(1.0, 0.0));
  widget.getCurves().front()->getData()->appendPoint(QPointF(2.0, 1e-3));
  widget.getCurves().front()->getData()->appendPoint(QPointF(3.0, 10.0));

  const BoundingRectangle bounds = widget.getPreferredScale();
  EXPECT_DOUBLE_EQ(bounds.getMinimum().y(), 1e-3);
  EXPECT_DOUBLE_EQ(bounds.getMaximum().y(), 10.0);

  QwtPlot* plot = plotOf(widget);
  ASSERT_NE(plot, nullptr);
  EXPECT_NE(dynamic_cast<QwtLogScaleEngine*>(plot->axisScaleEngine(QwtPlot::yLeft)), nullptr);
  EXPECT_NE(dynamic_cast<OffsetScaleEngine*>(plot->axisScaleEngine(QwtPlot::xBottom)), nullptr);
}

TEST(PlotWidget, logYWithoutPositiveValuesFallsBackToUnitDecade) {
  ensureApplication();
  PlotConfig config;
  config.addCurve();
  config.getAxesConfig()->getAxisConfig(PlotAxesConfig::Y)->setLogScale(true);
  PlotWidget widget;
  widget.setConfig(&config);
  widget.getCurves().front()->getData()->appendPoint(QPointF(1.0, 0.0));
  widget.getCurves().front()->getData()->appendPoint(QPointF(2.0, -5.0));

  const BoundingRectangle bounds = widget.getPreferredScale();
  EXPECT_DOUBLE_EQ(bounds.getMinimum().y(), 1.0);
  EXPECT_DOUBLE_EQ(bounds.getMaximum().y(), 10.0);
  EXPECT_DOUBLE_EQ(bounds.getMinimum().x(), 1.0);
  EXPECT_DOUBLE_EQ(bounds.getMaximum().x(), 2.0);
}

TEST(PlotWidget, logAxisGridFollowsEveryTick) {
  ensureApplication();
  PlotConfig config;
  PlotWidget widget;
  widget.setConfig(&config);

  QwtPlot* plot = plotOf(widget);
  ASSERT_NE(plot, nullptr);
  QwtPlotGrid* grid = gridOf(*plot);
  ASSERT_NE(grid, nullptr);
  EXPECT_FALSE(grid->xMinEnabled());
  EXPECT_FALSE(grid->yMinEnabled());

  config.getAxesConfig()->getAxisConfig(PlotAxesConfig::Y)->setLogScale(true);
  EXPECT_FALSE(grid->xMinEnabled());
  EXPECT_TRUE(grid->yMinEnabled());

  config.getAxesConfig()->getAxisConfig(PlotAxesConfig::X)->setLogScale(true);
  EXPECT_TRUE(grid->xMinEnabled());
  EXPECT_TRUE(grid->yMinEnabled());

  config.getAxesConfig()->getAxisConfig(PlotAxesConfig::Y)->setLogScale(false);
  EXPECT_TRUE(grid->xMinEnabled());
  EXPECT_FALSE(grid->yMinEnabled());

  widget.setCurrentScale(BoundingRectangle(QPointF(1.0, 1.0), QPointF(10.0, 10.0)));
  const QList<double> minorTicks = plot->axisScaleDiv(QwtPlot::xBottom).ticks(QwtScaleDiv::MinorTick);
  EXPECT_FALSE(minorTicks.isEmpty());
}

TEST(PlotTableWidget, linkedScaleLeavesLogPlotAlone) {
  ensureApplication();
  PlotTableConfig config(nullptr);
  config.setLinkScale(true);
  PlotTableWidget table;
  table.resize(800, 600);
  table.setConfig(&config);
  table.show();
  config.splitPlot(table.getPlotWidgets().front()->getConfig(), Qt::Horizontal);
  ASSERT_EQ(table.getNumPlots(), 2u);

  PlotWidget* linear = table.getPlotWidgets().at(0);
  PlotWidget* logarithmic = table.getPlotWidgets().at(1);
  logarithmic->getConfig()->getAxesConfig()->getAxisConfig(PlotAxesConfig::Y)->setLogScale(true);
  ASSERT_TRUE(logarithmic->usesLogScale());
  ASSERT_FALSE(linear->usesLogScale());

  const BoundingRectangle logBefore = logarithmic->getCurrentScale();
  linear->setCurrentScale(BoundingRectangle(QPointF(0.0, -20.0), QPointF(10.0, 80.0)));
  const BoundingRectangle logAfter = logarithmic->getCurrentScale();
  EXPECT_DOUBLE_EQ(logAfter.getMinimum().x(), logBefore.getMinimum().x());
  EXPECT_DOUBLE_EQ(logAfter.getMinimum().y(), logBefore.getMinimum().y());
  EXPECT_DOUBLE_EQ(logAfter.getMaximum().x(), logBefore.getMaximum().x());
  EXPECT_DOUBLE_EQ(logAfter.getMaximum().y(), logBefore.getMaximum().y());
  EXPECT_DOUBLE_EQ(linear->getCurrentScale().getMaximum().y(), 80.0);

  const BoundingRectangle linearBefore = linear->getCurrentScale();
  logarithmic->setCurrentScale(BoundingRectangle(QPointF(2.0, 2.0), QPointF(50.0, 50.0)));
  EXPECT_NE(logarithmic->getCurrentScale().getMaximum().y(), logAfter.getMaximum().y());
  EXPECT_DOUBLE_EQ(linear->getCurrentScale().getMinimum().y(), linearBefore.getMinimum().y());
  EXPECT_DOUBLE_EQ(linear->getCurrentScale().getMaximum().y(), linearBefore.getMaximum().y());
}

}  // namespace
