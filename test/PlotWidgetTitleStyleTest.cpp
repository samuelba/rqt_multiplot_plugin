#include <cstdlib>

#include <QApplication>
#include <QLineEdit>
#include <QPalette>
#include <QPixmap>

#include <gtest/gtest.h>
#include <qwt/qwt_plot.h>

#include "rqt_multiplot/PlotConfig.hpp"
#include "rqt_multiplot/PlotWidget.hpp"
#include "rqt_multiplot/Theme.hpp"

namespace {

using rqt_multiplot::PlotConfig;
using rqt_multiplot::PlotTitleStyle;
using rqt_multiplot::PlotWidget;
using rqt_multiplot::Theme;

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

TEST(PlotWidget, setPlotTitleStyleUpdatesTitleField) {
  ensureApplication();

  PlotWidget widget;
  PlotTitleStyle style = PlotTitleStyle::factory();
  style.fontSize = 16;
  style.bold = true;
  style.autoColor = false;
  style.customColor = QColor(0x11, 0x22, 0x33);

  widget.setPlotTitleStyle(style);

  auto* title = widget.findChild<QLineEdit*>("lineEditTitle");
  ASSERT_NE(title, nullptr);
  EXPECT_EQ(title->font().pointSize(), 16);
  EXPECT_TRUE(title->font().bold());
  EXPECT_EQ(title->palette().color(QPalette::Text), QColor(0x11, 0x22, 0x33));
}

TEST(PlotWidget, plotTitleAutoColorFollowsPalette) {
  ensureApplication();

  PlotWidget widget;
  PlotTitleStyle style = PlotTitleStyle::factory();
  style.autoColor = true;
  widget.setPlotTitleStyle(style);

  auto* title = widget.findChild<QLineEdit*>("lineEditTitle");
  ASSERT_NE(title, nullptr);
  EXPECT_EQ(title->palette().color(QPalette::Text), widget.palette().color(QPalette::Text));
}

TEST(PlotWidget, customPlotTitleColorSurvivesApplyPlotChrome) {
  ensureApplication();

  PlotWidget widget;
  PlotTitleStyle style = PlotTitleStyle::factory();
  style.autoColor = false;
  style.customColor = QColor(0xaa, 0xbb, 0xcc);
  widget.setPlotTitleStyle(style);

  widget.applyPlotChrome();

  auto* title = widget.findChild<QLineEdit*>("lineEditTitle");
  ASSERT_NE(title, nullptr);
  EXPECT_EQ(title->palette().color(QPalette::Text), QColor(0xaa, 0xbb, 0xcc));
}

TEST(PlotWidget, renderToPixmapFillsTitleBackground) {
  ensureApplication();

  PlotConfig config(nullptr, QStringLiteral("Array Plot"));
  PlotWidget widget;
  widget.setConfig(&config);

  const QColor background(0x1e, 0x1e, 0x1e);
  QPalette palette = widget.palette();
  palette.setColor(QPalette::Window, background);
  widget.setPalette(palette);
  widget.applyPlotChrome();
  widget.resize(640, 480);

  auto* plot = widget.findChild<QwtPlot*>();
  ASSERT_NE(plot, nullptr);
  const QSize size = plot->size();
  ASSERT_FALSE(size.isEmpty());

  QPixmap pixmap(size);
  pixmap.fill(Qt::transparent);
  widget.renderToPixmap(pixmap);

  const QRgb topCenter = pixmap.toImage().pixel(size.width() / 2, 2);
  EXPECT_EQ(qAlpha(topCenter), 255);
  EXPECT_EQ(QColor::fromRgb(topCenter).rgb(), background.rgb());
}

}  // namespace
