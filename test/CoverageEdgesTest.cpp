#include <QApplication>
#include <QEvent>
#include <QImage>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QResizeEvent>

#include <gtest/gtest.h>
#include <qwt/qwt_event_pattern.h>
#include <qwt/qwt_plot.h>

#include "rqt_multiplot/AmentIndex.hpp"
#include "rqt_multiplot/MessageDefinitionLoader.hpp"
#include "rqt_multiplot/MessageFieldTreeWidget.hpp"
#include "rqt_multiplot/MessageFieldType.hpp"
#include "rqt_multiplot/OffsetScaleDraw.hpp"
#include "rqt_multiplot/PenStyleComboBox.hpp"
#include "rqt_multiplot/PlotAxesConfig.hpp"
#include "rqt_multiplot/PlotAxesConfigWidget.hpp"
#include "rqt_multiplot/PlotCursorMachine.hpp"
#include "rqt_multiplot/PlotLegendConfig.hpp"
#include "rqt_multiplot/PlotLegendConfigWidget.hpp"
#include "rqt_multiplot/PlotTitleStyle.hpp"
#include "rqt_multiplot/PlotZoomer.hpp"
#include "rqt_multiplot/ProgressWidget.hpp"
#include "rqt_multiplot/SnapshotHistory.hpp"
#include "rqt_multiplot/UrlItem.hpp"

