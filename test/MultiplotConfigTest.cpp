#include <QBuffer>
#include <QColor>
#include <QDataStream>
#include <QDir>
#include <QIODevice>
#include <QSettings>
#include <QTemporaryDir>

#include <gtest/gtest.h>

#include "rqt_multiplot/CurveConfig.hpp"
#include "rqt_multiplot/MultiplotConfig.hpp"
#include "rqt_multiplot/PlotConfig.hpp"
#include "rqt_multiplot/PlotTableConfig.hpp"
#include "rqt_multiplot/PlotTitleStyle.hpp"
#include "rqt_multiplot/UserPreferences.hpp"
#include "rqt_multiplot/XmlSettings.hpp"

namespace {

using rqt_multiplot::CurveConfig;
using rqt_multiplot::MultiplotConfig;
using rqt_multiplot::PlotConfig;
using rqt_multiplot::PlotTableConfig;
using rqt_multiplot::PlotTitleStyle;
using rqt_multiplot::UserPreferences;
using rqt_multiplot::XmlSettings;

QString settingsPath(const QTemporaryDir& dir, const char* name) {
  return dir.filePath(QString::fromUtf8(name));
}

void beginMultiplot(QSettings& settings) {
  settings.beginGroup("rqt_multiplot");
}

class IsolatedUserPreferencesEnvironment : public ::testing::Environment {
 public:
  void SetUp() override {
    ASSERT_TRUE(tempDir_.isValid());
    UserPreferences::setTestSettingsFile(tempDir_.filePath(QStringLiteral("preferences.ini")));
    UserPreferences::factory().save();
  }

  void TearDown() override { UserPreferences::clearTestSettingsFile(); }

 private:
  QTemporaryDir tempDir_;
};

TEST(MultiplotConfig, defaultsToOneTabWithOnePlot) {
  MultiplotConfig config(nullptr);

  ASSERT_EQ(config.getNumTabs(), 1u);
  ASSERT_NE(config.getTableConfig(0), nullptr);
  EXPECT_EQ(config.getTableConfig(0)->getTitle(), QString("Tab 1"));
  EXPECT_EQ(config.getTableConfig(0)->getNumRows(), 1u);
  EXPECT_EQ(config.getTableConfig(0)->getNumColumns(), 1u);
  EXPECT_EQ(config.getCurrentTabIndex(), 0u);
}

TEST(MultiplotConfig, addTabUsesUniqueTitleAndIndependentSettings) {
  MultiplotConfig config(nullptr);
  auto* first = config.getTableConfig(0);
  first->setBackgroundColor(Qt::red);
  first->setLinkScale(true);
  first->setNumPlots(2, 1);

  auto* second = config.addTab();
  ASSERT_NE(second, nullptr);
  ASSERT_EQ(config.getNumTabs(), 2u);
  EXPECT_EQ(second->getTitle(), QString("Tab 2"));
  EXPECT_EQ(config.getCurrentTabIndex(), 1u);
  EXPECT_EQ(second->getNumRows(), 1u);
  EXPECT_EQ(second->getNumColumns(), 1u);
  EXPECT_FALSE(second->isScaleLinked());
  EXPECT_NE(second->getBackgroundColor(), first->getBackgroundColor());

  second->setBackgroundColor(Qt::blue);
  second->setLinkCursor(true);
  EXPECT_TRUE(first->isScaleLinked());
  EXPECT_FALSE(second->isScaleLinked());
  EXPECT_FALSE(first->isCursorLinked());
  EXPECT_TRUE(second->isCursorLinked());
}

TEST(MultiplotConfig, removeTabRefusesTheLastTab) {
  MultiplotConfig config(nullptr);

  config.removeTab(0);

  ASSERT_EQ(config.getNumTabs(), 1u);
  EXPECT_EQ(config.getTableConfig(0)->getTitle(), QString("Tab 1"));
}

TEST(MultiplotConfig, removeTabKeepsRemainingTabs) {
  MultiplotConfig config(nullptr);
  config.getTableConfig(0)->setTitle("Left");
  config.addTab()->setTitle("Middle");
  config.addTab()->setTitle("Right");

  config.removeTab(1);

  ASSERT_EQ(config.getNumTabs(), 2u);
  EXPECT_EQ(config.getTableConfig(0)->getTitle(), QString("Left"));
  EXPECT_EQ(config.getTableConfig(1)->getTitle(), QString("Right"));
}

TEST(MultiplotConfig, setTabTitleRejectsEmptyName) {
  MultiplotConfig config(nullptr);

  config.setTabTitle(0, QString());
  EXPECT_EQ(config.getTableConfig(0)->getTitle(), QString("Tab 1"));

  config.setTabTitle(0, "  ");
  EXPECT_EQ(config.getTableConfig(0)->getTitle(), QString("Tab 1"));

  config.setTabTitle(0, "Motors");
  EXPECT_EQ(config.getTableConfig(0)->getTitle(), QString("Motors"));
}

TEST(MultiplotConfig, assignmentCopiesTabsWithoutSharing) {
  MultiplotConfig source(nullptr);
  source.getTableConfig(0)->setTitle("Left");
  source.getTableConfig(0)->setLinkScale(true);
  auto* second = source.addTab();
  second->setTitle("Right");
  second->setTrackPoints(true);

  MultiplotConfig dest(nullptr);
  dest = source;

  ASSERT_EQ(dest.getNumTabs(), 2u);
  EXPECT_EQ(dest.getTableConfig(0)->getTitle(), QString("Left"));
  EXPECT_TRUE(dest.getTableConfig(0)->isScaleLinked());
  EXPECT_EQ(dest.getTableConfig(1)->getTitle(), QString("Right"));
  EXPECT_TRUE(dest.getTableConfig(1)->arePointsTracked());
  EXPECT_NE(dest.getTableConfig(0), source.getTableConfig(0));
  dest.getTableConfig(0)->setTitle("Changed");
  EXPECT_EQ(source.getTableConfig(0)->getTitle(), QString("Left"));
}

TEST(MultiplotConfig, roundTripsTwoTabsThroughXml) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const QString path = settingsPath(dir, "tabs.xml");

