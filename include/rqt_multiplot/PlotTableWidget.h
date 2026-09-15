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

#ifndef RQT_MULTIPLOT_PLOT_TABLE_WIDGET_H
#define RQT_MULTIPLOT_PLOT_TABLE_WIDGET_H

#include <QHash>
#include <QList>
#include <QPainter>
#include <QRectF>
#include <QVBoxLayout>
#include <QWidget>

#include <rqt_multiplot/BagReader.h>
#include <rqt_multiplot/BoundingRectangle.h>
#include <rqt_multiplot/MessageSubscriberRegistry.h>
#include <rqt_multiplot/PlotTableConfig.h>

class QShowEvent;
class QSplitter;

namespace rqt_multiplot {
class PlotLayoutConfig;
class PlotWidget;

class PlotTableWidget : public QWidget {
  Q_OBJECT
 public:
  explicit PlotTableWidget(QWidget* parent = nullptr);
  ~PlotTableWidget() override;

  void setConfig(PlotTableConfig* config);
  PlotTableConfig* getConfig() const;
  size_t getNumRows() const;
  size_t getNumColumns() const;
  size_t getNumPlots() const;
  PlotWidget* getPlotWidget(size_t row, size_t column) const;
  const QList<PlotWidget*>& getPlotWidgets() const;
  MessageSubscriberRegistry* getRegistry() const;
  BagReader* getBagReader() const;

  void runPlots();
  void pausePlots();
  void clearPlots();

  void requestReplot();
  void forceReplot();

  void renderToPainter(QPainter& painter, const QRectF& bounds = QRectF());
  void renderToPixmap(QPixmap& pixmap);
  void writeFormattedCurveAxisTitles(QStringList& formattedAxisTitles);
  void writeFormattedCurveData(QList<QStringList>& formattedData);

  void loadFromBagFile(const QString& fileName);
  void saveToImageFile(const QString& fileName);
  void saveToTextFile(const QString& fileName);
  void storeSplitterRatios();

 signals:
  void plotPausedChanged();
  void jobStarted(const QString& toolTip);
  void jobProgressChanged(double progress);
  void jobFinished(const QString& toolTip);
  void jobFailed(const QString& toolTip);

 protected:
  void showEvent(QShowEvent* event) override;

 private:
  QVBoxLayout* layout_;
  QWidget* rootWidget_;
  QList<PlotWidget*> plotWidgets_;
  QHash<QSplitter*, PlotLayoutConfig*> splitterNodes_;

  PlotTableConfig* config_;

  MessageSubscriberRegistry* registry_;
  BagReader* bagReader_;

  void updatePlotScale(const BoundingRectangle& bounds, PlotWidget* excluded = nullptr);
  bool anyPlotUserScaleLocked() const;
  void rebuildLayout();
  QWidget* createNodeWidget(PlotLayoutConfig* node, QHash<PlotConfig*, PlotWidget*>& existing);
  PlotWidget* createPlotWidget();
  void connectPlotWidget(PlotWidget* plot);
  static void applyStretch(QSplitter* splitter, PlotLayoutConfig* node);
  void applyAllStretch();
  void updatePlotControls();

 private slots:
  void configBackgroundColorChanged(const QColor& color);
  void configForegroundColorChanged(const QColor& color);
  void configLayoutChanged();
  void configLinkScaleChanged(bool link);
  void configTrackPointsChanged(bool track);

  void plotPreferredScaleChanged(const BoundingRectangle& bounds);
  void plotCurrentScaleChanged(const BoundingRectangle& bounds);
  void plotUserScaleLockedChanged(bool locked);
  void plotCursorActiveChanged(bool active);
  void plotCursorCurrentPositionChanged(const QPointF& position);
  void plotPausedChanged(bool paused);
  void plotStateChanged(int state);
  void plotSplitRequested(Qt::Orientation orientation, bool insertBefore);
  void plotCloseRequested();
  void splitterMoved(int pos, int index);

  void bagReaderReadingStarted();
  void bagReaderReadingProgressChanged(double progress);
  void bagReaderReadingFinished();
  void bagReaderReadingFailed(const QString& error);
};
}  // namespace rqt_multiplot

#endif
