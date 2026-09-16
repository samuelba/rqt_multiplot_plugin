#include <cstdlib>

#include <QAction>
#include <QApplication>
#include <QGridLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>

#include <gtest/gtest.h>

#include <rqt_multiplot/ConfigComboBox.h>
#include <rqt_multiplot/MultiplotConfigWidget.h>
#include <rqt_multiplot/MultiplotWidget.h>
#include <rqt_multiplot/PlotTableConfigWidget.h>
#include <rqt_multiplot/PlotWidget.h>

namespace {

using rqt_multiplot::ConfigComboBox;
using rqt_multiplot::MultiplotConfigWidget;
using rqt_multiplot::MultiplotWidget;
using rqt_multiplot::PlotTableConfigWidget;
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

QStringList fileMenuActionTexts(const QMenuBar& menuBar) {
  QStringList texts;
  const QList<QAction*> topLevelActions = menuBar.actions();
  if (topLevelActions.isEmpty()) {
    return texts;
  }

  QMenu* fileMenu = topLevelActions.first()->menu();
  if (fileMenu == nullptr) {
    return texts;
  }

  for (QAction* action : fileMenu->actions()) {
    if (action->isSeparator()) {
      texts.append(QStringLiteral("<separator>"));
    } else {
      texts.append(action->text());
    }
  }
  return texts;
}

int gridLayoutRow(QGridLayout& layout, const QWidget& widget) {
  for (int index = 0; index < layout.count(); ++index) {
    QLayoutItem* item = layout.itemAt(index);
    if (item == nullptr || item->widget() != &widget) {
      continue;
    }

    int row = 0;
    int column = 0;
    int rowSpan = 0;
    int columnSpan = 0;
    layout.getItemPosition(index, &row, &column, &rowSpan, &columnSpan);
    return row;
  }

  return -1;
}

bool shareToolbarRow(const MultiplotWidget& widget, const QWidget& left, const QWidget& right) {
  auto* toolbarLayout = widget.findChild<QGridLayout*>("gridLayout_3");
  if (toolbarLayout == nullptr) {
    return false;
  }

  const int leftRow = gridLayoutRow(*toolbarLayout, left);
  const int rightRow = gridLayoutRow(*toolbarLayout, right);
  return leftRow >= 0 && leftRow == rightRow;
}

}  // namespace

TEST(MultiplotFileMenu, hasFileMenuWithExpectedActions) {
  ensureApplication();

  MultiplotWidget widget;
  const auto* menuBar = widget.findChild<QMenuBar*>("menuBar");
  ASSERT_NE(menuBar, nullptr);

  const QStringList texts = fileMenuActionTexts(*menuBar);
  EXPECT_EQ(
      texts,
      QStringList({QStringLiteral("New configuration"), QStringLiteral("Open configuration..."), QStringLiteral("Save configuration"),
                   QStringLiteral("Save configuration as..."), QStringLiteral("<separator>"), QStringLiteral("Clear configuration history"),
                   QStringLiteral("<separator>"), QStringLiteral("Import from bag file..."), QStringLiteral("Import from bag directory..."),
                   QStringLiteral("<separator>"), QStringLiteral("Export to image file..."), QStringLiteral("Export to text file...")}));
}

TEST(MultiplotFileMenu, configButtonsRemovedFromToolbar) {
  ensureApplication();

  MultiplotWidget widget;
  auto* configWidget = widget.findChild<MultiplotConfigWidget*>("configWidget");
  ASSERT_NE(configWidget, nullptr);

  EXPECT_EQ(configWidget->findChild<QPushButton*>("pushButtonNew"), nullptr);
  EXPECT_EQ(configWidget->findChild<QPushButton*>("pushButtonOpen"), nullptr);
  EXPECT_EQ(configWidget->findChild<QPushButton*>("pushButtonSave"), nullptr);
  EXPECT_EQ(configWidget->findChild<QPushButton*>("pushButtonSaveAs"), nullptr);
  EXPECT_EQ(configWidget->findChild<QPushButton*>("pushButtonClearHistory"), nullptr);
  EXPECT_NE(configWidget->findChild<ConfigComboBox*>("configComboBox"), nullptr);
}

TEST(MultiplotFileMenu, globalImportExportButtonsRemovedFromPlotsToolbar) {
  ensureApplication();

  MultiplotWidget widget;
  auto* plotToolbar = widget.findChild<PlotTableConfigWidget*>("plotTableConfigWidget");
  ASSERT_NE(plotToolbar, nullptr);

  EXPECT_EQ(plotToolbar->findChild<QPushButton*>("pushButtonImport"), nullptr);
  EXPECT_EQ(plotToolbar->findChild<QPushButton*>("pushButtonExport"), nullptr);
  EXPECT_NE(plotToolbar->findChild<QPushButton*>("pushButtonRun"), nullptr);
  EXPECT_NE(plotToolbar->findChild<QPushButton*>("pushButtonPause"), nullptr);
  EXPECT_NE(plotToolbar->findChild<QPushButton*>("pushButtonClear"), nullptr);
}

TEST(MultiplotFileMenu, configAndPlotsShareOneToolbarRow) {
  ensureApplication();

  MultiplotWidget widget;
  auto* configWidget = widget.findChild<MultiplotConfigWidget*>("configWidget");
  auto* plotToolbar = widget.findChild<PlotTableConfigWidget*>("plotTableConfigWidget");
  ASSERT_NE(configWidget, nullptr);
  ASSERT_NE(plotToolbar, nullptr);

  EXPECT_TRUE(shareToolbarRow(widget, *configWidget, *plotToolbar));

  const auto* configLabel = widget.findChild<QLabel*>("labelConfig");
  const auto* plotsLabel = widget.findChild<QLabel*>("labelPlots");
  ASSERT_NE(configLabel, nullptr);
  ASSERT_NE(plotsLabel, nullptr);
  EXPECT_EQ(configLabel->text(), QStringLiteral("Config:"));
  EXPECT_EQ(plotsLabel->text(), QStringLiteral("Plots:"));
}

TEST(MultiplotFileMenu, saveAndClearHistoryStartDisabled) {
  ensureApplication();

  MultiplotConfigWidget configWidget;
  EXPECT_FALSE(configWidget.getActionSave()->isEnabled());
  EXPECT_FALSE(configWidget.getActionClearHistory()->isEnabled());
}

TEST(MultiplotFileMenu, perPlotExportButtonRemains) {
  ensureApplication();

  PlotWidget plotWidget;
  EXPECT_NE(plotWidget.findChild<QPushButton*>("pushButtonImportExport"), nullptr);
}
