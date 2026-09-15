#include <QBuffer>
#include <QColor>
#include <QDataStream>
#include <QIODevice>
#include <QSettings>
#include <QTemporaryDir>

#include <gtest/gtest.h>

#include <rqt_multiplot/PlotConfig.h>
#include <rqt_multiplot/PlotLayoutConfig.h>
#include <rqt_multiplot/PlotTableConfig.h>

namespace {

using rqt_multiplot::PlotConfig;
using rqt_multiplot::PlotLayoutConfig;
using rqt_multiplot::PlotTableConfig;

QString settingsPath(const QTemporaryDir& dir, const char* name) {
  return dir.filePath(QString::fromUtf8(name));
}

TEST(PlotTableConfig, defaultsToTab1Title) {
  PlotTableConfig config(nullptr);

  EXPECT_EQ(config.getTitle(), QString("Tab 1"));
  EXPECT_EQ(config.getNumRows(), 1u);
  EXPECT_EQ(config.getNumColumns(), 1u);
}

TEST(PlotTableConfig, setTitleEmitsTitleChanged) {
  PlotTableConfig config(nullptr);
  QString lastTitle;
  QObject::connect(&config, &PlotTableConfig::titleChanged, [&lastTitle](const QString& title) { lastTitle = title; });

  config.setTitle("Motors");

  EXPECT_EQ(config.getTitle(), QString("Motors"));
  EXPECT_EQ(lastTitle, QString("Motors"));
}

TEST(PlotTableConfig, savesAndLoadsTitle) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  {
    PlotTableConfig config(nullptr);
    config.setTitle("IMU");

    QSettings settings(settingsPath(dir, "title.ini"), QSettings::IniFormat);
    config.save(settings);
    settings.sync();
  }

  PlotTableConfig loaded(nullptr);
  loaded.setTitle("other");
  QSettings settings(settingsPath(dir, "title.ini"), QSettings::IniFormat);
  loaded.load(settings);

  EXPECT_EQ(loaded.getTitle(), QString("IMU"));
}

TEST(PlotTableConfig, missingTitleDefaultsToTab1) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  QSettings settings(settingsPath(dir, "legacy.ini"), QSettings::IniFormat);
  settings.setValue("link_scale", true);
  settings.sync();

  PlotTableConfig config(nullptr);
  config.setTitle("other");
  config.load(settings);

  EXPECT_EQ(config.getTitle(), QString("Tab 1"));
  EXPECT_TRUE(config.isScaleLinked());
}

TEST(PlotTableConfig, assignmentCopiesTitle) {
  PlotTableConfig source(nullptr);
  source.setTitle("Battery");

  PlotTableConfig dest(nullptr);
  dest = source;

  EXPECT_EQ(dest.getTitle(), QString("Battery"));
}

TEST(PlotTableConfig, roundTripsTitleThroughDataStream) {
  PlotTableConfig source(nullptr);
  source.setTitle("IMU");
  source.setLinkScale(true);

  QBuffer buffer;
  buffer.open(QIODevice::ReadWrite);
  QDataStream out(&buffer);
  source.write(out);

  buffer.seek(0);
  QDataStream in(&buffer);
  PlotTableConfig loaded(nullptr);
  loaded.setTitle("other");
  loaded.read(in);

  EXPECT_EQ(loaded.getTitle(), QString("IMU"));
  EXPECT_TRUE(loaded.isScaleLinked());
}

TEST(PlotTableConfig, legacyStreamWithoutTitleLeavesExistingTitle) {
  PlotTableConfig source(nullptr);
  source.setBackgroundColor(QColor(1, 2, 3));
  source.setLinkCursor(true);

  QBuffer buffer;
  buffer.open(QIODevice::ReadWrite);
  QDataStream out(&buffer);
  out << source.getBackgroundColor();
  out << source.getForegroundColor();
  out << static_cast<quint64>(source.getNumRows()) << static_cast<quint64>(source.getNumColumns());
  source.getPlotConfig(0, 0)->write(out);
  out << source.isScaleLinked();
  out << source.isCursorLinked();
  out << source.arePointsTracked();

  buffer.seek(0);
  QDataStream in(&buffer);
  PlotTableConfig loaded(nullptr);
  loaded.setTitle("other");
  loaded.read(in);

  EXPECT_EQ(loaded.getTitle(), QString("other"));
  EXPECT_EQ(loaded.getBackgroundColor(), QColor(1, 2, 3));
  EXPECT_TRUE(loaded.isCursorLinked());
}

