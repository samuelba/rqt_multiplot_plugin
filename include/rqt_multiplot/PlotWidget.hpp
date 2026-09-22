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

#include <QAction>
#include <QColor>
#include <QEvent>
#include <QIcon>
#include <QList>
#include <QMenu>
#include <QPainter>
#include <QRect>
#include <QRectF>
#include <QStringList>
#include <QTimeZone>
#include <QTimer>
#include <QVector>
#include <QWidget>

#include <qwt/qwt_plot_grid.h>

#include "rqt_multiplot/BoundingRectangle.hpp"
#include "rqt_multiplot/CurveConfig.hpp"
#include "rqt_multiplot/MessageBroker.hpp"
#include "rqt_multiplot/PlotConfig.hpp"
#include "rqt_multiplot/PlotTableConfig.hpp"
#include "rqt_multiplot/PlotTitleStyle.hpp"

namespace Ui {

class PlotWidget;

}

namespace rqt_multiplot {

class PlotCursor;
class PlotCurve;
class PlotLegend;
class PlotMagnifier;
class PlotPanner;
class PlotZoomer;

class PlotWidget : public QWidget {
  Q_OBJECT
 public:
  enum State { Normal, Maximized };

  explicit PlotWidget(QWidget* parent = nullptr);
  ~PlotWidget() override;

  void setConfig(PlotConfig* config);
  PlotConfig* getConfig() const;
  void setBroker(MessageBroker* broker);
  MessageBroker* getBroker() const;
  PlotCursor* getCursor() const;
  void setTimeAxisFormat(PlotTableConfig::TimeAxisFormat format);
  PlotTableConfig::TimeAxisFormat getTimeAxisFormat() const;
  void setTimeZone(const QTimeZone& zone);
  const QTimeZone& getTimeZone() const;
  void setGridVisible(bool visible);
  bool isGridVisible() const;
  void setGridForegroundColor(const QColor& color);
  BoundingRectangle getPreferredScale() const;
  void setCurrentScale(const BoundingRectangle& bounds);
  const BoundingRectangle& getCurrentScale() const;
  bool isPaused() const;
  bool isReplotRequested() const;
  void setState(State state);
  State getState() const;
  void setCanChangeState(bool can);
  bool canChangeState() const;
  void setCanClose(bool can);
  bool canClose() const;
  void setUserScaleLocked(bool locked);
  bool isUserScaleLocked() const;
  void setXScaleLocked(bool locked);
  bool isXScaleLocked() const;
  void setYScaleLocked(bool locked);
  bool isYScaleLocked() const;
  void syncScaleLocksFrom(const PlotWidget& source);
  void resetZoom();
  void resetZoomHorizontal();
  void resetZoomVertical();
  void setOpenGLCanvasEnabled(bool enabled);
  bool isOpenGLCanvasEnabled() const;
  void setPlotTitleStyle(const PlotTitleStyle& style);
  PlotTitleStyle plotTitleStyle() const;

  const QVector<PlotCurve*>& getCurves() const;

  void run();
  void pause();
  void clear();

  void requestReplot();
  void forceReplot();

  void renderToPainter(QPainter& painter, const QRectF& bounds = QRectF());
  void renderToPixmap(QPixmap& pixmap, const QRectF& bounds = QRectF());
  void writeFormattedCurveAxisTitles(QStringList& formattedAxisTitles);
  void writeFormattedCurveData(QList<QStringList>& formattedData);

  void saveToImageFile(const QString& fileName);
  void saveToTextFile(const QString& fileName);

  void bindAxisOrigin(CurveConfig::Axis axis, double value);
  void applyPlotChrome();

 signals:
  void preferredScaleChanged(const BoundingRectangle& bounds);
  void currentScaleChanged(const BoundingRectangle& bounds);
  void pausedChanged(bool paused);
  void stateChanged(int state);
  void splitRequested(Qt::Orientation orientation, bool insertBefore);
  void closeRequested();
  void cleared();
  void userScaleLockedChanged(bool locked);
  void canvasChanged();

 protected:
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dropEvent(QDropEvent* event) override;
  void changeEvent(QEvent* event) override;

  bool eventFilter(QObject* object, QEvent* event) override;

