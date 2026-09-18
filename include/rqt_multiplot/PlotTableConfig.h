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

#ifndef RQT_MULTIPLOT_PLOT_TABLE_CONFIG_H
#define RQT_MULTIPLOT_PLOT_TABLE_CONFIG_H

#include <QColor>
#include <QList>
#include <QString>

#include <rqt_multiplot/Config.h>
#include <rqt_multiplot/PlotConfig.h>
#include <rqt_multiplot/PlotLayoutConfig.h>

namespace rqt_multiplot {
class PlotTableConfig : public Config {
  Q_OBJECT
 public:
  explicit PlotTableConfig(QObject* parent, QColor backgroundColor = Qt::white, QColor foregroundColor = Qt::black, size_t numRows = 1,
                           size_t numColumns = 1, bool linkScale = false, bool linkCursor = false, bool trackPoints = false,
                           QString title = "Tab 1");
  ~PlotTableConfig() override;

  void setTitle(const QString& title);
  const QString& getTitle() const;
  void setBackgroundColor(const QColor& color);
  const QColor& getBackgroundColor() const;
  void setForegroundColor(const QColor& color);
  const QColor& getForegroundColor() const;
  void setNumPlots(size_t numRows, size_t numColumns);
  void setNumRows(size_t numRows);
  size_t getNumRows() const;
  void setNumColumns(size_t numColumns);
  size_t getNumColumns() const;
  PlotConfig* getPlotConfig(size_t row, size_t column) const;
  PlotLayoutConfig* getLayout() const;
  size_t plotCount() const;
  QList<PlotConfig*> plotConfigs() const;
  PlotConfig* splitPlot(PlotConfig* plot, Qt::Orientation orientation, bool insertBefore = false);
  bool closePlot(PlotConfig* plot);
  void setLinkScale(bool link);
  bool isScaleLinked() const;
  void setLinkCursor(bool link);
  bool isCursorLinked() const;
  void setTrackPoints(bool track);
  bool arePointsTracked() const;

  enum TimeAxisFormat { Timestamp = 0, StartFromZero = 1, DateTime = 2 };
  Q_ENUM(TimeAxisFormat)
  void setTimeAxisFormat(TimeAxisFormat format);
  TimeAxisFormat getTimeAxisFormat() const;
  void setTimeAxisStartFromZero(bool enabled);
  bool isTimeAxisStartFromZero() const;
  void setTimeAxisDateTime(bool enabled);
  bool isTimeAxisDateTime() const;
  void setSidebarVisible(bool visible);
  bool isSidebarVisible() const;
  void setSidebarWidth(int width);
  int getSidebarWidth() const;

  static constexpr int kDefaultSidebarWidth = 280;

  void save(QSettings& settings) const override;
  void load(QSettings& settings) override;
  void reset() override;

  void write(QDataStream& stream) const override;
  void read(QDataStream& stream) override;

  PlotTableConfig& operator=(const PlotTableConfig& src);

 signals:
  void titleChanged(const QString& title);
  void backgroundColorChanged(const QColor& color);
  void foregroundColorChanged(const QColor& color);
  void numPlotsChanged(size_t numRows, size_t numColumns);
  void layoutChanged();
  void linkScaleChanged(bool link);
  void linkCursorChanged(bool link);
  void trackPointsChanged(bool track);
  void timeAxisFormatChanged(TimeAxisFormat format);
  void sidebarVisibleChanged(bool visible);
  void sidebarWidthChanged(int width);

 private:
  QString title_;
  QColor backgroundColor_;
  QColor foregroundColor_;
  PlotLayoutConfig* layout_;
  bool linkScale_;
  bool linkCursor_;
  bool trackPoints_;
  TimeAxisFormat timeAxisFormat_;
  bool sidebarVisible_;
  int sidebarWidth_;

  void connectLayout();
  void loadLegacyPlots(QSettings& settings);
  void readLegacyGridStream(QDataStream& stream);
  bool anyXAxisLabelFromZero() const;

 private slots:
  void layoutConfigChanged();
  void layoutStructureChanged();
};
}  // namespace rqt_multiplot

#endif
