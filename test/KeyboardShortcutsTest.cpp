#include <cstdlib>

#include <QAction>
#include <QApplication>
#include <QKeySequence>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QSize>
#include <QTableWidget>
#include <QTest>
#include <QWidget>

#include <gtest/gtest.h>

#include "rqt_multiplot/CheatsheetDialog.hpp"
#include "rqt_multiplot/MultiplotWidget.hpp"
#include "rqt_multiplot/PackageResource.hpp"
#include "rqt_multiplot/PlotTabWidget.hpp"
#include "rqt_multiplot/PlotTableWidget.hpp"
#include "rqt_multiplot/PlotWidget.hpp"

namespace {

using rqt_multiplot::CheatsheetDialog;
using rqt_multiplot::MultiplotWidget;
using rqt_multiplot::packageIcon;
using rqt_multiplot::PlotTableWidget;
using rqt_multiplot::PlotTabWidget;
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

QMenu* findMenu(const QMenuBar& menuBar, const QString& text) {
  for (QAction* action : menuBar.actions()) {
    if (action->text() == text) {
      return action->menu();
    }
  }
  return nullptr;
}

QStringList menuActionTexts(const QMenu& menu) {
  QStringList texts;
  for (QAction* action : menu.actions()) {
    if (action->isSeparator()) {
      texts.append(QStringLiteral("<separator>"));
    } else {
      texts.append(action->text());
    }
  }
  return texts;
}

PlotTabWidget* plotTabs(MultiplotWidget& widget) {
  return widget.findChild<PlotTabWidget*>(QStringLiteral("plotTabWidget"));
}

bool anyPlotRunning(const MultiplotWidget& widget) {
  const auto* tabs = widget.findChild<PlotTabWidget*>(QStringLiteral("plotTabWidget"));
  if (tabs == nullptr) {
    return false;
  }

  for (size_t index = 0; index < tabs->getNumPlotTables(); ++index) {
    const PlotTableWidget* table = tabs->getPlotTable(index);
    if (table == nullptr) {
      continue;
    }
    for (const PlotWidget* plot : table->getPlotWidgets()) {
      if ((plot != nullptr) && !plot->isPaused()) {
        return true;
      }
    }
  }
  return false;
}

void activateTestWindow(QWidget& window) {
  window.activateWindow();
#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
  QApplication::setActiveWindow(&window);
#endif
  QApplication::processEvents();
}

QWidget* focusTarget(MultiplotWidget& widget) {
  widget.show();
  activateTestWindow(widget);

  auto* target = new QWidget(&widget);
  target->setObjectName(QStringLiteral("shortcutFocusTarget"));
  target->setFocusPolicy(Qt::StrongFocus);
  target->resize(10, 10);
  target->show();
  target->setFocus();
  QApplication::processEvents();
  return target;
}

void expectWidgetShortcut(const QAction& action, const QKeySequence& sequence) {
  EXPECT_EQ(action.shortcut(), sequence);
  EXPECT_EQ(action.shortcutContext(), Qt::WidgetWithChildrenShortcut);
}

void expectIcon(const QAction& action, const char* path) {
  const QIcon expected = packageIcon(QString::fromLatin1(path), QSize(16, 16));
  EXPECT_EQ(action.icon().pixmap(QSize(16, 16)).toImage(), expected.pixmap(QSize(16, 16)).toImage());
}

}  // namespace

