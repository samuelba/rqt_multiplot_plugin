#include <cstdlib>

#include <QAbstractButton>
#include <QApplication>
#include <QIcon>
#include <QMessageBox>
#include <QString>

#include <gtest/gtest.h>

#include <rqt_multiplot/MultiplotConfig.h>
#include <rqt_multiplot/MultiplotConfigWidget.h>
#include <rqt_multiplot/PlotTableConfig.h>

namespace {

using rqt_multiplot::MultiplotConfig;
using rqt_multiplot::MultiplotConfigWidget;
using rqt_multiplot::PlotTableConfig;

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

TEST(MultiplotConfigWidget, startsUnmodified) {
  ensureApplication();

  MultiplotConfig config(nullptr);
  MultiplotConfigWidget widget;
  widget.setConfig(&config);

  EXPECT_FALSE(widget.isCurrentConfigModified());
}

TEST(MultiplotConfigWidget, doesNotStayModifiedAfterAddAndRemoveTab) {
  ensureApplication();

  MultiplotConfig config(nullptr);
  MultiplotConfigWidget widget;
  widget.setConfig(&config);

  config.addTab();
  ASSERT_TRUE(widget.isCurrentConfigModified());

  config.removeTab(1);
  EXPECT_FALSE(widget.isCurrentConfigModified());
}

TEST(MultiplotConfigWidget, doesNotStayModifiedAfterSwitchingTabAndBack) {
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

TEST(MultiplotConfigWidget, marksModifiedWhenTitleChangesAndClearsWhenReverted) {
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

TEST(MultiplotConfigWidget, applySavePromptIconsSetsSaveAndDiscard) {
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
