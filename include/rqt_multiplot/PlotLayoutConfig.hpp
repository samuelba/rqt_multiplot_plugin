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

#include <QList>
#include <QString>
#include <QVector>

#include "rqt_multiplot/Config.hpp"
#include "rqt_multiplot/PlotConfig.hpp"

namespace rqt_multiplot {

class PlotLayoutConfig : public Config {
  Q_OBJECT
 public:
  enum Type { Plot, Horizontal, Vertical };

  explicit PlotLayoutConfig(QObject* parent = nullptr, PlotConfig* plotConfig = nullptr);
  ~PlotLayoutConfig() override;

  Type getType() const;
  PlotConfig* getPlotConfig() const;
  const QList<PlotLayoutConfig*>& getChildren() const;
  const QList<int>& getStretch() const;
  void setStretch(const QList<int>& stretch);

  size_t plotCount() const;
  QList<PlotConfig*> plotConfigs() const;
  PlotConfig* plotConfigAt(size_t row, size_t column) const;
  size_t getNumRows() const;
  size_t getNumColumns() const;

  PlotConfig* splitPlot(PlotConfig* plot, Qt::Orientation orientation, bool insertBefore = false);
  bool closePlot(PlotConfig* plot);

  QList<PlotConfig*> detachPlotConfigs();
  void resetToRectangularGrid(size_t numRows, size_t numColumns, QList<PlotConfig*>& preserved);
  void equalizeStretch();

  void save(QSettings& settings) const override;
  void load(QSettings& settings) override;
  void reset() override;

  void write(QDataStream& stream) const override;
  void read(QDataStream& stream) override;

  PlotLayoutConfig& operator=(const PlotLayoutConfig& src);

 signals:
  void structureChanged();

 private:
  Type type_;
  PlotConfig* plotConfig_;
  QList<PlotLayoutConfig*> children_;
  QList<int> stretch_;
  bool closeDonatesToNext_{false};

  PlotLayoutConfig* parentLayout() const;
  PlotLayoutConfig* findLeaf(const PlotConfig* plot);
  PlotConfig* insertPlotSibling(PlotLayoutConfig* sibling, bool insertBefore);
  PlotConfig* convertToSplit(Type splitType, bool insertBefore);
  void addChild(PlotLayoutConfig* child, int stretch = 1, int index = -1);
  void removeChild(PlotLayoutConfig* child);
  void collapseSingleChild();
  void clearContents();
  void adoptPlotConfig(PlotConfig* plotConfig);
  void collectPlotConfigs(QList<PlotConfig*>& plots) const;
  QVector<QVector<PlotConfig*>> rectangularGrid() const;
  static QList<int> reducedStretch(QList<int> stretch);
  static QList<int> parseStretch(const QString& value, int childCount);
  static QString joinStretch(const QList<int>& stretch);
  static int childGroupIndex(const QString& group);
  static QString typeName(Type type);
  static Type typeFromName(const QString& name);

 private slots:
  void childConfigChanged();
  void childStructureChanged();
};

}  // namespace rqt_multiplot
