#include <algorithm>
#include <cstdlib>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>

#include <gtest/gtest.h>

#include "rqt_multiplot/PlotTitleStyle.hpp"
#include "rqt_multiplot/PreferencesDialog.hpp"
#include "rqt_multiplot/Theme.hpp"

namespace {

using rqt_multiplot::PlotTitleStyle;
using rqt_multiplot::PreferencesDialog;
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

TEST(PreferencesDialog, hasTitleAndWestGeneralTab) {
  ensureApplication();

  PreferencesDialog dialog;
  EXPECT_EQ(dialog.windowTitle(), QStringLiteral("Preferences"));

  auto* tabs = dialog.findChild<QTabWidget*>();
  ASSERT_NE(tabs, nullptr);
  EXPECT_EQ(tabs->tabPosition(), QTabWidget::West);
  EXPECT_EQ(tabs->tabText(0), QStringLiteral("General"));
  EXPECT_EQ(tabs->tabText(1), QStringLiteral("Appearance"));
}

TEST(PreferencesDialog, themeComboHasLightAndDarkOnly) {
  ensureApplication();

  PreferencesDialog dialog;
  auto* combo = dialog.findChild<QComboBox*>(QStringLiteral("comboTheme"));
  ASSERT_NE(combo, nullptr);
  ASSERT_EQ(combo->count(), 2);
  EXPECT_EQ(combo->itemText(0), QStringLiteral("Light"));
  EXPECT_EQ(combo->itemData(0).toString(), QStringLiteral("light"));
  EXPECT_EQ(combo->itemText(1), QStringLiteral("Dark"));
  EXPECT_EQ(combo->itemData(1).toString(), QStringLiteral("dark"));
}

TEST(PreferencesDialog, okWritesSelectedThemeId) {
  ensureApplication();

  PreferencesDialog dialog;
  dialog.setThemeId(QStringLiteral("dark"));

  auto* buttons = dialog.findChild<QDialogButtonBox*>();
  ASSERT_NE(buttons, nullptr);
  buttons->button(QDialogButtonBox::Ok)->click();

  EXPECT_EQ(dialog.themeId(), QStringLiteral("dark"));
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

TEST(PreferencesDialog, openGLCanvasDefaultsOff) {
  ensureApplication();

  PreferencesDialog dialog;
  auto* check = dialog.findChild<QCheckBox*>(QStringLiteral("checkOpenGLCanvas"));
  ASSERT_NE(check, nullptr);
  EXPECT_FALSE(check->isChecked());
  EXPECT_FALSE(dialog.isOpenGLCanvasEnabled());
}

TEST(PreferencesDialog, showsOverrideStatusAndPersistButtons) {
  ensureApplication();

  PreferencesDialog dialog;
  auto* status = dialog.findChild<QLabel*>(QStringLiteral("labelStatus"));
  auto* clearOverride = dialog.findChild<QPushButton*>(QStringLiteral("buttonClearOverride"));
  auto* saveDefaults = dialog.findChild<QPushButton*>(QStringLiteral("buttonSaveAsDefaults"));
  auto* overrideConfig = dialog.findChild<QPushButton*>(QStringLiteral("buttonOverrideConfiguration"));
  auto* restoreFactory = dialog.findChild<QPushButton*>(QStringLiteral("buttonRestoreFactory"));
  ASSERT_NE(status, nullptr);
  ASSERT_NE(clearOverride, nullptr);
  ASSERT_NE(saveDefaults, nullptr);
  ASSERT_NE(overrideConfig, nullptr);
  ASSERT_NE(restoreFactory, nullptr);

  EXPECT_EQ(status->text(), QStringLiteral("Using user defaults"));
  EXPECT_FALSE(clearOverride->isEnabled());

  dialog.setOverrideActive(true);
  EXPECT_EQ(status->text(), QStringLiteral("This configuration overrides user defaults"));
  EXPECT_TRUE(clearOverride->isEnabled());
}

TEST(PreferencesDialog, plotTitleControlsExistWithDefaults) {
  ensureApplication();

  PreferencesDialog dialog;
  auto* spin = dialog.findChild<QSpinBox*>(QStringLiteral("spinPlotTitleFontSize"));
  auto* weight = dialog.findChild<QComboBox*>(QStringLiteral("comboPlotTitleWeight"));
  auto* autoColor = dialog.findChild<QCheckBox*>(QStringLiteral("checkPlotTitleAutoColor"));
  auto* preview = dialog.findChild<QLabel*>(QStringLiteral("labelPlotTitlePreview"));
  ASSERT_NE(spin, nullptr);
  ASSERT_NE(weight, nullptr);
  ASSERT_NE(autoColor, nullptr);
  ASSERT_NE(preview, nullptr);

  EXPECT_EQ(spin->value(), 11);
  EXPECT_EQ(weight->currentIndex(), 1);
  EXPECT_TRUE(autoColor->isChecked());
  EXPECT_EQ(preview->text(), QStringLiteral("Untitled Plot"));
}

TEST(PreferencesDialog, okWritesPlotTitleStyle) {
  ensureApplication();

  PreferencesDialog dialog;
  PlotTitleStyle style = PlotTitleStyle::factory();
  style.fontSize = 18;
  style.bold = true;
  style.autoColor = false;
  style.customColor = QColor(0x33, 0x44, 0x55);
  dialog.setPlotTitleStyle(style);

  auto* buttons = dialog.findChild<QDialogButtonBox*>();
  ASSERT_NE(buttons, nullptr);
  buttons->button(QDialogButtonBox::Ok)->click();

  const PlotTitleStyle saved = dialog.plotTitleStyle();
  EXPECT_EQ(saved.fontSize, 18);
  EXPECT_TRUE(saved.bold);
  EXPECT_FALSE(saved.autoColor);
  EXPECT_EQ(saved.customColor, QColor(0x33, 0x44, 0x55));
}

TEST(PreferencesDialog, autoColorDisablesSwatch) {
  ensureApplication();

  PreferencesDialog dialog;
  auto* autoColor = dialog.findChild<QCheckBox*>(QStringLiteral("checkPlotTitleAutoColor"));
  auto* swatch = dialog.findChild<QLabel*>(QStringLiteral("labelPlotTitleColorSwatch"));
  ASSERT_NE(autoColor, nullptr);
  ASSERT_NE(swatch, nullptr);

  EXPECT_FALSE(swatch->isEnabled());
  autoColor->setChecked(false);
  EXPECT_TRUE(swatch->isEnabled());
  autoColor->setChecked(true);
  EXPECT_FALSE(swatch->isEnabled());
}

TEST(PreferencesDialog, previewUsesSelectedThemeWhenAutoColor) {
  ensureApplication();

  PreferencesDialog dialog;
  dialog.setThemeId(QStringLiteral("dark"));
  PlotTitleStyle style = PlotTitleStyle::factory();
  style.autoColor = true;
  dialog.setPlotTitleStyle(style);

  auto* preview = dialog.findChild<QLabel*>(QStringLiteral("labelPlotTitlePreview"));
  ASSERT_NE(preview, nullptr);
  EXPECT_EQ(preview->palette().color(QPalette::WindowText), Theme::plotForeground(Theme::Id::Dark));
}

TEST(PreferencesDialog, okWritesOpenGLCanvasEnabled) {
  ensureApplication();

  PreferencesDialog dialog;
  dialog.setOpenGLCanvasEnabled(true);

  auto* check = dialog.findChild<QCheckBox*>(QStringLiteral("checkOpenGLCanvas"));
  ASSERT_NE(check, nullptr);
  EXPECT_TRUE(check->isChecked());

  auto* buttons = dialog.findChild<QDialogButtonBox*>();
  ASSERT_NE(buttons, nullptr);
  buttons->button(QDialogButtonBox::Ok)->click();

  EXPECT_TRUE(dialog.isOpenGLCanvasEnabled());
}

}  // namespace