  {
    MultiplotConfig config(nullptr);
    auto* first = config.getTableConfig(0);
    first->setTitle("Left");
    first->setNumPlots(2, 1);
    first->setLinkScale(true);
    first->setBackgroundColor(QColor(255, 0, 0));
    first->getPlotConfig(1, 0)->addCurve()->setTitle("pannant");

    auto* second = config.addTab();
    second->setTitle("Right");
    second->setNumPlots(1, 3);
    second->setTrackPoints(true);
    second->setLinkCursor(true);

    QSettings settings(path, XmlSettings::format);
    beginMultiplot(settings);
    config.save(settings);
    settings.endGroup();
    settings.sync();
    ASSERT_EQ(settings.status(), QSettings::NoError);
  }

  QSettings written(path, XmlSettings::format);
  beginMultiplot(written);
  EXPECT_TRUE(written.childGroups().contains("tabs"));
  EXPECT_FALSE(written.childGroups().contains("table"));
  written.endGroup();

  MultiplotConfig loaded(nullptr);
  loaded.addTab();
  QSettings settings(path, XmlSettings::format);
  beginMultiplot(settings);
  loaded.load(settings);
  settings.endGroup();

  ASSERT_EQ(loaded.getNumTabs(), 2u);
  EXPECT_EQ(loaded.getTableConfig(0)->getTitle(), QString("Left"));
  EXPECT_EQ(loaded.getTableConfig(0)->getNumRows(), 2u);
  EXPECT_EQ(loaded.getTableConfig(0)->getNumColumns(), 1u);
  EXPECT_TRUE(loaded.getTableConfig(0)->isScaleLinked());
  EXPECT_EQ(loaded.getTableConfig(0)->getBackgroundColor(), QColor(Qt::white));
  ASSERT_EQ(loaded.getTableConfig(0)->getPlotConfig(1, 0)->getNumCurves(), 1u);
  EXPECT_EQ(loaded.getTableConfig(0)->getPlotConfig(1, 0)->getCurveConfig(0)->getTitle(), QString("pannant"));

  EXPECT_EQ(loaded.getTableConfig(1)->getTitle(), QString("Right"));
  EXPECT_EQ(loaded.getTableConfig(1)->getNumRows(), 1u);
  EXPECT_EQ(loaded.getTableConfig(1)->getNumColumns(), 3u);
  EXPECT_TRUE(loaded.getTableConfig(1)->arePointsTracked());
  EXPECT_TRUE(loaded.getTableConfig(1)->isCursorLinked());
  EXPECT_EQ(loaded.getCurrentTabIndex(), 1u);
}

TEST(MultiplotConfig, loadsLegacyTableXmlOntoSingleTab) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const QString path = settingsPath(dir, "legacy.xml");

  {
    PlotTableConfig table(nullptr);
    table.setTitle("should not persist");
    table.setNumPlots(2, 1);
    table.setBackgroundColor(QColor(0, 128, 255));
    table.setForegroundColor(QColor(32, 32, 32));
    table.setLinkScale(true);
    table.setLinkCursor(true);
    table.setTrackPoints(true);
    table.getPlotConfig(0, 0)->setTitle("Legacy Plot");
    table.getPlotConfig(1, 0)->addCurve()->setTitle("pannant");

    QSettings settings(path, XmlSettings::format);
    beginMultiplot(settings);
    settings.beginGroup("table");
    table.save(settings);
    settings.remove("title");
    settings.endGroup();
    settings.endGroup();
    settings.sync();
    ASSERT_EQ(settings.status(), QSettings::NoError);
  }

  MultiplotConfig loaded(nullptr);
  loaded.addTab();
  loaded.addTab();
  QSettings settings(path, XmlSettings::format);
  beginMultiplot(settings);
  loaded.load(settings);
  settings.endGroup();

  ASSERT_EQ(loaded.getNumTabs(), 1u);
  const PlotTableConfig* tab = loaded.getTableConfig(0);
  ASSERT_NE(tab, nullptr);
  EXPECT_EQ(tab->getTitle(), QString("Tab 1"));
  EXPECT_EQ(tab->getNumRows(), 2u);
  EXPECT_EQ(tab->getNumColumns(), 1u);
  EXPECT_EQ(tab->getBackgroundColor(), QColor(Qt::white));
  EXPECT_EQ(tab->getForegroundColor(), QColor(Qt::black));
  EXPECT_TRUE(tab->isScaleLinked());
  EXPECT_TRUE(tab->isCursorLinked());
  EXPECT_TRUE(tab->arePointsTracked());
  EXPECT_EQ(tab->getPlotConfig(0, 0)->getTitle(), QString("Legacy Plot"));
  ASSERT_EQ(tab->getPlotConfig(1, 0)->getNumCurves(), 1u);
  EXPECT_EQ(tab->getPlotConfig(1, 0)->getCurveConfig(0)->getTitle(), QString("pannant"));
  EXPECT_EQ(loaded.getCurrentTabIndex(), 0u);
}

