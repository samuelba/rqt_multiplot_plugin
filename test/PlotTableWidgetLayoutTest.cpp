#include <cstdlib>

#include <QApplication>
#include <QEventLoop>
#include <QList>
#include <QSettings>
#include <QSplitter>
#include <QTemporaryDir>
#include <QTimer>

#include <gtest/gtest.h>

#include "rqt_multiplot/CurveValuesWidget.hpp"
#include "rqt_multiplot/PlotConfig.hpp"
#include "rqt_multiplot/PlotLayoutConfig.hpp"
#include "rqt_multiplot/PlotTableConfig.hpp"
#include "rqt_multiplot/PlotTableWidget.hpp"
#include "rqt_multiplot/PlotWidget.hpp"

namespace {

using rqt_multiplot::CurveValuesWidget;
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

void waitForLayout() {
  QApplication::processEvents();
  QEventLoop loop;
  QTimer::singleShot(0, &loop, &QEventLoop::quit);
  loop.exec();
  QApplication::processEvents();
}

QSplitter* rootSplitter(QWidget& widget) {
  const QList<QSplitter*> splitters = widget.findChildren<QSplitter*>();
  for (QSplitter* splitter : splitters) {
    if (splitter->objectName() == QLatin1String("curveValuesSplitter")) {
      continue;
    }
    if (qobject_cast<QSplitter*>(splitter->parentWidget()) == nullptr) {
      return splitter;
    }
  }
  return nullptr;
}

double sizeRatio(const QList<int>& sizes) {
  return static_cast<double>(sizes.at(0)) / static_cast<double>(sizes.at(1));
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
  ASSERT_GT(widget.findChildren<QSplitter*>().count(), 1);
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

  auto* splitter = rootSplitter(widget);
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

TEST(PlotTableWidget, restoresNestedSplitterRatiosWhenAlreadyVisible) {
  ensureApplication();

  PlotTableConfig saved(nullptr);
  PlotConfig* left = saved.getPlotConfig(0, 0);
  PlotConfig* right = saved.splitPlot(left, Qt::Horizontal);
  saved.splitPlot(right, Qt::Vertical);
  saved.getLayout()->setStretch({3, 1});
  saved.getLayout()->getChildren().at(1)->setStretch({2, 1});

  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  QSettings settings(dir.filePath("layout.ini"), QSettings::IniFormat);
  saved.save(settings);
  settings.sync();

  PlotTableConfig config(nullptr);
  PlotTableWidget widget;
  widget.resize(800, 600);
  widget.setConfig(&config);
  widget.show();
  waitForLayout();

  config.load(settings);
  waitForLayout();

  QSplitter* root = rootSplitter(widget);
  ASSERT_NE(root, nullptr);
  EXPECT_EQ(root->orientation(), Qt::Horizontal);
  const QList<int> rootSizes = root->sizes();
  ASSERT_EQ(rootSizes.count(), 2);
  ASSERT_GT(rootSizes.at(1), 0);
  EXPECT_NEAR(sizeRatio(rootSizes), 3.0, 0.2);

  auto* nested = root->findChild<QSplitter*>();
  ASSERT_NE(nested, nullptr);
  const QList<int> nestedSizes = nested->sizes();
  ASSERT_EQ(nestedSizes.count(), 2);
  ASSERT_GT(nestedSizes.at(1), 0);
  EXPECT_NEAR(sizeRatio(nestedSizes), 2.0, 0.2);
}

TEST(PlotTableWidget, orthogonalSplitKeepsOtherPlotSize) {
  ensureApplication();

  PlotTableConfig config(nullptr);
  PlotConfig* left = config.getPlotConfig(0, 0);
  PlotConfig* right = config.splitPlot(left, Qt::Horizontal);
  config.getLayout()->setStretch({3, 1});

  PlotTableWidget widget;
  widget.resize(800, 600);
  widget.setConfig(&config);
  widget.show();
  waitForLayout();

  QSplitter* root = rootSplitter(widget);
  ASSERT_NE(root, nullptr);
  const QList<int> before = root->sizes();
  ASSERT_EQ(before.count(), 2);
  ASSERT_GT(before.at(1), 0);
  EXPECT_NEAR(sizeRatio(before), 3.0, 0.2);

  config.splitPlot(right, Qt::Vertical);
  waitForLayout();

  root = rootSplitter(widget);
  ASSERT_NE(root, nullptr);
  const QList<int> after = root->sizes();
  ASSERT_EQ(after.count(), 2);
  ASSERT_GT(after.at(1), 0);
  EXPECT_NEAR(sizeRatio(after), 3.0, 0.2);
  EXPECT_NEAR(static_cast<double>(after.at(0)), static_cast<double>(before.at(0)), 40.0);
}

TEST(PlotTableWidget, sameOrientationSplitOnlyHalvesTarget) {
  ensureApplication();

  PlotTableConfig config(nullptr);
  PlotConfig* left = config.getPlotConfig(0, 0);
  PlotConfig* right = config.splitPlot(left, Qt::Horizontal);
  config.getLayout()->setStretch({3, 1});

  PlotTableWidget widget;
  widget.resize(800, 600);
  widget.setConfig(&config);
  widget.show();
  waitForLayout();

  config.splitPlot(right, Qt::Horizontal);
  waitForLayout();

  EXPECT_EQ(config.getLayout()->getStretch(), (QList<int>{6, 1, 1}));

  QSplitter* root = rootSplitter(widget);
  ASSERT_NE(root, nullptr);
  const QList<int> sizes = root->sizes();
  ASSERT_EQ(sizes.count(), 3);
  ASSERT_GT(sizes.at(2), 0);
  const int total = sizes.at(0) + sizes.at(1) + sizes.at(2);
  EXPECT_NEAR(static_cast<double>(sizes.at(0)) / static_cast<double>(total), 0.75, 0.05);
  EXPECT_NEAR(static_cast<double>(sizes.at(1)), static_cast<double>(sizes.at(2)), 20.0);
}

TEST(PlotTableWidget, closePlotOnlyExpandsSplitSibling) {
  ensureApplication();

  PlotTableConfig config(nullptr);
  PlotConfig* left = config.getPlotConfig(0, 0);
  PlotConfig* middle = config.splitPlot(left, Qt::Horizontal);
  PlotConfig* right = config.splitPlot(middle, Qt::Horizontal);
  config.getLayout()->setStretch({6, 1, 1});

  PlotTableWidget widget;
  widget.resize(800, 600);
  widget.setConfig(&config);
  widget.show();
  waitForLayout();

  EXPECT_TRUE(config.closePlot(right));
  waitForLayout();

  EXPECT_EQ(config.getLayout()->getStretch(), (QList<int>{3, 1}));

  QSplitter* root = rootSplitter(widget);
  ASSERT_NE(root, nullptr);
  const QList<int> sizes = root->sizes();
  ASSERT_EQ(sizes.count(), 2);
  ASSERT_GT(sizes.at(1), 0);
  EXPECT_NEAR(sizeRatio(sizes), 3.0, 0.2);
}

TEST(PlotTableWidget, closeInsertedBeforePlotExpandsOriginNotPrevious) {
  ensureApplication();

  PlotTableConfig config(nullptr);
  PlotConfig* first = config.getPlotConfig(0, 0);
  PlotConfig* second = config.splitPlot(first, Qt::Horizontal);
  config.getLayout()->setStretch({3, 1});
  PlotConfig* added = config.splitPlot(second, Qt::Horizontal, true);
  ASSERT_NE(added, nullptr);

  PlotTableWidget widget;
  widget.resize(800, 600);
  widget.setConfig(&config);
  widget.show();
  waitForLayout();

  EXPECT_TRUE(config.closePlot(added));
  waitForLayout();

  EXPECT_EQ(config.getLayout()->getStretch(), (QList<int>{3, 1}));

  QSplitter* root = rootSplitter(widget);
  ASSERT_NE(root, nullptr);
  const QList<int> sizes = root->sizes();
  ASSERT_EQ(sizes.count(), 2);
  ASSERT_GT(sizes.at(1), 0);
  EXPECT_NEAR(sizeRatio(sizes), 3.0, 0.2);
}

TEST(PlotTableWidget, closeInsertedAbovePlotExpandsOriginNotPrevious) {
  ensureApplication();

  PlotTableConfig config(nullptr);
  PlotConfig* first = config.getPlotConfig(0, 0);
  PlotConfig* second = config.splitPlot(first, Qt::Vertical);
  config.getLayout()->setStretch({3, 1});
  PlotConfig* added = config.splitPlot(second, Qt::Vertical, true);
  ASSERT_NE(added, nullptr);

  PlotTableWidget widget;
  widget.resize(800, 600);
  widget.setConfig(&config);
  widget.show();
  waitForLayout();

  EXPECT_TRUE(config.closePlot(added));
  waitForLayout();

  EXPECT_EQ(config.getLayout()->getStretch(), (QList<int>{3, 1}));

  QSplitter* root = rootSplitter(widget);
  ASSERT_NE(root, nullptr);
  EXPECT_EQ(root->orientation(), Qt::Vertical);
  const QList<int> sizes = root->sizes();
  ASSERT_EQ(sizes.count(), 2);
  ASSERT_GT(sizes.at(1), 0);
  EXPECT_NEAR(sizeRatio(sizes), 3.0, 0.2);
}

TEST(PlotTableWidget, closeNestedPlotKeepsOtherPlotSize) {
  ensureApplication();

  PlotTableConfig config(nullptr);
  PlotConfig* left = config.getPlotConfig(0, 0);
  PlotConfig* right = config.splitPlot(left, Qt::Horizontal);
  PlotConfig* bottomRight = config.splitPlot(right, Qt::Vertical);
  config.getLayout()->setStretch({3, 1});

  PlotTableWidget widget;
  widget.resize(800, 600);
  widget.setConfig(&config);
  widget.show();
  waitForLayout();

  QSplitter* root = rootSplitter(widget);
  ASSERT_NE(root, nullptr);
  const QList<int> before = root->sizes();
  ASSERT_EQ(before.count(), 2);
  ASSERT_GT(before.at(1), 0);
  EXPECT_NEAR(sizeRatio(before), 3.0, 0.2);

  EXPECT_TRUE(config.closePlot(bottomRight));
  waitForLayout();

  EXPECT_EQ(config.getLayout()->getStretch(), (QList<int>{3, 1}));
  root = rootSplitter(widget);
  ASSERT_NE(root, nullptr);
  const QList<int> after = root->sizes();
  ASSERT_EQ(after.count(), 2);
  ASSERT_GT(after.at(1), 0);
  EXPECT_NEAR(sizeRatio(after), 3.0, 0.2);
  EXPECT_NEAR(static_cast<double>(after.at(0)), static_cast<double>(before.at(0)), 40.0);
}

TEST(PlotTableWidget, resetEvenDistributionEqualizesSplitters) {
  ensureApplication();

  PlotTableConfig config(nullptr);
  PlotConfig* left = config.getPlotConfig(0, 0);
  config.splitPlot(left, Qt::Horizontal);
  config.getLayout()->setStretch({3, 1});

  PlotTableWidget widget;
  widget.resize(800, 600);
  widget.setConfig(&config);
  widget.show();
  waitForLayout();

  widget.resetEvenDistribution();
  waitForLayout();

  EXPECT_EQ(config.getLayout()->getStretch(), (QList<int>{1, 1}));

  QSplitter* root = rootSplitter(widget);
  ASSERT_NE(root, nullptr);
  const QList<int> sizes = root->sizes();
  ASSERT_EQ(sizes.count(), 2);
  ASSERT_GT(sizes.at(1), 0);
  EXPECT_NEAR(sizeRatio(sizes), 1.0, 0.15);
}

TEST(PlotTableWidget, resetEvenDistributionEqualizesNestedSplitters) {
  ensureApplication();

  PlotTableConfig config(nullptr);
  PlotConfig* left = config.getPlotConfig(0, 0);
  PlotConfig* right = config.splitPlot(left, Qt::Horizontal);
  config.getLayout()->setStretch({3, 1});
  config.splitPlot(right, Qt::Vertical);
  config.getLayout()->getChildren().at(1)->setStretch({2, 1});

  PlotTableWidget widget;
  widget.resize(800, 600);
  widget.setConfig(&config);
  widget.show();
  waitForLayout();

  widget.resetEvenDistribution();
  waitForLayout();

  EXPECT_EQ(config.getLayout()->getStretch(), (QList<int>{1, 1}));
  EXPECT_EQ(config.getLayout()->getChildren().at(1)->getStretch(), (QList<int>{1, 1}));

  QSplitter* root = rootSplitter(widget);
  ASSERT_NE(root, nullptr);
  const QList<int> rootSizes = root->sizes();
  ASSERT_EQ(rootSizes.count(), 2);
  ASSERT_GT(rootSizes.at(1), 0);
  EXPECT_NEAR(sizeRatio(rootSizes), 1.0, 0.15);

  auto* nested = root->findChild<QSplitter*>();
  ASSERT_NE(nested, nullptr);
  const QList<int> nestedSizes = nested->sizes();
  ASSERT_EQ(nestedSizes.count(), 2);
  ASSERT_GT(nestedSizes.at(1), 0);
  EXPECT_NEAR(sizeRatio(nestedSizes), 1.0, 0.15);
}

TEST(PlotTableWidget, hostsHiddenCurveValuesSidebar) {
  ensureApplication();

  PlotTableConfig config(nullptr);
  PlotTableWidget widget;
  widget.resize(800, 600);
  widget.setConfig(&config);
  widget.show();
  waitForLayout();

  auto* splitter = widget.findChild<QSplitter*>("curveValuesSplitter");
  ASSERT_NE(splitter, nullptr);
  ASSERT_NE(widget.getCurveValuesWidget(), nullptr);
  EXPECT_FALSE(widget.getCurveValuesWidget()->isVisibleTo(&widget));
  EXPECT_FALSE(widget.getCurveValuesWidget()->hasLiveUpdates());

  config.setSidebarVisible(true);
  waitForLayout();
  EXPECT_TRUE(widget.getCurveValuesWidget()->isVisibleTo(&widget));
  EXPECT_TRUE(widget.getCurveValuesWidget()->hasLiveUpdates());
}

TEST(PlotTableWidget, defaultPlotsHideGrid) {
  ensureApplication();

  PlotTableConfig config(nullptr);
  PlotTableWidget widget;
  widget.resize(800, 600);
  widget.setConfig(&config);
  widget.show();
  waitForLayout();

  ASSERT_EQ(widget.getPlotWidgets().count(), 1);
  EXPECT_FALSE(widget.getPlotWidgets().front()->isGridVisible());
}

TEST(PlotTableWidget, gridVisibleAppliesToExistingPlots) {
  ensureApplication();

  PlotTableConfig config(nullptr);
  PlotTableWidget widget;
  widget.resize(800, 600);
  widget.setConfig(&config);
  widget.show();
  waitForLayout();

  config.setGridVisible(true);
  waitForLayout();

  for (PlotWidget* plot : widget.getPlotWidgets()) {
    EXPECT_TRUE(plot->isGridVisible());
  }

  config.setGridVisible(false);
  waitForLayout();

  for (PlotWidget* plot : widget.getPlotWidgets()) {
    EXPECT_FALSE(plot->isGridVisible());
  }
}

TEST(PlotTableWidget, newPlotsFollowGridConfig) {
  ensureApplication();

  PlotTableConfig config(nullptr);
  config.setGridVisible(true);

  PlotTableWidget widget;
  widget.resize(800, 600);
  widget.setConfig(&config);
  widget.show();
  waitForLayout();

  PlotWidget* first = widget.getPlotWidgets().front();
  ASSERT_NE(first, nullptr);
  config.splitPlot(first->getConfig(), Qt::Horizontal);
  waitForLayout();

  ASSERT_EQ(widget.getPlotWidgets().count(), 2);
  for (PlotWidget* plot : widget.getPlotWidgets()) {
    EXPECT_TRUE(plot->isGridVisible());
  }
}

TEST(PlotTableWidget, restoresSidebarWidthFromConfig) {
  ensureApplication();

  PlotTableConfig config(nullptr);
  config.setSidebarVisible(true);
  config.setSidebarWidth(320);

  PlotTableWidget widget;
  widget.resize(800, 600);
  widget.setConfig(&config);
  widget.show();
  waitForLayout();

  auto* splitter = widget.findChild<QSplitter*>("curveValuesSplitter");
  ASSERT_NE(splitter, nullptr);
  const QList<int> sizes = splitter->sizes();
  ASSERT_GE(sizes.count(), 1);
  EXPECT_NEAR(sizes.at(0), 320, 20);
}

}  // namespace
