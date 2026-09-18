#include <algorithm>
#include <cstdlib>

#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTabWidget>

#include <gtest/gtest.h>

#include <rqt_multiplot/PreferencesDialog.h>

namespace {

using rqt_multiplot::PreferencesDialog;

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

TEST(PreferencesDialog, hasTitleAndWestGeneralTab) {
  ensureApplication();

  PreferencesDialog dialog;
  EXPECT_EQ(dialog.windowTitle(), QStringLiteral("Preferences"));

  auto* tabs = dialog.findChild<QTabWidget*>();
  ASSERT_NE(tabs, nullptr);
  EXPECT_EQ(tabs->tabPosition(), QTabWidget::West);
  EXPECT_EQ(tabs->tabText(0), QStringLiteral("General"));
}

TEST(PreferencesDialog, includesLocalAndUtcInTimeZoneCombo) {
  ensureApplication();

  PreferencesDialog dialog;
  auto* combo = dialog.findChild<QComboBox*>(QStringLiteral("comboTimeZone"));
  ASSERT_NE(combo, nullptr);

  QStringList labels;
  for (int index = 0; index < combo->count(); ++index) {
    labels.append(combo->itemText(index));
  }

  EXPECT_TRUE(std::any_of(labels.begin(), labels.end(), [](const QString& label) { return label.startsWith(QStringLiteral("Local (")); }));
  EXPECT_TRUE(labels.contains(QStringLiteral("UTC")));
}

TEST(PreferencesDialog, okWritesSelectedTimeZoneId) {
  ensureApplication();

  PreferencesDialog dialog;
  dialog.setTimeZoneId(QStringLiteral("utc"));

  auto* buttons = dialog.findChild<QDialogButtonBox*>();
  ASSERT_NE(buttons, nullptr);
  buttons->button(QDialogButtonBox::Ok)->click();

  EXPECT_EQ(dialog.timeZoneId(), QStringLiteral("utc"));
}

}  // namespace
