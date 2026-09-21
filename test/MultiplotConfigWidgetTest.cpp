#include <cstdlib>

#include <QAbstractButton>
#include <QApplication>
#include <QIcon>
#include <QMessageBox>
#include <QString>

#include <gtest/gtest.h>

#include <QTemporaryDir>

#include "rqt_multiplot/MultiplotConfig.hpp"
#include "rqt_multiplot/MultiplotConfigWidget.hpp"
#include "rqt_multiplot/PlotTableConfig.hpp"
#include "rqt_multiplot/PlotTitleStyle.hpp"
#include "rqt_multiplot/UserPreferences.hpp"

namespace {

using rqt_multiplot::MultiplotConfig;
using rqt_multiplot::MultiplotConfigWidget;
using rqt_multiplot::PlotTableConfig;
using rqt_multiplot::UserPreferences;

class MultiplotConfigWidgetTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(tempDir_.isValid());
    UserPreferences::setTestSettingsFile(tempDir_.filePath(QStringLiteral("preferences.ini")));
    UserPreferences::factory().save();
  }

  void TearDown() override { UserPreferences::clearTestSettingsFile(); }

  QTemporaryDir tempDir_;
};

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

TEST_F(MultiplotConfigWidgetTest, startsUnmodified) {
  ensureApplication();

  MultiplotConfig config(nullptr);
  MultiplotConfigWidget widget;
  widget.setConfig(&config);

  EXPECT_FALSE(widget.isCurrentConfigModified());
}

TEST_F(MultiplotConfigWidgetTest, doesNotStayModifiedAfterAddAndRemoveTab) {
  ensureApplication();

  MultiplotConfig config(nullptr);
  MultiplotConfigWidget widget;
  widget.setConfig(&config);

  config.addTab();
  ASSERT_TRUE(widget.isCurrentConfigModified());

  config.removeTab(1);
  EXPECT_FALSE(widget.isCurrentConfigModified());
}

TEST_F(MultiplotConfigWidgetTest, doesNotStayModifiedAfterSwitchingTabAndBack) {
  ensureApplication();

  MultiplotConfig config(nullptr);
  config.addTab();

  MultiplotConfigWidget widget;
  widget.setConfig(&config);
  ASSERT_FALSE(widget.isCurrentConfigModified());

  config.setCurrentTabIndex(0);
  ASSERT_TRUE(widget.isCurrentConfigModified());

  config.setCurrentTabIndex(1);
  EXPECT_FALSE(widget.isCurrentConfigModified());
}

TEST_F(MultiplotConfigWidgetTest, marksModifiedWhenTitleChangesAndClearsWhenReverted) {
  ensureApplication();

  MultiplotConfig config(nullptr);
  MultiplotConfigWidget widget;
  widget.setConfig(&config);
  PlotTableConfig* table = config.getTableConfig(0);
  ASSERT_NE(table, nullptr);
  const QString originalTitle = table->getTitle();

  table->setTitle("Motors");
  ASSERT_TRUE(widget.isCurrentConfigModified());

  table->setTitle(originalTitle);
  EXPECT_FALSE(widget.isCurrentConfigModified());
}

TEST_F(MultiplotConfigWidgetTest, clearOverrideMarksModifiedWhenBaselineHadOverride) {
  ensureApplication();

  MultiplotConfig config(nullptr);
  config.setPreferencesOverridden(true);
  config.setThemeId(QStringLiteral("dark"));

  MultiplotConfigWidget widget;
  widget.setConfig(&config);
  widget.setCurrentConfigModified(false);
  ASSERT_FALSE(widget.isCurrentConfigModified());

  config.setPreferencesOverridden(false);
  config.applyUserDefaults();
  EXPECT_TRUE(widget.isCurrentConfigModified());
}

TEST_F(MultiplotConfigWidgetTest, plotTitleStyleChangeWithoutOverrideDoesNotMarkModified) {
  ensureApplication();

  MultiplotConfig config(nullptr);
  MultiplotConfigWidget widget;
  widget.setConfig(&config);
  ASSERT_FALSE(config.isPreferencesOverridden());

  rqt_multiplot::PlotTitleStyle style = rqt_multiplot::PlotTitleStyle::factory();
  style.fontSize = 18;
  config.setPlotTitleStyle(style);
  EXPECT_FALSE(widget.isCurrentConfigModified());
}

TEST_F(MultiplotConfigWidgetTest, themeChangeWithoutOverrideCanSettleSnapshot) {
  ensureApplication();

  MultiplotConfig config(nullptr);
  MultiplotConfigWidget widget;
  widget.setConfig(&config);
  ASSERT_FALSE(config.isPreferencesOverridden());

  config.setThemeId(QStringLiteral("dark"));
  ASSERT_TRUE(widget.isCurrentConfigModified());

  widget.setCurrentConfigModified(false);
  EXPECT_FALSE(widget.isCurrentConfigModified());
}

TEST_F(MultiplotConfigWidgetTest, applySavePromptIconsSetsSaveAndDiscard) {
  ensureApplication();

  QMessageBox messageBox;
  messageBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard);

  MultiplotConfigWidget::applySavePromptIcons(messageBox);

  QAbstractButton* saveButton = messageBox.button(QMessageBox::Save);
  QAbstractButton* discardButton = messageBox.button(QMessageBox::Discard);
  ASSERT_NE(saveButton, nullptr);
  ASSERT_NE(discardButton, nullptr);
  EXPECT_FALSE(saveButton->icon().isNull());
  EXPECT_FALSE(discardButton->icon().isNull());
}

}  // namespace
