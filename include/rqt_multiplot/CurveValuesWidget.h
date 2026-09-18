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

#ifndef RQT_MULTIPLOT_CURVE_VALUES_WIDGET_H
#define RQT_MULTIPLOT_CURVE_VALUES_WIDGET_H

#include <QColor>
#include <QIcon>
#include <QShowEvent>
#include <QVector>
#include <QWidget>

class QTimer;
class QTreeWidget;
class QTreeWidgetItem;

namespace rqt_multiplot {
class CurveConfig;
class PlotCurve;
class PlotTableWidget;
class PlotWidget;

class CurveValuesWidget : public QWidget {
  Q_OBJECT
 public:
  explicit CurveValuesWidget(QWidget* parent = nullptr);
  ~CurveValuesWidget() override;

  void setPlotTable(PlotTableWidget* plotTable);
  PlotTableWidget* getPlotTable() const;
  void setLiveUpdates(bool enabled);
  bool hasLiveUpdates() const;
  void refresh();

 protected:
  void showEvent(QShowEvent* event) override;

 private:
  struct CurveRow {
    PlotWidget* plot = nullptr;
    PlotCurve* curve = nullptr;
    QTreeWidgetItem* item = nullptr;
  };

  QTreeWidget* tree_;
  PlotTableWidget* plotTable_;
  QTimer* refreshTimer_;
  QVector<CurveRow> rows_;
  bool rebuilding_;

  void rebuild();
  void refreshValues();
  void expandPlotRows();
  void disconnectPlotTable();
  static QIcon colorSwatch(const QColor& color);
  static QString formatAxisValue(PlotWidget* plot, const QPointF& point, bool isX);
  static PlotCurve* curveForConfig(PlotWidget* plot, CurveConfig* curveConfig);

 private slots:
  void plotTableLayoutChanged();
  void curveColorChanged();
  void plotCleared();
};

}  // namespace rqt_multiplot

#endif
