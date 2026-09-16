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

#ifndef RQT_MULTIPLOT_PLOT_TABLE_CONFIG_WIDGET_H
#define RQT_MULTIPLOT_PLOT_TABLE_CONFIG_WIDGET_H

#include <QAction>
#include <QWidget>

#include <rqt_multiplot/PlotTableConfig.h>

namespace Ui {
class PlotTableConfigWidget;
}

namespace rqt_multiplot {
class PlotTableWidget;
class PlotTabWidget;

class PlotTableConfigWidget : public QWidget {
  Q_OBJECT
 public:
  explicit PlotTableConfigWidget(QWidget* parent = nullptr);
  ~PlotTableConfigWidget() override;

  void setConfig(PlotTableConfig* config);
  PlotTableConfig* getConfig() const;
  void setPlotTabs(PlotTabWidget* plotTabs);
  PlotTabWidget* getPlotTabs() const;
  void setPlotTable(PlotTableWidget* plotTable);
  PlotTableWidget* getPlotTableWidget() const;
  void runPlots();

  QAction* getActionImportBagFile() const;
  QAction* getActionImportBagDirectory() const;
  QAction* getActionExportImageFile() const;
  QAction* getActionExportTextFile() const;

 protected:
  bool eventFilter(QObject* object, QEvent* event) override;

 private:
  Ui::PlotTableConfigWidget* ui_;

  QAction* actionImportBagFile_;
  QAction* actionImportBagDirectory_;
  QAction* actionExportImageFile_;
  QAction* actionExportTextFile_;

  PlotTableConfig* config_;
  PlotTabWidget* plotTabs_;
  PlotTableWidget* plotTable_;
  int playbackJobCount_;
  QString lastJobFailure_;

  void unbindPlaybackSignals();
  void bindPlaybackSignals();
  void completePlaybackJob(const QString& toolTip, bool failed);

 private slots:
  void configBackgroundColorChanged(const QColor& color);
  void configForegroundColorChanged(const QColor& color);
  void configLinkScaleChanged(bool link);
  void configLinkCursorChanged(bool link);
  void configTrackPointsChanged(bool track);

  void checkBoxLinkScaleStateChanged(int state);
  void checkBoxLinkCursorStateChanged(int state);
  void checkBoxTrackPointsStateChanged(int state);

  void pushButtonRunClicked();
  void pushButtonPauseClicked();
  void pushButtonClearClicked();
  void menuImportBagFileTriggered();
  void menuImportBagDirectoryTriggered();
  void menuExportImageFileTriggered();
  void menuExportTextFileTriggered();

  void plotTablePlotPausedChanged();
  void plotTableJobStarted(const QString& toolTip);
  void plotTableJobProgressChanged(double progress);
  void plotTableJobFinished(const QString& toolTip);
  void plotTableJobFailed(const QString& toolTip);
};
}  // namespace rqt_multiplot

#endif
