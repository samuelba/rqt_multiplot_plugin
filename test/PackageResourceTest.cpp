#include <cstdlib>

#include <QAbstractButton>
#include <QApplication>
#include <QDir>
#include <QDirIterator>
#include <QIcon>
#include <QImage>
#include <QPixmap>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QTabWidget>
#include <QToolButton>
#include <QWidget>

#include <gtest/gtest.h>

#include <rqt_multiplot/CurveConfigWidget.h>
#include <rqt_multiplot/PackageResource.h>
#include <rqt_multiplot/PlotConfigWidget.h>
#include <rqt_multiplot/PlotTabWidget.h>
#include <rqt_multiplot/PlotTableConfigWidget.h>
#include <rqt_multiplot/PlotWidget.h>

namespace {

using rqt_multiplot::CurveConfigWidget;
using rqt_multiplot::packageIcon;
using rqt_multiplot::packagePixmap;
using rqt_multiplot::packageShareDirectory;
using rqt_multiplot::PlotConfigWidget;
using rqt_multiplot::PlotTableConfigWidget;
using rqt_multiplot::PlotTabWidget;
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

bool pixmapHasOpaquePixel(const QPixmap& pixmap) {
  if (pixmap.isNull()) {
    return false;
  }

  const QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
  for (int y = 0; y < image.height(); ++y) {
    for (int x = 0; x < image.width(); ++x) {
      if (qAlpha(image.pixel(x, y)) > 0) {
        return true;
      }
    }
  }
  return false;
}

void expectVisibleIcon(const QIcon& icon, const QSize& size = QSize(16, 16)) {
  ASSERT_FALSE(icon.isNull());
  EXPECT_FALSE(icon.availableSizes().isEmpty());
  const QPixmap pixmap = icon.pixmap(size);
  ASSERT_FALSE(pixmap.isNull());
  EXPECT_TRUE(pixmapHasOpaquePixel(pixmap));
}

void expectVisibleButtonIcon(QWidget* parent, const char* objectName, const QSize& size = QSize(16, 16)) {
  auto* button = parent->findChild<QAbstractButton*>(QString::fromUtf8(objectName));
  ASSERT_NE(button, nullptr) << objectName;
  expectVisibleIcon(button->icon(), size);
}

TEST(PackageResource, returnsNullIconForMissingSvg) {
  ensureApplication();

  EXPECT_TRUE(packageIcon("resource/not-a-real-icon.svg").isNull());
  EXPECT_TRUE(packagePixmap("resource/not-a-real-icon.svg").isNull());
}

TEST(PackageResource, rasterizesBundledSvgsToVisiblePixels) {
  ensureApplication();

  const QString resourcePath = packageShareDirectory() + "/resource";
  ASSERT_TRUE(QDir(resourcePath).exists());

  const QDir shareDir(packageShareDirectory());
  QStringList svgs;
  QDirIterator iterator(resourcePath, QStringList() << "*.svg", QDir::Files, QDirIterator::Subdirectories);
  while (iterator.hasNext()) {
    iterator.next();
    svgs.append(shareDir.relativeFilePath(iterator.filePath()));
  }
  ASSERT_FALSE(svgs.isEmpty());

  for (const QString& relativePath : svgs) {
    const QIcon icon = packageIcon(relativePath, QSize(16, 16));
    EXPECT_FALSE(icon.isNull()) << relativePath.toStdString();
    EXPECT_FALSE(icon.availableSizes().isEmpty()) << relativePath.toStdString();
    EXPECT_TRUE(pixmapHasOpaquePixel(packagePixmap(relativePath, QSize(16, 16)))) << relativePath.toStdString();
  }
}

TEST(PlotWidget, toolbarButtonsHaveVisibleRasterIcons) {
  ensureApplication();

  PlotWidget widget;
  expectVisibleButtonIcon(&widget, "pushButtonRunPause");
  expectVisibleButtonIcon(&widget, "pushButtonClear");
  expectVisibleButtonIcon(&widget, "pushButtonImportExport");
  expectVisibleButtonIcon(&widget, "pushButtonSetup");
  expectVisibleButtonIcon(&widget, "pushButtonSplit");
  expectVisibleButtonIcon(&widget, "pushButtonState");
  expectVisibleButtonIcon(&widget, "pushButtonClose");
}

TEST(PlotTableConfigWidget, toolbarButtonsHaveVisibleRasterIcons) {
  ensureApplication();

  PlotTableConfigWidget widget;
  expectVisibleButtonIcon(&widget, "pushButtonRun");
  expectVisibleButtonIcon(&widget, "pushButtonPause");
  expectVisibleButtonIcon(&widget, "pushButtonClear");
  expectVisibleButtonIcon(&widget, "pushButtonResetLayout");
  expectVisibleButtonIcon(&widget, "pushButtonStartAtZero");
  expectVisibleButtonIcon(&widget, "pushButtonDateTime");
}

TEST(CurveConfigWidget, axisCopyButtonsHaveVisibleRasterIcons) {
  ensureApplication();

  CurveConfigWidget widget;
  expectVisibleButtonIcon(&widget, "pushButtonCopyRight", QSize(22, 22));
  expectVisibleButtonIcon(&widget, "pushButtonCopyLeft", QSize(22, 22));
  expectVisibleButtonIcon(&widget, "pushButtonSwap", QSize(22, 22));
}

TEST(PlotConfigWidget, curveButtonsHaveVisibleRasterIcons) {
  ensureApplication();

  PlotConfigWidget widget;
  expectVisibleButtonIcon(&widget, "pushButtonAddCurve");
  expectVisibleButtonIcon(&widget, "pushButtonEditCurve");
  expectVisibleButtonIcon(&widget, "pushButtonRemoveCurves");
  expectVisibleButtonIcon(&widget, "pushButtonCopyCurves");
  expectVisibleButtonIcon(&widget, "pushButtonPasteCurves");
}

TEST(PlotTabWidget, addTabButtonHasVisibleRasterIcon) {
  ensureApplication();

  PlotTabWidget widget;
  auto* tabs = widget.findChild<QTabWidget*>();
  ASSERT_NE(tabs, nullptr);
  auto* addButton = qobject_cast<QToolButton*>(tabs->cornerWidget(Qt::TopRightCorner));
  ASSERT_NE(addButton, nullptr);
  expectVisibleIcon(addButton->icon());
}

}  // namespace
