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

#include <QColorDialog>
#include <QFileDialog>

#include <rqt_multiplot/PackageResource.h>
#include <rqt_multiplot/PlotExport.h>

#include <rqt_multiplot/PlotTabWidget.h>
#include <rqt_multiplot/PlotTableWidget.h>
#include <rqt_multiplot/PlotWidget.h>

#include <ui_PlotTableConfigWidget.h>

#include "rqt_multiplot/PlotTableConfigWidget.h"

namespace rqt_multiplot {

/*****************************************************************************/
/* Constructors and Destructor                                               */
/*****************************************************************************/

PlotTableConfigWidget::PlotTableConfigWidget(QWidget* parent)
    : QWidget(parent),
      ui_(new Ui::PlotTableConfigWidget()),
      menuImportExport_(new QMenu(this)),
      config_(nullptr),
      plotTabs_(nullptr),
      plotTable_(nullptr),
      playbackJobCount_(0),
      lastJobFailure_() {
  ui_->setupUi(this);

  ui_->labelBackgroundColor->setAutoFillBackground(true);
  ui_->labelForegroundColor->setAutoFillBackground(true);

  ui_->widgetProgress->setEnabled(false);

  ui_->pushButtonRun->setIcon(QIcon(packageResourcePath("resource/16x16/run.png")));
  ui_->pushButtonPause->setIcon(QIcon(packageResourcePath("resource/16x16/pause.png")));
  ui_->pushButtonClear->setIcon(QIcon(packageResourcePath("resource/16x16/clear.png")));
  ui_->pushButtonImportExport->setIcon(QIcon(packageResourcePath("resource/16x16/eject.png")));

  ui_->pushButtonPause->setEnabled(false);

  menuImportExport_->addAction("Import from bag file...", this, SLOT(menuImportBagFileTriggered()));
  menuImportExport_->addAction("Import from bag directory...", this, SLOT(menuImportBagDirectoryTriggered()));
  menuImportExport_->addSeparator();
  menuImportExport_->addAction("Export to image file...", this, SLOT(menuExportImageFileTriggered()));
  menuImportExport_->addAction("Export to text file...", this, SLOT(menuExportTextFileTriggered()));

  connect(ui_->checkBoxLinkScale, SIGNAL(stateChanged(int)), this, SLOT(checkBoxLinkScaleStateChanged(int)));
  connect(ui_->checkBoxLinkCursor, SIGNAL(stateChanged(int)), this, SLOT(checkBoxLinkCursorStateChanged(int)));
  connect(ui_->checkBoxTrackPoints, SIGNAL(stateChanged(int)), this, SLOT(checkBoxTrackPointsStateChanged(int)));

  connect(ui_->pushButtonRun, SIGNAL(clicked()), this, SLOT(pushButtonRunClicked()));
  connect(ui_->pushButtonPause, SIGNAL(clicked()), this, SLOT(pushButtonPauseClicked()));
  connect(ui_->pushButtonClear, SIGNAL(clicked()), this, SLOT(pushButtonClearClicked()));
  connect(ui_->pushButtonImportExport, SIGNAL(clicked()), this, SLOT(pushButtonImportExportClicked()));

  ui_->labelBackgroundColor->installEventFilter(this);
  ui_->labelForegroundColor->installEventFilter(this);
}

PlotTableConfigWidget::~PlotTableConfigWidget() {
  delete ui_;
}

/*****************************************************************************/
/* Accessors                                                                 */
/*****************************************************************************/

void PlotTableConfigWidget::setConfig(PlotTableConfig* config) {
  if (config != config_) {
    if (config_ != nullptr) {
      disconnect(config_, SIGNAL(backgroundColorChanged(const QColor&)), this, SLOT(configBackgroundColorChanged(const QColor&)));
      disconnect(config_, SIGNAL(foregroundColorChanged(const QColor&)), this, SLOT(configForegroundColorChanged(const QColor&)));
      disconnect(config_, SIGNAL(linkScaleChanged(bool)), this, SLOT(configLinkScaleChanged(bool)));
      disconnect(config_, SIGNAL(linkCursorChanged(bool)), this, SLOT(configLinkCursorChanged(bool)));
      disconnect(config_, SIGNAL(trackPointsChanged(bool)), this, SLOT(configTrackPointsChanged(bool)));
    }

    config_ = config;

    if (config != nullptr) {
      connect(config, SIGNAL(backgroundColorChanged(const QColor&)), this, SLOT(configBackgroundColorChanged(const QColor&)));
      connect(config, SIGNAL(foregroundColorChanged(const QColor&)), this, SLOT(configForegroundColorChanged(const QColor&)));
      connect(config, SIGNAL(linkScaleChanged(bool)), this, SLOT(configLinkScaleChanged(bool)));
      connect(config, SIGNAL(linkCursorChanged(bool)), this, SLOT(configLinkCursorChanged(bool)));
      connect(config, SIGNAL(trackPointsChanged(bool)), this, SLOT(configTrackPointsChanged(bool)));

      configBackgroundColorChanged(config->getBackgroundColor());
      configForegroundColorChanged(config->getForegroundColor());
      configLinkScaleChanged(config_->isScaleLinked());
      configLinkCursorChanged(config_->isCursorLinked());
      configTrackPointsChanged(config_->arePointsTracked());
    }
  }
}

PlotTableConfig* PlotTableConfigWidget::getConfig() const {
  return config_;
}

void PlotTableConfigWidget::setPlotTabs(PlotTabWidget* plotTabs) {
  if (plotTabs != plotTabs_) {
    unbindPlaybackSignals();
    plotTabs_ = plotTabs;
    bindPlaybackSignals();
  }
}

PlotTabWidget* PlotTableConfigWidget::getPlotTabs() const {
  return plotTabs_;
}

void PlotTableConfigWidget::setPlotTable(PlotTableWidget* plotTable) {
  if (plotTable == plotTable_) {
    return;
  }

  if (plotTabs_ != nullptr) {
    plotTable_ = plotTable;
    return;
  }

  unbindPlaybackSignals();
  plotTable_ = plotTable;
  bindPlaybackSignals();
}

PlotTableWidget* PlotTableConfigWidget::getPlotTableWidget() const {
  return plotTable_;
}

void PlotTableConfigWidget::runPlots() {
  if (plotTabs_ != nullptr) {
    plotTabs_->runPlots();
  } else if (plotTable_ != nullptr) {
    plotTable_->runPlots();
  }
}

void PlotTableConfigWidget::unbindPlaybackSignals() {
  if (plotTabs_ != nullptr) {
    disconnect(plotTabs_, SIGNAL(plotPausedChanged()), this, SLOT(plotTablePlotPausedChanged()));
    disconnect(plotTabs_, SIGNAL(jobStarted(const QString&)), this, SLOT(plotTableJobStarted(const QString&)));
    disconnect(plotTabs_, SIGNAL(jobProgressChanged(double)), this, SLOT(plotTableJobProgressChanged(double)));
    disconnect(plotTabs_, SIGNAL(jobFinished(const QString&)), this, SLOT(plotTableJobFinished(const QString&)));
    disconnect(plotTabs_, SIGNAL(jobFailed(const QString&)), this, SLOT(plotTableJobFailed(const QString&)));
  } else if (plotTable_ != nullptr) {
    disconnect(plotTable_, SIGNAL(plotPausedChanged()), this, SLOT(plotTablePlotPausedChanged()));
    disconnect(plotTable_, SIGNAL(jobStarted(const QString&)), this, SLOT(plotTableJobStarted(const QString&)));
    disconnect(plotTable_, SIGNAL(jobProgressChanged(double)), this, SLOT(plotTableJobProgressChanged(double)));
    disconnect(plotTable_, SIGNAL(jobFinished(const QString&)), this, SLOT(plotTableJobFinished(const QString&)));
    disconnect(plotTable_, SIGNAL(jobFailed(const QString&)), this, SLOT(plotTableJobFailed(const QString&)));
  }

  playbackJobCount_ = 0;
  lastJobFailure_.clear();
}

void PlotTableConfigWidget::bindPlaybackSignals() {
  if (plotTabs_ != nullptr) {
    connect(plotTabs_, SIGNAL(plotPausedChanged()), this, SLOT(plotTablePlotPausedChanged()));
    connect(plotTabs_, SIGNAL(jobStarted(const QString&)), this, SLOT(plotTableJobStarted(const QString&)));
    connect(plotTabs_, SIGNAL(jobProgressChanged(double)), this, SLOT(plotTableJobProgressChanged(double)));
    connect(plotTabs_, SIGNAL(jobFinished(const QString&)), this, SLOT(plotTableJobFinished(const QString&)));
    connect(plotTabs_, SIGNAL(jobFailed(const QString&)), this, SLOT(plotTableJobFailed(const QString&)));
  } else if (plotTable_ != nullptr) {
    connect(plotTable_, SIGNAL(plotPausedChanged()), this, SLOT(plotTablePlotPausedChanged()));
    connect(plotTable_, SIGNAL(jobStarted(const QString&)), this, SLOT(plotTableJobStarted(const QString&)));
    connect(plotTable_, SIGNAL(jobProgressChanged(double)), this, SLOT(plotTableJobProgressChanged(double)));
    connect(plotTable_, SIGNAL(jobFinished(const QString&)), this, SLOT(plotTableJobFinished(const QString&)));
    connect(plotTable_, SIGNAL(jobFailed(const QString&)), this, SLOT(plotTableJobFailed(const QString&)));
  }

  if (plotTabs_ != nullptr || plotTable_ != nullptr) {
    plotTablePlotPausedChanged();
  }
}

/*****************************************************************************/
/* Methods                                                                   */
/*****************************************************************************/

bool PlotTableConfigWidget::eventFilter(QObject* object, QEvent* event) {
  if (config_ != nullptr) {
    if (((object == ui_->labelBackgroundColor) || (object == ui_->labelForegroundColor)) && (event->type() == QEvent::MouseButtonPress)) {
      QColorDialog dialog(this);

      dialog.setCurrentColor((object == ui_->labelBackgroundColor) ? config_->getBackgroundColor() : config_->getForegroundColor());

      if (dialog.exec() == QDialog::Accepted) {
        if (object == ui_->labelBackgroundColor) {
          config_->setBackgroundColor(dialog.currentColor());
        } else {
          config_->setForegroundColor(dialog.currentColor());
        }
      }
    }
  }

  return false;
}

/*****************************************************************************/
/* Slots                                                                     */
/*****************************************************************************/

void PlotTableConfigWidget::configBackgroundColorChanged(const QColor& color) {
  QPalette palette = ui_->labelBackgroundColor->palette();
  palette.setColor(QPalette::Window, color);

  ui_->labelBackgroundColor->setPalette(palette);
}

void PlotTableConfigWidget::configForegroundColorChanged(const QColor& color) {
  QPalette palette = ui_->labelForegroundColor->palette();
  palette.setColor(QPalette::Window, color);

  ui_->labelForegroundColor->setPalette(palette);
}

void PlotTableConfigWidget::configLinkScaleChanged(bool link) {
  ui_->checkBoxLinkScale->setCheckState(link ? Qt::Checked : Qt::Unchecked);
}

void PlotTableConfigWidget::configLinkCursorChanged(bool link) {
  ui_->checkBoxLinkCursor->setCheckState(link ? Qt::Checked : Qt::Unchecked);
}

void PlotTableConfigWidget::configTrackPointsChanged(bool track) {
  ui_->checkBoxTrackPoints->setCheckState(track ? Qt::Checked : Qt::Unchecked);
}

void PlotTableConfigWidget::checkBoxLinkScaleStateChanged(int state) {
  if (config_ != nullptr) {
    config_->setLinkScale(state == Qt::Checked);
  }
}

void PlotTableConfigWidget::checkBoxLinkCursorStateChanged(int state) {
  if (config_ != nullptr) {
    config_->setLinkCursor(state == Qt::Checked);
  }
}

void PlotTableConfigWidget::checkBoxTrackPointsStateChanged(int state) {
  if (config_ != nullptr) {
    config_->setTrackPoints(state == Qt::Checked);
  }
}

void PlotTableConfigWidget::pushButtonRunClicked() {
  runPlots();
}

void PlotTableConfigWidget::pushButtonPauseClicked() {
  if (plotTabs_ != nullptr) {
    plotTabs_->pausePlots();
  } else if (plotTable_ != nullptr) {
    plotTable_->pausePlots();
  }
}

void PlotTableConfigWidget::pushButtonClearClicked() {
  if (plotTabs_ != nullptr) {
    plotTabs_->clearPlots();
  } else if (plotTable_ != nullptr) {
    plotTable_->clearPlots();
  }
}

void PlotTableConfigWidget::pushButtonImportExportClicked() {
  menuImportExport_->popup(QCursor::pos());
}

void PlotTableConfigWidget::menuImportBagFileTriggered() {
  QFileDialog dialog(this, "Open Bag File", QDir::homePath(), "ROS 2 bags (*.mcap *.db3);;All files (*)");

  dialog.setAcceptMode(QFileDialog::AcceptOpen);
  dialog.setFileMode(QFileDialog::ExistingFile);

  if (dialog.exec() == QDialog::Accepted) {
    const auto files = dialog.selectedFiles();
    if (!files.isEmpty()) {
      if (plotTabs_ != nullptr) {
        plotTabs_->loadFromBagFile(files.first());
      } else if (plotTable_ != nullptr) {
        plotTable_->loadFromBagFile(files.first());
      }
    }
  }
}

void PlotTableConfigWidget::menuImportBagDirectoryTriggered() {
  QFileDialog dialog(this, "Open Bag Directory", QDir::homePath());

  dialog.setAcceptMode(QFileDialog::AcceptOpen);
  dialog.setFileMode(QFileDialog::Directory);
  dialog.setOption(QFileDialog::ShowDirsOnly);

  if (dialog.exec() == QDialog::Accepted) {
    const auto files = dialog.selectedFiles();
    if (!files.isEmpty()) {
      if (plotTabs_ != nullptr) {
        plotTabs_->loadFromBagFile(files.first());
      } else if (plotTable_ != nullptr) {
        plotTable_->loadFromBagFile(files.first());
      }
    }
  }
}

void PlotTableConfigWidget::menuExportImageFileTriggered() {
  QFileDialog dialog(this, "Save Image File", QDir::homePath(),
                     "Portable Network Graphics (*.png);;Scalable Vector Graphics (*.svg);;Portable Document Format (*.pdf)");

  dialog.setAcceptMode(QFileDialog::AcceptSave);
  dialog.setFileMode(QFileDialog::AnyFile);
  dialog.selectFile("rqt_multiplot.png");

  if ((dialog.exec() == QDialog::Accepted) && (plotTable_ != nullptr)) {
    const auto files = dialog.selectedFiles();
    if (!files.isEmpty()) {
      plotTable_->saveToImageFile(ensureFileSuffix(files.first(), suffixFromNameFilter(dialog.selectedNameFilter())));
    }
  }
}

void PlotTableConfigWidget::menuExportTextFileTriggered() {
  QFileDialog dialog(this, "Save Text File", QDir::homePath(), "Text file (*.txt);;CSV (*.csv)");

  dialog.setAcceptMode(QFileDialog::AcceptSave);
  dialog.setFileMode(QFileDialog::AnyFile);
  dialog.selectFile("rqt_multiplot.txt");

  if ((dialog.exec() == QDialog::Accepted) && (plotTable_ != nullptr)) {
    const auto files = dialog.selectedFiles();
    if (!files.isEmpty()) {
      plotTable_->saveToTextFile(ensureFileSuffix(files.first(), suffixFromNameFilter(dialog.selectedNameFilter())));
    }
  }
}

void PlotTableConfigWidget::plotTablePlotPausedChanged() {
  bool allPlotsPaused = true;
  bool anyPlotPaused = false;
  bool hasPlots = false;

  auto accumulate = [&](PlotTableWidget* plotTable) {
    if (plotTable == nullptr) {
      return;
    }

    for (PlotWidget* plot : plotTable->getPlotWidgets()) {
      if (plot == nullptr) {
        continue;
      }

      hasPlots = true;
      allPlotsPaused &= plot->isPaused();
      anyPlotPaused |= plot->isPaused();
    }
  };

  if (plotTabs_ != nullptr) {
    for (size_t index = 0; index < plotTabs_->getNumPlotTables(); ++index) {
      accumulate(plotTabs_->getPlotTable(index));
    }
  } else {
    accumulate(plotTable_);
  }

  ui_->pushButtonRun->setEnabled(hasPlots && anyPlotPaused);
  ui_->pushButtonPause->setEnabled(hasPlots && !allPlotsPaused);
}

void PlotTableConfigWidget::plotTableJobStarted(const QString& toolTip) {
  ++playbackJobCount_;
  ui_->widgetProgress->setEnabled(true);
  ui_->widgetProgress->start(toolTip);
}

void PlotTableConfigWidget::plotTableJobProgressChanged(double progress) {
  ui_->widgetProgress->setCurrentProgress(progress);
}

void PlotTableConfigWidget::plotTableJobFinished(const QString& toolTip) {
  completePlaybackJob(toolTip, false);
}

void PlotTableConfigWidget::plotTableJobFailed(const QString& toolTip) {
  completePlaybackJob(toolTip, true);
}

void PlotTableConfigWidget::completePlaybackJob(const QString& toolTip, bool failed) {
  if (playbackJobCount_ > 0) {
    --playbackJobCount_;
  }

  if (failed) {
    lastJobFailure_ = toolTip;
  }

  if (playbackJobCount_ > 0) {
    return;
  }

  if (!lastJobFailure_.isEmpty()) {
    ui_->widgetProgress->fail(lastJobFailure_);
    lastJobFailure_.clear();
  } else {
    ui_->widgetProgress->finish(toolTip);
  }
}

}  // namespace rqt_multiplot
