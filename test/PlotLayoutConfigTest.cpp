#include <QBuffer>
#include <QDataStream>
#include <QIODevice>
#include <QSettings>
#include <QTemporaryDir>

#include <gtest/gtest.h>

#include <rqt_multiplot/PlotLayoutConfig.h>

namespace {

using rqt_multiplot::PlotConfig;
using rqt_multiplot::PlotLayoutConfig;

QString settingsPath(const QTemporaryDir& dir, const char* name) {
  return dir.filePath(QString::fromUtf8(name));
}

TEST(PlotLayoutConfig, defaultsToSinglePlot) {
  PlotLayoutConfig layout(nullptr);

  EXPECT_EQ(layout.getType(), PlotLayoutConfig::Plot);
  ASSERT_NE(layout.getPlotConfig(), nullptr);
  EXPECT_EQ(layout.plotCount(), 1u);
  EXPECT_EQ(layout.getNumRows(), 1u);
  EXPECT_EQ(layout.getNumColumns(), 1u);
}

TEST(PlotLayoutConfig, splitPlotNestsOrthogonalSplit) {
  PlotLayoutConfig layout(nullptr);
  PlotConfig* original = layout.getPlotConfig();
  original->setTitle("Left");

  PlotConfig* added = layout.splitPlot(original, Qt::Horizontal);
  ASSERT_NE(added, nullptr);
  EXPECT_EQ(layout.getType(), PlotLayoutConfig::Horizontal);
  EXPECT_EQ(layout.plotCount(), 2u);
  EXPECT_EQ(layout.getChildren().count(), 2);
  EXPECT_EQ(layout.getStretch(), (QList<int>{1, 1}));
  EXPECT_EQ(layout.plotConfigs().at(0), original);
  EXPECT_EQ(layout.plotConfigs().at(0)->getTitle(), QString("Left"));
  EXPECT_EQ(layout.plotConfigs().at(1), added);
}

TEST(PlotLayoutConfig, splitPlotInsertsSiblingForSameOrientation) {
  PlotLayoutConfig layout(nullptr);
  PlotConfig* first = layout.getPlotConfig();
  PlotConfig* second = layout.splitPlot(first, Qt::Horizontal);
  PlotConfig* third = layout.splitPlot(second, Qt::Horizontal);

  ASSERT_NE(third, nullptr);
  EXPECT_EQ(layout.getType(), PlotLayoutConfig::Horizontal);
  EXPECT_EQ(layout.getChildren().count(), 3);
  EXPECT_EQ(layout.plotCount(), 3u);
  EXPECT_EQ(layout.plotConfigs().at(2), third);
  EXPECT_EQ(layout.getStretch(), (QList<int>{2, 1, 1}));
}

TEST(PlotLayoutConfig, splitPlotHalvesOnlyTargetStretch) {
  PlotLayoutConfig layout(nullptr);
  PlotConfig* first = layout.getPlotConfig();
  PlotConfig* second = layout.splitPlot(first, Qt::Horizontal);
  layout.setStretch({3, 1});

  PlotConfig* third = layout.splitPlot(second, Qt::Horizontal);

  ASSERT_NE(third, nullptr);
  EXPECT_EQ(layout.getStretch(), (QList<int>{6, 1, 1}));
  EXPECT_EQ(layout.plotConfigs().at(0), first);
  EXPECT_EQ(layout.plotConfigs().at(1), second);
  EXPECT_EQ(layout.plotConfigs().at(2), third);
}

TEST(PlotLayoutConfig, splitPlotHalvesTargetWhenInsertingBefore) {
  PlotLayoutConfig layout(nullptr);
  PlotConfig* first = layout.getPlotConfig();
  PlotConfig* second = layout.splitPlot(first, Qt::Horizontal);
  layout.setStretch({3, 1});

  PlotConfig* added = layout.splitPlot(first, Qt::Horizontal, true);

  ASSERT_NE(added, nullptr);
  EXPECT_EQ(layout.getStretch(), (QList<int>{3, 3, 2}));
  EXPECT_EQ(layout.plotConfigs().at(0), added);
  EXPECT_EQ(layout.plotConfigs().at(1), first);
  EXPECT_EQ(layout.plotConfigs().at(2), second);
}

TEST(PlotLayoutConfig, splitPlotInsertsBeforeWhenRequested) {
  PlotLayoutConfig layout(nullptr);
  PlotConfig* original = layout.getPlotConfig();
  original->setTitle("Right");

  PlotConfig* added = layout.splitPlot(original, Qt::Horizontal, true);
  ASSERT_NE(added, nullptr);
  added->setTitle("Left");

  EXPECT_EQ(layout.getType(), PlotLayoutConfig::Horizontal);
  EXPECT_EQ(layout.plotConfigs().at(0), added);
  EXPECT_EQ(layout.plotConfigs().at(1), original);
}

TEST(PlotLayoutConfig, splitPlotNestsVerticalInsideHorizontal) {
  PlotLayoutConfig layout(nullptr);
  PlotConfig* left = layout.getPlotConfig();
  PlotConfig* right = layout.splitPlot(left, Qt::Horizontal);
  PlotConfig* bottomRight = layout.splitPlot(right, Qt::Vertical);

  ASSERT_NE(bottomRight, nullptr);
  EXPECT_EQ(layout.getType(), PlotLayoutConfig::Horizontal);
  EXPECT_EQ(layout.getChildren().count(), 2);
  EXPECT_EQ(layout.getChildren().at(1)->getType(), PlotLayoutConfig::Vertical);
  EXPECT_EQ(layout.plotCount(), 3u);
}

TEST(PlotLayoutConfig, closePlotCollapsesLastSibling) {
  PlotLayoutConfig layout(nullptr);
  PlotConfig* first = layout.getPlotConfig();
  first->setTitle("Keep");
  PlotConfig* second = layout.splitPlot(first, Qt::Vertical);

  EXPECT_TRUE(layout.closePlot(second));
  EXPECT_EQ(layout.getType(), PlotLayoutConfig::Plot);
  ASSERT_NE(layout.getPlotConfig(), nullptr);
  EXPECT_EQ(layout.getPlotConfig()->getTitle(), QString("Keep"));
  EXPECT_EQ(layout.plotCount(), 1u);
}

TEST(PlotLayoutConfig, closePlotRefusesLastPlot) {
  PlotLayoutConfig layout(nullptr);
  PlotConfig* only = layout.getPlotConfig();

  EXPECT_FALSE(layout.closePlot(only));
  EXPECT_EQ(layout.plotCount(), 1u);
}

TEST(PlotLayoutConfig, closePlotCollapsesNestedSplitter) {
  PlotLayoutConfig layout(nullptr);
  PlotConfig* left = layout.getPlotConfig();
  left->setTitle("Left");
  PlotConfig* right = layout.splitPlot(left, Qt::Horizontal);
  right->setTitle("Right");
  PlotConfig* bottomRight = layout.splitPlot(right, Qt::Vertical);
  layout.setStretch({3, 1});

  EXPECT_TRUE(layout.closePlot(bottomRight));
  EXPECT_EQ(layout.getType(), PlotLayoutConfig::Horizontal);
  EXPECT_EQ(layout.getChildren().count(), 2);
  EXPECT_EQ(layout.getChildren().at(1)->getType(), PlotLayoutConfig::Plot);
  EXPECT_EQ(layout.plotConfigs().at(1)->getTitle(), QString("Right"));
  EXPECT_EQ(layout.getStretch(), (QList<int>{3, 1}));
}

TEST(PlotLayoutConfig, closePlotGivesStretchToSplitSibling) {
  PlotLayoutConfig layout(nullptr);
  PlotConfig* first = layout.getPlotConfig();
  PlotConfig* second = layout.splitPlot(first, Qt::Horizontal);
  PlotConfig* third = layout.splitPlot(second, Qt::Horizontal);
  layout.setStretch({6, 1, 1});

  EXPECT_TRUE(layout.closePlot(third));
  EXPECT_EQ(layout.getChildren().count(), 2);
  EXPECT_EQ(layout.getStretch(), (QList<int>{3, 1}));
}

TEST(PlotLayoutConfig, closePlotGivesStretchToNextWhenClosingFirst) {
  PlotLayoutConfig layout(nullptr);
  PlotConfig* first = layout.getPlotConfig();
  PlotConfig* second = layout.splitPlot(first, Qt::Horizontal);
  layout.splitPlot(second, Qt::Horizontal);
  layout.setStretch({6, 1, 1});

  EXPECT_TRUE(layout.closePlot(first));
  EXPECT_EQ(layout.getStretch(), (QList<int>{7, 1}));
}

TEST(PlotLayoutConfig, setStretchReducesRatios) {
  PlotLayoutConfig layout(nullptr);
  layout.splitPlot(layout.getPlotConfig(), Qt::Horizontal);

  layout.setStretch({400, 200});
  EXPECT_EQ(layout.getStretch(), (QList<int>{2, 1}));
}

TEST(PlotLayoutConfig, stretchRatiosMatchAcrossPixelSizes) {
  PlotLayoutConfig small(nullptr);
  small.splitPlot(small.getPlotConfig(), Qt::Horizontal);
  small.setStretch({400, 200});

  PlotLayoutConfig large(nullptr);
  large.splitPlot(large.getPlotConfig(), Qt::Horizontal);
  large.setStretch({800, 400});

  EXPECT_EQ(small.getStretch(), (QList<int>{2, 1}));
  EXPECT_EQ(large.getStretch(), small.getStretch());
}

TEST(PlotLayoutConfig, roundTripsNestedLayoutAndStretchThroughSettings) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  {
    PlotLayoutConfig layout(nullptr);
    PlotConfig* left = layout.getPlotConfig();
    left->setTitle("Left");
    PlotConfig* right = layout.splitPlot(left, Qt::Horizontal);
    right->setTitle("RightTop");
    layout.splitPlot(right, Qt::Vertical);
    layout.setStretch({2, 1});
    layout.getChildren().at(1)->setStretch({3, 1});

    QSettings settings(settingsPath(dir, "layout.ini"), QSettings::IniFormat);
    layout.save(settings);
    settings.sync();
  }

