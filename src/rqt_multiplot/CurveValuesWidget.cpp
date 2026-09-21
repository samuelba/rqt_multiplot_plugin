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

#include "rqt_multiplot/CurveValuesWidget.hpp"

#include <algorithm>
#include <cmath>

#include <QColor>
#include <QFont>
#include <QHeaderView>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QSize>
#include <QTimeZone>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "rqt_multiplot/AxisTimeFormat.hpp"
#include "rqt_multiplot/BoundingRectangle.hpp"
#include "rqt_multiplot/CurveColorConfig.hpp"
#include "rqt_multiplot/CurveConfig.hpp"
#include "rqt_multiplot/CurveData.hpp"
#include "rqt_multiplot/PlotConfig.hpp"
#include "rqt_multiplot/PlotCursor.hpp"
#include "rqt_multiplot/PlotCurve.hpp"
#include "rqt_multiplot/PlotTableConfig.hpp"
#include "rqt_multiplot/PlotTableWidget.hpp"
#include "rqt_multiplot/PlotWidget.hpp"

namespace rqt_multiplot {

namespace {

constexpr int kSwatchSize = 10;
constexpr int kGroupGapPx = 10;
constexpr int kHeadingMarginPx = 8;
constexpr int kHeadingInnerMarginPx = 4;
constexpr int kHeadingSpacingPx = 6;
constexpr int kRefreshIntervalMs = 100;
constexpr double kLiveReadoutMaxSpan = 0.1;
constexpr Qt::Alignment kValueAlignment = Qt::AlignRight | Qt::AlignVCenter;

}  // namespace

CurveValuesWidget::CurveValuesWidget(QWidget* parent)
    : QWidget(parent), tree_(new QTreeWidget(this)), plotTable_(nullptr), refreshTimer_(new QTimer(this)), rebuilding_(false) {
  setObjectName(QStringLiteral("curveValuesWidget"));

  auto* heading = new QLabel(tr("Curve values"), this);
  heading->setObjectName(QStringLiteral("curveValuesHeading"));
  QFont headingFont = heading->font();
  headingFont.setBold(true);
  heading->setFont(headingFont);

  tree_->setObjectName(QStringLiteral("curveValuesTree"));
  tree_->setColumnCount(3);
  tree_->setHeaderHidden(true);
  tree_->setRootIsDecorated(false);
  tree_->setItemsExpandable(false);
  tree_->setExpandsOnDoubleClick(false);
  tree_->setIndentation(16);
  tree_->setUniformRowHeights(false);
  tree_->setFrameShape(QFrame::NoFrame);
  tree_->setSelectionMode(QAbstractItemView::NoSelection);
  tree_->setFocusPolicy(Qt::NoFocus);
  tree_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  tree_->setAnimated(false);

  tree_->header()->setStretchLastSection(false);
  tree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
  tree_->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  tree_->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(kHeadingMarginPx, kHeadingMarginPx, kHeadingInnerMarginPx, kHeadingInnerMarginPx);
  layout->setSpacing(kHeadingSpacingPx);
  layout->addWidget(heading);
  layout->addWidget(tree_, 1);

  refreshTimer_->setInterval(kRefreshIntervalMs);
  connect(refreshTimer_, &QTimer::timeout, this, [this]() { refreshValues(); });
}

CurveValuesWidget::~CurveValuesWidget() {
  plotTable_ = nullptr;
  rows_.clear();
}

void CurveValuesWidget::setPlotTable(PlotTableWidget* plotTable) {
  disconnectPlotTable();
  plotTable_ = plotTable;
  if ((plotTable_ != nullptr) && (plotTable_->getConfig() != nullptr)) {
    connect(plotTable_->getConfig(), &PlotTableConfig::layoutChanged, this, &CurveValuesWidget::plotTableLayoutChanged,
            Qt::UniqueConnection);
  }
  rebuild();
}

PlotTableWidget* CurveValuesWidget::getPlotTable() const {
  return plotTable_;
}

void CurveValuesWidget::setLiveUpdates(bool enabled) {
  if (enabled) {
    if (!refreshTimer_->isActive()) {
      refreshTimer_->start();
    }
    rebuild();
    expandPlotRows();
    return;
  }

  refreshTimer_->stop();
}

bool CurveValuesWidget::hasLiveUpdates() const {
  return refreshTimer_->isActive();
}

void CurveValuesWidget::refresh() {
  rebuild();
}

void CurveValuesWidget::rebuild() {
  if (rebuilding_) {
    return;
  }
  rebuilding_ = true;
  tree_->clear();
  rows_.clear();

  if (plotTable_ == nullptr) {
    rebuilding_ = false;
    return;
  }

  for (PlotWidget* plot : plotTable_->getPlotWidgets()) {
    if (plot == nullptr) {
      continue;
    }

    connect(plot, &PlotWidget::cleared, this, &CurveValuesWidget::plotCleared, Qt::UniqueConnection);

    PlotConfig* plotConfig = plot->getConfig();
    auto* plotItem = new QTreeWidgetItem(tree_);
    plotItem->setText(0, plotConfig != nullptr ? plotConfig->getTitle() : QString());
    QFont titleFont = plotItem->font(0);
    titleFont.setBold(true);
    plotItem->setFont(0, titleFont);
    plotItem->setFlags(Qt::ItemIsEnabled);

    if (plotConfig != nullptr) {
      connect(plotConfig, &PlotConfig::titleChanged, this, &CurveValuesWidget::plotTableLayoutChanged, Qt::UniqueConnection);
      connect(plotConfig, &PlotConfig::curveAdded, this, &CurveValuesWidget::plotTableLayoutChanged, Qt::UniqueConnection);
      connect(plotConfig, &PlotConfig::curveRemoved, this, &CurveValuesWidget::plotTableLayoutChanged, Qt::UniqueConnection);
      connect(plotConfig, &PlotConfig::curvesCleared, this, &CurveValuesWidget::plotTableLayoutChanged, Qt::UniqueConnection);

      for (size_t index = 0; index < plotConfig->getNumCurves(); ++index) {
        CurveConfig* curveConfig = plotConfig->getCurveConfig(index);
        if (curveConfig == nullptr) {
          continue;
        }

        PlotCurve* curve = curveForConfig(plot, curveConfig);
        auto* curveItem = new QTreeWidgetItem(plotItem);
        curveItem->setText(0, curveConfig->getTitle());
        curveItem->setTextAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
        curveItem->setTextAlignment(1, kValueAlignment);
        curveItem->setTextAlignment(2, kValueAlignment);
        curveItem->setFlags(Qt::ItemIsEnabled);

        if (curveConfig->getColorConfig() != nullptr) {
          curveItem->setIcon(0, colorSwatch(curveConfig->getColorConfig()->getCurrentColor()));
          connect(curveConfig->getColorConfig(), &CurveColorConfig::currentColorChanged, this, &CurveValuesWidget::curveColorChanged,
                  Qt::UniqueConnection);
        }
        connect(curveConfig, &CurveConfig::titleChanged, this, &CurveValuesWidget::plotTableLayoutChanged, Qt::UniqueConnection);

        rows_.push_back(CurveRow{plot, curve, curveItem});
      }
    }

    auto* gapItem = new QTreeWidgetItem(plotItem);
    gapItem->setFlags(Qt::NoItemFlags);
    gapItem->setSizeHint(0, QSize(0, kGroupGapPx));
    plotItem->setExpanded(true);
  }

  expandPlotRows();
  refreshValues();
  rebuilding_ = false;
}

void CurveValuesWidget::refreshValues() {
  for (const CurveRow& row : rows_) {
    if ((row.item == nullptr) || (row.curve == nullptr) || (row.plot == nullptr)) {
      continue;
    }

    CurveData* data = row.curve->getData();
    if ((data == nullptr) || data->isEmpty()) {
      row.item->setText(1, QString());
      row.item->setText(2, QString());
      continue;
    }

    const QPointF point = data->getPoint(data->getNumPoints() - 1);
    row.item->setText(1, formatAxisValue(row.plot, point, true));
    row.item->setText(2, formatAxisValue(row.plot, point, false));
  }
}

void CurveValuesWidget::expandPlotRows() {
  tree_->expandAll();
  for (int index = 0; index < tree_->topLevelItemCount(); ++index) {
    QTreeWidgetItem* plotItem = tree_->topLevelItem(index);
    if (plotItem != nullptr) {
      plotItem->setExpanded(true);
    }
  }
}

void CurveValuesWidget::showEvent(QShowEvent* event) {
  QWidget::showEvent(event);
  expandPlotRows();
  QTimer::singleShot(0, this, [this]() { expandPlotRows(); });
}

PlotCurve* CurveValuesWidget::curveForConfig(PlotWidget* plot, CurveConfig* curveConfig) {
  if ((plot == nullptr) || (curveConfig == nullptr)) {
    return nullptr;
  }
  for (PlotCurve* curve : plot->getCurves()) {
    if ((curve != nullptr) && (curve->getConfig() == curveConfig)) {
      return curve;
    }
  }
  return nullptr;
}

void CurveValuesWidget::disconnectPlotTable() {
  if (plotTable_ == nullptr) {
    return;
  }

  if (plotTable_->getConfig() != nullptr) {
    disconnect(plotTable_->getConfig(), nullptr, this, nullptr);
  }

  for (PlotWidget* plot : plotTable_->getPlotWidgets()) {
    if (plot == nullptr) {
      continue;
    }
    disconnect(plot, nullptr, this, nullptr);
    if (plot->getConfig() != nullptr) {
      disconnect(plot->getConfig(), nullptr, this, nullptr);
      for (size_t index = 0; index < plot->getConfig()->getNumCurves(); ++index) {
        CurveConfig* curveConfig = plot->getConfig()->getCurveConfig(index);
        if (curveConfig == nullptr) {
          continue;
        }
        disconnect(curveConfig, nullptr, this, nullptr);
        if (curveConfig->getColorConfig() != nullptr) {
          disconnect(curveConfig->getColorConfig(), nullptr, this, nullptr);
        }
      }
    }
  }
}

QIcon CurveValuesWidget::colorSwatch(const QColor& color) {
  QPixmap pixmap(kSwatchSize, kSwatchSize);
  pixmap.fill(Qt::transparent);
  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing, false);
  painter.fillRect(0, 0, kSwatchSize, kSwatchSize, color);
  painter.end();
  return QIcon(pixmap);
}

