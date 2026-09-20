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
constexpr auto kPlotTitleFontSizeKey = "plot_title_font_size";
constexpr auto kPlotTitleBoldKey = "plot_title_bold";
constexpr auto kPlotTitleAutoColorKey = "plot_title_auto_color";
constexpr auto kPlotTitleColorKey = "plot_title_color";
constexpr auto kTimeZoneLocal = "local";

PlotTitleStyle loadPlotTitleStyle(const QSettings& settings) {
  const PlotTitleStyle factory = PlotTitleStyle::factory();
  PlotTitleStyle style;
  style.fontSize = PlotTitleStyle::clampFontSize(settings.value(QString::fromLatin1(kPlotTitleFontSizeKey), factory.fontSize).toInt());
  style.bold = settings.value(QString::fromLatin1(kPlotTitleBoldKey), factory.bold).toBool();
  style.autoColor = settings.value(QString::fromLatin1(kPlotTitleAutoColorKey), factory.autoColor).toBool();
  style.customColor = QColor(settings.value(QString::fromLatin1(kPlotTitleColorKey), factory.customColor.name()).toString());
  if (!style.customColor.isValid()) {
    style.customColor = factory.customColor;
  }
  return style;
}

void savePlotTitleStyle(QSettings& settings, const PlotTitleStyle& style) {
  settings.setValue(QString::fromLatin1(kPlotTitleFontSizeKey), style.fontSize);
  settings.setValue(QString::fromLatin1(kPlotTitleBoldKey), style.bold);
  settings.setValue(QString::fromLatin1(kPlotTitleAutoColorKey), style.autoColor);
  settings.setValue(QString::fromLatin1(kPlotTitleColorKey), style.customColor.name());
}

bool hasAnyPreferenceKey(const QSettings& settings) {
  return settings.contains(QString::fromLatin1(kTimeZoneKey)) || settings.contains(QString::fromLatin1(kThemeKey)) ||
         settings.contains(QString::fromLatin1(kOpenGLCanvasKey)) || settings.contains(QString::fromLatin1(kPlotTitleFontSizeKey)) ||
         settings.contains(QString::fromLatin1(kPlotTitleBoldKey)) || settings.contains(QString::fromLatin1(kPlotTitleAutoColorKey)) ||
         settings.contains(QString::fromLatin1(kPlotTitleColorKey));
}
}  // namespace

QString UserPreferences::testSettingsFile_;

UserPreferences UserPreferences::factory() {
  UserPreferences prefs;
  prefs.timeZoneId = QString::fromLatin1(kTimeZoneLocal);
  prefs.themeId = QString::fromLatin1(Theme::kLightId);
  prefs.openGLCanvasEnabled = false;
  prefs.plotTitleStyle = PlotTitleStyle::factory();
  return prefs;
}

UserPreferences UserPreferences::load() {
  QSettings settings = testSettingsFile_.isEmpty() ? QSettings(QSettings::NativeFormat, QSettings::UserScope,
                                                               QString::fromLatin1(kOrganization), QString::fromLatin1(kApplication))
                                                   : QSettings(testSettingsFile_, QSettings::IniFormat);

  if (!hasAnyPreferenceKey(settings)) {
    return factory();
  }

  UserPreferences prefs;
  prefs.timeZoneId = settings.value(QString::fromLatin1(kTimeZoneKey), QString::fromLatin1(kTimeZoneLocal)).toString();
  prefs.themeId =
      Theme::toId(Theme::fromId(settings.value(QString::fromLatin1(kThemeKey), QString::fromLatin1(Theme::kLightId)).toString()));
  prefs.openGLCanvasEnabled = settings.value(QString::fromLatin1(kOpenGLCanvasKey), false).toBool();
  prefs.plotTitleStyle = loadPlotTitleStyle(settings);
  return prefs;
}

void UserPreferences::save() const {
  QSettings settings = testSettingsFile_.isEmpty() ? QSettings(QSettings::NativeFormat, QSettings::UserScope,
                                                               QString::fromLatin1(kOrganization), QString::fromLatin1(kApplication))
                                                   : QSettings(testSettingsFile_, QSettings::IniFormat);

  settings.setValue(QString::fromLatin1(kTimeZoneKey), timeZoneId);
  settings.setValue(QString::fromLatin1(kThemeKey), themeId);
  settings.setValue(QString::fromLatin1(kOpenGLCanvasKey), openGLCanvasEnabled);
  savePlotTitleStyle(settings, plotTitleStyle);
  settings.sync();
}

void UserPreferences::setTestSettingsFile(const QString& path) {
  testSettingsFile_ = path;
}

void UserPreferences::clearTestSettingsFile() {
  testSettingsFile_.clear();
}

}  // namespace rqt_multiplot