void writeLegacyTableStream(QDataStream& stream, const PlotTableConfig& table) {
  stream << table.getBackgroundColor();
  stream << table.getForegroundColor();
  stream << static_cast<quint64>(table.getNumRows()) << static_cast<quint64>(table.getNumColumns());

  for (size_t row = 0; row < table.getNumRows(); ++row) {
    for (size_t column = 0; column < table.getNumColumns(); ++column) {
      table.getPlotConfig(row, column)->write(stream);
    }
  }

  stream << table.isScaleLinked();
  stream << table.isCursorLinked();
  stream << table.arePointsTracked();
}

TEST(MultiplotConfig, roundTripsTwoTabsThroughDataStream) {
  MultiplotConfig source(nullptr);
  source.getTableConfig(0)->setTitle("Left");
  source.getTableConfig(0)->setLinkScale(true);
  source.getTableConfig(0)->setBackgroundColor(QColor(255, 0, 0));
  auto* second = source.addTab();
  second->setTitle("Right");
  second->setTrackPoints(true);
  second->setNumPlots(1, 2);

  QBuffer buffer;
  buffer.open(QIODevice::ReadWrite);
  QDataStream out(&buffer);
  source.write(out);

  buffer.seek(0);
  QDataStream in(&buffer);
  MultiplotConfig loaded(nullptr);
  loaded.addTab();
  loaded.read(in);

  ASSERT_EQ(loaded.getNumTabs(), 2u);
  EXPECT_EQ(loaded.getTableConfig(0)->getTitle(), QString("Left"));
  EXPECT_TRUE(loaded.getTableConfig(0)->isScaleLinked());
  EXPECT_EQ(loaded.getTableConfig(0)->getBackgroundColor(), QColor(Qt::white));
  EXPECT_EQ(loaded.getTableConfig(1)->getTitle(), QString("Right"));
  EXPECT_TRUE(loaded.getTableConfig(1)->arePointsTracked());
  EXPECT_EQ(loaded.getTableConfig(1)->getNumColumns(), 2u);
  EXPECT_EQ(loaded.getCurrentTabIndex(), 1u);
}

TEST(MultiplotConfig, loadsLegacyTableStreamOntoSingleTab) {
  PlotTableConfig table(nullptr);
  table.setTitle("should not persist");
  table.setNumPlots(2, 1);
  table.setBackgroundColor(QColor(0, 128, 255));
  table.setLinkScale(true);
  table.setLinkCursor(true);
  table.setTrackPoints(true);
  table.getPlotConfig(0, 0)->setTitle("Legacy Plot");
  table.getPlotConfig(1, 0)->addCurve()->setTitle("pannant");

  QBuffer buffer;
  buffer.open(QIODevice::ReadWrite);
  QDataStream out(&buffer);
  writeLegacyTableStream(out, table);

  buffer.seek(0);
  QDataStream in(&buffer);
  MultiplotConfig loaded(nullptr);
  loaded.addTab();
  loaded.read(in);

  ASSERT_EQ(loaded.getNumTabs(), 1u);
  const PlotTableConfig* tab = loaded.getTableConfig(0);
  ASSERT_NE(tab, nullptr);
  EXPECT_EQ(tab->getTitle(), QString("Tab 1"));
  EXPECT_EQ(tab->getNumRows(), 2u);
  EXPECT_EQ(tab->getNumColumns(), 1u);
  EXPECT_EQ(tab->getBackgroundColor(), QColor(Qt::white));
  EXPECT_TRUE(tab->isScaleLinked());
  EXPECT_TRUE(tab->isCursorLinked());
  EXPECT_TRUE(tab->arePointsTracked());
  EXPECT_EQ(tab->getPlotConfig(0, 0)->getTitle(), QString("Legacy Plot"));
  ASSERT_EQ(tab->getPlotConfig(1, 0)->getNumCurves(), 1u);
  EXPECT_EQ(tab->getPlotConfig(1, 0)->getCurveConfig(0)->getTitle(), QString("pannant"));
  EXPECT_EQ(loaded.getCurrentTabIndex(), 0u);
}

QByteArray snapshotOf(const MultiplotConfig& config) {
  QByteArray bytes;
  QBuffer buffer(&bytes);
  buffer.open(QIODevice::WriteOnly);
  QDataStream stream(&buffer);
  config.write(stream);
  return bytes;
}

