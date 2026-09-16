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

#include <QCursor>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFontMetrics>
#include <QGridLayout>
#include <QMimeData>
#include <QPainter>
#include <QPixmap>
#include <QSize>
#include <QTextStream>
#include <QToolButton>
#include <QWidgetAction>

#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_canvas.h>
#include <qwt/qwt_plot_curve.h>
#include <qwt/qwt_plot_picker.h>
#include <qwt/qwt_plot_renderer.h>
#include <qwt/qwt_scale_widget.h>

#include <rqt_multiplot/PackageResource.h>
#include <rqt_multiplot/PlotExport.h>

#include <rqt_multiplot/CurveData.h>
#include <rqt_multiplot/OffsetScaleDraw.h>
#include <rqt_multiplot/OffsetScaleEngine.h>
#include <rqt_multiplot/PlotConfigDialog.h>
#include <rqt_multiplot/PlotConfigWidget.h>
#include <rqt_multiplot/PlotCursor.h>
#include <rqt_multiplot/PlotCurve.h>
#include <rqt_multiplot/PlotLegend.h>
#include <rqt_multiplot/PlotMagnifier.h>
#include <rqt_multiplot/PlotMouseBindings.h>
#include <rqt_multiplot/PlotPanner.h>
#include <rqt_multiplot/PlotZoomer.h>

#include <ui_PlotWidget.h>

#include "rqt_multiplot/PlotWidget.h"

namespace rqt_multiplot {

/*****************************************************************************/
/* Constructors and Destructor                                               */
/*****************************************************************************/

PlotWidget::PlotWidget(QWidget* parent)
    : QWidget(parent),
      ui_(new Ui::PlotWidget()),
      timer_(new QTimer(this)),
      menuImportExport_(new QMenu(this)),
      menuSplit_(new QMenu(this)),
      config_(nullptr),
      broker_(nullptr),
      legend_(nullptr),
      cursor_(nullptr),
      panner_(nullptr),
      magnifier_(nullptr),
      zoomer_(nullptr),
      paused_(true),
      rescale_(false),
      replot_(false),
      userScaleLocked_(false),
      state_(Normal),
      xOriginSet_(false),
      yOriginSet_(false),
      xOrigin_(0.0),
      yOrigin_(0.0) {
  qRegisterMetaType<BoundingRectangle>("BoundingRectangle");

  ui_->setupUi(this);

  setAcceptDrops(true);

  runIcon_ = QIcon(packageResourcePath("resource/16x16/run.png"));
  pauseIcon_ = QIcon(packageResourcePath("resource/16x16/pause.png"));
  normalIcon_ = QIcon(packageResourcePath("resource/16x16/zoom_in.png"));
  maximizedIcon_ = QIcon(packageResourcePath("resource/16x16/zoom_out.png"));

  ui_->pushButtonRunPause->setIcon(runIcon_);
  ui_->pushButtonClear->setIcon(QIcon(packageResourcePath("resource/16x16/clear.png")));
  ui_->pushButtonImportExport->setIcon(QIcon(packageResourcePath("resource/16x16/eject.png")));
  ui_->pushButtonSetup->setIcon(QIcon(packageResourcePath("resource/16x16/setup.png")));
  ui_->pushButtonSplit->setIcon(packageIcon("resource/split/layout.svg", QSize(16, 16)));
  ui_->pushButtonSplit->setIconSize(QSize(16, 16));
  ui_->pushButtonState->setIcon(normalIcon_);
  ui_->pushButtonClose->setIcon(packageIcon("resource/close.svg", QSize(16, 16)));
  ui_->pushButtonClose->setIconSize(QSize(16, 16));
  ui_->pushButtonClose->setEnabled(false);

  ui_->plot->setAutoReplot(false);
  ui_->plot->setAutoDelete(false);
  dynamic_cast<QFrame*>(ui_->plot->canvas())->setFrameStyle(QFrame::NoFrame);

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

  menuImportExport_->addAction("Export to image file...", this, SLOT(menuExportImageFileTriggered()));
  menuImportExport_->addAction("Export to text file...", this, SLOT(menuExportTextFileTriggered()));
  buildSplitMenu();

  auto* canvas = dynamic_cast<QwtPlotCanvas*>(ui_->plot->canvas());
  if (canvas != nullptr) {
    canvas->setContextMenuPolicy(Qt::NoContextMenu);
  }
  cursor_ = new PlotCursor(canvas);
  magnifier_ = new PlotMagnifier(canvas);
  panner_ = new PlotPanner(canvas);
  zoomer_ = new PlotZoomer(canvas);
  zoomer_->setTrackerMode(QwtPicker::AlwaysOff);

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
  connect(ui_->pushButtonImportExport, SIGNAL(clicked()), this, SLOT(pushButtonImportExportClicked()));
  connect(ui_->pushButtonSplit, SIGNAL(clicked()), this, SLOT(pushButtonSplitClicked()));
  connect(ui_->pushButtonState, SIGNAL(clicked()), this, SLOT(pushButtonStateClicked()));
  connect(ui_->pushButtonClose, SIGNAL(clicked()), this, SLOT(pushButtonCloseClicked()));

  connect(ui_->plot->axisWidget(QwtPlot::xBottom), SIGNAL(scaleDivChanged()), this, SLOT(plotXBottomScaleDivChanged()));
  connect(ui_->plot->axisWidget(QwtPlot::yLeft), SIGNAL(scaleDivChanged()), this, SLOT(plotYLeftScaleDivChanged()));
  connect(zoomer_, SIGNAL(zoomed(const QRectF&)), this, SLOT(plotZoomed(const QRectF&)));
  connect(zoomer_, SIGNAL(zoomResetRequested()), this, SLOT(plotZoomResetRequested()));

  connect(timer_, SIGNAL(timeout()), this, SLOT(timerTimeout()));

  ui_->plot->axisWidget(QwtPlot::yLeft)->installEventFilter(this);
  ui_->plot->axisWidget(QwtPlot::yRight)->installEventFilter(this);
}

PlotWidget::~PlotWidget() {
  timer_->stop();
  pause();
  for (auto* curve : curves_) {
    curve->detach();
    delete curve;
  }
  curves_.clear();
  delete ui_;
}

/*****************************************************************************/
/* Accessors                                                                 */
/*****************************************************************************/

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

