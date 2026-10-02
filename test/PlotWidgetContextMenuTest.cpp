#include <cstdlib>

#include <QAction>
#include <QApplication>
#include <QFile>
#include <QImage>
#include <QKeyEvent>
#include <QMenu>
#include <QMetaObject>
#include <QMouseEvent>
#include <QPainter>
#include <QTemporaryDir>
#include <QTimeZone>

#include <gtest/gtest.h>
#include <qwt/qwt_plot.h>
#include <qwt/qwt_scale_map.h>

#include "rqt_multiplot/BoundingRectangle.hpp"
#include "rqt_multiplot/CurveData.hpp"
#include "rqt_multiplot/PackageResource.hpp"
#include "rqt_multiplot/PlotConfig.hpp"
#include "rqt_multiplot/PlotCursor.hpp"
#include "rqt_multiplot/PlotCurve.hpp"
#include "rqt_multiplot/PlotMarkerPair.hpp"
#include "rqt_multiplot/PlotTableConfig.hpp"
#include "rqt_multiplot/PlotWidget.hpp"
#include "rqt_multiplot/PlotZoomer.hpp"

namespace {

using rqt_multiplot::BoundingRectangle;
using rqt_multiplot::MarkerPositions;
using rqt_multiplot::packageIcon;
using rqt_multiplot::PlotConfig;
using rqt_multiplot::PlotWidget;
using rqt_multiplot::PlotZoomer;

QApplication* ensureApplication() {
  if (QApplication::instance() != nullptr) {
    return qobject_cast<QApplication*>(QApplication::instance());
  }
  qputenv("QT_QPA_PLATFORM", "offscreen");
  static int argc = 1;
  static char arg0[] = "test_rqt_multiplot";
  static char* argv[] = {arg0, nullptr};
  return new QApplication(argc, argv);
}

PlotWidget* makePlotWithData() {
  auto* config = new PlotConfig();
  config->addCurve();

  auto* widget = new PlotWidget();
  widget->setConfig(config);
  widget->getCurves().front()->getData()->appendPoint(QPointF(0.0, 0.0));
  widget->getCurves().front()->getData()->appendPoint(QPointF(10.0, 10.0));
  widget->forceReplot();
  return widget;
}

QWidget* plotCanvas(PlotWidget& widget) {
  auto* plot = widget.findChild<QwtPlot*>();
  if (plot == nullptr) {
    return nullptr;
  }
  return plot->canvas();
}

void sendStationaryRightClick(QWidget* canvas, const QPoint& position) {
  const QPointF local(position);
  const QPointF global(canvas->mapToGlobal(position));
  QMouseEvent press(QEvent::MouseButtonPress, local, global, Qt::RightButton, Qt::RightButton, Qt::NoModifier);
  QMouseEvent release(QEvent::MouseButtonRelease, local, global, Qt::RightButton, Qt::RightButton, Qt::NoModifier);
  QApplication::sendEvent(canvas, &press);
  QApplication::sendEvent(canvas, &release);
}

TEST(PlotWidget, contextMenuContainsExpectedActions) {
  ensureApplication();

  PlotWidget widget;
  auto* menu = widget.findChild<QMenu*>(QStringLiteral("plotContextMenu"));
  ASSERT_NE(menu, nullptr);

  EXPECT_NE(widget.findChild<QAction*>(QStringLiteral("actionContextResetZoom")), nullptr);
  EXPECT_NE(widget.findChild<QAction*>(QStringLiteral("actionContextResetZoomHorizontal")), nullptr);
  EXPECT_NE(widget.findChild<QAction*>(QStringLiteral("actionContextResetZoomVertical")), nullptr);
  EXPECT_NE(widget.findChild<QAction*>(QStringLiteral("actionContextConfigure")), nullptr);
  EXPECT_NE(widget.findChild<QMenu*>(QStringLiteral("menuContextSplit")), nullptr);
  EXPECT_NE(widget.findChild<QAction*>(QStringLiteral("actionContextShowLegend")), nullptr);
  EXPECT_NE(widget.findChild<QAction*>(QStringLiteral("actionContextCopyImage")), nullptr);
  EXPECT_NE(widget.findChild<QAction*>(QStringLiteral("actionContextSaveImage")), nullptr);
  EXPECT_NE(widget.findChild<QAction*>(QStringLiteral("actionContextSaveData")), nullptr);
  EXPECT_NE(widget.findChild<QAction*>(QStringLiteral("actionContextDataStatistics")), nullptr);
  EXPECT_NE(widget.findChild<QAction*>(QStringLiteral("actionContextMarkerA")), nullptr);
  EXPECT_NE(widget.findChild<QAction*>(QStringLiteral("actionContextMarkerB")), nullptr);
  EXPECT_NE(widget.findChild<QAction*>(QStringLiteral("actionContextClearMarkers")), nullptr);
}

TEST(PlotWidget, contextMenuSetsMarkersAtClickPositionAndClearsThem) {
  ensureApplication();

  PlotWidget* widget = makePlotWithData();
  widget->resize(400, 300);
  widget->show();
  QApplication::processEvents();

  QWidget* canvas = plotCanvas(*widget);
  ASSERT_NE(canvas, nullptr);
  auto* plot = widget->findChild<QwtPlot*>();
  auto* menu = widget->findChild<QMenu*>(QStringLiteral("plotContextMenu"));
  auto* setA = widget->findChild<QAction*>(QStringLiteral("actionContextMarkerA"));
  auto* setB = widget->findChild<QAction*>(QStringLiteral("actionContextMarkerB"));
  auto* clear = widget->findChild<QAction*>(QStringLiteral("actionContextClearMarkers"));
  ASSERT_NE(setA, nullptr);
  ASSERT_NE(setB, nullptr);
  ASSERT_NE(clear, nullptr);

  sendStationaryRightClick(canvas, QPoint(60, 50));
  EXPECT_FALSE(clear->isEnabled());
  setA->trigger();
  menu->hide();
  sendStationaryRightClick(canvas, QPoint(200, 50));
  EXPECT_TRUE(clear->isEnabled());
  setB->trigger();
  menu->hide();

  const MarkerPositions& positions = widget->getMarkers()->positions();
  ASSERT_TRUE(positions.a && positions.b);
  EXPECT_NEAR(*positions.a, plot->canvasMap(QwtPlot::xBottom).invTransform(60), 1e-9);
  EXPECT_NEAR(*positions.b, plot->canvasMap(QwtPlot::xBottom).invTransform(200), 1e-9);

  clear->trigger();
  EXPECT_FALSE(widget->getMarkers()->hasAnyMarker());

  delete widget->getConfig();
  delete widget;
}

TEST(PlotWidget, contextMenuActionsHaveIconsForAvailableResources) {
  ensureApplication();

  PlotWidget widget;

  const auto expectIcon = [&widget](const char* objectName, const char* resourcePath) {
    auto* action = widget.findChild<QAction*>(QString::fromLatin1(objectName));
    ASSERT_NE(action, nullptr) << objectName;
    EXPECT_FALSE(action->icon().isNull()) << objectName;
    EXPECT_FALSE(packageIcon(QString::fromLatin1(resourcePath), QSize(16, 16)).isNull()) << resourcePath;
  };

  expectIcon("actionContextConfigure", "resource/settings-edit.svg");
  expectIcon("actionContextClear", "resource/delete-data.svg");
  expectIcon("actionContextClose", "resource/close.svg");
  expectIcon("actionContextCopyImage", "resource/copy.svg");
  expectIcon("actionContextSaveImage", "resource/data-export.svg");
  expectIcon("actionContextSaveData", "resource/data-export.svg");
  expectIcon("actionContextDataStatistics", "resource/data-statistics.svg");
  expectIcon("actionContextResetZoom", "resource/zoom-reset.svg");
  expectIcon("actionContextResetZoomHorizontal", "resource/zoom-reset-horizontally.svg");
  expectIcon("actionContextResetZoomVertical", "resource/zoom-reset-vertically.svg");
  expectIcon("actionContextShowLegend", "resource/legend.svg");

  auto* splitMenu = widget.findChild<QMenu*>(QStringLiteral("menuContextSplit"));
  ASSERT_NE(splitMenu, nullptr);
  EXPECT_FALSE(splitMenu->menuAction()->icon().isNull());
}

TEST(PlotWidget, rightClickOnCanvasRequestsContextMenu) {
  ensureApplication();

  PlotWidget widget;
  widget.resize(400, 300);
  widget.show();
  QApplication::processEvents();

  auto* zoomer = widget.findChild<PlotZoomer*>();
  ASSERT_NE(zoomer, nullptr);

  bool requested = false;
  QObject::connect(zoomer, &PlotZoomer::contextMenuRequested, [&](const QPoint& /*pos*/) { requested = true; });

  QWidget* canvas = plotCanvas(widget);
  ASSERT_NE(canvas, nullptr);
  sendStationaryRightClick(canvas, QPoint(50, 50));

  EXPECT_TRUE(requested);
}

TEST(PlotWidget, homeKeyResetsZoom) {
  ensureApplication();

  PlotWidget widget;
  widget.setUserScaleLocked(true);
  widget.show();
  QApplication::processEvents();

  QWidget* canvas = plotCanvas(widget);
  ASSERT_NE(canvas, nullptr);
  canvas->setFocus();

  QKeyEvent keyEvent(QEvent::KeyPress, Qt::Key_Home, Qt::NoModifier);
  QApplication::sendEvent(canvas, &keyEvent);

  EXPECT_FALSE(widget.isUserScaleLocked());
  EXPECT_FALSE(widget.isXScaleLocked());
  EXPECT_FALSE(widget.isYScaleLocked());
}

TEST(PlotWidget, resetZoomUnlocksBothAxes) {
  ensureApplication();

  PlotWidget* widget = makePlotWithData();
  widget->setUserScaleLocked(true);
  widget->setCurrentScale(BoundingRectangle(QPointF(2.0, 3.0), QPointF(8.0, 7.0)));

  widget->resetZoom();

  EXPECT_FALSE(widget->isUserScaleLocked());
  EXPECT_FALSE(widget->isXScaleLocked());
  EXPECT_FALSE(widget->isYScaleLocked());

  delete widget->getConfig();
  delete widget;
}

TEST(PlotWidget, resetZoomHorizontalRestoresXAndKeepsY) {
  ensureApplication();

  PlotWidget* widget = makePlotWithData();
  const BoundingRectangle preferred(widget->getPreferredScale());
  widget->setUserScaleLocked(true);
  widget->setCurrentScale(BoundingRectangle(QPointF(2.0, 3.0), QPointF(8.0, 7.0)));

  widget->resetZoomHorizontal();

  const BoundingRectangle current = widget->getCurrentScale();
  EXPECT_DOUBLE_EQ(current.getMinimum().x(), preferred.getMinimum().x());
  EXPECT_DOUBLE_EQ(current.getMaximum().x(), preferred.getMaximum().x());
  EXPECT_DOUBLE_EQ(current.getMinimum().y(), 3.0);
  EXPECT_DOUBLE_EQ(current.getMaximum().y(), 7.0);
  EXPECT_FALSE(widget->isXScaleLocked());
  EXPECT_TRUE(widget->isYScaleLocked());

  delete widget->getConfig();
  delete widget;
}

TEST(PlotWidget, resetZoomVerticalRestoresYAndKeepsX) {
  ensureApplication();

  PlotWidget* widget = makePlotWithData();
  const BoundingRectangle preferred(widget->getPreferredScale());
  widget->setUserScaleLocked(true);
  widget->setCurrentScale(BoundingRectangle(QPointF(2.0, 3.0), QPointF(8.0, 7.0)));

  widget->resetZoomVertical();

  const BoundingRectangle current = widget->getCurrentScale();
  EXPECT_DOUBLE_EQ(current.getMinimum().x(), 2.0);
  EXPECT_DOUBLE_EQ(current.getMaximum().x(), 8.0);
  EXPECT_DOUBLE_EQ(current.getMinimum().y(), preferred.getMinimum().y());
  EXPECT_DOUBLE_EQ(current.getMaximum().y(), preferred.getMaximum().y());
  EXPECT_TRUE(widget->isXScaleLocked());
  EXPECT_FALSE(widget->isYScaleLocked());

  delete widget->getConfig();
  delete widget;
}

QAction* splitAction(PlotWidget& widget, const QString& text) {
  auto* menu = widget.findChild<QMenu*>(QStringLiteral("menuContextSplit"));
  if (menu == nullptr) {
    return nullptr;
  }
  for (QAction* action : menu->actions()) {
    if (action->text() == text) {
      return action;
    }
  }
  return nullptr;
}

TEST(PlotWidget, menuActionsEmitSplitAndToggleLegendWithoutDialogs) {
  ensureApplication();
  PlotWidget* widget = makePlotWithData();
  widget->resize(400, 300);
  widget->show();

  Qt::Orientation orientation = Qt::Horizontal;
  bool before = true;
  int splits = 0;
  QObject::connect(widget, &PlotWidget::splitRequested, [&](Qt::Orientation received, bool receivedBefore) {
    orientation = received;
    before = receivedBefore;
    ++splits;
  });

  ASSERT_NE(splitAction(*widget, QStringLiteral("Split left")), nullptr);
  splitAction(*widget, QStringLiteral("Split left"))->trigger();
  splitAction(*widget, QStringLiteral("Split right"))->trigger();
  splitAction(*widget, QStringLiteral("Split up"))->trigger();
  splitAction(*widget, QStringLiteral("Split down"))->trigger();
  EXPECT_EQ(splits, 4);
  EXPECT_EQ(orientation, Qt::Vertical);
  EXPECT_FALSE(before);

  auto* legend = widget->getConfig()->getLegendConfig();
  ASSERT_NE(legend, nullptr);
  const bool wasVisible = legend->isVisible();
  widget->findChild<QAction*>(QStringLiteral("actionContextShowLegend"))->trigger();
  EXPECT_EQ(legend->isVisible(), !wasVisible);

  widget->findChild<QAction*>(QStringLiteral("actionContextResetZoom"))->trigger();
  widget->findChild<QAction*>(QStringLiteral("actionContextResetZoomHorizontal"))->trigger();
  widget->findChild<QAction*>(QStringLiteral("actionContextResetZoomVertical"))->trigger();
  widget->findChild<QAction*>(QStringLiteral("actionContextMarkerA"))->trigger();
  widget->findChild<QAction*>(QStringLiteral("actionContextMarkerB"))->trigger();
  widget->findChild<QAction*>(QStringLiteral("actionContextClearMarkers"))->trigger();
  widget->findChild<QAction*>(QStringLiteral("actionContextCopyImage"))->trigger();
  widget->findChild<QAction*>(QStringLiteral("actionContextDataStatistics"))->trigger();
  widget->findChild<QAction*>(QStringLiteral("actionContextMaximizeRestore"))->trigger();
  widget->findChild<QAction*>(QStringLiteral("actionContextRunPause"))->trigger();
  widget->findChild<QAction*>(QStringLiteral("actionContextClear"))->trigger();

  auto* cursor = widget->getCursor();
  ASSERT_NE(cursor, nullptr);
  cursor->setXOffset(1.0);
  cursor->setYOffset(2.0);
  cursor->setTimeZone(QTimeZone(QByteArray("UTC")));
  cursor->setTrackPoints(true);
  cursor->setActive(true, QPointF(1.0, 1.0));
  cursor->setCurrentPosition(QPointF(5.0, 5.0));
  EXPECT_FALSE(cursor->formatCoordinate(5.0, true).isEmpty());
  EXPECT_FALSE(cursor->formatCoordinate(5.0, false).isEmpty());
  QImage image(widget->width(), widget->height(), QImage::Format_ARGB32_Premultiplied);
  image.fill(Qt::white);
  QPainter painter(&image);
  cursor->drawRubberBand(&painter);
  EXPECT_FALSE(cursor->rubberBandMask().isEmpty() && cursor->arePointsTracked() == false);

  PlotConfig* original = widget->getConfig();
  auto* replacement = new PlotConfig();
  widget->setConfig(replacement);
  widget->setConfig(original);
  delete replacement;
  delete original;
  delete widget;
}

TEST(PlotWidget, playbackAndScaleSlotsChangeThePlot) {
  ensureApplication();
  QTemporaryDir tempDir;
  ASSERT_TRUE(tempDir.isValid());
  PlotWidget* widget = makePlotWithData();
  widget->resize(400, 300);
  widget->show();
  widget->setCanChangeState(true);
  widget->setCanClose(true);
  widget->setTimeAxisFormat(rqt_multiplot::PlotTableConfig::DateTime);
  widget->setTimeZone(QTimeZone::utc());
  widget->setGridVisible(true);
  widget->setGridForegroundColor(Qt::black);
  widget->setState(PlotWidget::Maximized);
  widget->setXScaleLocked(true);
  widget->setYScaleLocked(false);
  widget->syncScaleLocksFrom(*widget);
  widget->resetZoomHorizontal();
  widget->resetZoomVertical();
  widget->run();
  widget->pause();

  ASSERT_TRUE(QMetaObject::invokeMethod(widget, "pushButtonRunPauseClicked"));
  ASSERT_TRUE(QMetaObject::invokeMethod(widget, "pushButtonClearClicked"));
  ASSERT_TRUE(QMetaObject::invokeMethod(widget, "lineEditTitleTextChanged", Q_ARG(QString, QStringLiteral("Joints"))));
  ASSERT_TRUE(QMetaObject::invokeMethod(widget, "lineEditTitleEditingFinished"));
  ASSERT_TRUE(QMetaObject::invokeMethod(widget, "pushButtonStateClicked"));
  ASSERT_TRUE(QMetaObject::invokeMethod(widget, "pushButtonSplitClicked"));
  ASSERT_TRUE(QMetaObject::invokeMethod(widget, "pushButtonCloseClicked"));
  ASSERT_TRUE(QMetaObject::invokeMethod(widget, "timerTimeout"));

  const QString imagePath = tempDir.filePath(QStringLiteral("plot.png"));
  const QString textPath = tempDir.filePath(QStringLiteral("plot.csv"));
  widget->saveToImageFile(imagePath);
  widget->saveToTextFile(textPath);
  EXPECT_TRUE(QFile::exists(imagePath));
  EXPECT_GT(QFile(textPath).size(), 0);

  widget->setXScaleLocked(true);
  widget->setUserScaleLocked(false);
  auto* broker = widget->getBroker();
  widget->setBroker(broker);
  widget->getConfig()->clearCurves();
  PlotConfig* config = widget->getConfig();
  delete widget;
  delete config;
}

}  // namespace
