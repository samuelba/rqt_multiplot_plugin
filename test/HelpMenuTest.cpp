#include <cstdlib>

#include <QAction>
#include <QApplication>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QMenuBar>
#include <QPixmap>
#include <QSize>
#include <QTableWidget>
#include <QWidget>

#include <gtest/gtest.h>

#include "rqt_multiplot/AboutDialog.hpp"
#include "rqt_multiplot/CheatsheetDialog.hpp"
#include "rqt_multiplot/MultiplotWidget.hpp"
#include "rqt_multiplot/PackageResource.hpp"
#include "rqt_multiplot/Theme.hpp"

namespace {

using rqt_multiplot::AboutDialog;
using rqt_multiplot::CheatsheetDialog;
using rqt_multiplot::MultiplotWidget;
using rqt_multiplot::packagePixmap;
using rqt_multiplot::Theme;

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

QStringList topLevelMenuTexts(const QMenuBar& menuBar) {
  QStringList texts;
  for (QAction* action : menuBar.actions()) {
    texts.append(action->text());
  }
  return texts;
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

QPixmap labelPixmap(const QLabel& label) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  return label.pixmap();
#else
  return label.pixmap(Qt::ReturnByValue);
#endif
}

bool tableContainsText(const QTableWidget& table, const QString& substring) {
  for (int row = 0; row < table.rowCount(); ++row) {
    for (int column = 0; column < table.columnCount(); ++column) {
      const QTableWidgetItem* item = table.item(row, column);
      if (item != nullptr && item->text().contains(substring)) {
        return true;
      }
    }
  }
  return false;
}

bool dialogContainsText(const QWidget& root, const QString& substring) {
  const auto tables = root.findChildren<QTableWidget*>();
  for (const QTableWidget* table : tables) {
    if (tableContainsText(*table, substring)) {
      return true;
    }
  }
  return false;
}

bool hasSection(const QWidget& root, const QString& title) {
  const auto groups = root.findChildren<QGroupBox*>();
  for (const QGroupBox* group : groups) {
    if (group->title() == title) {
      return true;
    }
  }
  return false;
}

}  // namespace

TEST(HelpMenu, menuBarHasFileViewPlotsHelp) {
  ensureApplication();

  MultiplotWidget widget;
  const auto* menuBar = widget.findChild<QMenuBar*>("menuBar");
  ASSERT_NE(menuBar, nullptr);

  EXPECT_EQ(topLevelMenuTexts(*menuBar),
            QStringList({QStringLiteral("&File"), QStringLiteral("&View"), QStringLiteral("&Plots"), QStringLiteral("&Help")}));
}

TEST(HelpMenu, helpMenuHasExpectedActions) {
  ensureApplication();

  MultiplotWidget widget;
  const auto* menuBar = widget.findChild<QMenuBar*>("menuBar");
  ASSERT_NE(menuBar, nullptr);

  QMenu* helpMenu = findMenu(*menuBar, QStringLiteral("&Help"));
  ASSERT_NE(helpMenu, nullptr);
  EXPECT_EQ(helpMenu->objectName(), QStringLiteral("menuHelp"));

  const QStringList texts = menuActionTexts(*helpMenu);
  EXPECT_EQ(texts, QStringList({QStringLiteral("Keyboard shortcuts..."), QStringLiteral("About Multiplot...")}));

  QAction* shortcutsAction = widget.findChild<QAction*>("actionKeyboardShortcuts");
  ASSERT_NE(shortcutsAction, nullptr);
  EXPECT_EQ(shortcutsAction->shortcut(), QKeySequence::HelpContents);
}

TEST(AboutDialog, showsVersionLicenseAndProjectUrl) {
  ensureApplication();

  AboutDialog dialog;
  EXPECT_EQ(dialog.windowTitle(), QStringLiteral("About Multiplot"));

  const QString body = dialog.bodyText();
  EXPECT_FALSE(body.isEmpty());
  EXPECT_TRUE(body.contains(QStringLiteral("Multiplot")));
#ifndef RQT_MULTIPLOT_VERSION
#error "RQT_MULTIPLOT_VERSION must be defined for tests"
#endif
  EXPECT_TRUE(body.contains(QString::fromLatin1(RQT_MULTIPLOT_VERSION)));
  EXPECT_TRUE(body.contains(QStringLiteral("LGPL")));
  EXPECT_TRUE(body.contains(QStringLiteral("Qwt License, Version 1.0")));
  EXPECT_TRUE(body.contains(QStringLiteral("https://github.com/samuelba/rqt_multiplot_plugin")));

  auto* logo = dialog.findChild<QLabel*>(QStringLiteral("aboutLogoLabel"));
  ASSERT_NE(logo, nullptr);
  EXPECT_EQ(dialog.layout()->indexOf(logo), 0);
  EXPECT_EQ(dialog.layout()->itemAt(0)->alignment(), Qt::AlignHCenter);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  const QPixmap pixmap = logo->pixmap();
#else
  const QPixmap pixmap = logo->pixmap(Qt::ReturnByValue);
#endif
  EXPECT_EQ(pixmap.size(), QSize(256, 256));
  EXPECT_FALSE(pixmap.isNull());
}

