#include <QBuffer>
#include <QColor>
#include <QDataStream>
#include <QIODevice>
#include <QSettings>
#include <QTemporaryDir>

#include <gtest/gtest.h>

#include <rqt_multiplot/PlotTableConfig.h>

namespace {

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

}  // namespace
