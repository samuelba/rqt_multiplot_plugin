/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/StandaloneWindowSettings.hpp"

#include <QSettings>
#include <QWidget>

namespace rqt_multiplot {

namespace {

constexpr auto kOrganization = "rqt_multiplot";
constexpr auto kApplication = "multiplot";
constexpr auto kGeometryKey = "geometry";
constexpr auto kHistoryMaxLengthKey = "history/max_length";
constexpr auto kHistoryCountKey = "history/count";

}  // namespace

QString StandaloneWindowSettings::testSettingsFile_;

StandaloneWindowState StandaloneWindowSettings::load() {
  QSettings settings = testSettingsFile_.isEmpty() ? QSettings(QSettings::NativeFormat, QSettings::UserScope,
                                                               QString::fromLatin1(kOrganization), QString::fromLatin1(kApplication))
                                                   : QSettings(testSettingsFile_, QSettings::IniFormat);

  StandaloneWindowState state;
  state.geometry = settings.value(QString::fromLatin1(kGeometryKey)).toByteArray();
  state.maxConfigHistoryLength = settings.value(QString::fromLatin1(kHistoryMaxLengthKey), 0u).toUInt();

  const int savedCount = settings.value(QString::fromLatin1(kHistoryCountKey), -1).toInt();
  if (savedCount < 0) {
    int index = 0;
    while (settings.contains(QStringLiteral("history/config_") + QString::number(index))) {
      state.configHistory.append(settings.value(QStringLiteral("history/config_") + QString::number(index)).toString());
      ++index;
    }
  } else {
    for (int index = 0; index < savedCount; ++index) {
      state.configHistory.append(settings.value(QStringLiteral("history/config_") + QString::number(index)).toString());
    }
  }

  return state;
}

void StandaloneWindowSettings::save(const QWidget& widget, size_t maxConfigHistoryLength, const QStringList& configHistory) {
  QSettings settings = testSettingsFile_.isEmpty() ? QSettings(QSettings::NativeFormat, QSettings::UserScope,
                                                               QString::fromLatin1(kOrganization), QString::fromLatin1(kApplication))
                                                   : QSettings(testSettingsFile_, QSettings::IniFormat);

  settings.setValue(QString::fromLatin1(kGeometryKey), widget.saveGeometry());
  settings.setValue(QString::fromLatin1(kHistoryMaxLengthKey), static_cast<unsigned int>(maxConfigHistoryLength));
  settings.setValue(QString::fromLatin1(kHistoryCountKey), configHistory.count());

  for (int index = 0; index < configHistory.count(); ++index) {
    settings.setValue(QStringLiteral("history/config_") + QString::number(index), configHistory.at(index));
  }
  settings.sync();
}

}  // namespace rqt_multiplot