TEST(MultiplotConfig, snapshotMatchesAfterAddAndRemoveTab) {
  MultiplotConfig config(nullptr);
  const QByteArray original = snapshotOf(config);

  config.addTab();
  EXPECT_NE(snapshotOf(config), original);

  config.removeTab(1);
  EXPECT_EQ(snapshotOf(config), original);
}

TEST(MultiplotConfig, snapshotMatchesAfterSwitchingTabAndBack) {
  MultiplotConfig config(nullptr);
  config.addTab();
  const QByteArray original = snapshotOf(config);

  config.setCurrentTabIndex(0);
  EXPECT_NE(snapshotOf(config), original);

  config.setCurrentTabIndex(1);
  EXPECT_EQ(snapshotOf(config), original);
}

TEST(MultiplotConfig, defaultsToLocalTimeZone) {
  MultiplotConfig config(nullptr);

  EXPECT_EQ(config.getTimeZoneId(), QStringLiteral("local"));
}

TEST(MultiplotConfig, localTimeZoneUsesTzEnvironment) {
  const QTimeZone berlin(QStringLiteral("Europe/Berlin").toUtf8());
  if (!berlin.isValid()) {
    GTEST_SKIP() << "Europe/Berlin unavailable in Qt tzdata";
  }

  const QByteArray previous = qgetenv("TZ");
  qputenv("TZ", QByteArray("Europe/Berlin"));

  MultiplotConfig config(nullptr);
  EXPECT_EQ(config.getTimeZoneId(), QStringLiteral("local"));
  EXPECT_EQ(config.timeZone(), berlin);

  if (previous.isEmpty()) {
    qunsetenv("TZ");
  } else {
    qputenv("TZ", previous);
  }
}

TEST(MultiplotConfig, savesAndLoadsTimeZone) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const QString path = settingsPath(dir, "timezone.xml");

  {
    MultiplotConfig config(nullptr);
    config.setPreferencesOverridden(true);
    config.setTimeZoneId(QStringLiteral("Europe/Zurich"));

    QSettings settings(path, XmlSettings::format);
    beginMultiplot(settings);
    config.save(settings);
    settings.endGroup();
    settings.sync();
    ASSERT_EQ(settings.status(), QSettings::NoError);
  }

  MultiplotConfig loaded(nullptr);
  QSettings settings(path, XmlSettings::format);
  beginMultiplot(settings);
  loaded.load(settings);
  settings.endGroup();

  EXPECT_EQ(loaded.getTimeZoneId(), QStringLiteral("Europe/Zurich"));
}

TEST(MultiplotConfig, missingTimeZoneKeyKeepsLocalDefault) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const QString path = settingsPath(dir, "timezone-default.xml");

  {
    MultiplotConfig config(nullptr);
    QSettings settings(path, XmlSettings::format);
    beginMultiplot(settings);
    config.save(settings);
    settings.remove("time_zone");
    settings.endGroup();
    settings.sync();
  }

  MultiplotConfig loaded(nullptr);
  loaded.setTimeZoneId(QStringLiteral("utc"));
  QSettings settings(path, XmlSettings::format);
  beginMultiplot(settings);
  loaded.load(settings);
  settings.endGroup();

  EXPECT_EQ(loaded.getTimeZoneId(), QStringLiteral("local"));
}

TEST(MultiplotConfig, invalidTimeZoneIdFallsBackToLocal) {
  MultiplotConfig config(nullptr);
  config.setTimeZoneId(QStringLiteral("Not/A/Zone"));

  EXPECT_EQ(config.getTimeZoneId(), QStringLiteral("local"));
}

TEST(MultiplotConfig, roundTripsTimeZoneThroughDataStream) {
  MultiplotConfig source(nullptr);
  source.setPreferencesOverridden(true);
  source.setTimeZoneId(QStringLiteral("utc"));

  QBuffer buffer;
  buffer.open(QIODevice::ReadWrite);
  QDataStream out(&buffer);
  source.write(out);

  buffer.seek(0);
  QDataStream in(&buffer);
  MultiplotConfig loaded(nullptr);
  loaded.read(in);

  EXPECT_EQ(loaded.getTimeZoneId(), QStringLiteral("utc"));
}

TEST(MultiplotConfig, legacyDataStreamWithoutTimeZoneKeepsLocal) {
  MultiplotConfig source(nullptr);
  source.getTableConfig(0)->setTitle("Legacy");

  QBuffer buffer;
  buffer.open(QIODevice::ReadWrite);
  QDataStream out(&buffer);
  out << static_cast<quint32>(0x52544D31);
  out << static_cast<quint64>(1);
  out << static_cast<quint64>(0);
  source.getTableConfig(0)->write(out);

  buffer.seek(0);
  QDataStream in(&buffer);
  MultiplotConfig loaded(nullptr);
  loaded.setTimeZoneId(QStringLiteral("utc"));
  loaded.read(in);

  EXPECT_EQ(loaded.getTimeZoneId(), QStringLiteral("local"));
  EXPECT_EQ(loaded.getTableConfig(0)->getTitle(), QString("Legacy"));
}

