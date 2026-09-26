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

#include <array>
#include <optional>

#include <QApplication>
#include <QClipboard>
#include <QColor>
#include <QCursor>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFontMetrics>
#include <QGridLayout>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMetaObject>
#include <QMimeData>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QSignalBlocker>
#include <QSize>
#include <QTextStream>
#include <QTimeZone>
#include <QToolButton>
#include <QWidgetAction>

#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_curve.h>
#include <qwt/qwt_plot_grid.h>
#include <qwt/qwt_plot_picker.h>
#include <qwt/qwt_plot_renderer.h>
#include <qwt/qwt_scale_widget.h>
#include <qwt/qwt_text.h>

#include "rqt_multiplot/PackageResource.hpp"
#include "rqt_multiplot/PlotExport.hpp"

#include "rqt_multiplot/AxisTimeFormat.hpp"
#include "rqt_multiplot/CurveAxisConfig.hpp"
#include "rqt_multiplot/CurveData.hpp"
#include "rqt_multiplot/DataStatisticsDialog.hpp"
#include "rqt_multiplot/OffsetScaleDraw.hpp"
#include "rqt_multiplot/OffsetScaleEngine.hpp"
#include "rqt_multiplot/PlotCanvasPolicy.hpp"
#include "rqt_multiplot/PlotConfigDialog.hpp"
#include "rqt_multiplot/PlotConfigWidget.hpp"
#include "rqt_multiplot/PlotCursor.hpp"
#include "rqt_multiplot/PlotCurve.hpp"
#include "rqt_multiplot/PlotLegend.hpp"
#include "rqt_multiplot/PlotMagnifier.hpp"
#include "rqt_multiplot/PlotMouseBindings.hpp"
#include "rqt_multiplot/PlotPanner.hpp"
#include "rqt_multiplot/PlotReplotPolicy.hpp"
#include "rqt_multiplot/PlotZoomer.hpp"
#include "rqt_multiplot/Theme.hpp"
#include "rqt_multiplot/TimeZoneUtil.hpp"

#include <ui_PlotWidget.h>

#include "rqt_multiplot/PlotWidget.hpp"

namespace rqt_multiplot {
namespace {

void relayoutScaleWidget(QwtScaleWidget* widget) {
  if ((widget == nullptr) || (widget->scaleDraw() == nullptr)) {
    return;
  }
  // Qwt skips layoutScale when the scale division is unchanged (Timestamp <-> DateTime).
  widget->setLabelAlignment(widget->scaleDraw()->labelAlignment());
  [[maybe_unused]] const bool emitted = QMetaObject::invokeMethod(widget, "scaleDivChanged");
  Q_ASSERT(emitted);
}

QwtText axisTitleWithColor(const QString& text, const QColor& color) {
  QwtText title(text);
  title.setColor(color);
  return title;
}

class BoolGuard {
 public:
  explicit BoolGuard(bool& flag) : flag_(flag) { flag_ = true; }
  ~BoolGuard() { flag_ = false; }

  BoolGuard(const BoolGuard&) = delete;
  BoolGuard& operator=(const BoolGuard&) = delete;
  BoolGuard(BoolGuard&&) = delete;
  BoolGuard& operator=(BoolGuard&&) = delete;

