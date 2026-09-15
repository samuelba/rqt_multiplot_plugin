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

#include "rqt_multiplot/PlotTableWidget.h"

#include <algorithm>
#include <numeric>

#include <QApplication>
#include <QFile>
#include <QResizeEvent>
#include <QSet>
#include <QShowEvent>
#include <QSizePolicy>
#include <QSplitter>
#include <QTextStream>
#include <QTimer>

#include <rqt_multiplot/PlotCursor.h>
#include <rqt_multiplot/PlotExport.h>
#include <rqt_multiplot/PlotLayoutConfig.h>
#include <rqt_multiplot/PlotMouseBindings.h>
#include <rqt_multiplot/PlotSplitter.h>
#include <rqt_multiplot/PlotWidget.h>

namespace rqt_multiplot {

/*****************************************************************************/
/* Constructors and Destructor                                               */
/*****************************************************************************/

PlotTableWidget::PlotTableWidget(QWidget* parent)
    : QWidget(parent),
      layout_(new QVBoxLayout(this)),
      rootWidget_(nullptr),
      config_(nullptr),
      registry_(new MessageSubscriberRegistry(this)),
      bagReader_(new BagReader(this)) {
  setLayout(layout_);
  setAutoFillBackground(true);

  layout_->setContentsMargins(0, 0, 0, 0);
  layout_->setSpacing(0);

  connect(bagReader_, SIGNAL(readingStarted()), this, SLOT(bagReaderReadingStarted()));
  connect(bagReader_, SIGNAL(readingProgressChanged(double)), this, SLOT(bagReaderReadingProgressChanged(double)));
  connect(bagReader_, SIGNAL(readingFinished()), this, SLOT(bagReaderReadingFinished()));
  connect(bagReader_, SIGNAL(readingFailed(const QString&)), this, SLOT(bagReaderReadingFailed(const QString&)));
}

PlotTableWidget::~PlotTableWidget() = default;

/*****************************************************************************/
/* Accessors                                                                 */
/*****************************************************************************/

void PlotTableWidget::setConfig(PlotTableConfig* config) {
  if (config != config_) {
    if (config_ != nullptr) {
      disconnect(config_, SIGNAL(backgroundColorChanged(const QColor&)), this, SLOT(configBackgroundColorChanged(const QColor&)));
      disconnect(config_, SIGNAL(foregroundColorChanged(const QColor&)), this, SLOT(configForegroundColorChanged(const QColor&)));
      disconnect(config_, SIGNAL(layoutChanged()), this, SLOT(configLayoutChanged()));
      disconnect(config_, SIGNAL(linkScaleChanged(bool)), this, SLOT(configLinkScaleChanged(bool)));
      disconnect(config_, SIGNAL(trackPointsChanged(bool)), this, SLOT(configTrackPointsChanged(bool)));
    }

    config_ = config;

    if (config != nullptr) {
      connect(config, SIGNAL(backgroundColorChanged(const QColor&)), this, SLOT(configBackgroundColorChanged(const QColor&)));
      connect(config, SIGNAL(foregroundColorChanged(const QColor&)), this, SLOT(configForegroundColorChanged(const QColor&)));
      connect(config, SIGNAL(layoutChanged()), this, SLOT(configLayoutChanged()));
      connect(config, SIGNAL(linkScaleChanged(bool)), this, SLOT(configLinkScaleChanged(bool)));
      connect(config, SIGNAL(trackPointsChanged(bool)), this, SLOT(configTrackPointsChanged(bool)));

      configBackgroundColorChanged(config->getBackgroundColor());
      configForegroundColorChanged(config->getForegroundColor());
      configLayoutChanged();
      configLinkScaleChanged(config->isScaleLinked());
      configTrackPointsChanged(config->arePointsTracked());
    }
  }
}

PlotTableConfig* PlotTableWidget::getConfig() const {
  return config_;
}

size_t PlotTableWidget::getNumRows() const {
  return config_ != nullptr ? config_->getNumRows() : 0;
}

size_t PlotTableWidget::getNumColumns() const {
  return config_ != nullptr ? config_->getNumColumns() : 0;
}

size_t PlotTableWidget::getNumPlots() const {
  return static_cast<size_t>(plotWidgets_.count());
}

PlotWidget* PlotTableWidget::getPlotWidget(size_t row, size_t column) const {
  if (config_ == nullptr) {
    return nullptr;
  }

  PlotConfig* plotConfig = config_->getPlotConfig(row, column);
  if (plotConfig == nullptr) {
    return nullptr;
  }

  for (PlotWidget* plot : plotWidgets_) {
    if (plot->getConfig() == plotConfig) {
      return plot;
    }
  }

  return nullptr;
}

const QList<PlotWidget*>& PlotTableWidget::getPlotWidgets() const {
  return plotWidgets_;
}

MessageSubscriberRegistry* PlotTableWidget::getRegistry() const {
  return registry_;
}

BagReader* PlotTableWidget::getBagReader() const {
  return bagReader_;
}

/*****************************************************************************/
/* Methods                                                                   */
/*****************************************************************************/

void PlotTableWidget::runPlots() {
  for (PlotWidget* plot : plotWidgets_) {
    plot->run();
  }
}

void PlotTableWidget::pausePlots() {
  for (PlotWidget* plot : plotWidgets_) {
    plot->pause();
  }
}

void PlotTableWidget::clearPlots() {
  for (PlotWidget* plot : plotWidgets_) {
    plot->clear();
  }
}

void PlotTableWidget::requestReplot() {
  for (PlotWidget* plot : plotWidgets_) {
    plot->requestReplot();
  }
}

void PlotTableWidget::forceReplot() {
  for (PlotWidget* plot : plotWidgets_) {
    plot->forceReplot();
  }
}

void PlotTableWidget::renderToPainter(QPainter& painter, const QRectF& bounds) {
  if (plotWidgets_.isEmpty()) {
    return;
  }

  QRectF plotBounds = bounds;
  if (plotBounds.isEmpty() && (painter.device() != nullptr)) {
    plotBounds = QRectF(0, 0, painter.device()->width(), painter.device()->height());
  }

  const QRect tableRect = rect();
  if (tableRect.isEmpty()) {
    return;
  }

  for (PlotWidget* plot : plotWidgets_) {
    const QRect geometry(plot->mapTo(this, QPoint(0, 0)), plot->size());
    const double x = plotBounds.x() + plotBounds.width() * static_cast<double>(geometry.x() - tableRect.x()) / tableRect.width();
    const double y = plotBounds.y() + plotBounds.height() * static_cast<double>(geometry.y() - tableRect.y()) / tableRect.height();
    const double width = plotBounds.width() * static_cast<double>(geometry.width()) / tableRect.width();
    const double height = plotBounds.height() * static_cast<double>(geometry.height()) / tableRect.height();
    plot->renderToPainter(painter, QRectF(x, y, width, height));
  }
}

void PlotTableWidget::renderToPixmap(QPixmap& pixmap) {
  QPainter painter(&pixmap);
  renderToPainter(painter, QRectF(0, 0, pixmap.width(), pixmap.height()));
}

void PlotTableWidget::writeFormattedCurveAxisTitles(QStringList& formattedAxisTitles) {
  formattedAxisTitles.clear();

  for (PlotWidget* plot : plotWidgets_) {
    QStringList formattedCurveAxisTitles;
    plot->writeFormattedCurveAxisTitles(formattedCurveAxisTitles);
    formattedAxisTitles.append(formattedCurveAxisTitles);
  }
}

void PlotTableWidget::writeFormattedCurveData(QList<QStringList>& formattedData) {
  formattedData.clear();

  for (PlotWidget* plot : plotWidgets_) {
    QList<QStringList> formattedCurveData;
    plot->writeFormattedCurveData(formattedCurveData);
    formattedData.append(formattedCurveData);
  }
}

void PlotTableWidget::loadFromBagFile(const QString& fileName) {
  clearPlots();

  for (PlotWidget* plot : plotWidgets_) {
    plot->setBroker(bagReader_);
  }

  runPlots();

  bagReader_->read(fileName);
}

void PlotTableWidget::saveToImageFile(const QString& fileName) {
  renderExportImage(fileName, [this](QPainter& painter, const QRectF& bounds) { renderToPainter(painter, bounds); });
}

void PlotTableWidget::saveToTextFile(const QString& fileName) {
  QFile file(fileName);

  if (file.open(QIODevice::WriteOnly)) {
    QStringList formattedAxisTitles;
    QList<QStringList> formattedData;

    writeFormattedCurveAxisTitles(formattedAxisTitles);
    writeFormattedCurveData(formattedData);

    QTextStream stream(&file);
    writeCurveTable(stream, formattedAxisTitles, formattedData, headerStyleFromPath(fileName));
  }
}

void PlotTableWidget::storeSplitterRatios() {
  for (auto it = splitterNodes_.begin(); it != splitterNodes_.end(); ++it) {
    const QList<int> sizes = it.key()->sizes();
    bool usable = !sizes.isEmpty();
    for (int size : sizes) {
      if (size < 1) {
        usable = false;
        break;
      }
    }
    if (usable) {
      it.value()->setStretch(sizes);
    }
  }
}

bool PlotTableWidget::anyPlotUserScaleLocked() const {
  for (PlotWidget* plot : plotWidgets_) {
    if (plot->isUserScaleLocked()) {
      return true;
    }
  }

  return false;
}

void PlotTableWidget::updatePlotScale(const BoundingRectangle& bounds, PlotWidget* excluded) {
  BoundingRectangle validBounds = bounds;

  if (!bounds.isValid()) {
    BoundingRectangle currentBounds;

    for (PlotWidget* plot : plotWidgets_) {
      currentBounds += plot->getCurrentScale();
    }

    if (bounds.getMaximum().x() <= bounds.getMinimum().x()) {
      validBounds.getMinimum().setX(currentBounds.getMinimum().x());
      validBounds.getMaximum().setX(currentBounds.getMaximum().x());
    }

    if (bounds.getMaximum().y() <= bounds.getMinimum().y()) {
      validBounds.getMinimum().setY(currentBounds.getMinimum().y());
      validBounds.getMaximum().setY(currentBounds.getMaximum().y());
    }
  }

  for (PlotWidget* plot : plotWidgets_) {
    if (excluded != plot) {
      plot->setCurrentScale(validBounds);
    }
  }
}

void PlotTableWidget::rebuildLayout() {
  QHash<PlotConfig*, PlotWidget*> existing;
  QList<PlotWidget*> leftovers;

  if ((config_ != nullptr) && (config_->getLayout() != nullptr)) {
    const QList<PlotConfig*> live = config_->plotConfigs();
    const QSet<PlotConfig*> liveSet(live.begin(), live.end());
    for (PlotWidget* plot : plotWidgets_) {
      plot->setParent(nullptr);
      PlotConfig* plotConfig = plot->getConfig();
      if ((plotConfig != nullptr) && liveSet.contains(plotConfig) && !existing.contains(plotConfig)) {
        existing.insert(plotConfig, plot);
      } else {
        leftovers.append(plot);
      }
    }
  } else {
    leftovers = plotWidgets_;
    for (PlotWidget* plot : leftovers) {
      plot->setParent(nullptr);
    }
  }

  plotWidgets_.clear();
  splitterNodes_.clear();

  QWidget* oldRoot = rootWidget_;
  rootWidget_ = nullptr;
  if (oldRoot != nullptr) {
    layout_->removeWidget(oldRoot);
    if (qobject_cast<PlotWidget*>(oldRoot) == nullptr) {
      delete oldRoot;
    }
  }

  if ((config_ != nullptr) && (config_->getLayout() != nullptr) && (config_->plotCount() > 0)) {
    rootWidget_ = createNodeWidget(config_->getLayout(), existing);
    layout_->addWidget(rootWidget_);
  }

  leftovers.append(existing.values());
  for (PlotWidget* leftover : leftovers) {
    leftover->setParent(this);
    leftover->hide();
    leftover->deleteLater();
  }

  updatePlotControls();
  applyAllStretch();
  QTimer::singleShot(0, this, [this]() { applyAllStretch(); });
  emit plotPausedChanged();
}

QWidget* PlotTableWidget::createNodeWidget(PlotLayoutConfig* node, QHash<PlotConfig*, PlotWidget*>& existing) {
  if (node->getType() == PlotLayoutConfig::Plot) {
    PlotWidget* plot = existing.take(node->getPlotConfig());
    if (plot == nullptr) {
      plot = createPlotWidget();
    }
    plot->setParent(this);
    plot->setConfig(node->getPlotConfig());
    plot->setBroker(registry_);
    plot->getCursor()->setTrackPoints(config_->arePointsTracked());
    if (config_->isScaleLinked() && !plotWidgets_.isEmpty()) {
      plot->setCurrentScale(plotWidgets_.front()->getCurrentScale());
    }
    plot->show();
    plotWidgets_.append(plot);
    return plot;
  }

  auto* splitter = new PlotSplitter((node->getType() == PlotLayoutConfig::Horizontal) ? Qt::Horizontal : Qt::Vertical, this);
  splitterNodes_.insert(splitter, node);

  for (PlotLayoutConfig* child : node->getChildren()) {
    QWidget* childWidget = createNodeWidget(child, existing);
    childWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    splitter->addWidget(childWidget);
  }

  connect(splitter, SIGNAL(splitterMoved(int, int)), this, SLOT(splitterMoved(int, int)));
  return splitter;
}

PlotWidget* PlotTableWidget::createPlotWidget() {
  auto* plot = new PlotWidget(this);
  connectPlotWidget(plot);
  return plot;
}

void PlotTableWidget::connectPlotWidget(PlotWidget* plot) {
  connect(plot, SIGNAL(preferredScaleChanged(const BoundingRectangle&)), this, SLOT(plotPreferredScaleChanged(const BoundingRectangle&)));
  connect(plot, SIGNAL(currentScaleChanged(const BoundingRectangle&)), this, SLOT(plotCurrentScaleChanged(const BoundingRectangle&)));
  connect(plot, SIGNAL(userScaleLockedChanged(bool)), this, SLOT(plotUserScaleLockedChanged(bool)));
  connect(plot->getCursor(), SIGNAL(activeChanged(bool)), this, SLOT(plotCursorActiveChanged(bool)));
  connect(plot->getCursor(), SIGNAL(currentPositionChanged(const QPointF&)), this, SLOT(plotCursorCurrentPositionChanged(const QPointF&)));
  connect(plot, SIGNAL(pausedChanged(bool)), this, SLOT(plotPausedChanged(bool)));
  connect(plot, SIGNAL(stateChanged(int)), this, SLOT(plotStateChanged(int)));
  connect(plot, SIGNAL(splitRequested(Qt::Orientation, bool)), this, SLOT(plotSplitRequested(Qt::Orientation, bool)));
  connect(plot, SIGNAL(closeRequested()), this, SLOT(plotCloseRequested()));
}

void PlotTableWidget::applyStretch(QSplitter* splitter, PlotLayoutConfig* node) {
  const QList<int> stretch = node->getStretch();
  const int count = std::min(stretch.count(), splitter->count());
  if (count == 0) {
    return;
  }

  const QSignalBlocker blocker(splitter);
  int total = 0;
  for (int index = 0; index < count; ++index) {
    QWidget* child = splitter->widget(index);
    if (child != nullptr) {
      child->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    }
    splitter->setStretchFactor(index, stretch[index]);
    total += stretch[index];
  }

  const int span = (splitter->orientation() == Qt::Horizontal) ? splitter->width() : splitter->height();
  if ((total <= 0) || (span <= 0)) {
    return;
  }

  const int handleSpace = splitter->handleWidth() * std::max(0, count - 1);
  const int available = std::max(count, span - handleSpace);
  QList<int> pixels;
  pixels.reserve(count);
  int allocated = 0;
  for (int index = 0; index < count; ++index) {
    const int value = (index + 1 == count) ? std::max(1, available - allocated)
                                           : std::max(1, static_cast<int>(static_cast<qint64>(available) * stretch[index] / total));
    pixels.append(value);
    allocated += value;
  }
  splitter->setSizes(pixels);
}

void PlotTableWidget::applyStretchRecursive(QWidget* widget) {
  auto* splitter = qobject_cast<QSplitter*>(widget);
  if (splitter == nullptr) {
    return;
  }

  PlotLayoutConfig* node = splitterNodes_.value(splitter, nullptr);
  if (node != nullptr) {
    applyStretch(splitter, node);
  }
  for (int index = 0; index < splitter->count(); ++index) {
    applyStretchRecursive(splitter->widget(index));
  }
}

void PlotTableWidget::applyAllStretch() {
  applyStretchRecursive(rootWidget_);
}

void PlotTableWidget::updatePlotControls() {
  const bool multiple = plotWidgets_.count() > 1;
  for (PlotWidget* plot : plotWidgets_) {
    plot->setCanChangeState(multiple);
    plot->setCanClose(multiple);
  }
}

void PlotTableWidget::showEvent(QShowEvent* event) {
  QWidget::showEvent(event);
  applyAllStretch();
}

void PlotTableWidget::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  applyAllStretch();
}

