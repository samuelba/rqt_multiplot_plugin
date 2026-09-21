#include <cstdlib>

#include <initializer_list>

#include <QApplication>
#include <QEvent>
#include <QEventLoop>
#include <QList>
#include <QMouseEvent>
#include <QPoint>
#include <QPointF>
#include <QTimer>
#include <Qt>

#include <gtest/gtest.h>
#include <qwt/qwt_legend_label.h>

#include "rqt_multiplot/CurveConfig.hpp"
#include "rqt_multiplot/PlotConfig.hpp"
#include "rqt_multiplot/PlotCurve.hpp"
#include "rqt_multiplot/PlotLegend.hpp"
#include "rqt_multiplot/PlotWidget.hpp"

namespace {

using rqt_multiplot::CurveConfig;
using rqt_multiplot::PlotConfig;
using rqt_multiplot::PlotCurve;
using rqt_multiplot::PlotLegend;
using rqt_multiplot::PlotWidget;

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

void sendMouseButton(QWidget* target, QEvent::Type type, Qt::MouseButton button) {
  const QPointF localPos = target->rect().center();
  const Qt::MouseButtons buttons = (type == QEvent::MouseButtonRelease) ? Qt::MouseButtons() : Qt::MouseButtons(button);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  QMouseEvent event(type, localPos, target->mapToGlobal(localPos.toPoint()), button, buttons, Qt::NoModifier);
#else
  QMouseEvent event(type, localPos, button, buttons, Qt::NoModifier);
#endif
  QApplication::sendEvent(target, &event);
}

void sendMouseClick(QWidget* target, Qt::MouseButton button) {
  sendMouseButton(target, QEvent::MouseButtonPress, button);
  sendMouseButton(target, QEvent::MouseButtonRelease, button);
}

void waitForLegendToggle() {
  QEventLoop loop;
  QTimer::singleShot(QApplication::doubleClickInterval() + 50, &loop, &QEventLoop::quit);
  loop.exec();
}

QList<QwtLegendLabel*> legendLabels(PlotWidget* widget) {
  auto* legend = widget->findChild<PlotLegend*>();
  if (legend == nullptr) {
    return {};
  }
  return legend->contentsWidget()->findChildren<QwtLegendLabel*>();
}

PlotWidget* makePlotWithCurves(PlotConfig* config, std::initializer_list<const char*> titles) {
  for (const char* title : titles) {
    CurveConfig* curveConfig = config->addCurve();
    curveConfig->setTitle(QString::fromUtf8(title));
  }

  auto* widget = new PlotWidget();
  widget->setConfig(config);
  widget->show();
  widget->forceReplot();
  QApplication::processEvents();
  return widget;
}

TEST(PlotLegend, clickingItemTogglesCurveVisibility) {
  ensureApplication();

  PlotConfig config;
  PlotWidget* widget = makePlotWithCurves(&config, {"alpha"});
  const auto curves = widget->findChildren<PlotCurve*>();
  const auto labels = legendLabels(widget);
  ASSERT_EQ(curves.size(), 1);
  ASSERT_EQ(labels.size(), 1);
  EXPECT_TRUE(curves.front()->isVisible());

  sendMouseClick(labels.front(), Qt::LeftButton);
  waitForLegendToggle();
  EXPECT_FALSE(curves.front()->isVisible());
  EXPECT_TRUE(labels.front()->text().font().strikeOut());

  sendMouseClick(labels.front(), Qt::LeftButton);
  waitForLegendToggle();
  EXPECT_TRUE(curves.front()->isVisible());
  EXPECT_FALSE(labels.front()->text().font().strikeOut());

  delete widget;
}

TEST(PlotLegend, clickingItemTogglesOnlyThatCurve) {
  ensureApplication();

  PlotConfig config;
  PlotWidget* widget = makePlotWithCurves(&config, {"alpha", "beta"});
  const auto curves = widget->findChildren<PlotCurve*>();
  const auto labels = legendLabels(widget);
  ASSERT_EQ(curves.size(), 2);
  ASSERT_EQ(labels.size(), 2);

  sendMouseClick(labels.front(), Qt::LeftButton);
  waitForLegendToggle();

  int hiddenCount = 0;
  for (PlotCurve* curve : curves) {
    if (!curve->isVisible()) {
      ++hiddenCount;
    }
  }
  EXPECT_EQ(hiddenCount, 1);

  delete widget;
}

TEST(PlotLegend, rightClickDoesNotToggleVisibility) {
  ensureApplication();

  PlotConfig config;
  PlotWidget* widget = makePlotWithCurves(&config, {"alpha"});
  const auto curves = widget->findChildren<PlotCurve*>();
  const auto labels = legendLabels(widget);
  ASSERT_EQ(curves.size(), 1);
  ASSERT_EQ(labels.size(), 1);

  sendMouseClick(labels.front(), Qt::RightButton);
  waitForLegendToggle();
  EXPECT_TRUE(curves.front()->isVisible());

  delete widget;
}

TEST(PlotLegend, secondPressCancelsPendingToggle) {
  ensureApplication();

  PlotConfig config;
  PlotWidget* widget = makePlotWithCurves(&config, {"alpha"});
  const auto curves = widget->findChildren<PlotCurve*>();
  const auto labels = legendLabels(widget);
  ASSERT_EQ(curves.size(), 1);
  ASSERT_EQ(labels.size(), 1);

  sendMouseClick(labels.front(), Qt::LeftButton);
  sendMouseButton(labels.front(), QEvent::MouseButtonPress, Qt::LeftButton);
  waitForLegendToggle();
  EXPECT_TRUE(curves.front()->isVisible());

  delete widget;
}

}  // namespace
