#include <gtest/gtest.h>

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QPainter>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTextStream>

#include "rqt_multiplot/PlotExport.hpp"

namespace {

using rqt_multiplot::bagDirectoryToRemember;
using rqt_multiplot::CurveTableHeaderStyle;
using rqt_multiplot::DataExportFormat;
using rqt_multiplot::dataFormatFromPath;
using rqt_multiplot::ensureFileSuffix;
using rqt_multiplot::exportDirectoryToRemember;
using rqt_multiplot::ImageExportFormat;
using rqt_multiplot::imageFormatFromPath;
using rqt_multiplot::initialBagDialogDirectory;
using rqt_multiplot::initialExportDialogDirectory;
using rqt_multiplot::kExportImageHeight;
using rqt_multiplot::kExportImageWidth;
using rqt_multiplot::rememberSessionExportDirectory;
using rqt_multiplot::renderExportImage;
using rqt_multiplot::setSessionLastExportDirectory;
using rqt_multiplot::suffixFromNameFilter;
using rqt_multiplot::writeCurveTable;

QString writeTable(CurveTableHeaderStyle headerStyle) {
  const QStringList titles = {"a_x", "a_y", "b_x", "b_y"};
  const QList<QStringList> columns = {{"1", "2", "3"}, {"10", "20", "30"}, {"100", "200"}, {"1000", "2000"}};

  QString output;
  QTextStream stream(&output);
  writeCurveTable(stream, titles, columns, headerStyle);
  return output;
}

TEST(PlotExport, padsShorterCurvesWithEmptyCells) {
  const QString output = writeTable(CurveTableHeaderStyle::Comment);
  const QStringList lines = output.split('\n', Qt::SkipEmptyParts);

  ASSERT_EQ(lines.size(), 4);
  EXPECT_EQ(lines[3], QString("3, 30, , "));
}

TEST(PlotExport, writesTxtHeaderAsComment) {
  const QString output = writeTable(CurveTableHeaderStyle::Comment);
  const QStringList lines = output.split('\n', Qt::SkipEmptyParts);

  ASSERT_FALSE(lines.isEmpty());
  EXPECT_EQ(lines[0], QString("# a_x, a_y, b_x, b_y"));
}

TEST(PlotExport, writesCsvHeaderWithoutCommentPrefix) {
  const QString output = writeTable(CurveTableHeaderStyle::Csv);
  const QStringList lines = output.split('\n', Qt::SkipEmptyParts);

  ASSERT_FALSE(lines.isEmpty());
  EXPECT_EQ(lines[0], QString("a_x, a_y, b_x, b_y"));
  EXPECT_FALSE(lines[0].startsWith('#'));
}

TEST(PlotExport, usesStandardExportDimensions) {
  EXPECT_EQ(kExportImageWidth, 1280);
  EXPECT_EQ(kExportImageHeight, 1024);
}

TEST(PlotExport, mapsImageSuffixToFormat) {
  EXPECT_EQ(imageFormatFromPath("rqt_multiplot.png"), ImageExportFormat::Png);
  EXPECT_EQ(imageFormatFromPath("plot.SVG"), ImageExportFormat::Svg);
  EXPECT_EQ(imageFormatFromPath("/tmp/out.pdf"), ImageExportFormat::Pdf);
}

TEST(PlotExport, mapsDataSuffixToFormat) {
  EXPECT_EQ(dataFormatFromPath("rqt_multiplot.txt"), DataExportFormat::Txt);
  EXPECT_EQ(dataFormatFromPath("plot.CSV"), DataExportFormat::Csv);
}

TEST(PlotExport, extractsSuffixFromNameFilter) {
  EXPECT_EQ(suffixFromNameFilter("Portable Network Graphics (*.png)"), QString("png"));
  EXPECT_EQ(suffixFromNameFilter("Scalable Vector Graphics (*.svg)"), QString("svg"));
  EXPECT_EQ(suffixFromNameFilter("Portable Document Format (*.pdf)"), QString("pdf"));
  EXPECT_EQ(suffixFromNameFilter("CSV (*.csv)"), QString("csv"));
}

TEST(PlotExport, appendsMissingSuffix) {
  EXPECT_EQ(ensureFileSuffix("rqt_multiplot", "png"), QString("rqt_multiplot.png"));
  EXPECT_EQ(ensureFileSuffix("rqt_multiplot.png", "png"), QString("rqt_multiplot.png"));
}

TEST(PlotExport, replacesMismatchedSuffixWithSelectedFilter) {
  EXPECT_EQ(ensureFileSuffix("rqt_multiplot.png", "svg"), QString("rqt_multiplot.svg"));
  EXPECT_EQ(ensureFileSuffix("/tmp/out.txt", "csv"), QString("/tmp/out.csv"));
}

TEST(PlotExport, initialBagDialogDirectoryUsesHomeWhenEmpty) {
  EXPECT_EQ(initialBagDialogDirectory(QString()), QDir::homePath());
}

TEST(PlotExport, initialBagDialogDirectoryUsesLastWhenSet) {
  const QString last = QStringLiteral("/data/bags");
  EXPECT_EQ(initialBagDialogDirectory(last), last);
}

TEST(PlotExport, bagDirectoryToRememberFromFilePath) {
  const QString remembered = bagDirectoryToRemember({QStringLiteral("/data/bags/run.mcap")});
  EXPECT_EQ(remembered, QStringLiteral("/data/bags"));
}

TEST(PlotExport, bagDirectoryToRememberFromDirectoryPath) {
  QTemporaryDir tempDir;
  ASSERT_TRUE(tempDir.isValid());
  const QString remembered = bagDirectoryToRemember({tempDir.path()});
  EXPECT_EQ(remembered, QDir(tempDir.path()).absolutePath());
}

TEST(PlotExport, bagDirectoryToRememberEmptyForNoSelection) {
  EXPECT_TRUE(bagDirectoryToRemember({}).isEmpty());
}

TEST(PlotExport, initialExportDialogDirectoryUsesHomeWhenSessionEmpty) {
  setSessionLastExportDirectory({});
  EXPECT_EQ(initialExportDialogDirectory(), QDir::homePath());
}

TEST(PlotExport, initialExportDialogDirectoryUsesSessionDirectory) {
  const QString exportDir = QStringLiteral("/tmp/exports");
  setSessionLastExportDirectory(exportDir);
  EXPECT_EQ(initialExportDialogDirectory(), exportDir);
  setSessionLastExportDirectory({});
}

TEST(PlotExport, exportDirectoryToRememberFromSavePath) {
  EXPECT_EQ(exportDirectoryToRemember(QStringLiteral("/tmp/exports/plot.png")), QStringLiteral("/tmp/exports"));
}

TEST(PlotExport, exportDirectoryToRememberEmptyForEmptyPath) {
  EXPECT_TRUE(exportDirectoryToRemember({}).isEmpty());
}

TEST(PlotExport, rememberSessionExportDirectoryUpdatesInitialDirectory) {
  setSessionLastExportDirectory({});
  rememberSessionExportDirectory(QStringLiteral("/var/out/data.csv"));
  EXPECT_EQ(initialExportDialogDirectory(), QStringLiteral("/var/out"));
  setSessionLastExportDirectory({});
}

TEST(PlotExport, rendersPngSvgAndPdfAndRejectsUnwritablePng) {
  if (QApplication::instance() == nullptr) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    static int argc = 1;
    static char arg0[] = "test_rqt_multiplot";
    static char* argv[] = {arg0, nullptr};
    new QApplication(argc, argv);
  }
  QTemporaryDir tempDir;
  ASSERT_TRUE(tempDir.isValid());
  const auto draw = [](QPainter& painter, const QRectF& bounds) { painter.fillRect(bounds, Qt::red); };

  const QString png = tempDir.filePath(QStringLiteral("plot.png"));
  const QString svg = tempDir.filePath(QStringLiteral("plot.svg"));
  const QString pdf = tempDir.filePath(QStringLiteral("plot.pdf"));
  EXPECT_TRUE(renderExportImage(png, draw));
  EXPECT_TRUE(renderExportImage(svg, draw));
  EXPECT_TRUE(renderExportImage(pdf, draw));
  EXPECT_TRUE(QFile::exists(png));
  EXPECT_GT(QFile(png).size(), 0);
  EXPECT_TRUE(QFile::exists(svg));
  EXPECT_TRUE(QFile::exists(pdf));

  EXPECT_FALSE(renderExportImage(QStringLiteral("/proc/does-not-exist/plot.png"), draw));
}

}  // namespace
