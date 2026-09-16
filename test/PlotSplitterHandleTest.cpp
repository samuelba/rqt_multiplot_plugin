#include <cstdlib>

#include <QApplication>
#include <QColor>
#include <QEvent>
#include <QImage>
#include <QMouseEvent>
#include <QPalette>
#include <QSplitter>
#include <QWidget>
#include <QtGlobal>

#include <gtest/gtest.h>

#include <rqt_multiplot/PlotSplitter.h>
#include <rqt_multiplot/PlotTableConfig.h>
#include <rqt_multiplot/PlotTableWidget.h>

namespace {

using rqt_multiplot::PlotSplitter;
using rqt_multiplot::PlotSplitterHandle;
using rqt_multiplot::PlotTableConfig;
using rqt_multiplot::PlotTableWidget;

const QColor kHoverColor(255, 0, 0);
const QColor kRestColor(0, 255, 0);
const QColor kBackgroundColor(0, 0, 255);

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

void sendHover(QWidget* widget, bool hovered) {
  QEvent event(hovered ? QEvent::HoverEnter : QEvent::HoverLeave);
  QApplication::sendEvent(widget, &event);
  QApplication::processEvents();
}

void applyTestPalette(QWidget* widget) {
  QPalette palette = widget->palette();
  palette.setColor(QPalette::Highlight, kHoverColor);
  palette.setColor(QPalette::Mid, kRestColor);
  palette.setColor(QPalette::Window, kBackgroundColor);
  widget->setPalette(palette);
}

PlotSplitterHandle* visibleHandle(PlotSplitter* splitter) {
  auto* handle = dynamic_cast<PlotSplitterHandle*>(splitter->handle(1));
  if (handle != nullptr) {
    applyTestPalette(handle);
  }
  return handle;
}

PlotSplitter* makeShownSplitter(Qt::Orientation orientation, int width, int height) {
  auto* splitter = new PlotSplitter(orientation);
  splitter->addWidget(new QWidget());
  splitter->addWidget(new QWidget());
  applyTestPalette(splitter);
  splitter->resize(width, height);
  splitter->show();
  QApplication::processEvents();
  return splitter;
}

QImage renderHandle(PlotSplitterHandle* handle) {
  QImage image(handle->size(), QImage::Format_ARGB32);
  image.setDevicePixelRatio(1.0);
  image.fill(Qt::transparent);
  handle->render(&image);
  return image;
}

bool rowContains(const QImage& image, int y, const QColor& color) {
  const QRgb expected = color.rgb();
  for (int x = 0; x < image.width(); ++x) {
    if (image.pixelColor(x, y).rgb() == expected) {
      return true;
    }
  }
  return false;
}

TEST(PlotSplitterHandle, hoverFillsFullVerticalHandle) {
  ensureApplication();

  PlotSplitter* splitter = makeShownSplitter(Qt::Horizontal, 400, 240);
  PlotSplitterHandle* handle = visibleHandle(splitter);
  ASSERT_NE(handle, nullptr);
  ASSERT_GE(handle->height(), 200);

  sendHover(handle, true);
  EXPECT_TRUE(handle->isHovered());

  const QImage image = renderHandle(handle);
  EXPECT_EQ(image.pixelColor(image.width() / 2, 0).rgb(), kHoverColor.rgb());
  EXPECT_EQ(image.pixelColor(image.width() / 2, image.height() - 1).rgb(), kHoverColor.rgb());
  EXPECT_EQ(image.pixelColor(0, 0).rgb(), kHoverColor.rgb());

  delete splitter;
}

TEST(PlotSplitterHandle, hoverFillsFullHorizontalHandle) {
  ensureApplication();

  PlotSplitter* splitter = makeShownSplitter(Qt::Vertical, 320, 400);
  PlotSplitterHandle* handle = visibleHandle(splitter);
  ASSERT_NE(handle, nullptr);
  ASSERT_GE(handle->width(), 200);

  sendHover(handle, true);
  EXPECT_TRUE(handle->isHovered());

  const QImage image = renderHandle(handle);
  EXPECT_EQ(image.pixelColor(0, image.height() / 2).rgb(), kHoverColor.rgb());
  EXPECT_EQ(image.pixelColor(image.width() - 1, image.height() / 2).rgb(), kHoverColor.rgb());
  EXPECT_EQ(image.pixelColor(0, 0).rgb(), kHoverColor.rgb());

  delete splitter;
}

TEST(PlotSplitterHandle, restLineSpansFullLengthWithoutFillingHandle) {
  ensureApplication();

  PlotSplitter* splitter = makeShownSplitter(Qt::Horizontal, 400, 240);
  PlotSplitterHandle* handle = visibleHandle(splitter);
  ASSERT_NE(handle, nullptr);

  sendHover(handle, false);
  EXPECT_FALSE(handle->isHovered());

  const QImage image = renderHandle(handle);
  ASSERT_GT(image.width(), 1);
  EXPECT_TRUE(rowContains(image, 0, kRestColor));
  EXPECT_TRUE(rowContains(image, image.height() - 1, kRestColor));
  EXPECT_EQ(image.pixelColor(0, 0).rgb(), kBackgroundColor.rgb());

  delete splitter;
}

TEST(PlotSplitterHandle, pressFillsHandleWithoutHover) {
  ensureApplication();

  PlotSplitter* splitter = makeShownSplitter(Qt::Horizontal, 400, 240);
  PlotSplitterHandle* handle = visibleHandle(splitter);
  ASSERT_NE(handle, nullptr);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  QMouseEvent press(QEvent::MouseButtonPress, QPointF(1, 1), QPointF(1, 1), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
#else
  QMouseEvent press(QEvent::MouseButtonPress, QPointF(1, 1), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
#endif
  QApplication::sendEvent(handle, &press);
  QApplication::processEvents();

  EXPECT_FALSE(handle->isHovered());
  const QImage image = renderHandle(handle);
  EXPECT_EQ(image.pixelColor(0, 0).rgb(), kHoverColor.rgb());
  EXPECT_EQ(image.pixelColor(image.width() / 2, image.height() - 1).rgb(), kHoverColor.rgb());

  delete splitter;
}

TEST(PlotSplitterHandle, leaveHidesHoverFill) {
  ensureApplication();

  PlotSplitter* splitter = makeShownSplitter(Qt::Horizontal, 400, 240);
  PlotSplitterHandle* handle = visibleHandle(splitter);
  ASSERT_NE(handle, nullptr);

  sendHover(handle, true);
  ASSERT_TRUE(handle->isHovered());
  sendHover(handle, false);
  EXPECT_FALSE(handle->isHovered());

  const QImage image = renderHandle(handle);
  EXPECT_NE(image.pixelColor(0, 0).rgb(), kHoverColor.rgb());

  delete splitter;
}

TEST(PlotTableWidget, splitLayoutUsesPlotSplitterHandles) {
  ensureApplication();

  PlotTableConfig config(nullptr);
  PlotTableWidget widget;
  widget.resize(800, 600);
  widget.setConfig(&config);
  widget.show();
  QApplication::processEvents();

  config.splitPlot(config.getPlotConfig(0, 0), Qt::Horizontal);
  QApplication::processEvents();

  auto* splitter = widget.findChild<QSplitter*>();
  ASSERT_NE(splitter, nullptr);
  auto* handle = dynamic_cast<PlotSplitterHandle*>(splitter->handle(1));
  ASSERT_NE(handle, nullptr);
}

}  // namespace
