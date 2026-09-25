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

#include <QHash>
#include <QString>
#include <QWidget>

#include "rqt_multiplot/MultiplotConfig.hpp"

class QTabBar;
class QTabWidget;
class QToolButton;

namespace rqt_multiplot {

class PlotTableWidget;

class PlotTabWidget : public QWidget {
  Q_OBJECT
 public:
  explicit PlotTabWidget(QWidget* parent = nullptr);
  ~PlotTabWidget() override;

  void setConfig(MultiplotConfig* config);
  MultiplotConfig* getConfig() const;
  size_t getNumPlotTables() const;
  PlotTableWidget* getPlotTable(size_t index) const;
  PlotTableWidget* getCurrentPlotTable() const;
  QString getTabText(size_t index) const;
  void addTab();
  void closeTab(size_t index);

  void runPlots();
  void pausePlots();
  void clearPlots();
  void loadFromBagFile(const QString& fileName);

 signals:
  void currentPlotTableChanged(PlotTableWidget* plotTable);
  void plotPausedChanged();
  void jobStarted(const QString& toolTip);
  void jobProgressChanged(double progress);
  void jobFinished(const QString& toolTip);
  void jobFailed(const QString& toolTip);
  void bagFileImported(const QString& fileName);

 private:
  QTabWidget* tabWidget_;
  QToolButton* addButton_;
  MultiplotConfig* config_;
  QHash<PlotTableWidget*, int> activeJobCounts_;
  QHash<PlotTableWidget*, double> jobProgress_;

  void rebuildTabs();
  void clearPlotTables();
  void appendPlotTable(PlotTableConfig* tableConfig);
  void destroyPlotTable(QWidget* page);
  void connectPlotTableJobs(PlotTableWidget* plotTable);
  void completeTableJob(PlotTableWidget* plotTable);
  void emitAggregatedProgress();
  void updateCloseButtons();
  QToolButton* createTabCloseButton(QTabBar* bar);
  void forEachPlotTable(void (PlotTableWidget::*method)());
  void forEachPlotTable(void (PlotTableWidget::*method)(const QString&), const QString& argument);

 private slots:
  void configTabAdded(size_t index);
  void configTabRemoved(size_t index);
  void configTabsChanged();
  void configTabTitleChanged(size_t index, const QString& title);
  void configCurrentTabIndexChanged(size_t index);
  void configTimezoneChanged(const QString& timeZoneId);
  void configOpenGLCanvasChanged(bool enabled);
  void configPlotTitleStyleChanged(const PlotTitleStyle& style);

  void currentChanged(int index);
  void tabCloseRequested(int index);
  void tabCloseButtonClicked();
  void tabBarDoubleClicked(int index);
  void addButtonClicked();

  void plotTableJobStarted(const QString& toolTip);
  void plotTableJobProgressChanged(double progress);
  void plotTableJobFinished(const QString& toolTip);
  void plotTableJobFailed(const QString& toolTip);
};

}  // namespace rqt_multiplot
