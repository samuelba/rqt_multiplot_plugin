/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 *                                                                            *
 * This program is free software; you can redistribute it and/or modify       *
 * it under the terms of the Lesser GNU General Public License as published by*
 * the Free Software Foundation; either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the               *
 * Lesser GNU General Public License for more details.                        *
 *                                                                            *
 * You should have received a copy of the Lesser GNU General Public License   *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.       *
 ******************************************************************************/

#include <QCloseEvent>
#include <QCommandLineParser>
#include <QUrl>

#include <QDebug>

#include <pluginlib/class_list_macros.hpp>

#include <rqt_multiplot/MultiplotWidget.h>
#include <rqt_multiplot/RosContext.h>

#include "rqt_multiplot/MultiplotPlugin.h"

PLUGINLIB_EXPORT_CLASS(rqt_multiplot::MultiplotPlugin, rqt_gui_cpp::Plugin)

namespace rqt_multiplot {

/*****************************************************************************/
/* Constructors and Destructor                                               */
/*****************************************************************************/

MultiplotPlugin::MultiplotPlugin() : widget_(nullptr), runAllPlotsOnStart_(false) {
  setObjectName("MultiplotPlugin");
}

MultiplotPlugin::~MultiplotPlugin() = default;

/*****************************************************************************/
/* Methods                                                                   */
/*****************************************************************************/

void MultiplotPlugin::initPlugin(qt_gui_cpp::PluginContext& context) {
  RosContext::setNode(node_);

  widget_ = new MultiplotWidget();

  context.addWidget(widget_);

  parseArguments(context.argv());
}

void MultiplotPlugin::shutdownPlugin() {
  if (widget_ != nullptr) {
    widget_->pausePlots();
  }
}

void MultiplotPlugin::saveSettings(qt_gui_cpp::Settings& /*pluginSettings*/, qt_gui_cpp::Settings& instanceSettings) const {
  size_t maxConfigHistoryLength = widget_->getMaxConfigHistoryLength();
  QStringList configHistory = widget_->getConfigHistory();

  instanceSettings.remove("history");

  instanceSettings.setValue("history/max_length", (unsigned int)maxConfigHistoryLength);

  for (size_t i = 0; i < configHistory.count(); ++i) {
    instanceSettings.setValue("history/config_" + QString::number(i), configHistory[i]);
  }
}

void MultiplotPlugin::restoreSettings(const qt_gui_cpp::Settings&
                                      /*pluginSettings*/,
                                      const qt_gui_cpp::Settings& instanceSettings) {
  size_t maxConfigHistoryLength = widget_->getMaxConfigHistoryLength();

  // the config history may already be populated with one element
  // loaded from the command line, make sure that is kept before
  // appending more.
  QStringList configHistory = widget_->getConfigHistory();

  maxConfigHistoryLength = instanceSettings.value("history/max_length", (unsigned int)maxConfigHistoryLength).toUInt();

  while (instanceSettings.contains("history/config_" + QString::number(configHistory.count()))) {
    configHistory.append(instanceSettings.value("history/config_" + QString::number(configHistory.count())).toString());
  }

  widget_->setMaxConfigHistoryLength(maxConfigHistoryLength);
  widget_->setConfigHistory(configHistory);
  if (runAllPlotsOnStart_) {
    widget_->runPlots();
  }
}

void MultiplotPlugin::parseArguments(const QStringList& arguments) {
  QCommandLineParser parser;
  const QCommandLineOption configOption(QStringList() << "c" << "multiplot-config", "Load an xml plot configuration", "url");
  const QCommandLineOption bagOption(QStringList() << "b" << "multiplot-bag", "Load a rosbag2 file", "path");
  const QCommandLineOption runAllOption(QStringList() << "r" << "multiplot-run-all", "Run all plots on startup");
  parser.addOption(configOption);
  parser.addOption(bagOption);
  parser.addOption(runAllOption);
  parser.parse(QStringList() << "rqt_multiplot" << arguments);

  runAllPlotsOnStart_ = parser.isSet(runAllOption);
  if (parser.isSet(configOption)) {
    widget_->loadConfig(QUrl::fromUserInput(parser.value(configOption)).toString());
  }
  if (parser.isSet(bagOption)) {
    widget_->readBag(parser.value(bagOption));
  }
}

}  // namespace rqt_multiplot
