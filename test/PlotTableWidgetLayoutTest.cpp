#include <cstdlib>

#include <QApplication>
#include <QList>
#include <QSplitter>

#include <gtest/gtest.h>

#include <rqt_multiplot/PlotConfig.h>
#include <rqt_multiplot/PlotLayoutConfig.h>
#include <rqt_multiplot/PlotTableConfig.h>
#include <rqt_multiplot/PlotTableWidget.h>
#include <rqt_multiplot/PlotWidget.h>

namespace {

using rqt_multiplot::PlotConfig;
using rqt_multiplot::PlotLayoutConfig;
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

TEST(PlotTableWidget, splitAndCloseUpdateWidgets) {
  ensureApplication();

  PlotTableConfig config(nullptr);
  PlotTableWidget widget;
  widget.resize(800, 600);
  widget.setConfig(&config);
  widget.show();

  ASSERT_EQ(widget.getNumPlots(), 1u);
  PlotWidget* first = widget.getPlotWidgets().front();
  ASSERT_NE(first, nullptr);
  EXPECT_FALSE(first->canClose());
  EXPECT_FALSE(first->canChangeState());

  PlotConfig* added = config.splitPlot(first->getConfig(), Qt::Horizontal);
  ASSERT_NE(added, nullptr);
  ASSERT_EQ(widget.getNumPlots(), 2u);
  EXPECT_EQ(widget.getPlotWidgets().front(), first);
  ASSERT_NE(widget.findChild<QSplitter*>(), nullptr);
  EXPECT_TRUE(widget.getPlotWidgets().front()->canClose());
  EXPECT_TRUE(widget.getPlotWidgets().front()->canChangeState());

  EXPECT_TRUE(config.closePlot(added));
  ASSERT_EQ(widget.getNumPlots(), 1u);
  EXPECT_FALSE(widget.getPlotWidgets().front()->canClose());
}

TEST(PlotTableWidget, splitterDragStoresReducedRatios) {
  ensureApplication();

  PlotTableConfig config(nullptr);
  PlotTableWidget widget;
  widget.resize(800, 600);
  widget.setConfig(&config);
  widget.show();

  config.splitPlot(config.getPlotConfig(0, 0), Qt::Horizontal);
  ASSERT_EQ(widget.getNumPlots(), 2u);

  PlotLayoutConfig* root = config.getLayout();
  ASSERT_NE(root, nullptr);
  root->setStretch({400, 200});
  EXPECT_EQ(root->getStretch(), (QList<int>{2, 1}));

  auto* splitter = widget.findChild<QSplitter*>();
  ASSERT_NE(splitter, nullptr);
  splitter->resize(600, 400);
  QApplication::processEvents();
  splitter->setSizes({400, 200});
  QApplication::processEvents();
  widget.storeSplitterRatios();

  const QList<int> stretch = root->getStretch();
  ASSERT_EQ(stretch.count(), 2);
  ASSERT_GT(stretch.at(1), 0);
  const QList<int> sizes = splitter->sizes();
  if ((sizes.count() == 2) && (sizes.at(0) > 0) && (sizes.at(1) > 0)) {
    EXPECT_NEAR(static_cast<double>(stretch.at(0)) / static_cast<double>(stretch.at(1)),
                static_cast<double>(sizes.at(0)) / static_cast<double>(sizes.at(1)), 0.25);
  } else {
    EXPECT_EQ(stretch, (QList<int>{2, 1}));
  }
}

TEST(PlotTableWidget, nestedSplitCreatesVerticalChild) {
  ensureApplication();

  PlotTableConfig config(nullptr);
  PlotTableWidget widget;
  widget.setConfig(&config);

  PlotConfig* left = config.getPlotConfig(0, 0);
  PlotConfig* right = config.splitPlot(left, Qt::Horizontal);
  config.splitPlot(right, Qt::Vertical);

  EXPECT_EQ(widget.getNumPlots(), 3u);
  EXPECT_EQ(config.getLayout()->getType(), PlotLayoutConfig::Horizontal);
  EXPECT_EQ(config.getLayout()->getChildren().at(1)->getType(), PlotLayoutConfig::Vertical);
}

}  // namespace