TEST(KeyboardShortcuts, plotsMenuHasExpectedActions) {
  ensureApplication();

  MultiplotWidget widget;
  const auto* menuBar = widget.findChild<QMenuBar*>(QStringLiteral("menuBar"));
  ASSERT_NE(menuBar, nullptr);

  QMenu* plotsMenu = findMenu(*menuBar, QStringLiteral("&Plots"));
  ASSERT_NE(plotsMenu, nullptr);
  EXPECT_EQ(plotsMenu->objectName(), QStringLiteral("menuPlots"));
  EXPECT_EQ(menuActionTexts(*plotsMenu),
            QStringList({QStringLiteral("Play / Pause"), QStringLiteral("Clear plots"), QStringLiteral("Reset zoom"),
                         QStringLiteral("<separator>"), QStringLiteral("New tab"), QStringLiteral("Close tab"), QStringLiteral("Next tab"),
                         QStringLiteral("Previous tab")}));

  const auto* playPause = widget.findChild<QAction*>(QStringLiteral("actionTogglePlayPause"));
  const auto* clearPlots = widget.findChild<QAction*>(QStringLiteral("actionClearPlots"));
  const auto* resetZoom = widget.findChild<QAction*>(QStringLiteral("actionResetZoom"));
  const auto* newTab = widget.findChild<QAction*>(QStringLiteral("actionNewTab"));
  const auto* closeTab = widget.findChild<QAction*>(QStringLiteral("actionCloseTab"));
  const auto* nextTab = widget.findChild<QAction*>(QStringLiteral("actionNextTab"));
  const auto* previousTab = widget.findChild<QAction*>(QStringLiteral("actionPreviousTab"));
  ASSERT_NE(playPause, nullptr);
  ASSERT_NE(clearPlots, nullptr);
  ASSERT_NE(resetZoom, nullptr);
  ASSERT_NE(newTab, nullptr);
  ASSERT_NE(closeTab, nullptr);
  ASSERT_NE(nextTab, nullptr);
  ASSERT_NE(previousTab, nullptr);

  expectWidgetShortcut(*playPause, QKeySequence(Qt::Key_Space));
  expectWidgetShortcut(*clearPlots, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Delete));
  expectWidgetShortcut(*resetZoom, QKeySequence(Qt::Key_Home));
  expectWidgetShortcut(*newTab, QKeySequence(Qt::CTRL | Qt::Key_T));
  expectWidgetShortcut(*closeTab, QKeySequence(Qt::CTRL | Qt::Key_W));
  expectWidgetShortcut(*nextTab, QKeySequence(Qt::CTRL | Qt::Key_PageDown));
  expectWidgetShortcut(*previousTab, QKeySequence(Qt::CTRL | Qt::Key_PageUp));

  expectIcon(*playPause, "resource/play.svg");
  expectIcon(*clearPlots, "resource/delete-data.svg");
  expectIcon(*resetZoom, "resource/zoom-reset.svg");
  expectIcon(*newTab, "resource/new-tab.svg");
  expectIcon(*closeTab, "resource/close-tab.svg");
  EXPECT_TRUE(nextTab->icon().isNull());
  EXPECT_TRUE(previousTab->icon().isNull());
}

TEST(KeyboardShortcuts, tabJumpActionsUseAltNumber) {
  ensureApplication();

  MultiplotWidget widget;
  for (int number = 1; number <= 9; ++number) {
    const auto* action = widget.findChild<QAction*>(QStringLiteral("actionSelectTab%1").arg(number));
    ASSERT_NE(action, nullptr) << number;
    expectWidgetShortcut(*action, QKeySequence(Qt::ALT | (Qt::Key_0 + number)));
    EXPECT_TRUE(action->text().isEmpty());
  }
}

TEST(KeyboardShortcuts, viewAndFileActionsHaveShortcuts) {
  ensureApplication();

  MultiplotWidget widget;
  const auto* menuBar = widget.findChild<QMenuBar*>(QStringLiteral("menuBar"));
  ASSERT_NE(menuBar, nullptr);
  QMenu* viewMenu = findMenu(*menuBar, QStringLiteral("&View"));
  ASSERT_NE(viewMenu, nullptr);
  EXPECT_EQ(menuActionTexts(*viewMenu),
            QStringList({QStringLiteral("Topic browser"), QStringLiteral("Curve values"), QStringLiteral("Grid")}));

  const auto* topicBrowser = widget.findChild<QAction*>(QStringLiteral("actionTopicBrowser"));
  const auto* curveValues = widget.findChild<QAction*>(QStringLiteral("actionToggleCurveValues"));
  const auto* grid = widget.findChild<QAction*>(QStringLiteral("actionToggleGrid"));
  const auto* importBags = widget.findChild<QAction*>(QStringLiteral("actionImportBagFile"));
  const auto* addBags = widget.findChild<QAction*>(QStringLiteral("actionAddBagFiles"));
  const auto* exportImage = widget.findChild<QAction*>(QStringLiteral("actionExportImageFile"));
  ASSERT_NE(topicBrowser, nullptr);
  ASSERT_NE(curveValues, nullptr);
  ASSERT_NE(grid, nullptr);
  ASSERT_NE(importBags, nullptr);
  ASSERT_NE(addBags, nullptr);
  ASSERT_NE(exportImage, nullptr);

  expectWidgetShortcut(*topicBrowser, QKeySequence(Qt::CTRL | Qt::Key_B));
  expectWidgetShortcut(*curveValues, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_B));
  expectWidgetShortcut(*grid, QKeySequence(Qt::CTRL | Qt::Key_G));
  expectIcon(*topicBrowser, "resource/tree-view.svg");
  expectIcon(*curveValues, "resource/side-panel-open.svg");
  expectIcon(*grid, "resource/grid.svg");
  EXPECT_EQ(importBags->shortcut(), QKeySequence(Qt::CTRL | Qt::Key_I));
  EXPECT_EQ(importBags->shortcutContext(), Qt::WindowShortcut);
  EXPECT_EQ(addBags->shortcut(), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_I));
  EXPECT_EQ(addBags->shortcutContext(), Qt::WindowShortcut);
  EXPECT_EQ(exportImage->shortcut(), QKeySequence(Qt::CTRL | Qt::Key_E));
  EXPECT_EQ(exportImage->shortcutContext(), Qt::WindowShortcut);
}