TEST(MultiplotConfig, defaultsToLightThemeAndLightPlotColors) {
  MultiplotConfig config(nullptr);

  EXPECT_EQ(config.getThemeId(), QStringLiteral("light"));
  EXPECT_EQ(config.getTableConfig(0)->getBackgroundColor(), QColor(Qt::white));
  EXPECT_EQ(config.getTableConfig(0)->getForegroundColor(), QColor(Qt::black));
}

TEST(MultiplotConfig, defaultsOpenGLCanvasDisabled) {
  MultiplotConfig config(nullptr);

  EXPECT_FALSE(config.isOpenGLCanvasEnabled());
}

TEST(MultiplotConfig, setThemeIdWritesPlotColorsOnAllTabs) {
  MultiplotConfig config(nullptr);
  config.addTab();
  config.getTableConfig(0)->setBackgroundColor(Qt::red);
  config.getTableConfig(1)->setForegroundColor(Qt::blue);

  config.setThemeId(QStringLiteral("dark"));

  EXPECT_EQ(config.getThemeId(), QStringLiteral("dark"));
  EXPECT_EQ(config.getTableConfig(0)->getBackgroundColor(), QColor(0x1e, 0x1e, 0x1e));
  EXPECT_EQ(config.getTableConfig(0)->getForegroundColor(), QColor(0xe6, 0xe6, 0xe6));
  EXPECT_EQ(config.getTableConfig(1)->getBackgroundColor(), QColor(0x1e, 0x1e, 0x1e));
  EXPECT_EQ(config.getTableConfig(1)->getForegroundColor(), QColor(0xe6, 0xe6, 0xe6));
}

TEST(MultiplotConfig, addTabInheritsCurrentThemeColors) {
  MultiplotConfig config(nullptr);
  config.setThemeId(QStringLiteral("dark"));

  auto* second = config.addTab();

  ASSERT_NE(second, nullptr);
  EXPECT_EQ(second->getBackgroundColor(), QColor(0x1e, 0x1e, 0x1e));
  EXPECT_EQ(second->getForegroundColor(), QColor(0xe6, 0xe6, 0xe6));
}

TEST(MultiplotConfig, invalidThemeIdFallsBackToLight) {
  MultiplotConfig config(nullptr);
  config.setThemeId(QStringLiteral("custom"));

  EXPECT_EQ(config.getThemeId(), QStringLiteral("light"));
}

TEST(MultiplotConfig, savesAndLoadsTheme) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const QString path = settingsPath(dir, "theme.xml");

  {
    MultiplotConfig config(nullptr);
    config.setPreferencesOverridden(true);
    config.setThemeId(QStringLiteral("dark"));

    QSettings settings(path, XmlSettings::format);
    beginMultiplot(settings);
    config.save(settings);
    settings.endGroup();
    settings.sync();
    ASSERT_EQ(settings.status(), QSettings::NoError);
  }

  MultiplotConfig loaded(nullptr);
  QSettings settings(path, XmlSettings::format);
  beginMultiplot(settings);
  loaded.load(settings);
  settings.endGroup();

  EXPECT_TRUE(loaded.isPreferencesOverridden());
  EXPECT_EQ(loaded.getThemeId(), QStringLiteral("dark"));
  EXPECT_EQ(loaded.getTableConfig(0)->getBackgroundColor(), QColor(0x1e, 0x1e, 0x1e));
  EXPECT_EQ(loaded.getTableConfig(0)->getForegroundColor(), QColor(0xe6, 0xe6, 0xe6));
}

TEST(MultiplotConfig, missingThemeKeyLoadsLightAndOverwritesTabColors) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const QString path = settingsPath(dir, "theme-default.xml");

  {
    MultiplotConfig config(nullptr);
    config.getTableConfig(0)->setBackgroundColor(Qt::red);
    config.getTableConfig(0)->setForegroundColor(Qt::blue);

    QSettings settings(path, XmlSettings::format);
    beginMultiplot(settings);
    config.save(settings);
    settings.remove("theme");
    settings.endGroup();
    settings.sync();
  }

  MultiplotConfig loaded(nullptr);
  loaded.setThemeId(QStringLiteral("dark"));
  QSettings settings(path, XmlSettings::format);
  beginMultiplot(settings);
  loaded.load(settings);
  settings.endGroup();

  EXPECT_EQ(loaded.getThemeId(), QStringLiteral("light"));
  EXPECT_EQ(loaded.getTableConfig(0)->getBackgroundColor(), QColor(Qt::white));
  EXPECT_EQ(loaded.getTableConfig(0)->getForegroundColor(), QColor(Qt::black));
}

TEST(MultiplotConfig, roundTripsThemeThroughDataStream) {
  MultiplotConfig source(nullptr);
  source.setPreferencesOverridden(true);
  source.setThemeId(QStringLiteral("dark"));

  QBuffer buffer;
  buffer.open(QIODevice::ReadWrite);
  QDataStream out(&buffer);
  source.write(out);

  buffer.seek(0);
  QDataStream in(&buffer);
  MultiplotConfig loaded(nullptr);
  loaded.read(in);

  EXPECT_EQ(loaded.getThemeId(), QStringLiteral("dark"));
}

