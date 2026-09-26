#include <cstdlib>

#include <QApplication>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>

#include <gtest/gtest.h>
#include <qwt/qwt_plot.h>

#include "rqt_multiplot/BoundingRectangle.hpp"
#include "rqt_multiplot/CurveData.hpp"
#include "rqt_multiplot/PackageResource.hpp"
#include "rqt_multiplot/PlotConfig.hpp"
#include "rqt_multiplot/PlotCurve.hpp"
#include "rqt_multiplot/PlotWidget.hpp"
#include "rqt_multiplot/PlotZoomer.hpp"

namespace {

using rqt_multiplot::BoundingRectangle;
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
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  QMouseEvent press(QEvent::MouseButtonPress, position, position, Qt::RightButton, Qt::RightButton, Qt::NoModifier);
  QMouseEvent release(QEvent::MouseButtonRelease, position, position, Qt::RightButton, Qt::RightButton, Qt::NoModifier);
#else
  QMouseEvent press(QEvent::MouseButtonPress, position, Qt::RightButton, Qt::RightButton, Qt::NoModifier);
  QMouseEvent release(QEvent::MouseButtonRelease, position, Qt::RightButton, Qt::RightButton, Qt::NoModifier);
#endif
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

}  // namespace