TEST(KeyboardShortcuts, startupLeavesFocusOffTheConfigLineEdit) {
  ensureApplication();

  MultiplotWidget widget;
  widget.runPlots();
  ASSERT_TRUE(anyPlotRunning(widget));

  widget.show();
  activateTestWindow(widget);

  const QWidget* focused = QApplication::focusWidget();
  EXPECT_TRUE((focused == nullptr) || !focused->inherits("QLineEdit"));

  QWidget* keyTarget = (focused != nullptr) ? const_cast<QWidget*>(focused) : static_cast<QWidget*>(&widget);
  QTest::keyClick(keyTarget, Qt::Key_Space);
  EXPECT_FALSE(anyPlotRunning(widget));
}

TEST(KeyboardShortcuts, spaceTogglesPlayPauseWhenFocusIsInsideWidget) {
  ensureApplication();

  MultiplotWidget widget;
  widget.runPlots();
  ASSERT_TRUE(anyPlotRunning(widget));

  QWidget* target = focusTarget(widget);
  ASSERT_TRUE(target->hasFocus());

  QTest::keyClick(target, Qt::Key_Space);
  EXPECT_FALSE(anyPlotRunning(widget));

  QTest::keyClick(target, Qt::Key_Space);
  EXPECT_TRUE(anyPlotRunning(widget));
}

TEST(KeyboardShortcuts, spaceDoesNotToggleWhenAnotherWindowHasFocus) {
  ensureApplication();

  MultiplotWidget widget;
  widget.runPlots();
  ASSERT_TRUE(anyPlotRunning(widget));
  widget.show();

  QWidget otherWindow;
  otherWindow.setFocusPolicy(Qt::StrongFocus);
  otherWindow.resize(40, 40);
  otherWindow.show();
  activateTestWindow(otherWindow);
  otherWindow.setFocus();
  QApplication::processEvents();
  ASSERT_TRUE(otherWindow.hasFocus());

  QTest::keyClick(&otherWindow, Qt::Key_Space);
  EXPECT_TRUE(anyPlotRunning(widget));
}

TEST(KeyboardShortcuts, spaceInLineEditDoesNotTogglePlayback) {
  ensureApplication();

  MultiplotWidget widget;
  widget.runPlots();
  ASSERT_TRUE(anyPlotRunning(widget));

  widget.show();
  activateTestWindow(widget);

  QLineEdit lineEdit(&widget);
  lineEdit.show();
  lineEdit.setFocus();
  QApplication::processEvents();
  ASSERT_TRUE(lineEdit.hasFocus());

  QTest::keyClick(&lineEdit, Qt::Key_Space);
  EXPECT_TRUE(anyPlotRunning(widget));
  EXPECT_EQ(lineEdit.text(), QStringLiteral(" "));
}

TEST(KeyboardShortcuts, homeResetsZoomInActiveTab) {
  ensureApplication();

  MultiplotWidget widget;
  PlotTabWidget* tabs = plotTabs(widget);
  ASSERT_NE(tabs, nullptr);
  PlotTableWidget* table = tabs->getCurrentPlotTable();
  ASSERT_NE(table, nullptr);
  ASSERT_FALSE(table->getPlotWidgets().isEmpty());
  PlotWidget* plot = table->getPlotWidgets().first();
  ASSERT_NE(plot, nullptr);
  plot->setUserScaleLocked(true);

  QWidget* target = focusTarget(widget);
  ASSERT_TRUE(target->hasFocus());
  QTest::keyClick(target, Qt::Key_Home);

  EXPECT_FALSE(plot->isUserScaleLocked());
}

