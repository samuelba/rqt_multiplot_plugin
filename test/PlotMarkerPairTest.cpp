#include <QApplication>
#include <QMouseEvent>

#include <gtest/gtest.h>
#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_marker.h>
#include <qwt/qwt_scale_div.h>
#include <qwt/qwt_scale_map.h>
#include <qwt/qwt_text.h>

#include "rqt_multiplot/CurveData.hpp"
#include "rqt_multiplot/PlotConfig.hpp"
#include "rqt_multiplot/PlotCurve.hpp"
#include "rqt_multiplot/PlotMarkerPair.hpp"
#include "rqt_multiplot/PlotMarkerReadout.hpp"
#include "rqt_multiplot/PlotTableConfig.hpp"
#include "rqt_multiplot/PlotTableWidget.hpp"
#include "rqt_multiplot/PlotWidget.hpp"

namespace {

using rqt_multiplot::MarkerCurveSample;
using rqt_multiplot::MarkerId;
using rqt_multiplot::MarkerPositions;
using rqt_multiplot::MarkerReadoutFormat;
using rqt_multiplot::markerReadoutRect;
using rqt_multiplot::markerReadoutRows;
using rqt_multiplot::markerTagRect;
using rqt_multiplot::PlotConfig;
using rqt_multiplot::PlotMarkerPair;
using rqt_multiplot::PlotTableConfig;
using rqt_multiplot::PlotTableWidget;
using rqt_multiplot::PlotWidget;
using rqt_multiplot::ReadoutMark;

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

void sendMouse(QWidget* canvas, QEvent::Type type, const QPoint& position, Qt::MouseButton button, Qt::KeyboardModifiers modifiers) {
  const Qt::MouseButtons buttons = (type == QEvent::MouseButtonRelease) ? Qt::NoButton : Qt::MouseButtons(button);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  QMouseEvent event(type, QPointF(position), QPointF(position), button, buttons, modifiers);
#else
  QMouseEvent event(type, position, button, buttons, modifiers);
#endif
  QApplication::sendEvent(canvas, &event);
}

void sendClick(QWidget* canvas, const QPoint& position, Qt::KeyboardModifiers modifiers) {
  sendMouse(canvas, QEvent::MouseButtonPress, position, Qt::LeftButton, modifiers);
  sendMouse(canvas, QEvent::MouseButtonRelease, position, Qt::LeftButton, modifiers);
}

class PlotMarkerPairFixture : public ::testing::Test {
 protected:
  void SetUp() override {
    ensureApplication();
    config_ = new PlotConfig();
    config_->addCurve();
    widget_ = new PlotWidget();
    widget_->setConfig(config_);
    widget_->getCurves().front()->getData()->appendPoint(QPointF(0.0, 0.0));
    widget_->getCurves().front()->getData()->appendPoint(QPointF(10.0, 10.0));
    widget_->resize(400, 300);
    widget_->show();
    widget_->forceReplot();
    QApplication::processEvents();

    plot_ = widget_->findChild<QwtPlot*>();
    canvas_ = plot_->canvas();
    markers_ = widget_->getMarkers();
    QObject::connect(markers_, &PlotMarkerPair::markersChanged, [this](const MarkerPositions& /*positions*/) { ++emitted_; });
  }

  void TearDown() override {
    delete widget_;
    delete config_;
  }

  double xAt(int px) const { return plot_->canvasMap(QwtPlot::xBottom).invTransform(px); }

