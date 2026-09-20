/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/LaunchOptions.h"

#include <QCommandLineParser>
#include <QUrl>

namespace rqt_multiplot {

LaunchParseStatus parseLaunchOptions(const QStringList& arguments, LaunchOptions& options, bool processHelp) {
  options = LaunchOptions{};

  QCommandLineParser parser;
  parser.setApplicationDescription(QStringLiteral("ROS 2 multiplot GUI"));

  const QCommandLineOption configOption(QStringList() << "c" << "multiplot-config", "Load an xml plot configuration", "url");
  const QCommandLineOption bagOption(QStringList() << "b" << "multiplot-bag", "Load a rosbag2 file", "path");
  const QCommandLineOption runAllOption(QStringList() << "r" << "multiplot-run-all", "Run all plots on startup");
  parser.addOption(configOption);
  parser.addOption(bagOption);
  parser.addOption(runAllOption);

  QCommandLineOption helpOption(QStringList() << "h" << "help", "Displays help on commandline options.");
  if (processHelp) {
    parser.addOption(helpOption);
    parser.addVersionOption();
  }

  QStringList parseArguments = arguments;
  if (!processHelp) {
    if (arguments.isEmpty()) {
      parseArguments = QStringList{QStringLiteral("rqt_multiplot")};
    } else if (arguments.first() != QStringLiteral("rqt_multiplot") && arguments.first() != QStringLiteral("multiplot")) {
      parseArguments = QStringList{QStringLiteral("rqt_multiplot")} << arguments;
    }
  }

  if (!parser.parse(parseArguments)) {
    return LaunchParseStatus::Error;
  }

  if (processHelp && parser.isSet(helpOption)) {
    return LaunchParseStatus::HelpRequested;
  }

  options.runAllOnStart = parser.isSet(runAllOption);
  if (parser.isSet(configOption)) {
    options.configUrl = QUrl::fromUserInput(parser.value(configOption)).toString();
  }
  if (parser.isSet(bagOption)) {
    options.bagPath = parser.value(bagOption);
  }

  return LaunchParseStatus::Ok;
}

void printLaunchOptionsHelp(const QStringList& arguments) {
  QCommandLineParser parser;
  parser.setApplicationDescription(QStringLiteral("ROS 2 multiplot GUI"));
  parser.addOption(QCommandLineOption(QStringList() << "c" << "multiplot-config", "Load an xml plot configuration", "url"));
  parser.addOption(QCommandLineOption(QStringList() << "b" << "multiplot-bag", "Load a rosbag2 file", "path"));
  parser.addOption(QCommandLineOption(QStringList() << "r" << "multiplot-run-all", "Run all plots on startup"));
  parser.addHelpOption();
  parser.addVersionOption();
  parser.process(arguments);
  parser.showHelp(0);
}

}  // namespace rqt_multiplot
