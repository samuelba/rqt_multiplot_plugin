#include <cstdlib>

#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_canvas.h>
#include <QApplication>
#include <QOpenGLWidget>
#include <QStringList>
#include <QSurfaceFormat>

#include <gtest/gtest.h>

#include "rqt_multiplot/PlotCanvasPolicy.hpp"
#include "rqt_multiplot/PlotTableConfig.hpp"
#include "rqt_multiplot/PlotTableWidget.hpp"
#include "rqt_multiplot/PlotWidget.hpp"

namespace {

using rqt_multiplot::configurePlotCanvas;
using rqt_multiplot::createPlotCanvas;
using rqt_multiplot::isOpenGLPlotCanvas;
using rqt_multiplot::kOpenGLCanvasSamples;
using rqt_multiplot::openGLCanvasFallbackWarning;
using rqt_multiplot::openGLCanvasSurfaceFormat;
using rqt_multiplot::openGLPlotCanvasAvailable;
using rqt_multiplot::PlotTableConfig;
using rqt_multiplot::PlotTableWidget;
using rqt_multiplot::PlotWidget;
using rqt_multiplot::shouldWarnOpenGLCanvasFallback;
using rqt_multiplot::warnIfOpenGLCanvasFallback;

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

TEST(PlotCanvasPolicy, softwareCanvasIsNotOpenGL) {
  ensureApplication();

  QwtPlot plot;
  QWidget* canvas = createPlotCanvas(&plot, false);
  ASSERT_NE(canvas, nullptr);
  EXPECT_FALSE(isOpenGLPlotCanvas(canvas));
  EXPECT_NE(qobject_cast<QwtPlotCanvas*>(canvas), nullptr);
  configurePlotCanvas(canvas);
  plot.setCanvas(canvas);
}

TEST(PlotCanvasPolicy, openGLRequestUsesOpenGLCanvasWhenAvailable) {
  ensureApplication();
  if (!openGLPlotCanvasAvailable()) {
    GTEST_SKIP() << "Qwt OpenGL canvas is not available";
  }

  QwtPlot plot;
  QWidget* canvas = createPlotCanvas(&plot, true);
  ASSERT_NE(canvas, nullptr);
  EXPECT_TRUE(isOpenGLPlotCanvas(canvas));
  configurePlotCanvas(canvas);
  plot.setCanvas(canvas);
}

TEST(PlotCanvasPolicy, openGLSurfaceFormatRequestsMultisampling) {
  QSurfaceFormat format;
  format.setSamples(0);

  EXPECT_GE(openGLCanvasSurfaceFormat(format).samples(), kOpenGLCanvasSamples);
}

TEST(PlotCanvasPolicy, openGLSurfaceFormatKeepsHigherSampleCount) {
  QSurfaceFormat format;
  format.setSamples(8);

  EXPECT_EQ(openGLCanvasSurfaceFormat(format).samples(), 8);
}

TEST(PlotCanvasPolicy, openGLCanvasRequestsMultisampling) {
  ensureApplication();
  if (!openGLPlotCanvasAvailable()) {
    GTEST_SKIP() << "Qwt OpenGL canvas is not available";
  }

  QwtPlot plot;
  QWidget* canvas = createPlotCanvas(&plot, true);
  auto* glCanvas = qobject_cast<QOpenGLWidget*>(canvas);
  ASSERT_NE(glCanvas, nullptr);
  EXPECT_GE(glCanvas->format().samples(), kOpenGLCanvasSamples);
  plot.setCanvas(canvas);
}

TEST(PlotCanvasPolicy, warnsOnlyWhenOpenGLRequestedAndUnavailable) {
  EXPECT_TRUE(shouldWarnOpenGLCanvasFallback(true, false));
  EXPECT_FALSE(shouldWarnOpenGLCanvasFallback(true, true));
  EXPECT_FALSE(shouldWarnOpenGLCanvasFallback(false, false));
  EXPECT_FALSE(shouldWarnOpenGLCanvasFallback(false, true));
}

TEST(PlotCanvasPolicy, fallbackWarningMentionsSoftwareCanvas) {
  const QString message = openGLCanvasFallbackWarning();
  EXPECT_TRUE(message.contains(QStringLiteral("OpenGL")));
  EXPECT_TRUE(message.contains(QStringLiteral("software canvas")));
}

QStringList gOpenGLWarnings;

void captureOpenGLWarnings(QtMsgType type, const QMessageLogContext& /*context*/, const QString& message) {
  if (type == QtWarningMsg) {
    gOpenGLWarnings.append(message);
  }
}

TEST(PlotCanvasPolicy, writesQtWarningOnOpenGLFallback) {
  gOpenGLWarnings.clear();
  const QtMessageHandler previous = qInstallMessageHandler(captureOpenGLWarnings);
  warnIfOpenGLCanvasFallback(true, false);
  qInstallMessageHandler(previous);

  ASSERT_EQ(gOpenGLWarnings.size(), 1);
  EXPECT_EQ(gOpenGLWarnings.front(), openGLCanvasFallbackWarning());
}

TEST(PlotCanvasPolicy, writesNoWarningWhenOpenGLAvailable) {
  gOpenGLWarnings.clear();
  const QtMessageHandler previous = qInstallMessageHandler(captureOpenGLWarnings);
  warnIfOpenGLCanvasFallback(true, true);
  qInstallMessageHandler(previous);

  EXPECT_TRUE(gOpenGLWarnings.isEmpty());
}

TEST(PlotWidget, usesSoftwareCanvasByDefault) {
  ensureApplication();

  PlotWidget widget;
  auto* plot = widget.findChild<QwtPlot*>();
  ASSERT_NE(plot, nullptr);
  EXPECT_FALSE(widget.isOpenGLCanvasEnabled());
  EXPECT_FALSE(isOpenGLPlotCanvas(plot->canvas()));
  ASSERT_NE(widget.getCursor(), nullptr);
}

TEST(PlotWidget, swapsToOpenGLCanvasAndKeepsPickers) {
  ensureApplication();
  if (!openGLPlotCanvasAvailable()) {
    GTEST_SKIP() << "Qwt OpenGL canvas is not available";
  }

  PlotWidget widget;
  widget.setOpenGLCanvasEnabled(true);

  auto* plot = widget.findChild<QwtPlot*>();
  ASSERT_NE(plot, nullptr);
  EXPECT_TRUE(widget.isOpenGLCanvasEnabled());
  EXPECT_TRUE(isOpenGLPlotCanvas(plot->canvas()));
  ASSERT_NE(widget.getCursor(), nullptr);

  widget.setOpenGLCanvasEnabled(false);
  EXPECT_FALSE(widget.isOpenGLCanvasEnabled());
  EXPECT_FALSE(isOpenGLPlotCanvas(plot->canvas()));
  ASSERT_NE(widget.getCursor(), nullptr);
}

TEST(PlotTableWidget, appliesOpenGLCanvasToExistingAndNewPlots) {
  ensureApplication();
  if (!openGLPlotCanvasAvailable()) {
    GTEST_SKIP() << "Qwt OpenGL canvas is not available";
  }

  PlotTableConfig config(nullptr);
  PlotTableWidget widget;
  widget.setConfig(&config);
  ASSERT_EQ(widget.getNumPlots(), 1u);
  EXPECT_FALSE(widget.getPlotWidgets().front()->isOpenGLCanvasEnabled());

  widget.setOpenGLCanvasEnabled(true);
  EXPECT_TRUE(widget.getPlotWidgets().front()->isOpenGLCanvasEnabled());

  config.splitPlot(widget.getPlotWidgets().front()->getConfig(), Qt::Horizontal);
  ASSERT_EQ(widget.getNumPlots(), 2u);
  EXPECT_TRUE(widget.getPlotWidgets().front()->isOpenGLCanvasEnabled());
  EXPECT_TRUE(widget.getPlotWidgets().back()->isOpenGLCanvasEnabled());
}

}  // namespace