  PlotConfig* config_ = nullptr;
  PlotWidget* widget_ = nullptr;
  QwtPlot* plot_ = nullptr;
  QWidget* canvas_ = nullptr;
  PlotMarkerPair* markers_ = nullptr;
  int emitted_ = 0;
};

TEST_F(PlotMarkerPairFixture, shiftClickPlacesAThenBThenMovesTheNearerOne) {
  sendClick(canvas_, QPoint(80, 50), Qt::ShiftModifier);
  sendClick(canvas_, QPoint(250, 50), Qt::ShiftModifier);

  ASSERT_TRUE(markers_->positions().a && markers_->positions().b);
  EXPECT_NEAR(*markers_->positions().a, xAt(80), 1e-9);
  EXPECT_NEAR(*markers_->positions().b, xAt(250), 1e-9);
  EXPECT_TRUE(markers_->marker(MarkerId::A)->isVisible());
  EXPECT_TRUE(markers_->marker(MarkerId::B)->isVisible());
  EXPECT_EQ(emitted_, 2);

  sendClick(canvas_, QPoint(220, 50), Qt::ShiftModifier);

  EXPECT_NEAR(*markers_->positions().a, xAt(80), 1e-9);
  EXPECT_NEAR(*markers_->positions().b, xAt(220), 1e-9);
}

TEST_F(PlotMarkerPairFixture, dragOnMarkerLineMovesItWithoutPanning) {
  markers_->setMarker(MarkerId::A, xAt(100));
  const double xMin = plot_->axisScaleDiv(QwtPlot::xBottom).lowerBound();

  sendMouse(canvas_, QEvent::MouseButtonPress, QPoint(102, 50), Qt::LeftButton, Qt::NoModifier);
  sendMouse(canvas_, QEvent::MouseMove, QPoint(160, 80), Qt::LeftButton, Qt::NoModifier);
  sendMouse(canvas_, QEvent::MouseButtonRelease, QPoint(160, 80), Qt::LeftButton, Qt::NoModifier);

  EXPECT_NEAR(*markers_->positions().a, xAt(160), 1e-9);
  EXPECT_DOUBLE_EQ(plot_->axisScaleDiv(QwtPlot::xBottom).lowerBound(), xMin);
  EXPECT_GE(emitted_, 1);
}

TEST_F(PlotMarkerPairFixture, leftDragAwayFromMarkersStillPans) {
  markers_->setMarker(MarkerId::A, xAt(100));
  const double xMin = plot_->axisScaleDiv(QwtPlot::xBottom).lowerBound();

  sendMouse(canvas_, QEvent::MouseButtonPress, QPoint(250, 50), Qt::LeftButton, Qt::NoModifier);
  sendMouse(canvas_, QEvent::MouseMove, QPoint(300, 50), Qt::LeftButton, Qt::NoModifier);
  sendMouse(canvas_, QEvent::MouseButtonRelease, QPoint(300, 50), Qt::LeftButton, Qt::NoModifier);

  EXPECT_NE(plot_->axisScaleDiv(QwtPlot::xBottom).lowerBound(), xMin);
  EXPECT_EQ(emitted_, 0);
}

TEST_F(PlotMarkerPairFixture, programmaticChangesDoNotEmit) {
  markers_->setMarker(MarkerId::A, 1.0);
  markers_->setPositions(MarkerPositions{2.0, 3.0});
  markers_->clearMarkers();

  EXPECT_EQ(emitted_, 0);
  EXPECT_FALSE(markers_->hasAnyMarker());
  EXPECT_FALSE(markers_->marker(MarkerId::A)->isVisible());
}

TEST_F(PlotMarkerPairFixture, clearingThePlotClearsMarkers) {
  markers_->setPositions(MarkerPositions{2.0, 3.0});

  widget_->clear();

  EXPECT_FALSE(markers_->hasAnyMarker());
}

TEST_F(PlotMarkerPairFixture, readoutSamplesVisibleCurvesAtBothMarkers) {
  markers_->setPositions(MarkerPositions{2.0, 6.0});

  const QVector<MarkerCurveSample> samples = markers_->readout()->curveSamples();

  ASSERT_EQ(samples.size(), 1);
  EXPECT_DOUBLE_EQ(*samples.front().yA, 2.0);
  EXPECT_DOUBLE_EQ(*samples.front().yB, 6.0);
}

TEST(PlotMarkerPairLink, linkedCursorSharesMarkersAcrossPlotsOfATab) {
  ensureApplication();

  PlotTableConfig config(nullptr);
  config.setLinkCursor(true);
  PlotTableWidget table;
  table.resize(800, 600);
  table.setConfig(&config);
  config.splitPlot(table.getPlotWidgets().front()->getConfig(), Qt::Horizontal);
  ASSERT_EQ(table.getNumPlots(), 2u);

  table.getPlotWidgets().front()->getMarkers()->placeMarker(MarkerId::A, 1.5);
  EXPECT_EQ(table.getPlotWidgets().back()->getMarkers()->positions(), (MarkerPositions{1.5, std::nullopt}));

  config.splitPlot(table.getPlotWidgets().back()->getConfig(), Qt::Vertical);
  ASSERT_EQ(table.getNumPlots(), 3u);
  for (PlotWidget* plot : table.getPlotWidgets()) {
    EXPECT_EQ(plot->getMarkers()->positions(), (MarkerPositions{1.5, std::nullopt}));
  }

  config.setLinkCursor(false);
  table.getPlotWidgets().front()->getMarkers()->placeMarker(MarkerId::B, 4.0);
  EXPECT_FALSE(table.getPlotWidgets().back()->getMarkers()->positions().b.has_value());
}

MarkerReadoutFormat plainFormat() {
  return {[](double value, bool /*isX*/) { return QString::number(value); }, 0.01, 0.01, true};
}

TEST(PlotMarkerReadout, rowsShowDeltaAndSlopePerCurve) {
  const QVector<MarkerCurveSample> curves{{Qt::red, QStringLiteral("cmd"), 0.0, 4.0},
                                          {Qt::blue, QStringLiteral("late"), std::nullopt, 1.0}};

  const auto rows = markerReadoutRows(1.0, 3.0, curves, plainFormat());

  ASSERT_EQ(rows.size(), 4);
  EXPECT_EQ(rows[0].mark, ReadoutMark::None);
  EXPECT_EQ(rows[0].values.size(), 4);
  EXPECT_EQ(rows[1].title, QStringLiteral("t"));
  EXPECT_EQ(rows[1].values, (QStringList{QStringLiteral("1"), QStringLiteral("3"), QStringLiteral("2")}));
  EXPECT_EQ(rows[2].title, QStringLiteral("cmd"));
  EXPECT_EQ(rows[2].values, (QStringList{QStringLiteral("0"), QStringLiteral("4"), QStringLiteral("4"), QStringLiteral("2")}));
  EXPECT_EQ(rows[3].values, (QStringList{QString(), QStringLiteral("1")}));
}

TEST(PlotMarkerReadout, zeroDeltaShowsDashForSlope) {
  const QVector<MarkerCurveSample> curves{{Qt::red, QStringLiteral("cmd"), 1.0, 1.0}};
  MarkerReadoutFormat format = plainFormat();
  format.xIsTime = false;

  const auto rows = markerReadoutRows(2.0, 2.0, curves, format);

  EXPECT_EQ(rows[1].title, QStringLiteral("x"));
  EXPECT_EQ(rows[2].values.last(), QString::fromUtf8("\u2013"));
}

TEST(PlotMarkerReadout, placedRightOfMarkersAndFlippedWhenItDoesNotFit) {
  const QRect canvas(0, 0, 400, 300);

  EXPECT_EQ(markerReadoutRect(50, 100, QSize(80, 40), canvas), QRect(105, 5, 80, 40));
  EXPECT_EQ(markerReadoutRect(200, 350, QSize(80, 40), canvas), QRect(115, 5, 80, 40));
  EXPECT_EQ(markerReadoutRect(20, 350, QSize(80, 40), canvas), QRect(314, 5, 80, 40));
}

TEST(PlotMarkerTag, centredOnLineAtTopAndBottom) {
  const QRect canvas(0, 0, 400, 300);

  EXPECT_EQ(markerTagRect(100, QSize(14, 20), canvas, Qt::TopEdge), QRect(93, 0, 14, 20));
  EXPECT_EQ(markerTagRect(100, QSize(14, 20), canvas, Qt::BottomEdge), QRect(93, 280, 14, 20));
}

TEST(PlotMarkerTag, clampedInsideCanvasNearEdges) {
  const QRect canvas(0, 0, 400, 300);

  EXPECT_EQ(markerTagRect(2, QSize(14, 20), canvas, Qt::TopEdge).left(), 0);
  EXPECT_EQ(markerTagRect(398, QSize(14, 20), canvas, Qt::TopEdge).right(), 399);
}

TEST_F(PlotMarkerPairFixture, markerLabelsAreTagsInPlotColours) {
  markers_->setColors(Qt::black, Qt::white);

  for (const MarkerId id : {MarkerId::A, MarkerId::B}) {
    const QwtText label = markers_->marker(id)->label();
    EXPECT_EQ(label.color(), QColor(Qt::white));
    EXPECT_EQ(label.backgroundBrush().color(), QColor(Qt::black));
  }
  EXPECT_EQ(markers_->marker(MarkerId::A)->label().text(), QStringLiteral("A"));
  EXPECT_EQ(markers_->marker(MarkerId::B)->label().text(), QStringLiteral("B"));
}

}  // namespace
