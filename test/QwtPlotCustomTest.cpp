#include <cstdlib>

#include <QApplication>
#include <QSize>

#include <gtest/gtest.h>
#include <qwt/qwt_plot.h>

#include <rqt_multiplot/QwtPlotCustom.h>

namespace {

using rqt_multiplot::QwtPlotCustom;

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

TEST(QwtPlotCustom, skipsUpdateLayoutWhenSizeAndExtentsUnchanged) {
  ensureApplication();

  QwtPlotCustom plot;
  plot.resize(400, 300);
  plot.setAxisScale(QwtPlot::xBottom, 0.0, 1.0);
  plot.setAxisScale(QwtPlot::yLeft, 0.0, 1.0);
  plot.replot();

  const int afterFirst = plot.layoutUpdateCount();
  ASSERT_GT(afterFirst, 0);

  plot.replot();
  EXPECT_EQ(plot.layoutUpdateCount(), afterFirst);
}

TEST(QwtPlotCustom, relayoutsAfterResize) {
  ensureApplication();

  QwtPlotCustom plot;
  plot.resize(400, 300);
  plot.setAxisScale(QwtPlot::xBottom, 0.0, 1.0);
  plot.setAxisScale(QwtPlot::yLeft, 0.0, 1.0);
  plot.replot();
  const int afterFirst = plot.layoutUpdateCount();

  plot.resize(520, 300);
  plot.replot();
  EXPECT_GT(plot.layoutUpdateCount(), afterFirst);
}

TEST(QwtPlotCustom, relayoutsAfterInvalidateLayoutCache) {
  ensureApplication();

  QwtPlotCustom plot;
  plot.resize(400, 300);
  plot.setAxisScale(QwtPlot::xBottom, 0.0, 1.0);
  plot.setAxisScale(QwtPlot::yLeft, 0.0, 1.0);
  plot.replot();
  const int afterFirst = plot.layoutUpdateCount();

  plot.invalidateLayoutCache();
  plot.updateLayout();
  EXPECT_GT(plot.layoutUpdateCount(), afterFirst);
}

}  // namespace
