#include <gtest/gtest.h>

#include <QString>
#include <QStringList>
#include <QTextStream>

#include <rqt_multiplot/PlotExport.h>

namespace {

using rqt_multiplot::CurveTableHeaderStyle;
using rqt_multiplot::DataExportFormat;
using rqt_multiplot::ImageExportFormat;
using rqt_multiplot::dataFormatFromPath;
using rqt_multiplot::ensureFileSuffix;
using rqt_multiplot::imageFormatFromPath;
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

}  // namespace