  PlotLayoutConfig loaded(nullptr);
  QSettings settings(settingsPath(dir, "layout.ini"), QSettings::IniFormat);
  loaded.load(settings);

  EXPECT_EQ(loaded.getType(), PlotLayoutConfig::Horizontal);
  EXPECT_EQ(loaded.getStretch(), (QList<int>{2, 1}));
  ASSERT_EQ(loaded.plotCount(), 3u);
  EXPECT_EQ(loaded.plotConfigs().at(0)->getTitle(), QString("Left"));
  EXPECT_EQ(loaded.plotConfigs().at(1)->getTitle(), QString("RightTop"));
  EXPECT_EQ(loaded.getChildren().at(1)->getType(), PlotLayoutConfig::Vertical);
  EXPECT_EQ(loaded.getChildren().at(1)->getStretch(), (QList<int>{3, 1}));
}

TEST(PlotLayoutConfig, missingSizesDefaultsToEqualStretch) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  QSettings settings(settingsPath(dir, "nosizes.ini"), QSettings::IniFormat);
  settings.setValue("type", "horizontal");
  settings.beginGroup("child_0");
  settings.setValue("type", "plot");
  settings.setValue("title", "A");
  settings.endGroup();
  settings.beginGroup("child_1");
  settings.setValue("type", "plot");
  settings.setValue("title", "B");
  settings.endGroup();
  settings.sync();

  PlotLayoutConfig loaded(nullptr);
  loaded.load(settings);

  EXPECT_EQ(loaded.getStretch(), (QList<int>{1, 1}));
  ASSERT_EQ(loaded.plotCount(), 2u);
  EXPECT_EQ(loaded.plotConfigs().at(0)->getTitle(), QString("A"));
}