 private:
  bool& flag_;
};

constexpr QSize kContextMenuIconSize(16, 16);

void setContextMenuIcon(QAction* action, const QString& relativePath) {
  setThemeIcon(action, relativePath, kContextMenuIconSize);
}

}  // namespace

PlotWidget::PlotWidget(QWidget* parent)
    : QWidget(parent),
      ui_(new Ui::PlotWidget()),
      timer_(new QTimer(this)),
      menuSplit_(new QMenu(this)),
      menuContext_(new QMenu(this)),
      actionContextResetZoom_(nullptr),
      actionContextResetZoomHorizontal_(nullptr),
      actionContextResetZoomVertical_(nullptr),
      actionContextConfigure_(nullptr),
      menuContextSplit_(new QMenu(tr("Split"), menuContext_)),
      actionContextShowLegend_(nullptr),
      actionContextMaximizeRestore_(nullptr),
      actionContextRunPause_(nullptr),
      actionContextClear_(nullptr),
      actionContextClose_(nullptr),
      actionContextCopyImage_(nullptr),
      actionContextSaveImage_(nullptr),
      actionContextSaveData_(nullptr),
      actionContextDataStatistics_(nullptr),
      dataStatisticsDialog_(nullptr),
      config_(nullptr),
      broker_(nullptr),
      legend_(nullptr),
      cursor_(nullptr),
      grid_(nullptr),
      panner_(nullptr),
      magnifier_(nullptr),
      zoomer_(nullptr),
      paused_(true),
      rescale_(false),
      replot_(false),
      replotting_(false),
      gridVisible_(false),
      xScaleLocked_(false),
      yScaleLocked_(false),
      state_(Normal),
      xOriginSet_(false),
      yOriginSet_(false),
      xOrigin_(0.0),
      yOrigin_(0.0),
      timeAxisFormat_(PlotTableConfig::StartFromZero),
      timeZone_(TimeZoneUtil::localTimeZone()),
      gridForegroundColor_(Qt::black),
      plotTitleStyle_(PlotTitleStyle::factory()) {
  qRegisterMetaType<BoundingRectangle>("BoundingRectangle");

  ui_->setupUi(this);

  setAcceptDrops(true);

  runIcon_ = packageIcon("resource/play.svg", QSize(16, 16));
  pauseIcon_ = packageIcon("resource/pause.svg", QSize(16, 16));
  normalIcon_ = packageIcon("resource/maximize.svg", QSize(16, 16));
  maximizedIcon_ = packageIcon("resource/minimize.svg", QSize(16, 16));

  ui_->pushButtonRunPause->setIcon(runIcon_);
  setThemeIcon(ui_->pushButtonClear, QStringLiteral("resource/delete-data.svg"), QSize(16, 16));
  setThemeIcon(ui_->pushButtonSetup, QStringLiteral("resource/settings-edit.svg"), QSize(16, 16));
  setThemeIcon(ui_->pushButtonSplit, QStringLiteral("resource/split/layout.svg"), QSize(16, 16));
  ui_->pushButtonSplit->setIconSize(QSize(16, 16));
  ui_->pushButtonState->setIcon(normalIcon_);
  setThemeIcon(ui_->pushButtonClose, QStringLiteral("resource/close.svg"), QSize(16, 16));
  ui_->pushButtonClose->setIconSize(QSize(16, 16));
  ui_->pushButtonClose->setEnabled(false);

  ui_->plot->setAutoReplot(false);
  ui_->plot->setAutoDelete(false);

  ui_->plot->enableAxis(QwtPlot::xTop);
  ui_->plot->enableAxis(QwtPlot::yRight);

  ui_->plot->setAxisAutoScale(QwtPlot::yLeft, false);
  ui_->plot->setAxisAutoScale(QwtPlot::yRight, false);
  ui_->plot->setAxisAutoScale(QwtPlot::xTop, false);
  ui_->plot->setAxisAutoScale(QwtPlot::xBottom, false);

  ui_->plot->axisScaleDraw(QwtPlot::xTop)->enableComponent(QwtAbstractScaleDraw::Labels, false);
  ui_->plot->axisScaleDraw(QwtPlot::yRight)->enableComponent(QwtAbstractScaleDraw::Labels, false);

  ui_->plot->setAxisScaleDraw(QwtPlot::xBottom, new OffsetScaleDraw());
  ui_->plot->setAxisScaleDraw(QwtPlot::yLeft, new OffsetScaleDraw());
  ui_->plot->setAxisScaleEngine(QwtPlot::xBottom, new OffsetScaleEngine());
  ui_->plot->setAxisScaleEngine(QwtPlot::yLeft, new OffsetScaleEngine());

  ui_->horizontalSpacerRight->changeSize(ui_->plot->axisWidget(QwtPlot::yRight)->width() - 5, 20);

  timer_->setInterval(static_cast<int>(1e3 / 30.0));
  timer_->start();

  buildSplitMenu();
  buildContextMenu();

  grid_ = new QwtPlotGrid();
  grid_->attach(ui_->plot);
  grid_->enableX(true);
  grid_->enableY(true);
  grid_->enableXMin(false);
  grid_->enableYMin(false);
  grid_->setVisible(false);
  updateGridPen();
  createCanvasPickers();

#if QWT_VERSION >= 0x060100
  currentBounds_.getMinimum().setX(ui_->plot->axisScaleDiv(QwtPlot::xBottom).lowerBound());
  currentBounds_.getMinimum().setY(ui_->plot->axisScaleDiv(QwtPlot::yLeft).lowerBound());
  currentBounds_.getMaximum().setX(ui_->plot->axisScaleDiv(QwtPlot::xBottom).upperBound());
  currentBounds_.getMaximum().setY(ui_->plot->axisScaleDiv(QwtPlot::yLeft).upperBound());
#else
  currentBounds_.getMinimum().setX(ui_->plot->axisScaleDiv(QwtPlot::xBottom)->lowerBound());
  currentBounds_.getMinimum().setY(ui_->plot->axisScaleDiv(QwtPlot::yLeft)->lowerBound());
  currentBounds_.getMaximum().setX(ui_->plot->axisScaleDiv(QwtPlot::xBottom)->upperBound());
  currentBounds_.getMaximum().setY(ui_->plot->axisScaleDiv(QwtPlot::yLeft)->upperBound());
#endif

  connect(ui_->lineEditTitle, SIGNAL(textChanged(const QString&)), this, SLOT(lineEditTitleTextChanged(const QString&)));
  connect(ui_->lineEditTitle, SIGNAL(editingFinished()), this, SLOT(lineEditTitleEditingFinished()));

  connect(ui_->pushButtonRunPause, SIGNAL(clicked()), this, SLOT(pushButtonRunPauseClicked()));
  connect(ui_->pushButtonClear, SIGNAL(clicked()), this, SLOT(pushButtonClearClicked()));
  connect(ui_->pushButtonSetup, SIGNAL(clicked()), this, SLOT(pushButtonSetupClicked()));
  connect(ui_->pushButtonSplit, SIGNAL(clicked()), this, SLOT(pushButtonSplitClicked()));
  connect(ui_->pushButtonState, SIGNAL(clicked()), this, SLOT(pushButtonStateClicked()));
  connect(ui_->pushButtonClose, SIGNAL(clicked()), this, SLOT(pushButtonCloseClicked()));

  connect(ui_->plot->axisWidget(QwtPlot::xBottom), SIGNAL(scaleDivChanged()), this, SLOT(plotXBottomScaleDivChanged()));
  connect(ui_->plot->axisWidget(QwtPlot::yLeft), SIGNAL(scaleDivChanged()), this, SLOT(plotYLeftScaleDivChanged()));

  connect(timer_, SIGNAL(timeout()), this, SLOT(timerTimeout()));

  ui_->plot->axisWidget(QwtPlot::yLeft)->installEventFilter(this);
  ui_->plot->axisWidget(QwtPlot::yRight)->installEventFilter(this);
  applyPlotChrome();
}

PlotWidget::~PlotWidget() {
  timer_->stop();
  pause();
  for (auto* curve : curves_) {
    curve->detach();
    delete curve;
  }
  curves_.clear();
  if (grid_ != nullptr) {
    grid_->detach();
    delete grid_;
    grid_ = nullptr;
  }
  delete ui_;
}

void PlotWidget::setConfig(PlotConfig* config) {
  if (config != config_) {
    if (config_ != nullptr) {
      disconnect(config_, SIGNAL(titleChanged(const QString&)), this, SLOT(configTitleChanged(const QString&)));
      disconnect(config_, SIGNAL(curveAdded(size_t)), this, SLOT(configCurveAdded(size_t)));
      disconnect(config_, SIGNAL(curveRemoved(size_t)), this, SLOT(configCurveRemoved(size_t)));
      disconnect(config_, SIGNAL(curvesCleared()), this, SLOT(configCurvesCleared()));
      disconnect(config_, SIGNAL(curveConfigChanged(size_t)), this, SLOT(configCurveConfigChanged(size_t)));
      disconnect(config_->getAxesConfig()->getAxisConfig(PlotAxesConfig::X), SIGNAL(changed()), this, SLOT(configXAxisConfigChanged()));
      disconnect(config_->getAxesConfig()->getAxisConfig(PlotAxesConfig::Y), SIGNAL(changed()), this, SLOT(configYAxisConfigChanged()));
      disconnect(config_->getLegendConfig(), SIGNAL(changed()), this, SLOT(configLegendConfigChanged()));
      disconnect(config_, SIGNAL(plotRateChanged(double)), this, SLOT(configPlotRateChanged(double)));
      disconnect(config_, SIGNAL(timeWindowEnabledChanged(bool)), this, SLOT(configTimeWindowEnabledChanged(bool)));
      disconnect(config_, SIGNAL(timeWindowLengthChanged(int)), this, SLOT(configTimeWindowLengthChanged(int)));
      disconnect(config_, SIGNAL(destroyed()), this, SLOT(configDestroyed()));

      configCurvesCleared();
    }

    xOriginSet_ = false;
    yOriginSet_ = false;
    xOrigin_ = 0.0;
    yOrigin_ = 0.0;

    config_ = config;

    if (config != nullptr) {
      connect(config, SIGNAL(titleChanged(const QString&)), this, SLOT(configTitleChanged(const QString&)));
      connect(config, SIGNAL(curveAdded(size_t)), this, SLOT(configCurveAdded(size_t)));
      connect(config, SIGNAL(curveRemoved(size_t)), this, SLOT(configCurveRemoved(size_t)));
      connect(config, SIGNAL(curvesCleared()), this, SLOT(configCurvesCleared()));
      connect(config, SIGNAL(curveConfigChanged(size_t)), this, SLOT(configCurveConfigChanged(size_t)));
      connect(config->getAxesConfig()->getAxisConfig(PlotAxesConfig::X), SIGNAL(changed()), this, SLOT(configXAxisConfigChanged()));
      connect(config->getAxesConfig()->getAxisConfig(PlotAxesConfig::Y), SIGNAL(changed()), this, SLOT(configYAxisConfigChanged()));
      connect(config->getLegendConfig(), SIGNAL(changed()), this, SLOT(configLegendConfigChanged()));
      connect(config, SIGNAL(plotRateChanged(double)), this, SLOT(configPlotRateChanged(double)));
      connect(config, SIGNAL(timeWindowEnabledChanged(bool)), this, SLOT(configTimeWindowEnabledChanged(bool)));
      connect(config, SIGNAL(timeWindowLengthChanged(int)), this, SLOT(configTimeWindowLengthChanged(int)));
      connect(config, SIGNAL(destroyed()), this, SLOT(configDestroyed()));

      configTitleChanged(config->getTitle());
      configPlotRateChanged(config->getPlotRate());
      configXAxisConfigChanged();
      configYAxisConfigChanged();
      configLegendConfigChanged();

      for (size_t index = 0; index < config->getNumCurves(); ++index) {
        configCurveAdded(index);
      }
    } else {
      updateAxisTimeLabels();
    }
  }
}

PlotConfig* PlotWidget::getConfig() const {
  return config_;
}

void PlotWidget::setBroker(MessageBroker* broker) {
  if (broker != broker_) {
    broker_ = broker;

    for (int index = 0; index < curves_.count(); ++index) {
      curves_[index]->setBroker(broker);
    }
  }
}

MessageBroker* PlotWidget::getBroker() const {
  return broker_;
}

PlotCursor* PlotWidget::getCursor() const {
  return cursor_;
}

const QVector<PlotCurve*>& PlotWidget::getCurves() const {
  return curves_;
}

void PlotWidget::setTimeAxisFormat(PlotTableConfig::TimeAxisFormat format) {
  if (format == timeAxisFormat_) {
    return;
  }
  timeAxisFormat_ = format;
  updateAxisTimeLabels();
}

PlotTableConfig::TimeAxisFormat PlotWidget::getTimeAxisFormat() const {
  return timeAxisFormat_;
}

void PlotWidget::setTimeZone(const QTimeZone& zone) {
  timeZone_ = zone;
  updateAxisTimeLabels();
}

const QTimeZone& PlotWidget::getTimeZone() const {
  return timeZone_;
}

void PlotWidget::setGridVisible(bool visible) {
  if (visible == gridVisible_) {
    return;
  }
  gridVisible_ = visible;
  if (grid_ != nullptr) {
    grid_->setVisible(visible);
    requestReplot();
  }
}

bool PlotWidget::isGridVisible() const {
  return gridVisible_;
}

void PlotWidget::setGridForegroundColor(const QColor& color) {
  if (color == gridForegroundColor_) {
    return;
  }
  gridForegroundColor_ = color;
  updateGridPen();
}

BoundingRectangle PlotWidget::getPreferredScale() const {
  BoundingRectangle bounds;

  for (int index = 0; index < curves_.count(); ++index) {
    bounds += curves_[index]->getPreferredScale();
  }

  return bounds;
}

void PlotWidget::setCurrentScale(const BoundingRectangle& bounds) {
  if (bounds != currentBounds_) {
    if (bounds.getMaximum().x() == bounds.getMinimum().x()) {
      ui_->plot->setAxisScale(QwtPlot::xBottom, bounds.getMinimum().x() - 0.1, bounds.getMaximum().x() + 0.1);
    } else if (bounds.getMaximum().x() > bounds.getMinimum().x()) {
      ui_->plot->setAxisScale(QwtPlot::xBottom, bounds.getMinimum().x(), bounds.getMaximum().x());
    }
    if (bounds.getMaximum().y() == bounds.getMinimum().y()) {
      ui_->plot->setAxisScale(QwtPlot::yLeft, bounds.getMinimum().y() - 0.1, bounds.getMaximum().y() + 0.1);
    } else if (bounds.getMaximum().y() > bounds.getMinimum().y()) {
      ui_->plot->setAxisScale(QwtPlot::yLeft, bounds.getMinimum().y(), bounds.getMaximum().y());
    }

    rescale_ = false;

    if (shouldReplotAfterApplyingScale(replotting_)) {
      forceReplot();
    }
  }
}

const BoundingRectangle& PlotWidget::getCurrentScale() const {
  return currentBounds_;
}

bool PlotWidget::isPaused() const {
  return paused_;
}

bool PlotWidget::isReplotRequested() const {
  return replot_;
}

void PlotWidget::setState(State state) {
  if ((state != state_) && canChangeState()) {
    state_ = state;

    if (state == Maximized) {
      ui_->pushButtonState->setIcon(maximizedIcon_);
    } else {
      ui_->pushButtonState->setIcon(normalIcon_);
    }

    emit stateChanged(state);
  }
}

PlotWidget::State PlotWidget::getState() const {
  return state_;
}

void PlotWidget::setCanChangeState(bool can) {
  ui_->pushButtonState->setEnabled(can);
}

bool PlotWidget::canChangeState() const {
  return ui_->pushButtonState->isEnabled();
}

void PlotWidget::setCanClose(bool can) {
  ui_->pushButtonClose->setEnabled(can);
}

bool PlotWidget::canClose() const {
  return ui_->pushButtonClose->isEnabled();
}

void PlotWidget::setUserScaleLocked(bool locked) {
  if (locked) {
    setXScaleLocked(true);
    setYScaleLocked(true);
    return;
  }

  const bool wasLocked = isUserScaleLocked();
  xScaleLocked_ = false;
  yScaleLocked_ = false;
  if (wasLocked) {
    emit userScaleLockedChanged(false);
  }
  rescale_ = true;
  requestReplot();
}

bool PlotWidget::isUserScaleLocked() const {
  return xScaleLocked_ || yScaleLocked_;
}

void PlotWidget::setXScaleLocked(bool locked) {
  if (locked == xScaleLocked_) {
    return;
  }

  xScaleLocked_ = locked;
  emit userScaleLockedChanged(isUserScaleLocked());
  if (!locked) {
    rescale_ = true;
    requestReplot();
  }
}

bool PlotWidget::isXScaleLocked() const {
  return xScaleLocked_;
}

void PlotWidget::setYScaleLocked(bool locked) {
  if (locked == yScaleLocked_) {
    return;
  }

  yScaleLocked_ = locked;
  emit userScaleLockedChanged(isUserScaleLocked());
  if (!locked) {
    rescale_ = true;
    requestReplot();
  }
}

bool PlotWidget::isYScaleLocked() const {
  return yScaleLocked_;
}

void PlotWidget::syncScaleLocksFrom(const PlotWidget& source) {
  xScaleLocked_ = source.xScaleLocked_;
  yScaleLocked_ = source.yScaleLocked_;
}

void PlotWidget::resetZoom() {
  if (zoomer_ != nullptr) {
    zoomer_->zoom(0);
  }
  setUserScaleLocked(false);
}

void PlotWidget::resetZoomHorizontal() {
  const BoundingRectangle preferred = getPreferredScale();
  BoundingRectangle bounds = getCurrentScale();
  if (preferred.isValid()) {
    bounds.getMinimum().setX(preferred.getMinimum().x());
    bounds.getMaximum().setX(preferred.getMaximum().x());
  }

  if (xScaleLocked_) {
    xScaleLocked_ = false;
    emit userScaleLockedChanged(isUserScaleLocked());
  }
  setCurrentScale(bounds);
  if (zoomer_ != nullptr) {
    updateZoomBaseFromPreferred(preferred);
    if (!yScaleLocked_) {
      zoomer_->zoom(0);
    }
  }
  rescale_ = true;
}

void PlotWidget::resetZoomVertical() {
  const BoundingRectangle preferred = getPreferredScale();
  BoundingRectangle bounds = getCurrentScale();
  if (preferred.isValid()) {
    bounds.getMinimum().setY(preferred.getMinimum().y());
    bounds.getMaximum().setY(preferred.getMaximum().y());
  }

  if (yScaleLocked_) {
    yScaleLocked_ = false;
    emit userScaleLockedChanged(isUserScaleLocked());
  }
  setCurrentScale(bounds);
  if (zoomer_ != nullptr) {
    updateZoomBaseFromPreferred(preferred);
    if (!xScaleLocked_) {
      zoomer_->zoom(0);
    }
  }
  rescale_ = true;
}

BoundingRectangle PlotWidget::mergePreferredWithLocked(const BoundingRectangle& preferred) const {
  if (!xScaleLocked_ && !yScaleLocked_) {
    return preferred;
  }

  BoundingRectangle bounds = getCurrentScale();
  if (!xScaleLocked_ && preferred.isValid()) {
    bounds.getMinimum().setX(preferred.getMinimum().x());
    bounds.getMaximum().setX(preferred.getMaximum().x());
  }
  if (!yScaleLocked_ && preferred.isValid()) {
    bounds.getMinimum().setY(preferred.getMinimum().y());
    bounds.getMaximum().setY(preferred.getMaximum().y());
  }
  return bounds;
}

void PlotWidget::updateZoomBaseFromPreferred(const BoundingRectangle& preferred) {
  if (zoomer_ == nullptr) {
    return;
  }

  QRectF base = preferred.isValid() ? preferred.getRectangle() : zoomer_->zoomBase();
  if (xScaleLocked_) {
    base.setLeft(currentBounds_.getMinimum().x());
    base.setRight(currentBounds_.getMaximum().x());
  }
  if (yScaleLocked_) {
    base.setTop(currentBounds_.getMinimum().y());
    base.setBottom(currentBounds_.getMaximum().y());
  }
  zoomer_->setZoomBase(base);
}

void PlotWidget::setOpenGLCanvasEnabled(bool enabled) {
  const bool available = openGLPlotCanvasAvailable();
  warnIfOpenGLCanvasFallback(enabled, available);
  const bool wantOpenGL = enabled && available;
  if (isOpenGLPlotCanvas(ui_->plot->canvas()) == wantOpenGL) {
    return;
  }

  QPalette canvasPalette;
  if (QWidget* canvas = ui_->plot->canvas()) {
    canvasPalette = canvas->palette();
  }

  destroyCanvasPickers();
  ui_->plot->setCanvas(createPlotCanvas(ui_->plot, wantOpenGL));
  if (QWidget* canvas = ui_->plot->canvas()) {
    canvas->setPalette(canvasPalette);
  }
  ui_->plot->invalidateLayoutCache();
  createCanvasPickers();
  applyPlotChrome();
  emit canvasChanged();
}

void PlotWidget::setPlotTitleStyle(const PlotTitleStyle& style) {
  plotTitleStyle_ = style;
  plotTitleStyle_.fontSize = PlotTitleStyle::clampFontSize(plotTitleStyle_.fontSize);
  if (!plotTitleStyle_.customColor.isValid()) {
    plotTitleStyle_.customColor = PlotTitleStyle::factory().customColor;
  }
  applyPlotTitleStyle();
}

PlotTitleStyle PlotWidget::plotTitleStyle() const {
  return plotTitleStyle_;
}

bool PlotWidget::isOpenGLCanvasEnabled() const {
  return isOpenGLPlotCanvas(ui_->plot->canvas());
}

void PlotWidget::createCanvasPickers() {
  QWidget* canvas = ui_->plot->canvas();
  configurePlotCanvas(canvas);
  cursor_ = new PlotCursor(canvas);
  magnifier_ = new PlotMagnifier(canvas);
  panner_ = new PlotPanner(canvas);
  zoomer_ = new PlotZoomer(canvas);
  zoomer_->setTrackerMode(QwtPicker::AlwaysOff);
  connect(zoomer_, SIGNAL(zoomed(const QRectF&)), this, SLOT(plotZoomed(const QRectF&)));
  connect(zoomer_, SIGNAL(contextMenuRequested(QPoint)), this, SLOT(showPlotContextMenu(QPoint)));
  canvas->setFocusPolicy(Qt::StrongFocus);
  canvas->installEventFilter(this);
}

void PlotWidget::destroyCanvasPickers() {
  if (zoomer_ != nullptr) {
    disconnect(zoomer_, nullptr, this, nullptr);
  }
  delete zoomer_;
  zoomer_ = nullptr;
  delete magnifier_;
  magnifier_ = nullptr;
  delete panner_;
  panner_ = nullptr;
  delete cursor_;
  cursor_ = nullptr;
}

void PlotWidget::updateGridPen() {
  if (grid_ == nullptr) {
    return;
  }
  QColor penColor = gridForegroundColor_;
  penColor.setAlpha(64);
  QPen pen(penColor, 0.0, Qt::DotLine);
  grid_->setMajorPen(pen);
  if (gridVisible_) {
    requestReplot();
  }
}

void PlotWidget::buildSplitMenu() {
  auto* grid = new QWidget();
  grid->setObjectName("splitDirectionGrid");

  auto* layout = new QGridLayout(grid);
  layout->setContentsMargins(2, 2, 2, 2);
  layout->setSpacing(0);

  const auto addButton = [this, layout](int row, int column, const QString& objectName, const QString& iconPath, const QString& toolTip,
                                        const char* slot) {
    auto* button = new QToolButton();
    button->setObjectName(objectName);
    setThemeIcon(button, iconPath, QSize(18, 18));
    button->setFixedSize(24, 24);
    button->setToolTip(toolTip);
    button->setAutoRaise(true);
    button->setCursor(Qt::PointingHandCursor);
    button->setFocusPolicy(Qt::NoFocus);
    layout->addWidget(button, row, column);
    connect(button, SIGNAL(clicked()), this, slot);
    connect(button, SIGNAL(clicked()), menuSplit_, SLOT(hide()));
  };

  addButton(0, 0, "toolButtonSplitLeft", "resource/split/split-left.svg", "Split left", SLOT(menuSplitLeftTriggered()));
  addButton(0, 1, "toolButtonSplitRight", "resource/split/split-right.svg", "Split right", SLOT(menuSplitRightTriggered()));
  addButton(1, 0, "toolButtonSplitUp", "resource/split/split-up.svg", "Split up", SLOT(menuSplitTopTriggered()));
  addButton(1, 1, "toolButtonSplitDown", "resource/split/split-down.svg", "Split down", SLOT(menuSplitBottomTriggered()));

  auto* action = new QWidgetAction(menuSplit_);
  action->setDefaultWidget(grid);
  menuSplit_->addAction(action);
}

void PlotWidget::buildContextMenu() {
  menuContext_->setObjectName(QStringLiteral("plotContextMenu"));

  actionContextResetZoom_ = menuContext_->addAction(tr("Reset zoom"), this, SLOT(menuResetZoomTriggered()));
  actionContextResetZoom_->setObjectName(QStringLiteral("actionContextResetZoom"));
  setContextMenuIcon(actionContextResetZoom_, QStringLiteral("resource/zoom-reset.svg"));

  actionContextResetZoomHorizontal_ = menuContext_->addAction(tr("Zoom out horizontally"), this, SLOT(menuResetZoomHorizontalTriggered()));
  actionContextResetZoomHorizontal_->setObjectName(QStringLiteral("actionContextResetZoomHorizontal"));
  setContextMenuIcon(actionContextResetZoomHorizontal_, QStringLiteral("resource/zoom-reset-horizontally.svg"));

  actionContextResetZoomVertical_ = menuContext_->addAction(tr("Zoom out vertically"), this, SLOT(menuResetZoomVerticalTriggered()));
  actionContextResetZoomVertical_->setObjectName(QStringLiteral("actionContextResetZoomVertical"));
  setContextMenuIcon(actionContextResetZoomVertical_, QStringLiteral("resource/zoom-reset-vertically.svg"));

  menuContext_->addSeparator();

  actionContextConfigure_ = menuContext_->addAction(tr("Configure plot..."), this, SLOT(pushButtonSetupClicked()));
  actionContextConfigure_->setObjectName(QStringLiteral("actionContextConfigure"));
  setContextMenuIcon(actionContextConfigure_, QStringLiteral("resource/settings-edit.svg"));

  menuContextSplit_->setObjectName(QStringLiteral("menuContextSplit"));
  menuContext_->addMenu(menuContextSplit_);
  setContextMenuIcon(menuContextSplit_->menuAction(), QStringLiteral("resource/split/layout.svg"));
  setContextMenuIcon(menuContextSplit_->addAction(tr("Split left"), this, SLOT(menuSplitLeftTriggered())),
                     QStringLiteral("resource/split/split-left.svg"));
  setContextMenuIcon(menuContextSplit_->addAction(tr("Split right"), this, SLOT(menuSplitRightTriggered())),
                     QStringLiteral("resource/split/split-right.svg"));
  setContextMenuIcon(menuContextSplit_->addAction(tr("Split up"), this, SLOT(menuSplitTopTriggered())),
                     QStringLiteral("resource/split/split-up.svg"));
  setContextMenuIcon(menuContextSplit_->addAction(tr("Split down"), this, SLOT(menuSplitBottomTriggered())),
                     QStringLiteral("resource/split/split-down.svg"));

  actionContextShowLegend_ = menuContext_->addAction(QString(), this, SLOT(menuToggleLegendTriggered()));
  actionContextShowLegend_->setObjectName(QStringLiteral("actionContextShowLegend"));
  setContextMenuIcon(actionContextShowLegend_, QStringLiteral("resource/legend.svg"));

  actionContextMaximizeRestore_ = menuContext_->addAction(QString(), this, SLOT(pushButtonStateClicked()));
  actionContextMaximizeRestore_->setObjectName(QStringLiteral("actionContextMaximizeRestore"));

  actionContextRunPause_ = menuContext_->addAction(QString(), this, SLOT(pushButtonRunPauseClicked()));
  actionContextRunPause_->setObjectName(QStringLiteral("actionContextRunPause"));

  actionContextClear_ = menuContext_->addAction(tr("Clear"), this, SLOT(pushButtonClearClicked()));
  actionContextClear_->setObjectName(QStringLiteral("actionContextClear"));
  setContextMenuIcon(actionContextClear_, QStringLiteral("resource/delete-data.svg"));

  actionContextClose_ = menuContext_->addAction(tr("Close"), this, SLOT(pushButtonCloseClicked()));
  actionContextClose_->setObjectName(QStringLiteral("actionContextClose"));
  setContextMenuIcon(actionContextClose_, QStringLiteral("resource/close.svg"));

  menuContext_->addSeparator();

  actionContextCopyImage_ = menuContext_->addAction(tr("Copy image"), this, SLOT(menuCopyImageTriggered()));
  actionContextCopyImage_->setObjectName(QStringLiteral("actionContextCopyImage"));
  setContextMenuIcon(actionContextCopyImage_, QStringLiteral("resource/copy.svg"));

  actionContextSaveImage_ = menuContext_->addAction(tr("Save image..."), this, SLOT(menuExportImageFileTriggered()));
  actionContextSaveImage_->setObjectName(QStringLiteral("actionContextSaveImage"));
  setContextMenuIcon(actionContextSaveImage_, QStringLiteral("resource/data-export.svg"));

  actionContextSaveData_ = menuContext_->addAction(tr("Save data..."), this, SLOT(menuExportTextFileTriggered()));
  actionContextSaveData_->setObjectName(QStringLiteral("actionContextSaveData"));
  setContextMenuIcon(actionContextSaveData_, QStringLiteral("resource/data-export.svg"));

  actionContextDataStatistics_ = menuContext_->addAction(tr("Data statistics..."), this, SLOT(menuDataStatisticsTriggered()));
  actionContextDataStatistics_->setObjectName(QStringLiteral("actionContextDataStatistics"));
  setContextMenuIcon(actionContextDataStatistics_, QStringLiteral("resource/data-statistics.svg"));
}

void PlotWidget::updateContextMenuState() {
  if (actionContextMaximizeRestore_ != nullptr) {
    actionContextMaximizeRestore_->setText((state_ == Maximized) ? tr("Restore") : tr("Maximize"));
    actionContextMaximizeRestore_->setEnabled(canChangeState());
    setContextMenuIcon(actionContextMaximizeRestore_,
                       (state_ == Maximized) ? QStringLiteral("resource/minimize.svg") : QStringLiteral("resource/maximize.svg"));
  }
  if (actionContextRunPause_ != nullptr) {
    actionContextRunPause_->setText(paused_ ? tr("Run") : tr("Pause"));
    setContextMenuIcon(actionContextRunPause_, paused_ ? QStringLiteral("resource/play.svg") : QStringLiteral("resource/pause.svg"));
  }
  if (actionContextClose_ != nullptr) {
    actionContextClose_->setEnabled(canClose());
  }
  if ((actionContextShowLegend_ != nullptr) && (config_ != nullptr) && (config_->getLegendConfig() != nullptr)) {
    const bool legendVisible = config_->getLegendConfig()->isVisible();
    actionContextShowLegend_->setText(legendVisible ? tr("Hide legend") : tr("Show legend"));
    setContextMenuIcon(actionContextShowLegend_, QStringLiteral("resource/legend.svg"));
  }
}

void PlotWidget::showPlotContextMenu(const QPoint& globalPos) {
  updateContextMenuState();
  menuContext_->popup(globalPos);
}

void PlotWidget::run() {
  if (paused_) {
    paused_ = false;

    for (int index = 0; index < curves_.count(); ++index) {
      curves_[index]->run();
    }

    ui_->pushButtonRunPause->setIcon(pauseIcon_);

    emit pausedChanged(false);
  }
}

void PlotWidget::pause() {
  if (!paused_) {
    for (int index = 0; index < curves_.count(); ++index) {
      curves_[index]->pause();
    }

    paused_ = true;

    ui_->pushButtonRunPause->setIcon(runIcon_);

    emit pausedChanged(true);
  }
}

void PlotWidget::clear() {
  for (int index = 0; index < curves_.count(); ++index) {
    curves_[index]->clear();
  }

  resetAxisOrigins();
  forceReplot();

  emit cleared();
}

void PlotWidget::requestReplot() {
  replot_ = true;
}

void PlotWidget::forceReplot() {
  if (replotting_) {
    return;
  }
  const BoolGuard replotGuard(replotting_);

  BoundingRectangle preferredBounds = getPreferredScale();

  if (shouldApplyPreferredScale(rescale_, xScaleLocked_, yScaleLocked_)) {
    emit preferredScaleChanged(mergePreferredWithLocked(preferredBounds));

    rescale_ = false;
  }

  if (zoomer_ != nullptr) {
    if (!xScaleLocked_ && !yScaleLocked_) {
      zoomer_->setZoomBase(preferredBounds.getRectangle());
    } else {
      updateZoomBaseFromPreferred(preferredBounds);
    }
  }

  ui_->plot->replot();

  replot_ = false;
}

void PlotWidget::renderToPainter(QPainter& painter, const QRectF& bounds) {
  QRectF plotBounds = bounds;

  if (plotBounds.isEmpty() && (painter.device() != nullptr)) {
    plotBounds = QRectF(0, 0, painter.device()->width(), painter.device()->height());
  }

  painter.fillRect(plotBounds, ui_->plot->canvasBackground());

  QwtPlotRenderer renderer;

  qreal textHeight = 0;

  if (config_ != nullptr) {
    const QFont titleFont = plotTitleStyle_.toFont(painter.font());
    painter.setFont(titleFont);
    painter.setPen(plotTitleStyle_.resolvedColor(Theme::currentId()));
    textHeight = painter.fontMetrics().boundingRect(config_->getTitle()).height();

    painter.drawText(QRectF(plotBounds.x(), plotBounds.y(), plotBounds.width(), textHeight), Qt::AlignHCenter | Qt::AlignVCenter,
                     config_->getTitle());
  }

  renderer.render(ui_->plot, &painter,
                  QRectF(plotBounds.x(), plotBounds.y() + textHeight + 10, plotBounds.width(), plotBounds.height() - textHeight - 10));
}

void PlotWidget::renderToPixmap(QPixmap& pixmap, const QRectF& bounds) {
  QPainter painter(&pixmap);
  renderToPainter(painter, bounds.isEmpty() ? QRectF(0, 0, pixmap.width(), pixmap.height()) : bounds);
}

void PlotWidget::writeFormattedCurveData(QList<QStringList>& formattedData) {
  formattedData.clear();

  for (int index = 0; index < curves_.count(); ++index) {
    QStringList formattedX;
    QStringList formattedY;

    curves_[index]->getData()->writeFormatted(formattedX, formattedY);

    formattedData.append(formattedX);
    formattedData.append(formattedY);
  }
}

void PlotWidget::writeFormattedCurveAxisTitles(QStringList& formattedAxisTitles) {
  formattedAxisTitles.clear();

  for (int index = 0; index < curves_.count(); ++index) {
    CurveAxisConfig* xAxisConfig = curves_[index]->getConfig()->getAxisConfig(CurveConfig::X);
    CurveAxisConfig* yAxisConfig = curves_[index]->getConfig()->getAxisConfig(CurveConfig::Y);

    QString xAxisTitle = xAxisConfig->getTopic();
    QString yAxisTitle = yAxisConfig->getTopic();

    xAxisTitle += "/" + xAxisConfig->getFieldLabel();
    yAxisTitle += "/" + yAxisConfig->getFieldLabel();

    formattedAxisTitles.append(xAxisTitle);
    formattedAxisTitles.append(yAxisTitle);
  }
}

void PlotWidget::saveToImageFile(const QString& fileName) {
  renderExportImage(fileName, [this](QPainter& painter, const QRectF& bounds) { renderToPainter(painter, bounds); });
}

void PlotWidget::saveToTextFile(const QString& fileName) {
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

void PlotWidget::dragEnterEvent(QDragEnterEvent* event) {
  if (acceptsDrop(event->mimeData(), event->source())) {
    event->acceptProposedAction();
  } else {
    event->ignore();
  }
}

void PlotWidget::dropEvent(QDropEvent* event) {
  const QMimeData* mimeData = event->mimeData();
  if (!acceptsDrop(mimeData, event->source())) {
    event->ignore();
    return;
  }

  if (mimeData->hasFormat(CurveConfig::MimeType)) {
    QByteArray data = mimeData->data(CurveConfig::MimeType);
    QDataStream stream(&data, QIODevice::ReadOnly);

    CurveConfig* curveConfig = config_->addCurve();
    stream >> *curveConfig;
    makeCurveTitleUnique(curveConfig);
  } else {
    const QVector<TopicFieldRef> refs = decodeTopicFields(mimeData->data(kTopicFieldsMimeType));
    if (requiresDropConfirmation(static_cast<int>(refs.count())) &&
        (QMessageBox::question(this, tr("Add curves"), tr("Add %1 curves to this plot?").arg(refs.count())) != QMessageBox::Yes)) {
      event->ignore();
      return;
    }
    addTopicFieldCurves(refs);
  }

  event->acceptProposedAction();
}

bool PlotWidget::acceptsDrop(const QMimeData* mimeData, const QObject* source) const {
  if ((config_ == nullptr) || (mimeData == nullptr)) {
    return false;
  }
  if (mimeData->hasFormat(CurveConfig::MimeType)) {
    return source != legend_;
  }
  return mimeData->hasFormat(kTopicFieldsMimeType);
}

void PlotWidget::addTopicFieldCurves(const QVector<TopicFieldRef>& refs) {
  for (const auto& ref : refs) {
    CurveConfig* curveConfig = config_->addCurve();
    fillCurveFromTopicField(*curveConfig, ref);
    makeCurveTitleUnique(curveConfig);
  }
}

void PlotWidget::makeCurveTitleUnique(CurveConfig* curveConfig) const {
  while (config_->findCurves(curveConfig->getTitle()).count() > 1) {
    curveConfig->setTitle("Copy of " + curveConfig->getTitle());
  }
}

bool PlotWidget::eventFilter(QObject* object, QEvent* event) {
  if (object == ui_->plot->canvas()) {
    if (event->type() == QEvent::KeyPress) {
      const auto* keyEvent = dynamic_cast<QKeyEvent*>(event);
      if ((keyEvent != nullptr) && (keyEvent->key() == Qt::Key_Home)) {
        resetZoom();
        return true;
      }
    }
  } else if ((object == ui_->plot->axisWidget(QwtPlot::yLeft)) && (event->type() == QEvent::Resize)) {
    ui_->horizontalSpacerLeft->changeSize(ui_->plot->axisWidget(QwtPlot::yLeft)->width(), 20);
    layout()->update();
  } else if ((object == ui_->plot->axisWidget(QwtPlot::yRight)) && (event->type() == QEvent::Resize)) {
    ui_->horizontalSpacerRight->changeSize(ui_->plot->axisWidget(QwtPlot::yRight)->width() - 5, 20);
    layout()->update();
  }

  return false;
}

void PlotWidget::changeEvent(QEvent* event) {
  QWidget::changeEvent(event);
  if ((event->type() == QEvent::PaletteChange) || (event->type() == QEvent::StyleChange)) {
    applyPlotChrome();
  }
}

void PlotWidget::applyPlotChrome() {
  if (ui_->plot == nullptr) {
    return;
  }

  const QPalette pal = palette();
  const QColor background = pal.color(QPalette::Window);
  const QColor foreground = pal.color(QPalette::WindowText);
  ui_->plot->setCanvasBackground(background);
  if (QWidget* canvas = ui_->plot->canvas()) {
    QPalette canvasPalette = canvas->palette();
    canvasPalette.setColor(QPalette::Window, background);
    canvasPalette.setColor(QPalette::Base, background);
    canvasPalette.setColor(QPalette::WindowText, foreground);
    canvasPalette.setColor(QPalette::Text, foreground);
    canvas->setPalette(canvasPalette);
  }

  if (cursor_ != nullptr) {
    cursor_->updateOverlayPens();
  }
  if (zoomer_ != nullptr) {
    zoomer_->updateOverlayPens();
  }

  const std::array<QwtPlot::Axis, 4> axes = {QwtPlot::xBottom, QwtPlot::xTop, QwtPlot::yLeft, QwtPlot::yRight};
  for (QwtPlot::Axis axis : axes) {
    if (QWidget* axisWidget = ui_->plot->axisWidget(axis)) {
      axisWidget->setPalette(pal);
    }
  }

  setGridForegroundColor(foreground);
  refreshStatefulIcons();
  if (config_ != nullptr) {
    updateAxisTitle(PlotAxesConfig::X);
    updateAxisTitle(PlotAxesConfig::Y);
  }
  applyPlotTitleStyle();
}

void PlotWidget::applyPlotTitleStyle() {
  if (ui_->lineEditTitle == nullptr) {
    return;
  }

  ui_->lineEditTitle->setFont(plotTitleStyle_.toFont(ui_->lineEditTitle->font()));

  QPalette pal = ui_->lineEditTitle->palette();
  if (plotTitleStyle_.autoColor) {
    pal.setColor(QPalette::Text, palette().color(QPalette::Text));
  } else {
    pal.setColor(QPalette::Text, plotTitleStyle_.customColor);
  }
  ui_->lineEditTitle->setPalette(pal);
  lineEditTitleTextChanged(ui_->lineEditTitle->text());
}

void PlotWidget::refreshStatefulIcons() {
  runIcon_ = packageIcon(QStringLiteral("resource/play.svg"), QSize(16, 16));
  pauseIcon_ = packageIcon(QStringLiteral("resource/pause.svg"), QSize(16, 16));
  normalIcon_ = packageIcon(QStringLiteral("resource/maximize.svg"), QSize(16, 16));
  maximizedIcon_ = packageIcon(QStringLiteral("resource/minimize.svg"), QSize(16, 16));
  ui_->pushButtonRunPause->setIcon(paused_ ? runIcon_ : pauseIcon_);
  ui_->pushButtonState->setIcon((state_ == Maximized) ? maximizedIcon_ : normalIcon_);
}

void PlotWidget::updateAxisTitle(PlotAxesConfig::Axis axis) {
  QwtPlot::Axis plotAxis = (axis == PlotAxesConfig::Y) ? QwtPlot::yLeft : QwtPlot::xBottom;
  CurveConfig::Axis curveAxis = (axis == PlotAxesConfig::Y) ? CurveConfig::Y : CurveConfig::X;

  PlotAxisConfig* plotAxisConfig = config_->getAxesConfig()->getAxisConfig(axis);

  if (plotAxisConfig->isTitleVisible()) {
    if (plotAxisConfig->getTitleType() == PlotAxisConfig::AutoTitle) {
      QStringList titleParts;

      for (size_t index = 0; index < config_->getNumCurves(); ++index) {
        CurveAxisConfig* curveAxisConfig = config_->getCurveConfig(index)->getAxisConfig(curveAxis);

        QString titlePart = curveAxisConfig->getTopic() + "/" + curveAxisConfig->getFieldLabel();

        if (!titleParts.contains(titlePart)) {
          titleParts.append(titlePart);
        }
      }

      ui_->plot->setAxisTitle(plotAxis, axisTitleWithColor(titleParts.join(", "), palette().color(QPalette::WindowText)));
    } else {
      ui_->plot->setAxisTitle(plotAxis, axisTitleWithColor(plotAxisConfig->getCustomTitle(), palette().color(QPalette::WindowText)));
    }
  } else {
    ui_->plot->setAxisTitle(plotAxis, QwtText());
  }
}

bool PlotWidget::axisLabelsFromZero(CurveConfig::Axis axis) const {
  if (axis == CurveConfig::X) {
    return (timeAxisFormat_ == PlotTableConfig::StartFromZero) && axisUsesTimeFormat(CurveConfig::X);
  }

  if (config_ == nullptr) {
    return false;
  }

  for (size_t index = 0; index < config_->getNumCurves(); ++index) {
    if (config_->getCurveConfig(index)->getAxisConfig(axis)->isLabelFromZero()) {
      return true;
    }
  }

  return false;
}

bool PlotWidget::axisUsesTimeFormat(CurveConfig::Axis axis) const {
  if (config_ == nullptr) {
    return false;
  }

  for (size_t index = 0; index < config_->getNumCurves(); ++index) {
    if (config_->getCurveConfig(index)->getAxisConfig(axis)->usesTimeScale()) {
      return true;
    }
  }

  return false;
}

void PlotWidget::seedAxisOrigin(CurveConfig::Axis axis) {
  for (auto* curve : curves_) {
    CurveConfig* curveConfig = curve->getConfig();
    if (curveConfig == nullptr) {
      continue;
    }
    CurveAxisConfig* axisConfig = curveConfig->getAxisConfig(axis);
    if (axis == CurveConfig::X) {
      if (!axisConfig->usesTimeScale()) {
        continue;
      }
    } else if (!axisConfig->isLabelFromZero()) {
      continue;
    }
    CurveData* data = curve->getData();
    if ((data != nullptr) && !data->isEmpty()) {
      bindAxisOrigin(axis, data->getValue(0, axis));
      return;
    }
  }
}

void PlotWidget::resetAxisOrigins() {
  xOriginSet_ = false;
  yOriginSet_ = false;
  xOrigin_ = 0.0;
  yOrigin_ = 0.0;
  updateAxisTimeLabels();
}

void PlotWidget::updateAxisTimeLabels() {
  if (!axisLabelsFromZero(CurveConfig::X)) {
    xOriginSet_ = false;
    xOrigin_ = 0.0;
  } else if (!xOriginSet_) {
    seedAxisOrigin(CurveConfig::X);
  }
  if (!axisLabelsFromZero(CurveConfig::Y)) {
    yOriginSet_ = false;
    yOrigin_ = 0.0;
  } else if (!yOriginSet_) {
    seedAxisOrigin(CurveConfig::Y);
  }

  applyAxisTimeOffsets();
}

void PlotWidget::applyAxisTimeOffsets() {
  const double xOffset = (axisLabelsFromZero(CurveConfig::X) && xOriginSet_) ? xOrigin_ : 0.0;
  const double yOffset = (axisLabelsFromZero(CurveConfig::Y) && yOriginSet_) ? yOrigin_ : 0.0;
  const bool xTimeScale = axisUsesTimeFormat(CurveConfig::X);
  const bool yTimeScale = axisUsesTimeFormat(CurveConfig::Y);

  AxisTimeFormat::LabelMode xMode = AxisTimeFormat::LabelMode::Off;
  if (xTimeScale) {
    switch (timeAxisFormat_) {
      case PlotTableConfig::StartFromZero:
        xMode = AxisTimeFormat::LabelMode::Relative;
        break;
      case PlotTableConfig::DateTime:
        xMode = AxisTimeFormat::LabelMode::DateTime;
        break;
      case PlotTableConfig::Timestamp:
      default:
        xMode = AxisTimeFormat::LabelMode::Timestamp;
        break;
    }
  }
  const AxisTimeFormat::LabelMode yMode = yTimeScale ? AxisTimeFormat::LabelMode::Relative : AxisTimeFormat::LabelMode::Off;

  if (auto* draw = dynamic_cast<OffsetScaleDraw*>(ui_->plot->axisScaleDraw(QwtPlot::xBottom))) {
    draw->setTimeLabelMode(xMode);
    draw->setOffset(xOffset);
    draw->setTimeZone(timeZone_);
  }
  if (auto* engine = dynamic_cast<OffsetScaleEngine*>(ui_->plot->axisScaleEngine(QwtPlot::xBottom))) {
    engine->setOffset(xOffset);
  }
  if (auto* draw = dynamic_cast<OffsetScaleDraw*>(ui_->plot->axisScaleDraw(QwtPlot::yLeft))) {
    draw->setTimeLabelMode(yMode);
    draw->setOffset(yOffset);
    draw->setTimeZone(timeZone_);
  }
  if (auto* engine = dynamic_cast<OffsetScaleEngine*>(ui_->plot->axisScaleEngine(QwtPlot::yLeft))) {
    engine->setOffset(yOffset);
  }
  if (cursor_ != nullptr) {
    cursor_->setXTimeLabelMode(xMode);
    cursor_->setYTimeLabelMode(yMode);
    cursor_->setXOffset(xOffset);
    cursor_->setYOffset(yOffset);
    cursor_->setTimeZone(timeZone_);
  }

  if (currentBounds_.isValid()) {
    ui_->plot->setAxisScale(QwtPlot::xBottom, currentBounds_.getMinimum().x(), currentBounds_.getMaximum().x());
    ui_->plot->setAxisScale(QwtPlot::yLeft, currentBounds_.getMinimum().y(), currentBounds_.getMaximum().y());
  }

  relayoutScaleWidget(ui_->plot->axisWidget(QwtPlot::xBottom));
  relayoutScaleWidget(ui_->plot->axisWidget(QwtPlot::yLeft));
  ui_->plot->invalidateLayoutCache();
  forceReplot();
}

void PlotWidget::bindAxisOrigin(CurveConfig::Axis axis, double value) {
  if (!axisLabelsFromZero(axis)) {
    return;
  }

  bool& originSet = (axis == CurveConfig::X) ? xOriginSet_ : yOriginSet_;
  double& origin = (axis == CurveConfig::X) ? xOrigin_ : yOrigin_;
  if (originSet) {
    return;
  }

  originSet = true;
  origin = value;
  applyAxisTimeOffsets();
}

void PlotWidget::timerTimeout() {
  if (replot_) {
    forceReplot();
  }
}

void PlotWidget::configTitleChanged(const QString& /*title*/) {
  ui_->lineEditTitle->setText(config_->getTitle());
}

void PlotWidget::configCurveAdded(size_t index) {
  auto* curve = new PlotCurve(this);

  curve->attach(ui_->plot);
  curve->setConfig(config_->getCurveConfig(index));
  curve->setBroker(broker_);

  connect(curve, SIGNAL(replotRequested()), this, SLOT(curveReplotRequested()));

  curves_.insert(static_cast<int>(index), curve);

  configXAxisConfigChanged();
  configYAxisConfigChanged();
  updateAxisTimeLabels();
  applyPlotTimeWindow();

  forceReplot();
}

void PlotWidget::configCurveRemoved(size_t index) {
  curves_[static_cast<int>(index)]->detach();

  delete curves_[static_cast<int>(index)];

  curves_.remove(static_cast<int>(index));

  configXAxisConfigChanged();
  configYAxisConfigChanged();
  updateAxisTimeLabels();

  forceReplot();
}

void PlotWidget::configCurvesCleared() {
  for (int index = 0; index < curves_.count(); ++index) {
    curves_[index]->detach();

    delete curves_[index];
  }

  curves_.clear();

  configXAxisConfigChanged();
  configYAxisConfigChanged();
  updateAxisTimeLabels();

  forceReplot();
}

void PlotWidget::configCurveConfigChanged(size_t /*index*/) {
  configXAxisConfigChanged();
  configYAxisConfigChanged();
  updateAxisTimeLabels();
  applyPlotTimeWindow();
}

void PlotWidget::applyPlotTimeWindow() {
  if (config_ == nullptr) {
    return;
  }

  const bool apply = config_->isTimeWindowEnabled() && config_->canApplyTimeWindow();
  const std::optional<int> length = apply ? std::optional<int>(config_->getTimeWindowLength()) : std::nullopt;

  for (PlotCurve* curve : curves_) {
    curve->setPlotTimeWindowLength(length);
  }
}

void PlotWidget::configTimeWindowEnabledChanged(bool /*enabled*/) {
  applyPlotTimeWindow();
}

void PlotWidget::configTimeWindowLengthChanged(int /*length*/) {
  applyPlotTimeWindow();
}

void PlotWidget::configXAxisConfigChanged() {
  updateAxisTitle(PlotAxesConfig::X);
}

void PlotWidget::configYAxisConfigChanged() {
  updateAxisTitle(PlotAxesConfig::Y);
}

void PlotWidget::configLegendConfigChanged() {
  if ((legend_ == nullptr) && config_->getLegendConfig()->isVisible()) {
    legend_ = new PlotLegend(this);
    ui_->plot->insertLegend(legend_, QwtPlot::TopLegend);
  } else if ((legend_ != nullptr) && !config_->getLegendConfig()->isVisible()) {
    ui_->plot->insertLegend(nullptr);
    legend_ = nullptr;
  }
}

void PlotWidget::configPlotRateChanged(double rate) {
  timer_->setInterval(static_cast<int>(1e3 / rate));
}

void PlotWidget::curveReplotRequested() {
  rescale_ = true;

  requestReplot();
}

void PlotWidget::lineEditTitleTextChanged(const QString& text) {
  QFontMetrics fontMetrics(ui_->lineEditTitle->font());

  ui_->lineEditTitle->setMinimumWidth(std::max(100, fontMetrics.horizontalAdvance(text) + 10));
}

void PlotWidget::lineEditTitleEditingFinished() {
  if (config_ != nullptr) {
    config_->setTitle(ui_->lineEditTitle->text());
  }
}

void PlotWidget::pushButtonRunPauseClicked() {
  if (paused_) {
    run();
  } else {
    pause();
  }
}

void PlotWidget::pushButtonClearClicked() {
  clear();
}

void PlotWidget::pushButtonSetupClicked() {
  if (config_ != nullptr) {
    PlotConfigDialog dialog(this);

    dialog.setWindowTitle(config_->getTitle().isEmpty() ? "Configure Plot" : "Configure \"" + config_->getTitle() + "\"");
    dialog.getWidget()->setConfig(*config_);

    if (dialog.exec() == QDialog::Accepted) {
      *config_ = dialog.getWidget()->getConfig();
    }
  }
}

void PlotWidget::pushButtonStateClicked() {
  if (state_ == Maximized) {
    setState(Normal);
  } else {
    setState(Maximized);
  }
}

void PlotWidget::pushButtonSplitClicked() {
  menuSplit_->popup(QCursor::pos());
}

void PlotWidget::pushButtonCloseClicked() {
  if (canClose()) {
    emit closeRequested();
  }
}

void PlotWidget::menuSplitLeftTriggered() {
  emit splitRequested(Qt::Horizontal, true);
}

void PlotWidget::menuSplitRightTriggered() {
  emit splitRequested(Qt::Horizontal, false);
}

void PlotWidget::menuSplitTopTriggered() {
  emit splitRequested(Qt::Vertical, true);
}

void PlotWidget::menuSplitBottomTriggered() {
  emit splitRequested(Qt::Vertical, false);
}

void PlotWidget::configDestroyed() {
  for (PlotCurve* curve : curves_) {
    curve->setConfig(nullptr);
  }
  config_ = nullptr;
}

void PlotWidget::menuExportImageFileTriggered() {
  QFileDialog dialog(this, "Save Image File", QDir::homePath(),
                     "Portable Network Graphics (*.png);;Scalable Vector Graphics (*.svg);;Portable Document Format (*.pdf)");

  dialog.setAcceptMode(QFileDialog::AcceptSave);
  dialog.setFileMode(QFileDialog::AnyFile);
  dialog.selectFile("rqt_multiplot.png");

  if (dialog.exec() == QDialog::Accepted) {
    const auto files = dialog.selectedFiles();
    if (!files.isEmpty()) {
      saveToImageFile(ensureFileSuffix(files.first(), suffixFromNameFilter(dialog.selectedNameFilter())));
    }
  }
}

void PlotWidget::menuExportTextFileTriggered() {
  QFileDialog dialog(this, "Save Text File", QDir::homePath(), "Text file (*.txt);;CSV (*.csv)");

  dialog.setAcceptMode(QFileDialog::AcceptSave);
  dialog.setFileMode(QFileDialog::AnyFile);
  dialog.selectFile("rqt_multiplot.txt");

  if (dialog.exec() == QDialog::Accepted) {
    const auto files = dialog.selectedFiles();
    if (!files.isEmpty()) {
      saveToTextFile(ensureFileSuffix(files.first(), suffixFromNameFilter(dialog.selectedNameFilter())));
    }
  }
}

void PlotWidget::menuResetZoomTriggered() {
  resetZoom();
}

void PlotWidget::menuResetZoomHorizontalTriggered() {
  resetZoomHorizontal();
}

void PlotWidget::menuResetZoomVerticalTriggered() {
  resetZoomVertical();
}

void PlotWidget::menuToggleLegendTriggered() {
  if ((config_ != nullptr) && (config_->getLegendConfig() != nullptr)) {
    config_->getLegendConfig()->setVisible(!config_->getLegendConfig()->isVisible());
  }
}

void PlotWidget::menuCopyImageTriggered() {
  if (ui_->plot == nullptr) {
    return;
  }

  const QSize size(kExportImageWidth, kExportImageHeight);
  QPixmap pixmap(size);
  pixmap.fill(ui_->plot->canvasBackground().color());
  renderToPixmap(pixmap);
  QApplication::clipboard()->setPixmap(pixmap);
}

void PlotWidget::menuDataStatisticsTriggered() {
  if (dataStatisticsDialog_ == nullptr) {
    dataStatisticsDialog_ = new DataStatisticsDialog(this);
  }
  dataStatisticsDialog_->setPlot(this);
  dataStatisticsDialog_->show();
  dataStatisticsDialog_->raise();
  dataStatisticsDialog_->activateWindow();
}

void PlotWidget::plotXBottomScaleDivChanged() {
#if QWT_VERSION >= 0x060100
  const QwtScaleDiv& scale = ui_->plot->axisScaleDiv(QwtPlot::xBottom);
#else
  const QwtScaleDiv& scale = *ui_->plot->axisScaleDiv(QwtPlot::xBottom);
#endif

  ui_->plot->setAxisScaleDiv(QwtPlot::xTop, scale);

  currentBounds_.getMinimum().setX(scale.lowerBound());
  currentBounds_.getMaximum().setX(scale.upperBound());

  emit currentScaleChanged(currentBounds_);
}

void PlotWidget::plotYLeftScaleDivChanged() {
#if QWT_VERSION >= 0x060100
  const QwtScaleDiv& scale = ui_->plot->axisScaleDiv(QwtPlot::yLeft);
#else
  const QwtScaleDiv& scale = *ui_->plot->axisScaleDiv(QwtPlot::yLeft);
#endif

  ui_->plot->setAxisScaleDiv(QwtPlot::yRight, scale);

  currentBounds_.getMinimum().setY(scale.lowerBound());
  currentBounds_.getMaximum().setY(scale.upperBound());

  emit currentScaleChanged(currentBounds_);
}

void PlotWidget::plotZoomed(const QRectF& /*bounds*/) {
  const bool locked = zoomer_->zoomRectIndex() > 0;
  if ((xScaleLocked_ == locked) && (yScaleLocked_ == locked)) {
    return;
  }

  xScaleLocked_ = locked;
  yScaleLocked_ = locked;
  emit userScaleLockedChanged(locked);
}

}  // namespace rqt_multiplot
