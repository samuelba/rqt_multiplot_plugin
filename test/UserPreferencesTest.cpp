#include <QSettings>
#include <QTemporaryDir>

#include <gtest/gtest.h>

#include <rqt_multiplot/UserPreferences.h>

namespace {

using rqt_multiplot::UserPreferences;

class UserPreferencesTest : public ::testing::Test {
 protected:
  void SetUp() override {
    const QTemporaryDir* dir = &tempDir_;
    ASSERT_TRUE(dir->isValid());
    settingsPath_ = dir->filePath(QStringLiteral("preferences.ini"));
    UserPreferences::setTestSettingsFile(settingsPath_);
  }

  void TearDown() override { UserPreferences::clearTestSettingsFile(); }

  QTemporaryDir tempDir_;
  QString settingsPath_;
};

TEST_F(UserPreferencesTest, factoryReturnsLocalLightOpenGLDisabled) {
  const UserPreferences prefs = UserPreferences::factory();

  EXPECT_EQ(prefs.timeZoneId, QStringLiteral("local"));
  EXPECT_EQ(prefs.themeId, QStringLiteral("light"));
  EXPECT_FALSE(prefs.openGLCanvasEnabled);
}

TEST_F(UserPreferencesTest, loadReturnsFactoryWhenFileMissing) {
  const UserPreferences prefs = UserPreferences::load();

  EXPECT_EQ(prefs.timeZoneId, QStringLiteral("local"));
  EXPECT_EQ(prefs.themeId, QStringLiteral("light"));
  EXPECT_FALSE(prefs.openGLCanvasEnabled);
}

TEST_F(UserPreferencesTest, saveAndLoadRoundTrip) {
  UserPreferences prefs;
  prefs.timeZoneId = QStringLiteral("utc");
  prefs.themeId = QStringLiteral("dark");
  prefs.openGLCanvasEnabled = true;
  prefs.save();

  const UserPreferences loaded = UserPreferences::load();

  EXPECT_EQ(loaded.timeZoneId, QStringLiteral("utc"));
  EXPECT_EQ(loaded.themeId, QStringLiteral("dark"));
  EXPECT_TRUE(loaded.openGLCanvasEnabled);
}

TEST_F(UserPreferencesTest, usesTestSettingsFile) {
  {
    QSettings settings(settingsPath_, QSettings::IniFormat);
    settings.setValue(QStringLiteral("time_zone"), QStringLiteral("Europe/Berlin"));
    settings.setValue(QStringLiteral("theme"), QStringLiteral("dark"));
    settings.setValue(QStringLiteral("opengl_canvas"), true);
    settings.sync();
  }

  const UserPreferences loaded = UserPreferences::load();

  EXPECT_EQ(loaded.timeZoneId, QStringLiteral("Europe/Berlin"));
  EXPECT_EQ(loaded.themeId, QStringLiteral("dark"));
  EXPECT_TRUE(loaded.openGLCanvasEnabled);
}

}  // namespace