    forceReplot();
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
  if (locked == userScaleLocked_) {
    return;
  }

  userScaleLocked_ = locked;

  if (!locked) {
    rescale_ = true;
    requestReplot();
  }

  emit userScaleLockedChanged(locked);
}

bool PlotWidget::isUserScaleLocked() const {
  return userScaleLocked_;
}

/*****************************************************************************/
/* Methods                                                                   */
/*****************************************************************************/

void PlotWidget::buildSplitMenu() {
  auto* grid = new QWidget();
  grid->setObjectName("splitDirectionGrid");

  auto* layout = new QGridLayout(grid);
  layout->setContentsMargins(4, 4, 4, 4);
  layout->setSpacing(2);

  const auto addButton = [this, layout](int row, int column, const QString& objectName, const QString& iconPath, const QString& toolTip,
                                        const char* slot) {
    auto* button = new QToolButton();
    button->setObjectName(objectName);
    button->setIcon(packageIcon(iconPath, QSize(24, 24)));
    button->setIconSize(QSize(24, 24));
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
  BoundingRectangle preferredBounds = getPreferredScale();

  if (shouldApplyPreferredScale(rescale_, userScaleLocked_)) {
    emit preferredScaleChanged(preferredBounds);

    rescale_ = false;
  }

  if (!userScaleLocked_) {
    zoomer_->setZoomBase(preferredBounds.getRectangle());
  }

  ui_->plot->replot();

  replot_ = false;
}

void PlotWidget::renderToPainter(QPainter& painter, const QRectF& bounds) {
  QRectF plotBounds = bounds;

  if (plotBounds.isEmpty() && (painter.device() != nullptr)) {
    plotBounds = QRectF(0, 0, painter.device()->width(), painter.device()->height());
  }

  QwtPlotRenderer renderer;

  renderer.setDiscardFlag(QwtPlotRenderer::DiscardBackground, true);
  renderer.setDiscardFlag(QwtPlotRenderer::DiscardCanvasBackground, true);

  qreal textHeight = 0;

  if (config_ != nullptr) {
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
  if (event->mimeData()->hasFormat(CurveConfig::MimeType) && (event->source() != legend_) && (config_ != nullptr)) {
    event->acceptProposedAction();
  } else {
    event->ignore();
  }
}

void PlotWidget::dropEvent(QDropEvent* event) {
  if (event->mimeData()->hasFormat(CurveConfig::MimeType) && (event->source() != legend_) && (config_ != nullptr)) {
    QByteArray data = event->mimeData()->data(CurveConfig::MimeType);
    QDataStream stream(&data, QIODevice::ReadOnly);

    CurveConfig* curveConfig = config_->addCurve();
    stream >> *curveConfig;

    while (config_->findCurves(curveConfig->getTitle()).count() > 1) {
      curveConfig->setTitle("Copy of " + curveConfig->getTitle());
    }

    event->acceptProposedAction();
  } else {
    event->ignore();
  }
}

bool PlotWidget::eventFilter(QObject* object, QEvent* event) {
  if ((object == ui_->plot->axisWidget(QwtPlot::yLeft)) && (event->type() == QEvent::Resize)) {
    ui_->horizontalSpacerLeft->changeSize(ui_->plot->axisWidget(QwtPlot::yLeft)->width(), 20);
    layout()->update();
  } else if ((object == ui_->plot->axisWidget(QwtPlot::yRight)) && (event->type() == QEvent::Resize)) {
    ui_->horizontalSpacerRight->changeSize(ui_->plot->axisWidget(QwtPlot::yRight)->width() - 5, 20);
    layout()->update();
  }

  return false;
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

      ui_->plot->setAxisTitle(plotAxis, QwtText(titleParts.join(", ")));
    } else {
      ui_->plot->setAxisTitle(plotAxis, QwtText(plotAxisConfig->getCustomTitle()));
    }
  } else {
    ui_->plot->setAxisTitle(plotAxis, QwtText());
  }
}