TEST(PlotLayoutConfig, rectangularGridFromTwoByTwo) {
  PlotLayoutConfig layout(nullptr);
  QList<PlotConfig*> preserved;
  layout.resetToRectangularGrid(2, 2, preserved);

  EXPECT_EQ(layout.getType(), PlotLayoutConfig::Vertical);
  EXPECT_EQ(layout.getNumRows(), 2u);
  EXPECT_EQ(layout.getNumColumns(), 2u);
  ASSERT_NE(layout.plotConfigAt(1, 1), nullptr);
  layout.plotConfigAt(0, 0)->setTitle("r0c0");
  layout.plotConfigAt(1, 0)->setTitle("r1c0");
  EXPECT_EQ(layout.plotConfigAt(0, 0)->getTitle(), QString("r0c0"));
  EXPECT_EQ(layout.plotConfigAt(1, 0)->getTitle(), QString("r1c0"));
}

TEST(PlotLayoutConfig, assignmentCopiesWithoutSharing) {
  PlotLayoutConfig source(nullptr);
  source.getPlotConfig()->setTitle("Source");
  source.splitPlot(source.getPlotConfig(), Qt::Vertical);

  PlotLayoutConfig dest(nullptr);
  dest = source;

  ASSERT_EQ(dest.plotCount(), 2u);
  EXPECT_EQ(dest.plotConfigs().at(0)->getTitle(), QString("Source"));
  EXPECT_NE(dest.plotConfigs().at(0), source.plotConfigs().at(0));
  dest.plotConfigs().at(0)->setTitle("Changed");
  EXPECT_EQ(source.plotConfigs().at(0)->getTitle(), QString("Source"));
}

TEST(PlotLayoutConfig, roundTripsThroughDataStream) {
  PlotLayoutConfig source(nullptr);
  source.getPlotConfig()->setTitle("A");
  PlotConfig* second = source.splitPlot(source.getPlotConfig(), Qt::Horizontal);
  second->setTitle("B");
  source.setStretch({2, 1});

  QBuffer buffer;
  buffer.open(QIODevice::ReadWrite);
  QDataStream out(&buffer);
  source.write(out);

  buffer.seek(0);
  QDataStream in(&buffer);
  PlotLayoutConfig loaded(nullptr);
  loaded.read(in);

  EXPECT_EQ(loaded.getType(), PlotLayoutConfig::Horizontal);
  EXPECT_EQ(loaded.getStretch(), (QList<int>{2, 1}));
  ASSERT_EQ(loaded.plotCount(), 2u);
  EXPECT_EQ(loaded.plotConfigs().at(0)->getTitle(), QString("A"));
  EXPECT_EQ(loaded.plotConfigs().at(1)->getTitle(), QString("B"));
}

}  // namespace
