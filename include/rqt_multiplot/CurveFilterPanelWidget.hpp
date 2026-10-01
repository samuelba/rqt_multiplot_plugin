/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <QMetaObject>
#include <QPointer>
#include <QVector>
#include <QWidget>

#include "rqt_multiplot/CurveFilter.hpp"

class QListWidget;
class QLabel;
class QListWidgetItem;
class QPushButton;
class QTreeWidget;

namespace rqt_multiplot {

class CurveConfig;
class CurveFilterChainWidget;
class PlotConfig;
class PlotTableConfig;
class PlotCurve;
class PlotTableWidget;

class CurveFilterPanelWidget : public QWidget {
  Q_OBJECT
 public:
  explicit CurveFilterPanelWidget(QWidget* parent = nullptr);
  ~CurveFilterPanelWidget() override;

  CurveConfig* getCurrentCurve() const;
  void setCurrentCurve(CurveConfig* curveConfig);

 public slots:
  void setPlotTable(PlotTableWidget* plotTable);
  void selectPlot(PlotConfig* plotConfig);

 private:
  struct CurveRow {
    QPointer<PlotConfig> plot;
    QPointer<CurveConfig> curve;
  };

  QListWidget* palette_;
  QTreeWidget* curveTree_;
  CurveFilterChainWidget* chainWidget_;
  QLabel* chainHeading_;
  QPushButton* copyButton_;
  QPointer<PlotTableWidget> plotTable_;
  QPointer<PlotTableConfig> tableConfig_;
  QVector<CurveRow> rows_;
  QVector<QMetaObject::Connection> connections_;

  void rebuild();
  void disconnectConfigs();
  void watch(QObject* sender, const char* signal);
  int currentRow() const;
  void updateSelection();
  void addFilterToCurrentCurve(CurveFilterType type);
  void addFilteredCopy();
  PlotCurve* findPlotCurve(const PlotConfig* plotConfig, const CurveConfig* curveConfig) const;

 private slots:
  void configStructureChanged();
};

}  // namespace rqt_multiplot