TEST(MultiplotConfig, legacyDataStreamWithoutThemeKeepsLight) {
  MultiplotConfig source(nullptr);
  source.getTableConfig(0)->setTitle("Legacy");

  QBuffer buffer;
  buffer.open(QIODevice::ReadWrite);
  QDataStream out(&buffer);
  out << static_cast<quint32>(0x52544D31);
  out << static_cast<quint64>(1);
  out << static_cast<quint64>(0);
  source.getTableConfig(0)->write(out);
  out << QStringLiteral("utc");

  buffer.seek(0);
  QDataStream in(&buffer);
  MultiplotConfig loaded(nullptr);
  loaded.setThemeId(QStringLiteral("dark"));
  loaded.read(in);

  EXPECT_EQ(loaded.getThemeId(), QStringLiteral("light"));
  EXPECT_EQ(loaded.getTimeZoneId(), QStringLiteral("utc"));
  EXPECT_EQ(loaded.getTableConfig(0)->getTitle(), QString("Legacy"));
}

TEST(MultiplotConfig, savesAndLoadsOpenGLCanvas) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const QString path = settingsPath(dir, "opengl.xml");

  {
    MultiplotConfig config(nullptr);
    config.setPreferencesOverridden(true);
    config.setOpenGLCanvasEnabled(true);

    QSettings settings(path, XmlSettings::format);
    beginMultiplot(settings);
    config.save(settings);
    settings.endGroup();
    settings.sync();
    ASSERT_EQ(settings.status(), QSettings::NoError);
  }

  MultiplotConfig loaded(nullptr);
  QSettings settings(path, XmlSettings::format);
  beginMultiplot(settings);
  loaded.load(settings);
  settings.endGroup();

  EXPECT_TRUE(loaded.isPreferencesOverridden());
  EXPECT_TRUE(loaded.isOpenGLCanvasEnabled());
}

TEST(MultiplotConfig, missingOpenGLCanvasKeyLoadsDisabled) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const QString path = settingsPath(dir, "opengl-default.xml");

  {
    MultiplotConfig config(nullptr);
    config.setOpenGLCanvasEnabled(true);

    QSettings settings(path, XmlSettings::format);
    beginMultiplot(settings);
    config.save(settings);
    settings.remove("opengl_canvas");
    settings.endGroup();
    settings.sync();
  }

  MultiplotConfig loaded(nullptr);
  QSettings settings(path, XmlSettings::format);
  beginMultiplot(settings);
  loaded.load(settings);
  settings.endGroup();

  EXPECT_FALSE(loaded.isOpenGLCanvasEnabled());
}

TEST(MultiplotConfig, roundTripsOpenGLCanvasThroughDataStream) {
  MultiplotConfig source(nullptr);
  source.setPreferencesOverridden(true);
  source.setOpenGLCanvasEnabled(true);

  QBuffer buffer;
  buffer.open(QIODevice::ReadWrite);
  QDataStream out(&buffer);
  source.write(out);

  buffer.seek(0);
  QDataStream in(&buffer);
  MultiplotConfig loaded(nullptr);
  loaded.read(in);

  EXPECT_TRUE(loaded.isOpenGLCanvasEnabled());
}

TEST(MultiplotConfig, legacyDataStreamWithoutOpenGLCanvasKeepsDisabled) {
  MultiplotConfig source(nullptr);
  source.getTableConfig(0)->setTitle("Legacy");

  QBuffer buffer;
  buffer.open(QIODevice::ReadWrite);
  QDataStream out(&buffer);
  out << static_cast<quint32>(0x52544D31);
  out << static_cast<quint64>(1);
  out << static_cast<quint64>(0);
  source.getTableConfig(0)->write(out);
  out << QStringLiteral("utc");
  out << QStringLiteral("dark");

  buffer.seek(0);
  QDataStream in(&buffer);
  MultiplotConfig loaded(nullptr);
  loaded.setOpenGLCanvasEnabled(true);
  loaded.read(in);

  EXPECT_FALSE(loaded.isOpenGLCanvasEnabled());
  EXPECT_EQ(loaded.getThemeId(), QStringLiteral("dark"));
}

TEST(MultiplotConfig, saveOmitsPreferenceKeysWhenNotOverridden) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const QString path = settingsPath(dir, "no-pref-override.xml");

  {
    MultiplotConfig config(nullptr);
    config.setThemeId(QStringLiteral("dark"));
    config.setTimeZoneId(QStringLiteral("utc"));
    config.setOpenGLCanvasEnabled(true);

    QSettings settings(path, XmlSettings::format);
    beginMultiplot(settings);
    config.save(settings);
    settings.endGroup();
    settings.sync();
  }

  QSettings settings(path, XmlSettings::format);
  beginMultiplot(settings);
  EXPECT_FALSE(settings.contains(QStringLiteral("time_zone")));
  EXPECT_FALSE(settings.contains(QStringLiteral("theme")));
  EXPECT_FALSE(settings.contains(QStringLiteral("opengl_canvas")));
  EXPECT_FALSE(settings.contains(QStringLiteral("plot_title_font_size")));
  EXPECT_FALSE(settings.contains(QStringLiteral("plot_title_bold")));
  EXPECT_FALSE(settings.contains(QStringLiteral("plot_title_auto_color")));
  EXPECT_FALSE(settings.contains(QStringLiteral("plot_title_color")));
  settings.endGroup();
}