/*****************************************************************************/
/* Slots                                                                     */
/*****************************************************************************/

void PlotTableWidget::configBackgroundColorChanged(const QColor& color) {
  QPalette currentPalette = palette();

  currentPalette.setColor(QPalette::Window, color);
  currentPalette.setColor(QPalette::Base, color);

  setPalette(currentPalette);

  forceReplot();
}

void PlotTableWidget::configForegroundColorChanged(const QColor& color) {
  QPalette currentPalette = palette();

  currentPalette.setColor(QPalette::WindowText, color);
  currentPalette.setColor(QPalette::Text, color);

  setPalette(currentPalette);
}

void PlotTableWidget::configLayoutChanged() {
  rebuildLayout();
}

void PlotTableWidget::configLinkScaleChanged(bool link) {
  if (link) {
    BoundingRectangle bounds;

    for (PlotWidget* plot : plotWidgets_) {
      bounds += plot->getPreferredScale();
    }

    updatePlotScale(bounds);
  }
}

void PlotTableWidget::configTrackPointsChanged(bool track) {
  for (PlotWidget* plot : plotWidgets_) {
    plot->getCursor()->setTrackPoints(track);
  }
}

void PlotTableWidget::bagReaderReadingStarted() {
  emit jobStarted("Reading bag from [file://" + bagReader_->getFileName() + "]...");
}

