#include <cstdlib>

#include <QApplication>
#include <QColor>
#include <QMetaObject>
#include <QPushButton>
#include <QSettings>
#include <QTabBar>
#include <QTabWidget>
#include <QTemporaryDir>

#include <gtest/gtest.h>

#include <rqt_multiplot/MultiplotConfig.h>
#include <rqt_multiplot/PlotLayoutConfig.h>
#include <rqt_multiplot/PlotTabWidget.h>
#include <rqt_multiplot/PlotTableConfig.h>
#include <rqt_multiplot/PlotTableConfigWidget.h>
#include <rqt_multiplot/PlotTableWidget.h>
#include <rqt_multiplot/PlotWidget.h>
#include <rqt_multiplot/ProgressWidget.h>
#include <rqt_multiplot/XmlSettings.h>

namespace {

using rqt_multiplot::MultiplotConfig;
using rqt_multiplot::PlotLayoutConfig;
using rqt_multiplot::PlotTableConfig;
using rqt_multiplot::PlotTableConfigWidget;
using rqt_multiplot::PlotTableWidget;
using rqt_multiplot::PlotTabWidget;
using rqt_multiplot::PlotWidget;
using rqt_multiplot::ProgressWidget;
using rqt_multiplot::XmlSettings;

bool plotTablePaused(PlotTableWidget* plotTable) {
  if (plotTable == nullptr) {
    return true;
  }

  for (PlotWidget* plot : plotTable->getPlotWidgets()) {
    if (plot != nullptr && !plot->isPaused()) {
      return false;
    }
  }

  return true;
}

bool allPlotTablesPaused(const PlotTabWidget& tabs) {
  for (size_t index = 0; index < tabs.getNumPlotTables(); ++index) {
    if (!plotTablePaused(tabs.getPlotTable(index))) {
      return false;
    }
  }
  return true;
}

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

void startBagJob(PlotTableWidget* plotTable) {
  EXPECT_NE(plotTable, nullptr);
  EXPECT_TRUE(QMetaObject::invokeMethod(plotTable, "bagReaderReadingStarted", Qt::DirectConnection));
}

void progressBagJob(PlotTableWidget* plotTable, double progress) {
  EXPECT_NE(plotTable, nullptr);
  EXPECT_TRUE(QMetaObject::invokeMethod(plotTable, "bagReaderReadingProgressChanged", Qt::DirectConnection, Q_ARG(double, progress)));
}

void finishBagJob(PlotTableWidget* plotTable) {
  EXPECT_NE(plotTable, nullptr);
  EXPECT_TRUE(QMetaObject::invokeMethod(plotTable, "bagReaderReadingFinished", Qt::DirectConnection));
}

void failBagJob(PlotTableWidget* plotTable) {
  EXPECT_NE(plotTable, nullptr);
  EXPECT_TRUE(
      QMetaObject::invokeMethod(plotTable, "bagReaderReadingFailed", Qt::DirectConnection, Q_ARG(QString, QStringLiteral("error"))));
}

TEST(PlotTabWidget, startsWithOneTabFromConfig) {
  ensureApplication();

  MultiplotConfig config(nullptr);
  PlotTabWidget widget;
  widget.setConfig(&config);

  ASSERT_EQ(widget.getNumPlotTables(), 1u);
  EXPECT_EQ(widget.getTabText(0), QString("Tab 1"));
  ASSERT_NE(widget.getCurrentPlotTable(), nullptr);
  EXPECT_EQ(widget.getCurrentPlotTable()->getConfig(), config.getTableConfig(0));
}

TEST(PlotTabWidget, addTabCreatesIndependentPlotTable) {
  ensureApplication();

  MultiplotConfig config(nullptr);
  PlotTabWidget widget;
  widget.setConfig(&config);

  widget.addTab();

  ASSERT_EQ(widget.getNumPlotTables(), 2u);
  EXPECT_EQ(widget.getTabText(1), QString("Tab 2"));
  ASSERT_NE(widget.getPlotTable(0), nullptr);
  ASSERT_NE(widget.getPlotTable(1), nullptr);
  EXPECT_NE(widget.getPlotTable(0), widget.getPlotTable(1));
  EXPECT_EQ(widget.getCurrentPlotTable(), widget.getPlotTable(1));
  EXPECT_EQ(widget.getPlotTable(1)->getConfig(), config.getTableConfig(1));
}

TEST(PlotTabWidget, closeLastTabIsRefused) {
  ensureApplication();

  MultiplotConfig config(nullptr);
  PlotTabWidget widget;
  widget.setConfig(&config);

  widget.closeTab(0);

  ASSERT_EQ(widget.getNumPlotTables(), 1u);
  EXPECT_EQ(config.getNumTabs(), 1u);
}

TEST(PlotTabWidget, closeTabRemovesMatchingPlotTable) {
  ensureApplication();

  MultiplotConfig config(nullptr);
  PlotTabWidget widget;
  widget.setConfig(&config);
  widget.addTab();
  widget.addTab();
  config.setTabTitle(1, "Middle");

  widget.closeTab(1);

  ASSERT_EQ(widget.getNumPlotTables(), 2u);
  EXPECT_EQ(widget.getTabText(0), QString("Tab 1"));
  EXPECT_EQ(widget.getTabText(1), QString("Tab 3"));
}

TEST(PlotTabWidget, renameUpdatesTabText) {
  ensureApplication();

  MultiplotConfig config(nullptr);
  PlotTabWidget widget;
  widget.setConfig(&config);

  config.setTabTitle(0, "Motors");

  EXPECT_EQ(widget.getTabText(0), QString("Motors"));
  EXPECT_EQ(config.getTableConfig(0)->getTitle(), QString("Motors"));
}

TEST(PlotTabWidget, hidesCloseButtonOnLastTab) {
  ensureApplication();

  MultiplotConfig config(nullptr);
  PlotTabWidget widget;
  widget.setConfig(&config);

  auto* tabWidget = widget.findChild<QTabWidget*>();
  ASSERT_NE(tabWidget, nullptr);
  QWidget* closeButton = tabWidget->tabBar()->tabButton(0, QTabBar::RightSide);
  ASSERT_NE(closeButton, nullptr);
  EXPECT_FALSE(closeButton->isVisibleTo(tabWidget));

  widget.addTab();
  QWidget* firstClose = tabWidget->tabBar()->tabButton(0, QTabBar::RightSide);
  QWidget* secondClose = tabWidget->tabBar()->tabButton(1, QTabBar::RightSide);
  ASSERT_NE(firstClose, nullptr);
  ASSERT_NE(secondClose, nullptr);
  EXPECT_TRUE(firstClose->isVisibleTo(tabWidget));
  EXPECT_TRUE(secondClose->isVisibleTo(tabWidget));
}

TEST(PlotTabWidget, rebuildsTabsFromLoadedConfig) {
  ensureApplication();

  MultiplotConfig source(nullptr);
  source.getTableConfig(0)->setTitle("Left");
  source.addTab()->setTitle("Right");

  MultiplotConfig loaded(nullptr);
  loaded = source;

  PlotTabWidget widget;
  widget.setConfig(&loaded);

  ASSERT_EQ(widget.getNumPlotTables(), 2u);
  EXPECT_EQ(widget.getTabText(0), QString("Left"));
  EXPECT_EQ(widget.getTabText(1), QString("Right"));
}

TEST(PlotTabWidget, loadsLegacyTableXmlWithToolbarBound) {
  ensureApplication();

  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const QString path = dir.filePath("legacy.xml");

  {
    PlotTableConfig table(nullptr);
    table.setNumPlots(2, 1);
    table.setLinkScale(true);
    table.getPlotConfig(0, 0)->setTitle("Legacy Plot");

    QSettings settings(path, XmlSettings::format);
    settings.beginGroup("rqt_multiplot");
    settings.beginGroup("table");
    table.save(settings);
    settings.remove("title");
    settings.endGroup();
    settings.endGroup();
    settings.sync();
    ASSERT_EQ(settings.status(), QSettings::NoError);
  }

  MultiplotConfig config(nullptr);
  PlotTabWidget tabs;
  PlotTableConfigWidget toolbar;
  tabs.setConfig(&config);
  toolbar.setConfig(config.getTableConfig(0));
  toolbar.setPlotTabs(&tabs);
  toolbar.setPlotTable(tabs.getCurrentPlotTable());
  QObject::connect(&tabs, &PlotTabWidget::currentPlotTableChanged, [&toolbar](PlotTableWidget* plotTable) {
    toolbar.setConfig(plotTable != nullptr ? plotTable->getConfig() : nullptr);
    toolbar.setPlotTable(plotTable);
  });

  QSettings settings(path, XmlSettings::format);
  settings.beginGroup("rqt_multiplot");
  config.load(settings);
  settings.endGroup();

  ASSERT_EQ(config.getNumTabs(), 1u);
  ASSERT_EQ(tabs.getNumPlotTables(), 1u);
  ASSERT_NE(tabs.getCurrentPlotTable(), nullptr);
  EXPECT_EQ(toolbar.getPlotTableWidget(), tabs.getCurrentPlotTable());
  EXPECT_EQ(toolbar.getConfig(), config.getTableConfig(0));
  EXPECT_EQ(config.getTableConfig(0)->getNumRows(), 2u);
  EXPECT_EQ(config.getTableConfig(0)->getPlotConfig(0, 0)->getTitle(), QString("Legacy Plot"));
}

TEST(PlotTabWidget, runPauseAndClearAffectEveryTab) {
  ensureApplication();

  MultiplotConfig config(nullptr);
  PlotTabWidget tabs;
  tabs.setConfig(&config);
  tabs.addTab();

  ASSERT_EQ(tabs.getNumPlotTables(), 2u);
  EXPECT_TRUE(allPlotTablesPaused(tabs));

  tabs.runPlots();
  EXPECT_FALSE(plotTablePaused(tabs.getPlotTable(0)));
  EXPECT_FALSE(plotTablePaused(tabs.getPlotTable(1)));

  tabs.pausePlots();
  EXPECT_TRUE(allPlotTablesPaused(tabs));

  tabs.runPlots();
  tabs.clearPlots();
  EXPECT_FALSE(plotTablePaused(tabs.getPlotTable(0)));
  EXPECT_FALSE(plotTablePaused(tabs.getPlotTable(1)));
}

TEST(PlotTabWidget, toolbarRunPauseAndClearAffectEveryTab) {
  ensureApplication();

  MultiplotConfig config(nullptr);
  PlotTabWidget tabs;
  PlotTableConfigWidget toolbar;
  tabs.setConfig(&config);
  tabs.addTab();
  toolbar.setPlotTabs(&tabs);
  toolbar.setPlotTable(tabs.getCurrentPlotTable());

  ASSERT_EQ(tabs.getNumPlotTables(), 2u);
  ASSERT_EQ(tabs.getCurrentPlotTable(), tabs.getPlotTable(1));
  EXPECT_EQ(toolbar.getPlotTabs(), &tabs);

  auto* runButton = toolbar.findChild<QPushButton*>("pushButtonRun");
  auto* pauseButton = toolbar.findChild<QPushButton*>("pushButtonPause");
  auto* clearButton = toolbar.findChild<QPushButton*>("pushButtonClear");
  ASSERT_NE(runButton, nullptr);
  ASSERT_NE(pauseButton, nullptr);
  ASSERT_NE(clearButton, nullptr);

  EXPECT_TRUE(runButton->isEnabled());
  EXPECT_FALSE(pauseButton->isEnabled());

  runButton->click();
  EXPECT_FALSE(plotTablePaused(tabs.getPlotTable(0)));
  EXPECT_FALSE(plotTablePaused(tabs.getPlotTable(1)));
  EXPECT_FALSE(runButton->isEnabled());
  EXPECT_TRUE(pauseButton->isEnabled());

  tabs.getPlotTable(0)->pausePlots();
  EXPECT_TRUE(runButton->isEnabled());
  EXPECT_TRUE(pauseButton->isEnabled());

  pauseButton->click();
  EXPECT_TRUE(allPlotTablesPaused(tabs));
  EXPECT_TRUE(runButton->isEnabled());
  EXPECT_FALSE(pauseButton->isEnabled());

  runButton->click();
  clearButton->click();
  EXPECT_FALSE(plotTablePaused(tabs.getPlotTable(0)));
  EXPECT_FALSE(plotTablePaused(tabs.getPlotTable(1)));
}

TEST(PlotTabWidget, loadFromBagFileStartsEveryTab) {
  ensureApplication();

  MultiplotConfig config(nullptr);
  PlotTabWidget tabs;
  tabs.setConfig(&config);
  tabs.addTab();

  tabs.loadFromBagFile("/this/path/does/not/exist.mcap");

  EXPECT_EQ(tabs.getPlotTable(0)->getBagReader()->getFileName(), QString("/this/path/does/not/exist.mcap"));
  EXPECT_EQ(tabs.getPlotTable(1)->getBagReader()->getFileName(), QString("/this/path/does/not/exist.mcap"));
  EXPECT_FALSE(plotTablePaused(tabs.getPlotTable(0)));
  EXPECT_FALSE(plotTablePaused(tabs.getPlotTable(1)));
}

TEST(PlotTabWidget, progressRowHiddenWhenIdle) {
  ensureApplication();

  MultiplotConfig config(nullptr);
  PlotTabWidget tabs;
  PlotTableConfigWidget toolbar;
  tabs.setConfig(&config);
  tabs.addTab();
  toolbar.setPlotTabs(&tabs);
  toolbar.setPlotTable(tabs.getCurrentPlotTable());

  auto* progress = toolbar.findChild<ProgressWidget*>("widgetProgress");
  ASSERT_NE(progress, nullptr);
  EXPECT_TRUE(progress->isHidden());
}

TEST(PlotTabWidget, progressRowShownWhileBagJobActive) {
  ensureApplication();

  MultiplotConfig config(nullptr);
  PlotTabWidget tabs;
  tabs.setConfig(&config);
  tabs.addTab();
  PlotTableConfigWidget toolbar;
  toolbar.setPlotTabs(&tabs);
  toolbar.setPlotTable(tabs.getCurrentPlotTable());
  toolbar.show();

  auto* progress = toolbar.findChild<ProgressWidget*>("widgetProgress");
  ASSERT_NE(progress, nullptr);

  startBagJob(tabs.getPlotTable(0));
  EXPECT_FALSE(progress->isHidden());
  EXPECT_TRUE(progress->isStarted());
}

TEST(PlotTabWidget, progressRowHiddenAfterBagJobFinishes) {
  ensureApplication();

  MultiplotConfig config(nullptr);
  PlotTabWidget tabs;
  tabs.setConfig(&config);
  tabs.addTab();
  PlotTableConfigWidget toolbar;
  toolbar.setPlotTabs(&tabs);
  toolbar.setPlotTable(tabs.getCurrentPlotTable());

  auto* progress = toolbar.findChild<ProgressWidget*>("widgetProgress");
  ASSERT_NE(progress, nullptr);

  startBagJob(tabs.getPlotTable(0));
  finishBagJob(tabs.getPlotTable(0));

  EXPECT_FALSE(progress->isStarted());
  EXPECT_TRUE(progress->isHidden());
}

TEST(PlotTabWidget, progressRowHiddenAfterBagJobFails) {
  ensureApplication();

  MultiplotConfig config(nullptr);
  PlotTabWidget tabs;
  tabs.setConfig(&config);
  tabs.addTab();
  PlotTableConfigWidget toolbar;
  toolbar.setPlotTabs(&tabs);
  toolbar.setPlotTable(tabs.getCurrentPlotTable());

  auto* progress = toolbar.findChild<ProgressWidget*>("widgetProgress");
  ASSERT_NE(progress, nullptr);

  startBagJob(tabs.getPlotTable(0));
  failBagJob(tabs.getPlotTable(0));

  EXPECT_FALSE(progress->isStarted());
  EXPECT_TRUE(progress->isHidden());
}

TEST(PlotTabWidget, progressRowStaysVisibleWhileAnotherTabJobActive) {
  ensureApplication();

  MultiplotConfig config(nullptr);
  PlotTabWidget tabs;
  tabs.setConfig(&config);
  tabs.addTab();
  PlotTableConfigWidget toolbar;
  toolbar.setPlotTabs(&tabs);
  toolbar.setPlotTable(tabs.getCurrentPlotTable());
  toolbar.show();

  auto* progress = toolbar.findChild<ProgressWidget*>("widgetProgress");
  ASSERT_NE(progress, nullptr);

  startBagJob(tabs.getPlotTable(0));
  startBagJob(tabs.getPlotTable(1));
  finishBagJob(tabs.getPlotTable(0));

  EXPECT_FALSE(progress->isHidden());
  EXPECT_TRUE(progress->isStarted());
}

TEST(PlotTabWidget, closingTabWithActiveJobCompletesToolbarProgress) {
  ensureApplication();

  MultiplotConfig config(nullptr);
  PlotTabWidget tabs;
  PlotTableConfigWidget toolbar;
  tabs.setConfig(&config);
  tabs.addTab();
  toolbar.setPlotTabs(&tabs);
  toolbar.setPlotTable(tabs.getCurrentPlotTable());

  auto* progress = toolbar.findChild<ProgressWidget*>("widgetProgress");
  ASSERT_NE(progress, nullptr);

  startBagJob(tabs.getPlotTable(0));
  ASSERT_TRUE(progress->isStarted());

  tabs.closeTab(0);

  EXPECT_EQ(tabs.getNumPlotTables(), 1u);
  EXPECT_FALSE(progress->isStarted());
  EXPECT_TRUE(progress->isHidden());
}

TEST(PlotTabWidget, switchingTabsDoesNotResetActiveJobCount) {
  ensureApplication();

  MultiplotConfig config(nullptr);
  PlotTabWidget tabs;
  PlotTableConfigWidget toolbar;
  tabs.setConfig(&config);
  tabs.addTab();
  toolbar.setPlotTabs(&tabs);
  toolbar.setPlotTable(tabs.getCurrentPlotTable());
  QObject::connect(&tabs, &PlotTabWidget::currentPlotTableChanged, [&toolbar](PlotTableWidget* plotTable) {
    toolbar.setConfig(plotTable != nullptr ? plotTable->getConfig() : nullptr);
    toolbar.setPlotTable(plotTable);
  });

  auto* progress = toolbar.findChild<ProgressWidget*>("widgetProgress");
  ASSERT_NE(progress, nullptr);

  startBagJob(tabs.getPlotTable(0));
  startBagJob(tabs.getPlotTable(1));
  ASSERT_TRUE(progress->isStarted());

  config.setCurrentTabIndex(0);
  toolbar.setPlotTable(tabs.getCurrentPlotTable());
  EXPECT_TRUE(progress->isStarted());

  finishBagJob(tabs.getPlotTable(0));
  EXPECT_TRUE(progress->isStarted());

  finishBagJob(tabs.getPlotTable(1));
  EXPECT_FALSE(progress->isStarted());
  EXPECT_TRUE(progress->isHidden());
}

TEST(PlotTabWidget, aggregatesProgressAcrossActiveTabs) {
  ensureApplication();

  MultiplotConfig config(nullptr);
  PlotTabWidget tabs;
  PlotTableConfigWidget toolbar;
  tabs.setConfig(&config);
  tabs.addTab();
  toolbar.setPlotTabs(&tabs);
  toolbar.setPlotTable(tabs.getCurrentPlotTable());

  auto* progress = toolbar.findChild<ProgressWidget*>("widgetProgress");
  ASSERT_NE(progress, nullptr);

  startBagJob(tabs.getPlotTable(0));
  startBagJob(tabs.getPlotTable(1));
  progressBagJob(tabs.getPlotTable(0), 0.0);
  progressBagJob(tabs.getPlotTable(1), 1.0);

  EXPECT_NEAR(progress->getCurrentProgress(), 0.5, 1e-9);
}

TEST(PlotTabWidget, emptyTableDisablesRunAndPause) {
  ensureApplication();

  MultiplotConfig config(nullptr);
  PlotTabWidget tabs;
  PlotTableConfigWidget toolbar;
  tabs.setConfig(&config);
  toolbar.setPlotTabs(&tabs);
  toolbar.setPlotTable(tabs.getCurrentPlotTable());

  auto* runButton = toolbar.findChild<QPushButton*>("pushButtonRun");
  auto* pauseButton = toolbar.findChild<QPushButton*>("pushButtonPause");
  ASSERT_NE(runButton, nullptr);
  ASSERT_NE(pauseButton, nullptr);
  EXPECT_TRUE(runButton->isEnabled());
  EXPECT_FALSE(pauseButton->isEnabled());

  config.getTableConfig(0)->setNumPlots(0, 0);

  EXPECT_FALSE(runButton->isEnabled());
  EXPECT_FALSE(pauseButton->isEnabled());
}

TEST(PlotTabWidget, resetLayoutButtonEqualizesOnlyCurrentTab) {
  ensureApplication();

  MultiplotConfig config(nullptr);
  PlotTabWidget tabs;
  PlotTableConfigWidget toolbar;
  tabs.setConfig(&config);
  tabs.addTab();
  toolbar.setPlotTabs(&tabs);
  toolbar.setConfig(config.getTableConfig(1));
  toolbar.setPlotTable(tabs.getCurrentPlotTable());

  PlotTableConfig* tab0 = config.getTableConfig(0);
  PlotTableConfig* tab1 = config.getTableConfig(1);
  tab0->splitPlot(tab0->getPlotConfig(0, 0), Qt::Horizontal);
  tab0->getLayout()->setStretch({3, 1});
  tab1->splitPlot(tab1->getPlotConfig(0, 0), Qt::Horizontal);
  tab1->getLayout()->setStretch({4, 1});

  auto* resetButton = toolbar.findChild<QPushButton*>("pushButtonResetLayout");
  ASSERT_NE(resetButton, nullptr);
  EXPECT_TRUE(resetButton->isEnabled());

  resetButton->click();

  EXPECT_EQ(tab1->getLayout()->getStretch(), (QList<int>{1, 1}));
  EXPECT_EQ(tab0->getLayout()->getStretch(), (QList<int>{3, 1}));
}

TEST(PlotTabWidget, resetLayoutButtonDisabledForSinglePlot) {
  ensureApplication();

  MultiplotConfig config(nullptr);
  PlotTabWidget tabs;
  PlotTableConfigWidget toolbar;
  tabs.setConfig(&config);
  toolbar.setPlotTabs(&tabs);
  toolbar.setPlotTable(tabs.getCurrentPlotTable());

  auto* resetButton = toolbar.findChild<QPushButton*>("pushButtonResetLayout");
  ASSERT_NE(resetButton, nullptr);
  EXPECT_FALSE(resetButton->isEnabled());
}

}  // namespace