TEST(MultiplotConfig, savesAndLoadsPlotTitleStyle) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const QString path = settingsPath(dir, "plot-title-style.xml");

  PlotTitleStyle style = PlotTitleStyle::factory();
  style.fontSize = 16;
  style.bold = true;
  style.autoColor = false;
  style.customColor = QColor(0x22, 0x33, 0x44);

  {
    MultiplotConfig config(nullptr);
    config.setPreferencesOverridden(true);
    config.setPlotTitleStyle(style);

    QSettings settings(path, XmlSettings::format);
    beginMultiplot(settings);
    config.save(settings);
    settings.endGroup();
    settings.sync();
    ASSERT_EQ(settings.status(), QSettings::NoError);
  }

  MultiplotConfig loaded(nullptr);
  QSettings settings(path, XmlSettings::format);
  beginMultiplot(settings);
  loaded.load(settings);
  settings.endGroup();

  EXPECT_TRUE(loaded.isPreferencesOverridden());
  EXPECT_EQ(loaded.plotTitleStyle(), style);
}

TEST(MultiplotConfig, missingPlotTitleKeysUseFactoryDefaults) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const QString path = settingsPath(dir, "plot-title-defaults.xml");

  {
    MultiplotConfig config(nullptr);
    config.setPreferencesOverridden(true);
    config.setPlotTitleStyle(PlotTitleStyle::factory());

    QSettings settings(path, XmlSettings::format);
    beginMultiplot(settings);
    config.save(settings);
    settings.remove(QStringLiteral("plot_title_font_size"));
    settings.remove(QStringLiteral("plot_title_bold"));
    settings.remove(QStringLiteral("plot_title_auto_color"));
    settings.remove(QStringLiteral("plot_title_color"));
    settings.endGroup();
    settings.sync();
  }

  MultiplotConfig loaded(nullptr);
  QSettings settings(path, XmlSettings::format);
  beginMultiplot(settings);
  loaded.load(settings);
  settings.endGroup();

  EXPECT_EQ(loaded.plotTitleStyle(), PlotTitleStyle::factory());
}

TEST(MultiplotConfig, roundTripsPlotTitleStyleThroughDataStream) {
  PlotTitleStyle style = PlotTitleStyle::factory();
  style.fontSize = 20;
  style.bold = true;
  style.autoColor = false;
  style.customColor = QColor(0xaa, 0xbb, 0xcc);

  MultiplotConfig source(nullptr);
  source.setPreferencesOverridden(true);
  source.setPlotTitleStyle(style);

  QBuffer buffer;
  buffer.open(QIODevice::ReadWrite);
  QDataStream out(&buffer);
  source.write(out);

  buffer.seek(0);
  QDataStream in(&buffer);
  MultiplotConfig loaded(nullptr);
  loaded.read(in);

  EXPECT_EQ(loaded.plotTitleStyle(), style);
}

TEST(MultiplotConfig, legacyDataStreamWithoutPlotTitleStyleKeepsFactory) {
  MultiplotConfig source(nullptr);
  source.getTableConfig(0)->setTitle("Legacy");

  QBuffer buffer;
  buffer.open(QIODevice::ReadWrite);
  QDataStream out(&buffer);
  out << static_cast<quint32>(0x52544D31);
  out << static_cast<quint64>(1);
  out << static_cast<quint64>(0);
  source.getTableConfig(0)->write(out);
  out << QStringLiteral("__rtp_prefs_v2__");
  out << true;
  out << QStringLiteral("utc");
  out << QStringLiteral("dark");
  out << true;

  buffer.seek(0);
  QDataStream in(&buffer);
  MultiplotConfig loaded(nullptr);
  PlotTitleStyle customStyle = PlotTitleStyle::factory();
  customStyle.fontSize = 20;
  loaded.setPlotTitleStyle(customStyle);
  loaded.read(in);

  EXPECT_EQ(loaded.plotTitleStyle(), PlotTitleStyle::factory());
  EXPECT_TRUE(loaded.isOpenGLCanvasEnabled());
}

TEST(MultiplotConfig, loadUsesUserDefaultsWhenPreferenceKeysMissing) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const QString path = settingsPath(dir, "user-defaults.xml");
  const QString prefsPath = dir.filePath(QStringLiteral("preferences.ini"));
  UserPreferences::setTestSettingsFile(prefsPath);
  UserPreferences prefs;
  prefs.timeZoneId = QStringLiteral("utc");
  prefs.themeId = QStringLiteral("dark");
  prefs.openGLCanvasEnabled = true;
  prefs.save();

  {
    MultiplotConfig config(nullptr);
    QSettings settings(path, XmlSettings::format);
    beginMultiplot(settings);
    config.save(settings);
    settings.endGroup();
    settings.sync();
  }

  MultiplotConfig loaded(nullptr);
  QSettings settings(path, XmlSettings::format);
  beginMultiplot(settings);
  loaded.load(settings);
  settings.endGroup();

  EXPECT_FALSE(loaded.isPreferencesOverridden());
  EXPECT_EQ(loaded.getTimeZoneId(), QStringLiteral("utc"));
  EXPECT_EQ(loaded.getThemeId(), QStringLiteral("dark"));
  EXPECT_TRUE(loaded.isOpenGLCanvasEnabled());
  UserPreferences::clearTestSettingsFile();
}