void PlotTableWidget::bagReaderReadingProgressChanged(double progress) {
  emit jobProgressChanged(progress);
}

void PlotTableWidget::bagReaderReadingFinished() {
  pausePlots();

  for (PlotWidget* plot : plotWidgets_) {
    plot->setBroker(registry_);
  }

  emit jobFinished("Read bag from [file://" + bagReader_->getFileName() + "]");
}

void PlotTableWidget::bagReaderReadingFailed(const QString& /*error*/) {
  pausePlots();

  for (PlotWidget* plot : plotWidgets_) {
    plot->setBroker(registry_);
  }

  emit jobFailed("Failed to read bag from [file://" + bagReader_->getFileName() + "]");
}

void PlotTableWidget::plotPreferredScaleChanged(const BoundingRectangle& bounds) {
  if (config_ == nullptr) {
    return;
  }

  if (shouldIgnoreLinkedPreferredScale(config_->isScaleLinked(), anyPlotUserScaleLocked())) {
    return;
  }

  if (config_->isScaleLinked()) {
    BoundingRectangle preferredBounds;

    for (PlotWidget* plot : plotWidgets_) {
      preferredBounds += plot->getPreferredScale();
    }

    updatePlotScale(preferredBounds);
    return;
  }

  auto* plot = dynamic_cast<PlotWidget*>(sender());
  if ((plot != nullptr) && shouldApplyPreferredScale(true, plot->isUserScaleLocked())) {
    plot->setCurrentScale(bounds);
  }
}