 private:
  Ui::PlotWidget* ui_;

  QIcon runIcon_;
  QIcon pauseIcon_;
  QIcon normalIcon_;
  QIcon maximizedIcon_;
  QTimer* timer_;
  QMenu* menuSplit_;
  QMenu* menuContext_;
  QAction* actionContextResetZoom_;
  QAction* actionContextResetZoomHorizontal_;
  QAction* actionContextResetZoomVertical_;
  QAction* actionContextConfigure_;
  QMenu* menuContextSplit_;
  QAction* actionContextShowLegend_;
  QAction* actionContextMaximizeRestore_;
  QAction* actionContextRunPause_;
  QAction* actionContextClear_;
  QAction* actionContextClose_;
  QAction* actionContextCopyImage_;
  QAction* actionContextSaveImage_;
  QAction* actionContextSaveData_;

  PlotConfig* config_;

  MessageBroker* broker_;

  QVector<PlotCurve*> curves_;

  PlotLegend* legend_;
  PlotCursor* cursor_;
  QwtPlotGrid* grid_;
  PlotPanner* panner_;
  PlotMagnifier* magnifier_;
  PlotZoomer* zoomer_;

  bool paused_;
  bool rescale_;
  bool replot_;
  bool replotting_;
  bool gridVisible_;
  bool xScaleLocked_;
  bool yScaleLocked_;
  State state_;

  BoundingRectangle currentBounds_;

  bool xOriginSet_;
  bool yOriginSet_;
  double xOrigin_;
  double yOrigin_;
  PlotTableConfig::TimeAxisFormat timeAxisFormat_;
  QTimeZone timeZone_;
  QColor gridForegroundColor_;
  PlotTitleStyle plotTitleStyle_;

  void updateAxisTitle(PlotAxesConfig::Axis axis);
  void applyPlotTitleStyle();
  bool axisLabelsFromZero(CurveConfig::Axis axis) const;
  bool axisUsesTimeFormat(CurveConfig::Axis axis) const;
  void seedAxisOrigin(CurveConfig::Axis axis);
  void resetAxisOrigins();
  void updateAxisTimeLabels();
  void applyAxisTimeOffsets();
  void updateGridPen();
  void buildSplitMenu();
  void buildContextMenu();
  void updateContextMenuState();
  BoundingRectangle mergePreferredWithLocked(const BoundingRectangle& preferred) const;
  void updateZoomBaseFromPreferred(const BoundingRectangle& preferred);
  void applyPlotTimeWindow();
  void refreshStatefulIcons();
  void createCanvasPickers();
  void destroyCanvasPickers();

 private slots:
  void timerTimeout();

  void configTitleChanged(const QString& title);
  void configCurveAdded(size_t index);
  void configCurveRemoved(size_t index);
  void configCurvesCleared();
  void configCurveConfigChanged(size_t index);
  void configXAxisConfigChanged();
  void configYAxisConfigChanged();
  void configLegendConfigChanged();
  void configPlotRateChanged(double rate);
  void configTimeWindowEnabledChanged(bool enabled);
  void configTimeWindowLengthChanged(int length);

  void curveReplotRequested();

  void lineEditTitleTextChanged(const QString& text);
  void lineEditTitleEditingFinished();

  void pushButtonRunPauseClicked();
  void pushButtonClearClicked();
  void pushButtonSetupClicked();
  void pushButtonStateClicked();
  void pushButtonSplitClicked();
  void pushButtonCloseClicked();
  void menuSplitLeftTriggered();
  void menuSplitRightTriggered();
  void menuSplitTopTriggered();
  void menuSplitBottomTriggered();
  void menuExportImageFileTriggered();
  void menuExportTextFileTriggered();
  void menuResetZoomTriggered();
  void menuResetZoomHorizontalTriggered();
  void menuResetZoomVerticalTriggered();
  void menuToggleLegendTriggered();
  void menuCopyImageTriggered();
  void showPlotContextMenu(const QPoint& globalPos);
  void configDestroyed();

  void plotXBottomScaleDivChanged();
  void plotYLeftScaleDivChanged();
  void plotZoomed(const QRectF& bounds);
};

}  // namespace rqt_multiplot
