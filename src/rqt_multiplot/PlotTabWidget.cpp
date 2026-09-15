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

#include "rqt_multiplot/PlotTabWidget.h"

#include <QIcon>
#include <QInputDialog>
#include <QLineEdit>
#include <QSize>
#include <QTabBar>
#include <QTabWidget>
#include <QToolButton>
#include <QVBoxLayout>

#include <rqt_multiplot/PackageResource.h>
#include <rqt_multiplot/PlotTableWidget.h>

namespace rqt_multiplot {

/*****************************************************************************/
/* Constructors and Destructor                                               */
/*****************************************************************************/

PlotTabWidget::PlotTabWidget(QWidget* parent)
    : QWidget(parent), tabWidget_(new QTabWidget(this)), addButton_(new QToolButton(this)), config_(nullptr) {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(tabWidget_);

  tabWidget_->setTabsClosable(true);
  tabWidget_->tabBar()->setElideMode(Qt::ElideRight);

  addButton_->setAutoRaise(true);
  addButton_->setToolTip("Add tab");
  addButton_->setIcon(QIcon(packageResourcePath("resource/new-tab.svg")));
  addButton_->setIconSize(QSize(16, 16));
  tabWidget_->setCornerWidget(addButton_, Qt::TopRightCorner);

  connect(tabWidget_, SIGNAL(currentChanged(int)), this, SLOT(currentChanged(int)));
  connect(tabWidget_, SIGNAL(tabCloseRequested(int)), this, SLOT(tabCloseRequested(int)));
  connect(tabWidget_->tabBar(), SIGNAL(tabBarDoubleClicked(int)), this, SLOT(tabBarDoubleClicked(int)));
  connect(addButton_, SIGNAL(clicked()), this, SLOT(addButtonClicked()));
}

PlotTabWidget::~PlotTabWidget() = default;

/*****************************************************************************/
/* Accessors                                                                 */
/*****************************************************************************/

void PlotTabWidget::setConfig(MultiplotConfig* config) {
  if (config == config_) {
    return;
  }

  if (config_ != nullptr) {
    disconnect(config_, SIGNAL(tabAdded(size_t)), this, SLOT(configTabAdded(size_t)));
    disconnect(config_, SIGNAL(tabRemoved(size_t)), this, SLOT(configTabRemoved(size_t)));
    disconnect(config_, SIGNAL(tabsChanged()), this, SLOT(configTabsChanged()));
    disconnect(config_, SIGNAL(tabTitleChanged(size_t, const QString&)), this, SLOT(configTabTitleChanged(size_t, const QString&)));
    disconnect(config_, SIGNAL(currentTabIndexChanged(size_t)), this, SLOT(configCurrentTabIndexChanged(size_t)));
  }

  config_ = config;

  if (config_ != nullptr) {
    connect(config_, SIGNAL(tabAdded(size_t)), this, SLOT(configTabAdded(size_t)));
    connect(config_, SIGNAL(tabRemoved(size_t)), this, SLOT(configTabRemoved(size_t)));
    connect(config_, SIGNAL(tabsChanged()), this, SLOT(configTabsChanged()));
    connect(config_, SIGNAL(tabTitleChanged(size_t, const QString&)), this, SLOT(configTabTitleChanged(size_t, const QString&)));
    connect(config_, SIGNAL(currentTabIndexChanged(size_t)), this, SLOT(configCurrentTabIndexChanged(size_t)));
  }

  rebuildTabs();
}

MultiplotConfig* PlotTabWidget::getConfig() const {
  return config_;
}

size_t PlotTabWidget::getNumPlotTables() const {
  return static_cast<size_t>(tabWidget_->count());
}

PlotTableWidget* PlotTabWidget::getPlotTable(size_t index) const {
  return qobject_cast<PlotTableWidget*>(tabWidget_->widget(static_cast<int>(index)));
}

PlotTableWidget* PlotTabWidget::getCurrentPlotTable() const {
  return qobject_cast<PlotTableWidget*>(tabWidget_->currentWidget());
}

QString PlotTabWidget::getTabText(size_t index) const {
  return tabWidget_->tabText(static_cast<int>(index));
}

void PlotTabWidget::addTab() {
  if (config_ != nullptr) {
    config_->addTab();
  }
}

void PlotTabWidget::closeTab(size_t index) {
  if (config_ != nullptr) {
    config_->removeTab(index);
  }
}

void PlotTabWidget::runPlots() {
  forEachPlotTable(&PlotTableWidget::runPlots);
}

void PlotTabWidget::pausePlots() {
  forEachPlotTable(&PlotTableWidget::pausePlots);
}

void PlotTabWidget::clearPlots() {
  forEachPlotTable(&PlotTableWidget::clearPlots);
}

void PlotTabWidget::loadFromBagFile(const QString& fileName) {
  forEachPlotTable(&PlotTableWidget::loadFromBagFile, fileName);
}

/*****************************************************************************/
/* Methods                                                                   */
/*****************************************************************************/

void PlotTabWidget::rebuildTabs() {
  clearPlotTables();

  tabWidget_->blockSignals(true);

  if (config_ != nullptr) {
    for (size_t index = 0; index < config_->getNumTabs(); ++index) {
      appendPlotTable(config_->getTableConfig(index));
    }

    tabWidget_->setCurrentIndex(static_cast<int>(config_->getCurrentTabIndex()));
  }

  tabWidget_->blockSignals(false);
  updateCloseButtons();
  emit currentPlotTableChanged(getCurrentPlotTable());
}

void PlotTabWidget::clearPlotTables() {
  emit currentPlotTableChanged(nullptr);

  tabWidget_->blockSignals(true);
  while (tabWidget_->count() > 0) {
    QWidget* page = tabWidget_->widget(0);
    tabWidget_->removeTab(0);
    destroyPlotTable(page);
  }
  tabWidget_->blockSignals(false);
}

void PlotTabWidget::appendPlotTable(PlotTableConfig* tableConfig) {
  auto* plotTable = new PlotTableWidget(tabWidget_);
  plotTable->setConfig(tableConfig);
  connect(plotTable, SIGNAL(plotPausedChanged()), this, SIGNAL(plotPausedChanged()));
  connectPlotTableJobs(plotTable);
  tabWidget_->addTab(plotTable, tableConfig != nullptr ? tableConfig->getTitle() : QString());
}

void PlotTabWidget::destroyPlotTable(QWidget* page) {
  auto* plotTable = qobject_cast<PlotTableWidget*>(page);
  if (plotTable != nullptr) {
    const int outstanding = activeJobCounts_.value(plotTable, 0);
    plotTable->disconnect(this);
    activeJobCounts_.remove(plotTable);
    jobProgress_.remove(plotTable);
    for (int index = 0; index < outstanding; ++index) {
      emit jobFinished(QString());
    }
  }

  delete page;
}

void PlotTabWidget::connectPlotTableJobs(PlotTableWidget* plotTable) {
  connect(plotTable, SIGNAL(jobStarted(const QString&)), this, SLOT(plotTableJobStarted(const QString&)));
  connect(plotTable, SIGNAL(jobProgressChanged(double)), this, SLOT(plotTableJobProgressChanged(double)));
  connect(plotTable, SIGNAL(jobFinished(const QString&)), this, SLOT(plotTableJobFinished(const QString&)));
  connect(plotTable, SIGNAL(jobFailed(const QString&)), this, SLOT(plotTableJobFailed(const QString&)));
}

void PlotTabWidget::completeTableJob(PlotTableWidget* plotTable) {
  if (plotTable == nullptr) {
    return;
  }

  auto countIt = activeJobCounts_.find(plotTable);
  if ((countIt != activeJobCounts_.end()) && (countIt.value() > 0)) {
    --countIt.value();
    if (countIt.value() == 0) {
      activeJobCounts_.erase(countIt);
      jobProgress_.remove(plotTable);
    }
  } else {
    jobProgress_.remove(plotTable);
  }
}

void PlotTabWidget::emitAggregatedProgress() {
  if (jobProgress_.isEmpty()) {
    return;
  }

  double sum = 0.0;
  for (double progress : jobProgress_) {
    sum += progress;
  }

  emit jobProgressChanged(sum / static_cast<double>(jobProgress_.size()));
}

void PlotTabWidget::updateCloseButtons() {
  QTabBar* bar = tabWidget_->tabBar();
  const bool closable = tabWidget_->count() > 1;

  for (int index = 0; index < tabWidget_->count(); ++index) {
    if (QWidget* button = bar->tabButton(index, QTabBar::RightSide)) {
      button->setVisible(closable);
    }
  }
}

void PlotTabWidget::forEachPlotTable(void (PlotTableWidget::*method)()) {
  for (int index = 0; index < tabWidget_->count(); ++index) {
    if (PlotTableWidget* plotTable = getPlotTable(static_cast<size_t>(index))) {
      (plotTable->*method)();
    }
  }
}

void PlotTabWidget::forEachPlotTable(void (PlotTableWidget::*method)(const QString&), const QString& argument) {
  for (int index = 0; index < tabWidget_->count(); ++index) {
    if (PlotTableWidget* plotTable = getPlotTable(static_cast<size_t>(index))) {
      (plotTable->*method)(argument);
    }
  }
}

/*****************************************************************************/
/* Slots                                                                     */
/*****************************************************************************/

void PlotTabWidget::configTabAdded(size_t index) {
  if (config_ == nullptr) {
    return;
  }

  appendPlotTable(config_->getTableConfig(index));
  updateCloseButtons();
}

void PlotTabWidget::configTabRemoved(size_t index) {
  const int tabIndex = static_cast<int>(index);
  if ((tabIndex < 0) || (tabIndex >= tabWidget_->count())) {
    return;
  }

  emit currentPlotTableChanged(nullptr);

  tabWidget_->blockSignals(true);
  QWidget* page = tabWidget_->widget(tabIndex);
  tabWidget_->removeTab(tabIndex);
  tabWidget_->blockSignals(false);
  destroyPlotTable(page);
  updateCloseButtons();
  emit currentPlotTableChanged(getCurrentPlotTable());
}

void PlotTabWidget::configTabsChanged() {
  rebuildTabs();
}

void PlotTabWidget::configTabTitleChanged(size_t index, const QString& title) {
  tabWidget_->setTabText(static_cast<int>(index), title);
}

void PlotTabWidget::configCurrentTabIndexChanged(size_t index) {
  const int tabIndex = static_cast<int>(index);
  if (tabWidget_->currentIndex() != tabIndex) {
    tabWidget_->setCurrentIndex(tabIndex);
  }
}

void PlotTabWidget::currentChanged(int index) {
  if ((config_ != nullptr) && (index >= 0)) {
    config_->setCurrentTabIndex(static_cast<size_t>(index));
  }

  emit currentPlotTableChanged(getCurrentPlotTable());
}

void PlotTabWidget::tabCloseRequested(int index) {
  if (index >= 0) {
    closeTab(static_cast<size_t>(index));
  }
}

void PlotTabWidget::tabBarDoubleClicked(int index) {
  if ((config_ == nullptr) || (index < 0)) {
    return;
  }

  const QString currentTitle = tabWidget_->tabText(index);
  bool accepted = false;
  const QString title = QInputDialog::getText(this, "Rename Tab", "Tab name:", QLineEdit::Normal, currentTitle, &accepted);
  if (!accepted) {
    return;
  }

  config_->setTabTitle(static_cast<size_t>(index), title);
}

void PlotTabWidget::addButtonClicked() {
  addTab();
}

void PlotTabWidget::plotTableJobStarted(const QString& toolTip) {
  auto* plotTable = qobject_cast<PlotTableWidget*>(sender());
  if (plotTable != nullptr) {
    ++activeJobCounts_[plotTable];
    jobProgress_[plotTable] = 0.0;
  }

  emit jobStarted(toolTip);
}

void PlotTabWidget::plotTableJobProgressChanged(double progress) {
  auto* plotTable = qobject_cast<PlotTableWidget*>(sender());
  if (plotTable != nullptr) {
    jobProgress_[plotTable] = progress;
  }

  emitAggregatedProgress();
}

void PlotTabWidget::plotTableJobFinished(const QString& toolTip) {
  completeTableJob(qobject_cast<PlotTableWidget*>(sender()));
  emitAggregatedProgress();
  emit jobFinished(toolTip);
}

void PlotTabWidget::plotTableJobFailed(const QString& toolTip) {
  completeTableJob(qobject_cast<PlotTableWidget*>(sender()));
  emitAggregatedProgress();
  emit jobFailed(toolTip);
}

}  // namespace rqt_multiplot
