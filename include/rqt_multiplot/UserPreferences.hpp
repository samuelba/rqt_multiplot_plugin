/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <QString>

#include "rqt_multiplot/PlotTitleStyle.hpp"

namespace rqt_multiplot {

struct UserPreferences {
  QString timeZoneId;
  QString themeId;
  bool openGLCanvasEnabled = false;
  PlotTitleStyle plotTitleStyle;

  static UserPreferences factory();
  static UserPreferences load();
  void save() const;

  static void setTestSettingsFile(const QString& path);
  static void clearTestSettingsFile();

 private:
  static QString testSettingsFile_;
};

}  // namespace rqt_multiplot