TEST(AboutDialog, usesBlackLogoOnLightThemeAndWhiteLogoOnDarkTheme) {
  ensureApplication();
  const Theme::Id previousTheme = Theme::currentId();

  const auto expectLogo = [](Theme::Id theme, const char* path) {
    Theme::apply(nullptr, theme);
    AboutDialog dialog;
    const auto* logo = dialog.findChild<QLabel*>(QStringLiteral("aboutLogoLabel"));
    ASSERT_NE(logo, nullptr);
    const QPixmap actual = labelPixmap(*logo);
    const QPixmap expected = packagePixmap(QString::fromLatin1(path), actual.size());
    EXPECT_FALSE(actual.isNull());
    EXPECT_EQ(actual.toImage(), expected.toImage());
  };

  expectLogo(Theme::Id::Light, "resource/multiplot-logo-with-text-black.svg");
  expectLogo(Theme::Id::Dark, "resource/multiplot-logo-with-text-white.svg");
  Theme::apply(nullptr, previousTheme);
}

TEST(CheatsheetDialog, tablesIncludePlotInteractions) {
  ensureApplication();

  CheatsheetDialog dialog;
  EXPECT_EQ(dialog.windowTitle(), QStringLiteral("Keyboard shortcuts"));

  const auto* mouseTable = dialog.findChild<QTableWidget*>("mouseActionsTable");
  ASSERT_NE(mouseTable, nullptr);

  EXPECT_TRUE(dialogContainsText(dialog, QStringLiteral("Home")));
  EXPECT_TRUE(tableContainsText(*mouseTable, QStringLiteral("Left drag")));
  EXPECT_TRUE(tableContainsText(*mouseTable, QStringLiteral("Right drag")));
  EXPECT_TRUE(tableContainsText(*mouseTable, QStringLiteral("Right click")));
}

TEST(CheatsheetDialog, groupsActionsIntoTwoColumns) {
  ensureApplication();

  CheatsheetDialog dialog;
  auto* columns = dialog.findChild<QHBoxLayout*>(QStringLiteral("cheatsheetColumns"));
  auto* left = dialog.findChild<QWidget*>(QStringLiteral("cheatsheetLeftColumn"));
  auto* right = dialog.findChild<QWidget*>(QStringLiteral("cheatsheetRightColumn"));
  ASSERT_NE(columns, nullptr);
  ASSERT_NE(left, nullptr);
  ASSERT_NE(right, nullptr);
  EXPECT_EQ(columns->indexOf(left), 0);
  EXPECT_EQ(columns->indexOf(right), 1);
  EXPECT_GE(left->findChildren<QTableWidget*>().size(), 2);
  EXPECT_GE(right->findChildren<QTableWidget*>().size(), 2);

  EXPECT_TRUE(hasSection(dialog, QStringLiteral("File")));
  EXPECT_TRUE(hasSection(dialog, QStringLiteral("Import and export")));
  EXPECT_TRUE(hasSection(dialog, QStringLiteral("Plots")));
  EXPECT_TRUE(hasSection(dialog, QStringLiteral("General")));
  EXPECT_TRUE(hasSection(dialog, QStringLiteral("Tabs")));
  EXPECT_TRUE(hasSection(dialog, QStringLiteral("View")));
  EXPECT_TRUE(hasSection(dialog, QStringLiteral("Curves")));
  EXPECT_TRUE(hasSection(dialog, QStringLiteral("Mouse")));

  const auto* fileGroup = dialog.findChild<QGroupBox*>(QStringLiteral("cheatsheetSectionFile"));
  const auto* tabsGroup = dialog.findChild<QGroupBox*>(QStringLiteral("cheatsheetSectionTabs"));
  const auto* mouseGroup = dialog.findChild<QGroupBox*>(QStringLiteral("cheatsheetSectionMouse"));
  ASSERT_NE(fileGroup, nullptr);
  ASSERT_NE(tabsGroup, nullptr);
  ASSERT_NE(mouseGroup, nullptr);
  EXPECT_TRUE(left->isAncestorOf(fileGroup));
  EXPECT_TRUE(right->isAncestorOf(tabsGroup));
  EXPECT_FALSE(left->isAncestorOf(mouseGroup));
  EXPECT_FALSE(right->isAncestorOf(mouseGroup));

  const auto* fileTable = fileGroup->findChild<QTableWidget*>();
  const auto* tabsTable = tabsGroup->findChild<QTableWidget*>();
  ASSERT_NE(fileTable, nullptr);
  ASSERT_NE(tabsTable, nullptr);
  EXPECT_TRUE(tableContainsText(*fileTable, QStringLiteral("New configuration")));
  EXPECT_FALSE(tableContainsText(*fileTable, QStringLiteral("New tab")));
  EXPECT_TRUE(tableContainsText(*tabsTable, QStringLiteral("New tab")));

  const auto tables = dialog.findChildren<QTableWidget*>();
  for (const QTableWidget* table : tables) {
    for (int row = 0; row < table->rowCount(); ++row) {
      const QTableWidgetItem* input = table->item(row, 0);
      ASSERT_NE(input, nullptr);
      EXPECT_FALSE(input->text().trimmed().isEmpty());
    }
  }
}
