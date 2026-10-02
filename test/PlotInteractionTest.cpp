#include <QApplication>
#include <QMouseEvent>

#include <gtest/gtest.h>
#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_magnifier.h>
#include <qwt/qwt_scale_div.h>

#include "rqt_multiplot/CurveConfigDialog.hpp"
#include "rqt_multiplot/PlotConfigDialog.hpp"
#include "rqt_multiplot/PlotMagnifier.hpp"

namespace {

using rqt_multiplot::CurveConfigDialog;
using rqt_multiplot::PlotConfigDialog;
using rqt_multiplot::PlotMagnifier;

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

void sendMouse(QWidget* widget, QEvent::Type type, const QPoint& position, Qt::MouseButton button) {
  const Qt::MouseButtons buttons = type == QEvent::MouseButtonRelease ? Qt::NoButton : button;
  QMouseEvent event(type, QPointF(position), widget->mapToGlobal(position), button, buttons, Qt::NoModifier);
  QApplication::sendEvent(widget, &event);
}

}  // namespace

TEST(PlotMagnifier, dragChangesTheAxisScale) {
  ensureApplication();
  QwtPlot plot;
  plot.resize(400, 300);
  plot.setAxisScale(QwtPlot::xBottom, 0.0, 100.0);
  plot.setAxisScale(QwtPlot::yLeft, 0.0, 50.0);
  plot.replot();
  plot.show();

  PlotMagnifier magnifier(plot.canvas());
  magnifier.setMouseButton(Qt::MiddleButton, Qt::NoModifier);
  const double before = plot.axisScaleDiv(QwtPlot::xBottom).range();

  sendMouse(plot.canvas(), QEvent::MouseButtonPress, QPoint(40, 40), Qt::MiddleButton);
  sendMouse(plot.canvas(), QEvent::MouseMove, QPoint(80, 10), Qt::MiddleButton);
  sendMouse(plot.canvas(), QEvent::MouseButtonRelease, QPoint(80, 10), Qt::MiddleButton);

  EXPECT_NE(plot.axisScaleDiv(QwtPlot::xBottom).range(), before);
}

TEST(ConfigDialogs, constructWithoutShowingAModal) {
  ensureApplication();
  PlotConfigDialog plotDialog;
  EXPECT_NE(plotDialog.getWidget(), nullptr);
  CurveConfigDialog curveDialog;
  EXPECT_NE(curveDialog.getWidget(), nullptr);
}