TEST(KeyboardShortcuts, ctrlTAndCtrlWChangeTabCount) {
  ensureApplication();

  MultiplotWidget widget;
  ASSERT_EQ(widget.getConfig()->getNumTabs(), 1u);

  QWidget* target = focusTarget(widget);
  QTest::keyClick(target, Qt::Key_W, Qt::ControlModifier);
  EXPECT_EQ(widget.getConfig()->getNumTabs(), 1u);

  QTest::keyClick(target, Qt::Key_T, Qt::ControlModifier);
  EXPECT_EQ(widget.getConfig()->getNumTabs(), 2u);

  QTest::keyClick(target, Qt::Key_W, Qt::ControlModifier);
  EXPECT_EQ(widget.getConfig()->getNumTabs(), 1u);
}

TEST(KeyboardShortcuts, ctrlPageDownWrapsAndAltSelectsTab) {
  ensureApplication();

  MultiplotWidget widget;
  QWidget* target = focusTarget(widget);
  QTest::keyClick(target, Qt::Key_T, Qt::ControlModifier);
  QTest::keyClick(target, Qt::Key_T, Qt::ControlModifier);
  ASSERT_EQ(widget.getConfig()->getNumTabs(), 3u);
  ASSERT_EQ(widget.getConfig()->getCurrentTabIndex(), 2u);

  QTest::keyClick(target, Qt::Key_PageDown, Qt::ControlModifier);
  EXPECT_EQ(widget.getConfig()->getCurrentTabIndex(), 0u);

  QTest::keyClick(target, Qt::Key_2, Qt::AltModifier);
  EXPECT_EQ(widget.getConfig()->getCurrentTabIndex(), 1u);

  QTest::keyClick(target, Qt::Key_PageUp, Qt::ControlModifier);
  EXPECT_EQ(widget.getConfig()->getCurrentTabIndex(), 0u);
}

TEST(KeyboardShortcuts, viewShortcutsTogglePanels) {
  ensureApplication();

  MultiplotWidget widget;
  PlotTabWidget* tabs = plotTabs(widget);
  ASSERT_NE(tabs, nullptr);
  PlotTableWidget* table = tabs->getCurrentPlotTable();
  ASSERT_NE(table, nullptr);
  ASSERT_NE(table->getConfig(), nullptr);
  EXPECT_FALSE(widget.getConfig()->isTopicBrowserVisible());
  EXPECT_FALSE(table->getConfig()->isSidebarVisible());
  EXPECT_FALSE(table->getConfig()->isGridVisible());

  QWidget* target = focusTarget(widget);
  QTest::keyClick(target, Qt::Key_B, Qt::ControlModifier);
  QTest::keyClick(target, Qt::Key_B, Qt::ControlModifier | Qt::ShiftModifier);
  QTest::keyClick(target, Qt::Key_G, Qt::ControlModifier);

  EXPECT_TRUE(widget.getConfig()->isTopicBrowserVisible());
  EXPECT_TRUE(table->getConfig()->isSidebarVisible());
  EXPECT_TRUE(table->getConfig()->isGridVisible());
}

TEST(KeyboardShortcuts, cheatsheetListsNewShortcuts) {
  ensureApplication();

  CheatsheetDialog dialog;
  const auto tables = dialog.findChildren<QTableWidget*>();
  ASSERT_GE(tables.size(), 2);

  auto contains = [&tables](const QString& text) {
    for (const QTableWidget* table : tables) {
      for (int row = 0; row < table->rowCount(); ++row) {
        for (int column = 0; column < table->columnCount(); ++column) {
          const QTableWidgetItem* item = table->item(row, column);
          if ((item != nullptr) && item->text().contains(text)) {
            return true;
          }
        }
      }
    }
    return false;
  };

  EXPECT_TRUE(contains(QStringLiteral("Play / pause all plots")));
  EXPECT_TRUE(contains(QStringLiteral("Reset zoom of all plots in the active tab")));
  EXPECT_TRUE(contains(QStringLiteral("Select all curves")));
  EXPECT_TRUE(contains(QStringLiteral("Copy selected curves")));
  EXPECT_TRUE(contains(QStringLiteral("Paste curves")));
  EXPECT_TRUE(contains(QStringLiteral("Remove selected curves")));
}
