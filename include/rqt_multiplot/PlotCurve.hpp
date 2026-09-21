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

#include <optional>

#include <QList>
#include <QObject>
#include <QPair>
#include <QPointF>
#include <QVector>

#include <qwt/qwt_legend_data.h>
#include <qwt/qwt_plot_curve.h>

#include "rqt_multiplot/BoundingRectangle.hpp"
#include "rqt_multiplot/CurveConfig.hpp"
#include "rqt_multiplot/MessageBroker.hpp"
#include "rqt_multiplot/SnapshotHistory.hpp"

namespace rqt_multiplot {

class CurveData;
class CurveDataSequencer;

class PlotCurve : public QObject, public QwtPlotCurve {
  Q_OBJECT
 public:
  explicit PlotCurve(QObject* parent = nullptr);
  ~PlotCurve() override;

  void setConfig(CurveConfig* config);
  CurveConfig* getConfig() const;
  void setPlotTimeWindowLength(std::optional<int> length);
  void setBroker(MessageBroker* broker);
  MessageBroker* getBroker() const;
  CurveData* getData() const;
  CurveDataSequencer* getDataSequencer() const;
  QPair<double, double> getPreferredAxisScale(CurveConfig::Axis axis) const;
  BoundingRectangle getPreferredScale() const;
  void setVisible(bool on) override;
  QList<QwtLegendData> legendData() const override;

  void attach(QwtPlot* plot);
  void detach();

  void run();
  void pause();
  void clear();

 signals:
  void preferredScaleChanged(const BoundingRectangle& bounds);
  void replotRequested();

 private:
  CurveConfig* config_;

  MessageBroker* broker_;

  CurveData* data_;
  CurveDataSequencer* dataSequencer_;
  SnapshotHistory snapshotHistory_;
  QList<QwtPlotCurve*> ghosts_;

  bool paused_;
  bool snapshotDataBackend_;
  std::optional<int> plotTimeWindowLength_;
  CurveAxisConfig::UnitConversion appliedUnitConversion_[2];

  void createDataBackend();
  void rescaleStoredAxis(CurveConfig::Axis axis, double factor);
  void syncAppliedUnitConversions();
  void updateSnapshotHistoryCapacity();
  void syncGhosts();
  void restyleGhosts();
  void styleGhost(QwtPlotCurve* ghost, size_t age, size_t count) const;
  void clearGhosts();
  static QVector<QPointF> copyPoints(const CurveData& data);

 private slots:
  void configTitleChanged(const QString& title);
  void configAxisConfigChanged();
  void configColorConfigCurrentColorChanged(const QColor& color);
  void configStyleConfigChanged();
  void configDataConfigChanged();

  void dataSequencerPointReceived(const QPointF& point);
  void dataSequencerSeriesReceived(const QVector<QPointF>& points);
};

}  // namespace rqt_multiplot