TEST(MultiplotConfig, sessionPreferenceChangeDoesNotAffectSnapshotWhenNotOverridden) {
  MultiplotConfig config(nullptr);
  const QByteArray original = snapshotOf(config);

  config.setTimeZoneId(QStringLiteral("utc"));
  config.setOpenGLCanvasEnabled(true);
  EXPECT_EQ(snapshotOf(config), original);

  config.setPreferencesOverridden(true);
  EXPECT_NE(snapshotOf(config), original);
}

TEST(MultiplotConfig, saveOmitsPreferenceKeysAfterClearingOverride) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const QString path = settingsPath(dir, "clear-override.xml");

  {
    MultiplotConfig config(nullptr);
    config.setPreferencesOverridden(true);
    config.setThemeId(QStringLiteral("dark"));

    QSettings settings(path, XmlSettings::format);
    beginMultiplot(settings);
    config.save(settings);
    ASSERT_TRUE(settings.contains(QStringLiteral("theme")));
    settings.endGroup();
    settings.sync();
  }

  MultiplotConfig loaded(nullptr);
  QSettings settings(path, XmlSettings::format);
  beginMultiplot(settings);
  loaded.load(settings);
  settings.endGroup();
  ASSERT_TRUE(loaded.isPreferencesOverridden());

  loaded.setPreferencesOverridden(false);
  loaded.applyUserDefaults();

  QSettings saved(path, XmlSettings::format);
  beginMultiplot(saved);
  loaded.save(saved);
  saved.endGroup();
  saved.sync();

  QSettings verify(path, XmlSettings::format);
  beginMultiplot(verify);
  EXPECT_FALSE(verify.contains(QStringLiteral("time_zone")));
  EXPECT_FALSE(verify.contains(QStringLiteral("theme")));
  EXPECT_FALSE(verify.contains(QStringLiteral("opengl_canvas")));
  EXPECT_FALSE(verify.contains(QStringLiteral("plot_title_font_size")));
  EXPECT_FALSE(verify.contains(QStringLiteral("plot_title_bold")));
  EXPECT_FALSE(verify.contains(QStringLiteral("plot_title_auto_color")));
  EXPECT_FALSE(verify.contains(QStringLiteral("plot_title_color")));
  verify.endGroup();
}

TEST(MultiplotConfig, resetAppliesUserDefaults) {
  const QString prefsPath = QDir::tempPath() + QStringLiteral("/rqt_multiplot_reset_prefs.ini");
  UserPreferences::setTestSettingsFile(prefsPath);
  UserPreferences prefs;
  prefs.timeZoneId = QStringLiteral("utc");
  prefs.themeId = QStringLiteral("dark");
  prefs.openGLCanvasEnabled = true;
  prefs.save();

  MultiplotConfig config(nullptr);
  config.setPreferencesOverridden(true);
  config.setThemeId(QStringLiteral("light"));
  config.reset();

  EXPECT_FALSE(config.isPreferencesOverridden());
  EXPECT_EQ(config.getTimeZoneId(), QStringLiteral("utc"));
  EXPECT_EQ(config.getThemeId(), QStringLiteral("dark"));
  EXPECT_TRUE(config.isOpenGLCanvasEnabled());
  UserPreferences::clearTestSettingsFile();
}

TEST(MultiplotConfig, prefersTabsWhenBothGroupsExist) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const QString path = settingsPath(dir, "both.xml");

  {
    MultiplotConfig config(nullptr);
    config.getTableConfig(0)->setTitle("FromTabs");

    QSettings settings(path, XmlSettings::format);
    beginMultiplot(settings);
    config.save(settings);
    settings.beginGroup("table");
    PlotTableConfig legacy(nullptr);
    legacy.setTitle("FromTable");
    legacy.save(settings);
    settings.endGroup();
    settings.endGroup();
    settings.sync();
  }

  MultiplotConfig loaded(nullptr);
  QSettings settings(path, XmlSettings::format);
  beginMultiplot(settings);
  loaded.load(settings);
  settings.endGroup();

  ASSERT_EQ(loaded.getNumTabs(), 1u);
  EXPECT_EQ(loaded.getTableConfig(0)->getTitle(), QString("FromTabs"));
}

struct RegisterIsolatedUserPreferences {
  RegisterIsolatedUserPreferences() { ::testing::AddGlobalTestEnvironment(new IsolatedUserPreferencesEnvironment()); }
};

const RegisterIsolatedUserPreferences kRegisterIsolatedUserPreferences;

}  // namespace