TEST(PlotTableConfig, resetRestoresDefaultTitle) {
  PlotTableConfig config(nullptr);
  config.setTitle("Custom");
  config.setLinkScale(true);

  config.reset();

  EXPECT_EQ(config.getTitle(), QString("Tab 1"));
  EXPECT_FALSE(config.isScaleLinked());
}

TEST(PlotTableConfig, saveWritesLayoutNotPlots) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  PlotTableConfig config(nullptr);
  config.setNumPlots(2, 1);
  config.getPlotConfig(0, 0)->setTitle("Top");
  config.getPlotConfig(1, 0)->setTitle("Bottom");

  QSettings settings(settingsPath(dir, "layout.ini"), QSettings::IniFormat);
  config.save(settings);
  settings.sync();

  EXPECT_TRUE(settings.childGroups().contains("layout"));
  EXPECT_FALSE(settings.childGroups().contains("plots"));
}

TEST(PlotTableConfig, loadsLegacyPlotsGrid) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  QSettings settings(settingsPath(dir, "legacy.ini"), QSettings::IniFormat);
  settings.beginGroup("plots");
  settings.beginGroup("row_0");
  settings.beginGroup("column_0");
  settings.setValue("title", "Legacy Plot");
  settings.endGroup();
  settings.endGroup();
  settings.beginGroup("row_1");
  settings.beginGroup("column_0");
  settings.setValue("title", "Second");
  settings.endGroup();
  settings.endGroup();
  settings.endGroup();
  settings.sync();

  PlotTableConfig loaded(nullptr);
  loaded.load(settings);

  EXPECT_EQ(loaded.getNumRows(), 2u);
  EXPECT_EQ(loaded.getNumColumns(), 1u);
  EXPECT_EQ(loaded.getLayout()->getType(), PlotLayoutConfig::Vertical);
  EXPECT_EQ(loaded.getPlotConfig(0, 0)->getTitle(), QString("Legacy Plot"));
  EXPECT_EQ(loaded.getPlotConfig(1, 0)->getTitle(), QString("Second"));
}

TEST(PlotTableConfig, prefersLayoutWhenBothGroupsExist) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  QSettings settings(settingsPath(dir, "both.ini"), QSettings::IniFormat);
  settings.beginGroup("layout");
  settings.setValue("type", "plot");
  settings.setValue("title", "FromLayout");
  settings.endGroup();
  settings.beginGroup("plots");
  settings.beginGroup("row_0");
  settings.beginGroup("column_0");
  settings.setValue("title", "FromPlots");
  settings.endGroup();
  settings.endGroup();
  settings.endGroup();
  settings.sync();

  PlotTableConfig loaded(nullptr);
  loaded.load(settings);

  ASSERT_EQ(loaded.plotCount(), 1u);
  EXPECT_EQ(loaded.getPlotConfig(0, 0)->getTitle(), QString("FromLayout"));
}

TEST(PlotTableConfig, splitAndCloseUpdateLayout) {
  PlotTableConfig config(nullptr);
  PlotConfig* first = config.getPlotConfig(0, 0);
  PlotConfig* second = config.splitPlot(first, Qt::Horizontal);

  ASSERT_NE(second, nullptr);
  EXPECT_EQ(config.plotCount(), 2u);
  EXPECT_EQ(config.getLayout()->getType(), PlotLayoutConfig::Horizontal);

  EXPECT_TRUE(config.closePlot(second));
  EXPECT_EQ(config.plotCount(), 1u);
  EXPECT_EQ(config.getLayout()->getType(), PlotLayoutConfig::Plot);
}

}  // namespace
