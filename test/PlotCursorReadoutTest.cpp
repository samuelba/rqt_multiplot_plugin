#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QTimeZone>

#include <gtest/gtest.h>
#include <qwt/qwt_plot.h>

#include "rqt_multiplot/AxisTimeFormat.hpp"
#include "rqt_multiplot/CurveData.hpp"
#include "rqt_multiplot/PlotCursor.hpp"
#include "rqt_multiplot/PlotCurve.hpp"
#include "rqt_multiplot/PlotReadoutTable.hpp"

namespace {

using rqt_multiplot::AxisTimeFormat;
using rqt_multiplot::drawReadoutTable;
using rqt_multiplot::PlotCursor;
using rqt_multiplot::PlotCurve;
using rqt_multiplot::ReadoutRow;

class ReadableCursor : public PlotCursor {
 public:
  using PlotCursor::PlotCursor;
  using PlotCursor::trackedReadoutRect;
  using PlotCursor::trackedReadoutRows;
  using PlotCursor::updateDisplay;
};

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

}  // namespace

TEST(PlotCursor, trackedReadoutIncludesTheNearestCurvePoint) {
  ensureApplication();
  QwtPlot plot;
  plot.resize(400, 300);
  plot.setAxisScale(QwtPlot::xBottom, 0.0, 10.0);
  plot.setAxisScale(QwtPlot::yLeft, 0.0, 10.0);
  PlotCurve curve;
  curve.attach(&plot);
  curve.getData()->appendPoint(QPointF(0.0, 1.0));
  curve.getData()->appendPoint(QPointF(10.0, 4.0));
  plot.replot();
  plot.show();

  ReadableCursor cursor(plot.canvas());
  cursor.setTrackPoints(true);
  cursor.setTimeZone(QTimeZone(QByteArrayLiteral("Europe/Zurich")));
  cursor.setXTimeLabelMode(AxisTimeFormat::LabelMode::DateTime);
  cursor.setActive(true, QPointF(0.0, 1.0));
  cursor.setCurrentPosition(QPointF(0.2, 1.0));
  cursor.updateDisplay();

  EXPECT_FALSE(cursor.formatCoordinate(1.7e9, true).isEmpty());
  const auto rows = cursor.trackedReadoutRows();
  ASSERT_GE(rows.size(), 1);
  EXPECT_FALSE(cursor.trackedReadoutRect(plot.font()).isEmpty());

  QImage image(160, 48, QImage::Format_ARGB32);
  image.fill(Qt::white);
  QPainter painter(&image);
  drawReadoutTable(painter, QRect(0, 0, 160, 48), rows, Qt::black, Qt::white);
  painter.end();
}

TEST(PlotCursor, emptyTrackHasNoReadoutRows) {
  ensureApplication();
  QwtPlot plot;
  plot.resize(200, 150);
  plot.show();
  ReadableCursor cursor(plot.canvas());
  cursor.setTrackPoints(false);
  cursor.updateDisplay();
  EXPECT_TRUE(cursor.trackedReadoutRows().isEmpty());
  EXPECT_TRUE(cursor.trackedReadoutRect(plot.font()).isEmpty());
  EXPECT_FALSE(cursor.formatCoordinate(1.5, false).isEmpty());
}