QString CurveValuesWidget::formatAxisValue(PlotWidget* plot, const QPointF& point, bool isX) {
  PlotCursor* cursor = plot->getCursor();
  const BoundingRectangle& scale = plot->getCurrentScale();
  const double min = isX ? scale.getMinimum().x() : scale.getMinimum().y();
  const double max = isX ? scale.getMaximum().x() : scale.getMaximum().y();
  double span = max - min;
  if (!std::isfinite(span) || (span <= 0.0)) {
    span = 1.0;
  }

  const double value = isX ? point.x() : point.y();
  const double offset = (cursor != nullptr) ? (isX ? cursor->getXOffset() : cursor->getYOffset()) : 0.0;
  const AxisTimeFormat::LabelMode mode =
      (cursor != nullptr) ? (isX ? cursor->xTimeLabelMode() : cursor->yTimeLabelMode()) : AxisTimeFormat::LabelMode::Off;
  if ((mode == AxisTimeFormat::LabelMode::Relative) || (mode == AxisTimeFormat::LabelMode::Timestamp)) {
    span = std::min(span, kLiveReadoutMaxSpan);
  }
  const QTimeZone zone = (cursor != nullptr) ? cursor->timeZone() : plot->getTimeZone();
  return AxisTimeFormat::coordinate(value, offset, span, mode, zone);
}

void CurveValuesWidget::plotTableLayoutChanged() {
  rebuild();
}

void CurveValuesWidget::curveColorChanged() {
  rebuild();
}

void CurveValuesWidget::plotCleared() {
  refreshValues();
}

}  // namespace rqt_multiplot