void PlotTableWidget::plotUserScaleLockedChanged(bool locked) {
  if ((config_ == nullptr) || !config_->isScaleLinked()) {
    return;
  }

  for (PlotWidget* plot : plotWidgets_) {
    if (sender() != plot) {
      plot->setUserScaleLocked(locked);
    }
  }
}

void PlotTableWidget::plotCurrentScaleChanged(const BoundingRectangle& bounds) {
  if ((config_ != nullptr) && config_->isScaleLinked()) {
    updatePlotScale(bounds, dynamic_cast<PlotWidget*>(sender()));
  }
}

void PlotTableWidget::plotCursorActiveChanged(bool active) {
  if ((config_ != nullptr) && config_->isCursorLinked()) {
    for (PlotWidget* plot : plotWidgets_) {
      if (sender() != plot) {
        plot->getCursor()->setActive(active);
      }
    }
  }
}

void PlotTableWidget::plotCursorCurrentPositionChanged(const QPointF& position) {
  if ((config_ != nullptr) && config_->isCursorLinked()) {
    for (PlotWidget* plot : plotWidgets_) {
      if (sender() != plot) {
        plot->getCursor()->setCurrentPosition(position);
      }
    }
  }
}

void PlotTableWidget::plotPausedChanged(bool /*paused*/) {
  emit plotPausedChanged();
}

void PlotTableWidget::plotStateChanged(int state) {
  for (PlotWidget* plot : plotWidgets_) {
    if (state == PlotWidget::Maximized) {
      if (sender() != plot) {
        plot->hide();
      }
    } else if (state == PlotWidget::Normal) {
      plot->show();
    }
  }
}

void PlotTableWidget::plotSplitRequested(Qt::Orientation orientation, bool insertBefore) {
  auto* plot = dynamic_cast<PlotWidget*>(sender());
  if ((plot == nullptr) || (config_ == nullptr) || (plot->getConfig() == nullptr)) {
    return;
  }

  config_->splitPlot(plot->getConfig(), orientation, insertBefore);
}

void PlotTableWidget::plotCloseRequested() {
  auto* plot = dynamic_cast<PlotWidget*>(sender());
  if ((plot == nullptr) || (config_ == nullptr) || (plot->getConfig() == nullptr)) {
    return;
  }

  config_->closePlot(plot->getConfig());
}

void PlotTableWidget::splitterMoved(int /*pos*/, int /*index*/) {
  storeSplitterRatios();
}

}  // namespace rqt_multiplot
