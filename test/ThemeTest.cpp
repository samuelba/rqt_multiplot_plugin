#include <cstdlib>

#include <QAbstractButton>
#include <QApplication>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QFrame>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QPalette>
#include <QPushButton>
#include <QWidget>

#include <gtest/gtest.h>
#include <qwt/qwt_plot.h>

#include <rqt_multiplot/MultiplotWidget.h>
#include <rqt_multiplot/PackageResource.h>
#include <rqt_multiplot/PlotCursor.h>
#include <rqt_multiplot/PlotTableConfigWidget.h>
#include <rqt_multiplot/PlotWidget.h>
#include <rqt_multiplot/PlotZoomer.h>
#include <rqt_multiplot/Theme.h>

namespace {

using rqt_multiplot::MultiplotWidget;
using rqt_multiplot::packageIcon;
using rqt_multiplot::packagePixmap;
using rqt_multiplot::PlotCursor;
using rqt_multiplot::PlotTableConfigWidget;
using rqt_multiplot::PlotWidget;
using rqt_multiplot::PlotZoomer;
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

TEST(Theme, mapsIdsAndFallsBackToLight) {
  EXPECT_EQ(Theme::toId(Theme::Id::Light), QStringLiteral("light"));
  EXPECT_EQ(Theme::toId(Theme::Id::Dark), QStringLiteral("dark"));
  EXPECT_EQ(Theme::fromId(QStringLiteral("light")), Theme::Id::Light);
  EXPECT_EQ(Theme::fromId(QStringLiteral("dark")), Theme::Id::Dark);
  EXPECT_EQ(Theme::fromId(QStringLiteral("custom")), Theme::Id::Light);
  EXPECT_EQ(Theme::fromId(QString()), Theme::Id::Light);
}

TEST(Theme, plotColorsMatchNamedPresets) {
  EXPECT_EQ(Theme::plotBackground(Theme::Id::Light), QColor(Qt::white));
  EXPECT_EQ(Theme::plotForeground(Theme::Id::Light), QColor(Qt::black));
  EXPECT_EQ(Theme::plotBackground(Theme::Id::Dark), QColor(0x1e, 0x1e, 0x1e));
  EXPECT_EQ(Theme::plotForeground(Theme::Id::Dark), QColor(0xe6, 0xe6, 0xe6));
}

TEST(Theme, palettesHaveMatchingLightness) {
  EXPECT_GT(Theme::palette(Theme::Id::Light).color(QPalette::Window).lightnessF(), 0.5);
  EXPECT_LT(Theme::palette(Theme::Id::Dark).color(QPalette::Window).lightnessF(), 0.5);
  EXPECT_GT(Theme::palette(Theme::Id::Dark).color(QPalette::WindowText).lightnessF(), 0.5);
}

TEST(Theme, applySetsFusionPaletteOnWidget) {
  ensureApplication();

  QWidget widget;
  Theme::apply(&widget, Theme::Id::Dark);

  EXPECT_EQ(widget.palette().color(QPalette::Window), Theme::palette(Theme::Id::Dark).color(QPalette::Window));
  EXPECT_EQ(widget.style()->objectName().toLower(), QStringLiteral("fusion"));
  EXPECT_TRUE(widget.autoFillBackground());
  EXPECT_EQ(Theme::currentId(), Theme::Id::Dark);

  Theme::apply(&widget, Theme::Id::Light);
}

TEST(Theme, applySetsFusionOnChildChrome) {
  ensureApplication();

  QWidget root;
  auto* button = new QPushButton(&root);
  auto* combo = new QComboBox(&root);
  Theme::apply(&root, Theme::Id::Dark);

  EXPECT_EQ(button->style()->objectName().toLower(), QStringLiteral("fusion"));
  EXPECT_EQ(combo->style()->objectName().toLower(), QStringLiteral("fusion"));
  EXPECT_EQ(combo->palette().color(QPalette::Window), Theme::palette(Theme::Id::Dark).color(QPalette::Window));
}

TEST(Theme, applySetsFusionPaletteOnColorDialog) {
  ensureApplication();

  QColorDialog dialog;
  Theme::apply(&dialog, Theme::Id::Dark);

  EXPECT_EQ(dialog.palette().color(QPalette::Window), Theme::palette(Theme::Id::Dark).color(QPalette::Window));
  EXPECT_EQ(dialog.style()->objectName().toLower(), QStringLiteral("fusion"));
  EXPECT_TRUE(dialog.autoFillBackground());
}

TEST(Theme, applySetsPaletteOnMenus) {
  ensureApplication();

  QWidget root;
  auto* menuBar = new QMenuBar(&root);
  QMenu* fileMenu = menuBar->addMenu(QStringLiteral("&File"));
  Theme::apply(&root, Theme::Id::Dark);

  const QColor window = Theme::palette(Theme::Id::Dark).color(QPalette::Window);
  EXPECT_EQ(menuBar->style()->objectName().toLower(), QStringLiteral("fusion"));
  EXPECT_EQ(fileMenu->style()->objectName().toLower(), QStringLiteral("fusion"));
  EXPECT_EQ(menuBar->palette().color(QPalette::Window), window);
  EXPECT_EQ(fileMenu->palette().color(QPalette::Window), window);
  EXPECT_LT(window.lightnessF(), 0.5);
}

TEST(PlotTableConfigWidget, hasNoPlotColorPickers) {
  ensureApplication();

  PlotTableConfigWidget widget;
  EXPECT_EQ(widget.findChild<QLabel*>(QStringLiteral("labelBackgroundColor")), nullptr);
  EXPECT_EQ(widget.findChild<QLabel*>(QStringLiteral("labelForegroundColor")), nullptr);
  EXPECT_EQ(widget.findChild<QFrame*>(QStringLiteral("frameBackgroundColor")), nullptr);
  EXPECT_EQ(widget.findChild<QFrame*>(QStringLiteral("frameForegroundColor")), nullptr);
  EXPECT_EQ(widget.findChild<QLabel*>(QStringLiteral("label")), nullptr);
}

TEST(PlotWidget, canvasBackgroundFollowsPalette) {
  ensureApplication();

  PlotWidget widget;
  QPalette palette = widget.palette();
  palette.setColor(QPalette::Window, QColor(0x1e, 0x1e, 0x1e));
  palette.setColor(QPalette::WindowText, QColor(0xe6, 0xe6, 0xe6));
  widget.setPalette(palette);

  auto* plot = widget.findChild<QwtPlot*>();
  ASSERT_NE(plot, nullptr);
  EXPECT_EQ(plot->canvasBackground().color().rgb(), QColor(0x1e, 0x1e, 0x1e).rgb());
}

TEST(PlotWidget, runPauseShowsPlayIconWhenPausedAfterChromeRefresh) {
  ensureApplication();

  PlotWidget widget;
  ASSERT_TRUE(widget.isPaused());
  widget.applyPlotChrome();

  auto* button = widget.findChild<QAbstractButton*>(QStringLiteral("pushButtonRunPause"));
  ASSERT_NE(button, nullptr);
  const QImage actual = button->icon().pixmap(QSize(16, 16)).toImage().convertToFormat(QImage::Format_ARGB32);
  const QImage play = packageIcon(QStringLiteral("resource/play.svg"), QSize(16, 16))
                          .pixmap(QSize(16, 16))
                          .toImage()
                          .convertToFormat(QImage::Format_ARGB32);
  const QImage pause = packageIcon(QStringLiteral("resource/pause.svg"), QSize(16, 16))
                           .pixmap(QSize(16, 16))
                           .toImage()
                           .convertToFormat(QImage::Format_ARGB32);
  EXPECT_EQ(actual, play);
  EXPECT_NE(actual, pause);

  widget.run();
  widget.applyPlotChrome();
  const QImage running = button->icon().pixmap(QSize(16, 16)).toImage().convertToFormat(QImage::Format_ARGB32);
  EXPECT_EQ(running, pause);
}

TEST(Theme, disabledIconColorIsMutedAndVisible) {
  Theme::apply(nullptr, Theme::Id::Dark);
  EXPECT_LT(Theme::disabledIconColor().lightnessF(), Theme::iconColor().lightnessF());
  EXPECT_EQ(Theme::disabledIconColor(), QColor(0x75, 0x75, 0x75));

  Theme::apply(nullptr, Theme::Id::Light);
  EXPECT_GT(Theme::disabledIconColor().lightnessF(), Theme::iconColor().lightnessF());
  EXPECT_EQ(Theme::disabledIconColor(), QColor(0x9e, 0x9e, 0x9e));
}

TEST(PlotCursor, trackerColorsFollowCanvasPalette) {
  ensureApplication();

  PlotWidget widget;
  QPalette palette = widget.palette();
  palette.setColor(QPalette::Window, QColor(0x1e, 0x1e, 0x1e));
  palette.setColor(QPalette::WindowText, QColor(0xe6, 0xe6, 0xe6));
  widget.setPalette(palette);

  PlotCursor* cursor = widget.getCursor();
  ASSERT_NE(cursor, nullptr);
  EXPECT_EQ(cursor->trackerTextColor().rgb(), QColor(0xe6, 0xe6, 0xe6).rgb());
  EXPECT_EQ(cursor->trackerBackgroundColor().rgb(), QColor(0x1e, 0x1e, 0x1e).rgb());
  EXPECT_EQ(cursor->trackerBackgroundColor().alpha(), 230);
  EXPECT_EQ(cursor->rubberBandPen().color().rgb(), QColor(0xe6, 0xe6, 0xe6).rgb());
  EXPECT_EQ(cursor->rubberBandPen().style(), Qt::DashLine);
}

TEST(PlotZoomer, rubberBandPenFollowsCanvasPalette) {
  ensureApplication();

  PlotWidget widget;
  QPalette palette = widget.palette();
  palette.setColor(QPalette::Window, QColor(0x1e, 0x1e, 0x1e));
  palette.setColor(QPalette::WindowText, QColor(0xe6, 0xe6, 0xe6));
  widget.setPalette(palette);

  auto* zoomer = widget.findChild<PlotZoomer*>();
  ASSERT_NE(zoomer, nullptr);
  EXPECT_EQ(zoomer->rubberBandPen().color().rgb(), QColor(0xe6, 0xe6, 0xe6).rgb());
  EXPECT_EQ(zoomer->rubberBandPen().style(), Qt::DashLine);
}

TEST(PackageResource, tintsMonochromeIconsForDarkTheme) {
  ensureApplication();

  Theme::apply(nullptr, Theme::Id::Dark);
  const QImage image = packagePixmap(QStringLiteral("resource/grid.svg"), QSize(16, 16)).toImage().convertToFormat(QImage::Format_ARGB32);
  ASSERT_FALSE(image.isNull());

  bool foundLightPixel = false;
  for (int y = 0; y < image.height() && !foundLightPixel; ++y) {
    for (int x = 0; x < image.width(); ++x) {
      const QRgb pixel = image.pixel(x, y);
      if (qAlpha(pixel) < 200) {
        continue;
      }
      foundLightPixel = qGray(pixel) > 160;
      if (foundLightPixel) {
        break;
      }
    }
  }
  EXPECT_TRUE(foundLightPixel);

  Theme::apply(nullptr, Theme::Id::Light);
}

int averageOpaqueGray(const QImage& image) {
  long long graySum = 0;
  int count = 0;
  for (int y = 0; y < image.height(); ++y) {
    for (int x = 0; x < image.width(); ++x) {
      const QRgb pixel = image.pixel(x, y);
      if (qAlpha(pixel) < 200) {
        continue;
      }
      graySum += qGray(pixel);
      ++count;
    }
  }
  return (count == 0) ? 0 : static_cast<int>(graySum / count);
}

TEST(PackageResource, disabledPixmapIsMutedFromNormal) {
  ensureApplication();

  Theme::apply(nullptr, Theme::Id::Dark);
  const QIcon darkIcon = packageIcon(QStringLiteral("resource/grid.svg"), QSize(16, 16));
  const QImage darkNormal = darkIcon.pixmap(QSize(16, 16), QIcon::Normal).toImage().convertToFormat(QImage::Format_ARGB32);
  const QImage darkDisabled = darkIcon.pixmap(QSize(16, 16), QIcon::Disabled).toImage().convertToFormat(QImage::Format_ARGB32);
  EXPECT_LT(averageOpaqueGray(darkDisabled), averageOpaqueGray(darkNormal));
  EXPECT_GT(averageOpaqueGray(darkDisabled), 100);

  Theme::apply(nullptr, Theme::Id::Light);
  const QIcon lightIcon = packageIcon(QStringLiteral("resource/grid.svg"), QSize(16, 16));
  const QImage lightNormal = lightIcon.pixmap(QSize(16, 16), QIcon::Normal).toImage().convertToFormat(QImage::Format_ARGB32);
  const QImage lightDisabled = lightIcon.pixmap(QSize(16, 16), QIcon::Disabled).toImage().convertToFormat(QImage::Format_ARGB32);
  EXPECT_GT(averageOpaqueGray(lightDisabled), averageOpaqueGray(lightNormal));
  EXPECT_LT(averageOpaqueGray(lightDisabled), 160);
}

TEST(PackageResource, leavesStatusOkayGreenUnderDarkTheme) {
  ensureApplication();

  Theme::apply(nullptr, Theme::Id::Dark);
  const QImage image =
      packagePixmap(QStringLiteral("resource/status-okay.svg"), QSize(16, 16)).toImage().convertToFormat(QImage::Format_ARGB32);
  ASSERT_FALSE(image.isNull());

  bool foundGreen = false;
  for (int y = 0; y < image.height() && !foundGreen; ++y) {
    for (int x = 0; x < image.width(); ++x) {
      const QRgb pixel = image.pixel(x, y);
      if (qAlpha(pixel) < 200) {
        continue;
      }
      foundGreen = (qGreen(pixel) > qRed(pixel)) && (qGreen(pixel) > qBlue(pixel));
      if (foundGreen) {
        break;
      }
    }
  }
  EXPECT_TRUE(foundGreen);

  Theme::apply(nullptr, Theme::Id::Light);
}

TEST(MultiplotWidget, fileMenuAndToolbarFollowDarkTheme) {
  ensureApplication();

  MultiplotWidget widget;
  widget.getConfig()->setThemeId(QStringLiteral("dark"));

  const QColor window = Theme::palette(Theme::Id::Dark).color(QPalette::Window);
  const auto* menuBar = widget.findChild<QMenuBar*>("menuBar");
  ASSERT_NE(menuBar, nullptr);
  ASSERT_FALSE(menuBar->actions().isEmpty());
  QMenu* fileMenu = menuBar->actions().first()->menu();
  ASSERT_NE(fileMenu, nullptr);

  EXPECT_EQ(menuBar->style()->objectName().toLower(), QStringLiteral("fusion"));
  EXPECT_EQ(fileMenu->style()->objectName().toLower(), QStringLiteral("fusion"));
  EXPECT_EQ(menuBar->palette().color(QPalette::Window), window);
  EXPECT_EQ(fileMenu->palette().color(QPalette::Window), window);

  auto* combo = widget.findChild<QComboBox*>();
  ASSERT_NE(combo, nullptr);
  EXPECT_EQ(combo->style()->objectName().toLower(), QStringLiteral("fusion"));
  EXPECT_EQ(combo->palette().color(QPalette::Window), window);

  auto* plot = widget.findChild<QwtPlot*>();
  ASSERT_NE(plot, nullptr);
  EXPECT_EQ(plot->canvasBackground().color().rgb(), Theme::plotBackground(Theme::Id::Dark).rgb());
}

}  // namespace
