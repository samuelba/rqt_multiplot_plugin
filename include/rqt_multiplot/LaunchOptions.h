/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#ifndef RQT_MULTIPLOT_LAUNCH_OPTIONS_H
#define RQT_MULTIPLOT_LAUNCH_OPTIONS_H

#include <QString>
#include <QStringList>

namespace rqt_multiplot {

struct LaunchOptions {
  QString configUrl;
  QString bagPath;
  bool runAllOnStart = false;
};

enum class LaunchParseStatus { Ok, HelpRequested, Error };

LaunchParseStatus parseLaunchOptions(const QStringList& arguments, LaunchOptions& options, bool processHelp = false);
void printLaunchOptionsHelp(const QStringList& arguments);

}  // namespace rqt_multiplot

#endif
