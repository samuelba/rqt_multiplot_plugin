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

#pragma once

#include <QAbstractButton>
#include <QCloseEvent>
#include <QDockWidget>
#include <QEvent>
#include <QShowEvent>
#include <QStringList>
#include <QWidget>

#include "rqt_multiplot/MessageTypeRegistry.hpp"
#include "rqt_multiplot/MultiplotConfig.hpp"
#include "rqt_multiplot/PackageRegistry.hpp"

class QAction;
class QSplitter;

namespace Ui {

class MultiplotWidget;

}

namespace rqt_multiplot {

class PlotTableWidget;
class TopicBrowserWidget;

class MultiplotWidget : public QWidget {
  Q_OBJECT
 public:
  explicit MultiplotWidget(QWidget* parent = nullptr);
  ~MultiplotWidget() override;

  MultiplotConfig* getConfig() const;
  QDockWidget* getDockWidget() const;

  void setMaxConfigHistoryLength(size_t length);
  size_t getMaxConfigHistoryLength() const;
  void setConfigHistory(const QStringList& history);
  QStringList getConfigHistory() const;
  void runPlots();
  void pausePlots();

  void loadConfig(const QString& url);
  void readBag(const QString& url);

  bool confirmClose();

 protected:
  bool event(QEvent* event) override;
  bool eventFilter(QObject* object, QEvent* event) override;
  void closeEvent(QCloseEvent* event) override;
  void showEvent(QShowEvent* event) override;

 private:
  void installCloseGuard();
  void installStandaloneMenu();
  bool isCloseButtonActivation(QObject* object, QEvent* event) const;
  void setupTopicBrowser();
  void setupHelpMenu();
  void applyTopicBrowserState();

  Ui::MultiplotWidget* ui_;

  MultiplotConfig* config_;

  MessageTypeRegistry* messageTypeRegistry_;
  PackageRegistry* packageRegistry_;

  TopicBrowserWidget* topicBrowser_;
  QSplitter* topicBrowserSplitter_;
  QAction* actionTopicBrowser_;

  QObject* guardedDock_;
  QAbstractButton* guardedCloseButton_;
  bool closePromptCompleted_;
  bool closePromptOpen_;
  bool standaloneMenuInstalled_;

 private slots:
  void configWidgetCurrentConfigModifiedChanged(bool modified);
  void configWidgetCurrentConfigUrlChanged(const QString& url);
  void plotTabCurrentPlotTableChanged(PlotTableWidget* plotTable);
  void openPreferences();
  void openKeyboardShortcuts();
  void openAbout();
  void configThemeChanged(const QString& themeId);
  void topicBrowserSplitterMoved(int pos, int index);
};

}  // namespace rqt_multiplot