bool PlotWidget::axisLabelsFromZero(CurveConfig::Axis axis) const {
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
    if ((curveConfig == nullptr) || !curveConfig->getAxisConfig(axis)->isLabelFromZero()) {
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

  if (auto* draw = dynamic_cast<OffsetScaleDraw*>(ui_->plot->axisScaleDraw(QwtPlot::xBottom))) {
    draw->setUseTimeScale(xTimeScale);
    draw->setOffset(xOffset);
  }
  if (auto* engine = dynamic_cast<OffsetScaleEngine*>(ui_->plot->axisScaleEngine(QwtPlot::xBottom))) {
    engine->setOffset(xOffset);
  }
  if (auto* draw = dynamic_cast<OffsetScaleDraw*>(ui_->plot->axisScaleDraw(QwtPlot::yLeft))) {
    draw->setUseTimeScale(yTimeScale);
    draw->setOffset(yOffset);
  }
  if (auto* engine = dynamic_cast<OffsetScaleEngine*>(ui_->plot->axisScaleEngine(QwtPlot::yLeft))) {
    engine->setOffset(yOffset);
  }
  if (cursor_ != nullptr) {
    cursor_->setXUsesTimeScale(xTimeScale);
    cursor_->setYUsesTimeScale(yTimeScale);
    cursor_->setXOffset(xOffset);
    cursor_->setYOffset(yOffset);
  }

  if (currentBounds_.isValid()) {
    ui_->plot->setAxisScale(QwtPlot::xBottom, currentBounds_.getMinimum().x(), currentBounds_.getMaximum().x());
    ui_->plot->setAxisScale(QwtPlot::yLeft, currentBounds_.getMinimum().y(), currentBounds_.getMaximum().y());
  }
  requestReplot();
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

/*****************************************************************************/
/* Slots                                                                     */
/*****************************************************************************/

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

void PlotWidget::pushButtonImportExportClicked() {
  menuImportExport_->popup(QCursor::pos());
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
  setUserScaleLocked(zoomer_->zoomRectIndex() > 0);
}

void PlotWidget::plotZoomResetRequested() {
  setUserScaleLocked(false);
  rescale_ = true;
  requestReplot();
}

}  // namespace rqt_multiplot
