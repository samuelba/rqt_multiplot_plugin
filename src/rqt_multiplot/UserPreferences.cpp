/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/UserPreferences.h"

#include "rqt_multiplot/Theme.h"

#include <QSettings>

namespace rqt_multiplot {

namespace {
constexpr auto kOrganization = "rqt_multiplot";
constexpr auto kApplication = "preferences";
constexpr auto kTimeZoneKey = "time_zone";
constexpr auto kThemeKey = "theme";
constexpr auto kOpenGLCanvasKey = "opengl_canvas";
constexpr auto kTimeZoneLocal = "local";
}  // namespace

QString UserPreferences::testSettingsFile_;

UserPreferences UserPreferences::factory() {
  UserPreferences prefs;
  prefs.timeZoneId = QString::fromLatin1(kTimeZoneLocal);
  prefs.themeId = QString::fromLatin1(Theme::kLightId);
  prefs.openGLCanvasEnabled = false;
  return prefs;
}

UserPreferences UserPreferences::load() {
  QSettings settings = testSettingsFile_.isEmpty() ? QSettings(QSettings::NativeFormat, QSettings::UserScope,
                                                               QString::fromLatin1(kOrganization), QString::fromLatin1(kApplication))
                                                   : QSettings(testSettingsFile_, QSettings::IniFormat);

  if (!settings.contains(QString::fromLatin1(kTimeZoneKey)) && !settings.contains(QString::fromLatin1(kThemeKey)) &&
      !settings.contains(QString::fromLatin1(kOpenGLCanvasKey))) {
    return factory();
  }

  UserPreferences prefs;
  prefs.timeZoneId = settings.value(QString::fromLatin1(kTimeZoneKey), QString::fromLatin1(kTimeZoneLocal)).toString();
  prefs.themeId =
      Theme::toId(Theme::fromId(settings.value(QString::fromLatin1(kThemeKey), QString::fromLatin1(Theme::kLightId)).toString()));
  prefs.openGLCanvasEnabled = settings.value(QString::fromLatin1(kOpenGLCanvasKey), false).toBool();
  return prefs;
}

void UserPreferences::save() const {
  QSettings settings = testSettingsFile_.isEmpty() ? QSettings(QSettings::NativeFormat, QSettings::UserScope,
                                                               QString::fromLatin1(kOrganization), QString::fromLatin1(kApplication))
                                                   : QSettings(testSettingsFile_, QSettings::IniFormat);

  settings.setValue(QString::fromLatin1(kTimeZoneKey), timeZoneId);
  settings.setValue(QString::fromLatin1(kThemeKey), themeId);
  settings.setValue(QString::fromLatin1(kOpenGLCanvasKey), openGLCanvasEnabled);
  settings.sync();
}

void UserPreferences::setTestSettingsFile(const QString& path) {
  testSettingsFile_ = path;
}

void UserPreferences::clearTestSettingsFile() {
  testSettingsFile_.clear();
}

}  // namespace rqt_multiplot