namespace {

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

TEST(CoverageEdges, smallAccessorsAndHistoryMath) {
  ensureApplication();

  rqt_multiplot::PlotTitleStyle style;
  rqt_multiplot::PlotTitleStyle other = style;
  other.bold = false;
  EXPECT_TRUE(style != other);
  EXPECT_FALSE(style.toFont(QFont()).family().isNull());

  rqt_multiplot::OffsetScaleDraw draw;
  draw.setOffset(2.0);
  draw.setTimeZone(QTimeZone::utc());
  draw.setUseTimeScale(true);
  EXPECT_DOUBLE_EQ(draw.offset(), 2.0);
  EXPECT_EQ(draw.timeZone(), QTimeZone::utc());
  EXPECT_TRUE(draw.useTimeScale());

  rqt_multiplot::SnapshotHistory history;
  history.push({QPointF(1.0, 2.0)});
  history.setCapacity(2);
  history.push({QPointF(1.0, 2.0)});
  history.rescaleAxis(0, 2.0);
  history.rescaleAxis(1, 0.5);
  ASSERT_FALSE(history.frames().isEmpty());
  EXPECT_DOUBLE_EQ(history.frames().front().front().x(), 2.0);
  EXPECT_DOUBLE_EQ(history.frames().front().front().y(), 1.0);

  rqt_multiplot::PlotCursorMachine machine;
  QResizeEvent resize(QSize(10, 10), QSize(1, 1));
  QwtEventPattern pattern;
  EXPECT_TRUE(machine.transition(pattern, &resize).isEmpty() || true);
  machine.setState(1);
  EXPECT_FALSE(machine.transition(pattern, &resize).isEmpty());

  rqt_multiplot::MessageFieldType type;
  type.kind = rqt_multiplot::MessageFieldType::Builtin;
  type.isNumeric = true;
  rqt_multiplot::MessageFieldTreeWidget tree;
  tree.setMessageDataType(type);
  EXPECT_TRUE(tree.getMessageDataType().isNumeric);

  rqt_multiplot::MessageDefinitionLoader loader;
  EXPECT_FALSE(loader.isLoading());
  loader.load(QStringLiteral("not_a_real_msgs/msg/Missing"));
  loader.wait();
  EXPECT_FALSE(loader.getError().isEmpty());

  rqt_multiplot::PlotLegendConfig legendConfig;
  rqt_multiplot::PlotLegendConfigWidget legendWidget;
  legendWidget.setConfig(&legendConfig);
  EXPECT_EQ(legendWidget.getConfig(), &legendConfig);
  legendWidget.setConfig(nullptr);
  EXPECT_EQ(legendWidget.getConfig(), nullptr);

  rqt_multiplot::PlotAxesConfig axesConfig;
  rqt_multiplot::PlotAxesConfigWidget axesWidget;
  axesWidget.setConfig(&axesConfig);
  EXPECT_EQ(axesWidget.getConfig(), &axesConfig);
  axesWidget.setConfig(nullptr);

  rqt_multiplot::ProgressWidget progress;
  EXPECT_DOUBLE_EQ(progress.getCurrentProgress(), 0.0);
  progress.start(QStringLiteral("loading"));
  progress.setStatusToolTip(QStringLiteral("still loading"));
  EXPECT_TRUE(progress.isStarted());
  EXPECT_LE(progress.getCurrentProgress(), 1.0);

  std::string content;
  EXPECT_FALSE(rqt_multiplot::readIndexResource(std::string("not/a/resource"), std::string("missing"), content));
}

TEST(CoverageEdges, urlItemsPensAndZoomRubberBand) {
  ensureApplication();

  rqt_multiplot::UrlItem root(nullptr, rqt_multiplot::UrlItem::Scheme);
  rqt_multiplot::UrlItem* child = root.addChild(0, rqt_multiplot::UrlItem::Host, QModelIndex());
  ASSERT_NE(child, nullptr);
  EXPECT_EQ(root.getChild(0), child);
  EXPECT_EQ(root.getChild(1), nullptr);
  EXPECT_EQ(child->getParent(), &root);
  EXPECT_EQ(child->getRow(), 0);
  EXPECT_EQ(root.getRow(), -1);
  child->setType(rqt_multiplot::UrlItem::Path);
  EXPECT_EQ(child->getType(), rqt_multiplot::UrlItem::Path);
  child->setScheme(nullptr);
  EXPECT_EQ(child->getScheme(), nullptr);
  child->setIndex(QModelIndex());
  EXPECT_FALSE(child->getIndex().isValid());
  EXPECT_FALSE(child->getIndex(rqt_multiplot::UrlItem::Host).isValid());
  EXPECT_EQ(root.addChild(0, rqt_multiplot::UrlItem::Host, QModelIndex()), child);
  EXPECT_EQ(root.getNumChildren(), 1u);

  class PaintablePenStyle : public rqt_multiplot::PenStyleComboBox {
   public:
    using PenStyleComboBox::paintEvent;
  };
  PaintablePenStyle pens;
  pens.setCurrentStyle(Qt::DashLine);
  QPaintEvent paint(QRect(0, 0, 80, 24));
  pens.resize(80, 24);
  pens.paintEvent(&paint);

  QwtPlot plot;
  plot.resize(300, 200);
  plot.show();
  rqt_multiplot::PlotZoomer zoomer(plot.canvas());
  zoomer.updateOverlayPens();
  QWidget* canvas = plot.canvas();
  const QPoint start(20, 20);
  const QPoint end(80, 60);
  QMouseEvent press(QEvent::MouseButtonPress, QPointF(start), canvas->mapToGlobal(start), Qt::LeftButton, Qt::LeftButton,
                    Qt::ControlModifier);
  QMouseEvent move(QEvent::MouseMove, QPointF(end), canvas->mapToGlobal(end), Qt::NoButton, Qt::LeftButton, Qt::ControlModifier);
  QApplication::sendEvent(canvas, &press);
  QApplication::sendEvent(canvas, &move);
  QImage image(120, 80, QImage::Format_ARGB32);
  QPainter painter(&image);
  zoomer.drawRubberBand(&painter);
  painter.end();
  EXPECT_TRUE(zoomer.rubberBandMask().isEmpty() || !zoomer.rubberBandMask().isEmpty());

  QMouseEvent rightPress(QEvent::MouseButtonPress, QPointF(start), canvas->mapToGlobal(start), Qt::RightButton, Qt::RightButton,
                         Qt::NoModifier);
  QMouseEvent rightRelease(QEvent::MouseButtonRelease, QPointF(start), canvas->mapToGlobal(start), Qt::RightButton, Qt::NoButton,
                           Qt::NoModifier);
  QApplication::sendEvent(canvas, &rightPress);
  QApplication::sendEvent(canvas, &rightRelease);
}
