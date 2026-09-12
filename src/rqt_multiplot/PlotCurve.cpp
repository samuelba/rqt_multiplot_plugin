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

#include <algorithm>

#include <rqt_multiplot/CurveData.h>
#include <rqt_multiplot/CurveDataCircularBuffer.h>
#include <rqt_multiplot/CurveDataList.h>
#include <rqt_multiplot/CurveDataListTimeFrame.h>
#include <rqt_multiplot/CurveDataSequencer.h>
#include <rqt_multiplot/CurveDataVector.h>
#include <rqt_multiplot/PlotWidget.h>

#include <qwt/qwt_plot.h>

#include "rqt_multiplot/PlotCurve.h"

namespace rqt_multiplot {

/*****************************************************************************/
/* Constructors and Destructor                                               */
/*****************************************************************************/

PlotCurve::PlotCurve(QObject* parent)
    : QObject(parent),
      config_(nullptr),
      broker_(nullptr),
      data_(new CurveDataVector()),
      dataSequencer_(new CurveDataSequencer(this)),
      paused_(true),
      snapshotDataBackend_(false) {
  qRegisterMetaType<BoundingRectangle>("BoundingRectangle");
  qRegisterMetaType<QVector<QPointF>>("QVector<QPointF>");

  connect(dataSequencer_, SIGNAL(pointReceived(const QPointF&)), this, SLOT(dataSequencerPointReceived(const QPointF&)));
  connect(dataSequencer_, SIGNAL(seriesReceived(const QVector<QPointF>&)), this, SLOT(dataSequencerSeriesReceived(const QVector<QPointF>&)));

  setData(data_);
}

PlotCurve::~PlotCurve() {
  clearGhosts();
}

/*****************************************************************************/
/* Accessors                                                                 */
/*****************************************************************************/

void PlotCurve::setConfig(CurveConfig* config) {
  if (config != config_) {
    if (config_ != nullptr) {
      disconnect(config_, SIGNAL(changed(const QString&)), this, SLOT(configTitleChanged(const QString&)));
      disconnect(config_->getAxisConfig(CurveConfig::X), SIGNAL(changed()), this, SLOT(configAxisConfigChanged()));
      disconnect(config_->getAxisConfig(CurveConfig::Y), SIGNAL(changed()), this, SLOT(configAxisConfigChanged()));
      disconnect(config_->getColorConfig(), SIGNAL(currentColorChanged(const QColor&)), this,
                 SLOT(configColorConfigCurrentColorChanged(const QColor&)));
      disconnect(config_->getStyleConfig(), SIGNAL(changed()), this, SLOT(configStyleConfigChanged()));
      disconnect(config_->getDataConfig(), SIGNAL(changed()), this, SLOT(configDataConfigChanged()));

      dataSequencer_->setConfig(nullptr);
    }

    config_ = config;

    if (config != nullptr) {
      connect(config, SIGNAL(titleChanged(const QString&)), this, SLOT(configTitleChanged(const QString&)));
      connect(config->getAxisConfig(CurveConfig::X), SIGNAL(changed()), this, SLOT(configAxisConfigChanged()));
      connect(config->getAxisConfig(CurveConfig::Y), SIGNAL(changed()), this, SLOT(configAxisConfigChanged()));
      connect(config->getColorConfig(), SIGNAL(currentColorChanged(const QColor&)), this,
              SLOT(configColorConfigCurrentColorChanged(const QColor&)));
      connect(config->getStyleConfig(), SIGNAL(changed()), this, SLOT(configStyleConfigChanged()));
      connect(config->getDataConfig(), SIGNAL(changed()), this, SLOT(configDataConfigChanged()));

      configTitleChanged(config->getTitle());
      configAxisConfigChanged();
      configColorConfigCurrentColorChanged(config->getColorConfig()->getCurrentColor());
      configStyleConfigChanged();
      configDataConfigChanged();

      dataSequencer_->setConfig(config);
      updateSnapshotHistoryCapacity();
    }
  }
}

CurveConfig* PlotCurve::getConfig() const {
  return config_;
}

void PlotCurve::setBroker(MessageBroker* broker) {
  if (broker != broker_) {
    broker_ = broker;

    dataSequencer_->setBroker(broker);
  }
}

MessageBroker* PlotCurve::getBroker() const {
  return broker_;
}

CurveData* PlotCurve::getData() const {
  return data_;
}

CurveDataSequencer* PlotCurve::getDataSequencer() const {
  return dataSequencer_;
}

QPair<double, double> PlotCurve::getPreferredAxisScale(CurveConfig::Axis axis) const {
  QPair<double, double> axisBounds(0.0, -1.0);

  if (config_ != nullptr) {
    CurveAxisScaleConfig* axisScaleConfig = config_->getAxisConfig(axis)->getScaleConfig();

    if (axisScaleConfig->getType() == CurveAxisScaleConfig::Absolute) {
      axisBounds.first = axisScaleConfig->getAbsoluteMinimum();
      axisBounds.second = axisScaleConfig->getAbsoluteMaximum();
    } else if (axisScaleConfig->getType() == CurveAxisScaleConfig::Relative) {
      if (!data_->isEmpty()) {
        size_t index = data_->getNumPoints() - 1;

        axisBounds.first = data_->getValue(index, axis) + axisScaleConfig->getRelativeMinimum();
        axisBounds.second = data_->getValue(index, axis) + axisScaleConfig->getRelativeMaximum();
      }
    } else {
      axisBounds = data_->getAxisBounds(axis);
      for (const auto& frame : snapshotHistory_.frames()) {
        for (const auto& point : frame) {
          const double value = (axis == CurveConfig::X) ? point.x() : point.y();
          if (axisBounds.first > axisBounds.second) {
            axisBounds.first = value;
            axisBounds.second = value;
          } else {
            axisBounds.first = std::min(axisBounds.first, value);
            axisBounds.second = std::max(axisBounds.second, value);
          }
        }
      }
    }
  }

  return axisBounds;
}

BoundingRectangle PlotCurve::getPreferredScale() const {
  QPair<double, double> xAxisBounds = getPreferredAxisScale(CurveConfig::X);
  QPair<double, double> yAxisBounds = getPreferredAxisScale(CurveConfig::Y);

  return BoundingRectangle(QPointF(xAxisBounds.first, yAxisBounds.first), QPointF(xAxisBounds.second, yAxisBounds.second));
}

/*****************************************************************************/
/* Methods                                                                   */
/*****************************************************************************/

void PlotCurve::attach(QwtPlot* plot) {
  QwtPlotCurve::attach(plot);
  for (auto* ghost : ghosts_) {
    ghost->attach(plot);
  }
}

void PlotCurve::detach() {
  for (auto* ghost : ghosts_) {
    ghost->detach();
  }
  QwtPlotCurve::detach();
}

void PlotCurve::run() {
  CurveAxisConfig* xAxisConfig = config_->getAxisConfig(CurveConfig::X);
  CurveAxisConfig* yAxisConfig = config_->getAxisConfig(CurveConfig::Y);

  if (paused_ && xAxisConfig->hasConfiguredSource() && yAxisConfig->hasConfiguredSource()) {
    dataSequencer_->subscribe();

    paused_ = false;
  }
}

void PlotCurve::pause() {
  if (!paused_) {
    dataSequencer_->unsubscribe();

    paused_ = true;
  }
}

void PlotCurve::clear() {
  data_->clearPoints();
  snapshotHistory_.clear();
  clearGhosts();

  emit replotRequested();
}

QVector<QPointF> PlotCurve::copyPoints(const CurveData& data) {
  QVector<QPointF> points;
  points.reserve(static_cast<int>(data.getNumPoints()));
  for (size_t i = 0; i < data.getNumPoints(); ++i) {
    points.append(data.getPoint(i));
  }
  return points;
}

void PlotCurve::createDataBackend() {
  const bool snapshot = (config_ != nullptr) && CurveDataSequencer::isSnapshotConfig(*config_);
  snapshotDataBackend_ = snapshot;

  if (snapshot || (config_ == nullptr)) {
    data_ = new CurveDataVector();
  } else {
    CurveDataConfig* dataConfig = config_->getDataConfig();
    switch (dataConfig->getType()) {
      case CurveDataConfig::List:
        data_ = new CurveDataList();
        break;
      case CurveDataConfig::CircularBuffer:
        data_ = new CurveDataCircularBuffer(dataConfig->getCircularBufferCapacity());
        break;
      case CurveDataConfig::TimeFrame:
        data_ = new CurveDataListTimeFrame(dataConfig->getTimeFrameLength());
        break;
      case CurveDataConfig::Vector:
      default:
        data_ = new CurveDataVector();
        break;
    }
  }

  setData(data_);
}

void PlotCurve::updateSnapshotHistoryCapacity() {
  const bool snapshot = (config_ != nullptr) && CurveDataSequencer::isSnapshotConfig(*config_);
  const size_t capacity = snapshot ? config_->getStyleConfig()->getFadeHistory() : 0;
  snapshotHistory_.setCapacity(capacity);
  syncGhosts();
}

void PlotCurve::styleGhost(QwtPlotCurve* ghost, size_t age, size_t count) const {
  ghost->setStyle(style());
  ghost->setOrientation(orientation());
  ghost->setBaseline(baseline());
  ghost->setCurveAttribute(QwtPlotCurve::Fitted, testCurveAttribute(QwtPlotCurve::Fitted));
  ghost->setCurveAttribute(QwtPlotCurve::Inverted, testCurveAttribute(QwtPlotCurve::Inverted));
  ghost->setRenderHint(QwtPlotItem::RenderAntialiased, testRenderHint(QwtPlotItem::RenderAntialiased));
  ghost->setZ(z() - static_cast<double>(age));

  QPen ghostPen = pen();
  QColor color = ghostPen.color();
  const int baseAlpha = (color.alpha() > 0) ? color.alpha() : 255;
  color.setAlpha(snapshotFadeAlpha(baseAlpha, age, count));
  ghostPen.setColor(color);
  ghost->setPen(ghostPen);
}

void PlotCurve::restyleGhosts() {
  const size_t count = static_cast<size_t>(snapshotHistory_.frames().size());
  for (int i = 0; i < ghosts_.size(); ++i) {
    styleGhost(ghosts_[i], static_cast<size_t>(i + 1), count);
  }
}

void PlotCurve::clearGhosts() {
  for (auto* ghost : ghosts_) {
    ghost->detach();
    delete ghost;
  }
  ghosts_.clear();
}

void PlotCurve::syncGhosts() {
  const auto& frames = snapshotHistory_.frames();
  while (ghosts_.size() > frames.size()) {
    ghosts_.last()->detach();
    delete ghosts_.takeLast();
  }

  QwtPlot* attachedPlot = plot();
  while (ghosts_.size() < frames.size()) {
    auto* ghost = new QwtPlotCurve();
    ghost->setItemAttribute(QwtPlotItem::Legend, false);
    ghost->setTitle(QString());
    if (attachedPlot != nullptr) {
      ghost->attach(attachedPlot);
    }
    ghosts_.append(ghost);
  }

  const size_t count = static_cast<size_t>(frames.size());
  for (int i = 0; i < frames.size(); ++i) {
    styleGhost(ghosts_[i], static_cast<size_t>(i + 1), count);
    ghosts_[i]->setSamples(frames[i]);
  }
}

/*****************************************************************************/
/* Slots                                                                     */
/*****************************************************************************/

void PlotCurve::configTitleChanged(const QString& title) {
  setTitle(title);
}

void PlotCurve::configAxisConfigChanged() {
  const bool snapshot = (config_ != nullptr) && CurveDataSequencer::isSnapshotConfig(*config_);
  if (snapshot != snapshotDataBackend_) {
    createDataBackend();
  }
  updateSnapshotHistoryCapacity();
  emit preferredScaleChanged(getPreferredScale());
}

void PlotCurve::configColorConfigCurrentColorChanged(const QColor& color) {
  setPen(color);
  restyleGhosts();

  emit replotRequested();
}

void PlotCurve::configStyleConfigChanged() {
  rqt_multiplot::CurveStyleConfig* styleConfig = config_->getStyleConfig();

  if (styleConfig->getType() == rqt_multiplot::CurveStyleConfig::Sticks) {
    setStyle(QwtPlotCurve::Sticks);

    setOrientation(styleConfig->getSticksOrientation());
    setBaseline(styleConfig->getSticksBaseline());
  } else if (styleConfig->getType() == rqt_multiplot::CurveStyleConfig::Steps) {
    setStyle(QwtPlotCurve::Steps);

    setCurveAttribute(QwtPlotCurve::Inverted, styleConfig->areStepsInverted());
  } else if (styleConfig->getType() == rqt_multiplot::CurveStyleConfig::Points) {
    setStyle(QwtPlotCurve::Dots);
  } else {
    setStyle(QwtPlotCurve::Lines);

    setCurveAttribute(QwtPlotCurve::Fitted, styleConfig->areLinesInterpolated());
  }

  QPen pen = QwtPlotCurve::pen();

  pen.setWidth(styleConfig->getPenWidth());
  pen.setStyle(styleConfig->getPenStyle());

  setPen(pen);

  setRenderHint(QwtPlotItem::RenderAntialiased, styleConfig->isRenderAntialiased());
  updateSnapshotHistoryCapacity();

  emit replotRequested();
}

void PlotCurve::configDataConfigChanged() {
  createDataBackend();
  emit replotRequested();
}

void PlotCurve::dataSequencerPointReceived(const QPointF& point) {
  if (!paused_) {
    if (auto* plotWidget = qobject_cast<PlotWidget*>(parent())) {
      if ((config_ != nullptr) && config_->getAxisConfig(CurveConfig::X)->isLabelFromZero()) {
        plotWidget->bindAxisOrigin(CurveConfig::X, point.x());
      }
      if ((config_ != nullptr) && config_->getAxisConfig(CurveConfig::Y)->isLabelFromZero()) {
        plotWidget->bindAxisOrigin(CurveConfig::Y, point.y());
      }
    }

    BoundingRectangle oldBounds = getPreferredScale();

    data_->appendPoint(point);

    BoundingRectangle bounds = getPreferredScale();

    if (bounds != oldBounds) {
      emit preferredScaleChanged(bounds);
    }

    emit replotRequested();
  }
}

void PlotCurve::dataSequencerSeriesReceived(const QVector<QPointF>& points) {
  if (paused_) {
    return;
  }

  BoundingRectangle oldBounds = getPreferredScale();
  snapshotHistory_.push(copyPoints(*data_));
  data_->replacePoints(points);
  syncGhosts();
  BoundingRectangle bounds = getPreferredScale();

  if (bounds != oldBounds) {
    emit preferredScaleChanged(bounds);
  }

  emit replotRequested();
}

}  // namespace rqt_multiplot
