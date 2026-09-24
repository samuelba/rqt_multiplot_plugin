#include <QBuffer>
#include <QDataStream>
#include <QIODevice>
#include <QSettings>
#include <QTemporaryDir>

#include <gtest/gtest.h>

#include "rqt_multiplot/CurveAxisConfig.hpp"
#include "rqt_multiplot/CurveConfig.hpp"
#include "rqt_multiplot/PlotAxesConfig.hpp"
#include "rqt_multiplot/PlotAxisConfig.hpp"
#include "rqt_multiplot/PlotConfig.hpp"
#include "rqt_multiplot/XmlSettings.hpp"

namespace {

using rqt_multiplot::CurveAxisConfig;
using rqt_multiplot::CurveConfig;
using rqt_multiplot::PlotAxesConfig;
using rqt_multiplot::PlotAxisConfig;
using rqt_multiplot::PlotConfig;

QString settingsPath(const QTemporaryDir& dir, const char* name) {
  return dir.filePath(QString::fromUtf8(name));
}

void configureReceiptTimeX(CurveConfig* config) {
  config->getAxisConfig(CurveConfig::X)->setFieldType(CurveAxisConfig::MessageReceiptTime);
  config->getAxisConfig(CurveConfig::Y)->setField("linear/x");
}

void configureStampX(CurveConfig* config) {
  config->getAxisConfig(CurveConfig::X)->setField("header/stamp");
  config->getAxisConfig(CurveConfig::Y)->setField("linear/x");
}

TEST(PlotConfig, removeCurveDropsCountAndKeepsRemainingTitle) {
  PlotConfig config;
  auto* first = config.addCurve();
  first->setTitle("pannant");
  auto* second = config.addCurve();
  second->setTitle("tiltent");

  config.removeCurve(first);

  ASSERT_EQ(config.getNumCurves(), 1u);
  ASSERT_NE(config.getCurveConfig(0), nullptr);
  EXPECT_EQ(config.getCurveConfig(0)->getTitle(), QString("tiltent"));
}

TEST(PlotConfig, assignmentAfterRemoveMatchesDialogOk) {
  PlotConfig dialogConfig;
  dialogConfig.addCurve()->setTitle("pannant");
  dialogConfig.addCurve()->setTitle("tiltent");
  dialogConfig.removeCurve(static_cast<size_t>(0));

  PlotConfig liveConfig;
  liveConfig.addCurve()->setTitle("pannant");
  liveConfig.addCurve()->setTitle("tiltent");

  liveConfig = dialogConfig;

  ASSERT_EQ(liveConfig.getNumCurves(), 1u);
  ASSERT_NE(liveConfig.getCurveConfig(0), nullptr);
  EXPECT_EQ(liveConfig.getCurveConfig(0)->getTitle(), QString("tiltent"));
}

TEST(PlotConfig, timeWindowDefaultsToDisabledTenSeconds) {
  PlotConfig config;

  EXPECT_FALSE(config.isTimeWindowEnabled());
  EXPECT_EQ(config.getTimeWindowLength(), 10);
}

TEST(PlotConfig, canApplyTimeWindowRequiresAllCurvesWithTimeX) {
  PlotConfig empty;
  EXPECT_FALSE(empty.canApplyTimeWindow());

  PlotConfig receipt;
  configureReceiptTimeX(receipt.addCurve());
  EXPECT_TRUE(receipt.canApplyTimeWindow());

  PlotConfig mixed;
  configureReceiptTimeX(mixed.addCurve());
  mixed.addCurve()->getAxisConfig(CurveConfig::X)->setField("linear/x");
  EXPECT_FALSE(mixed.canApplyTimeWindow());

  PlotConfig stamp;
  configureStampX(stamp.addCurve());
  EXPECT_TRUE(stamp.canApplyTimeWindow());
}

TEST(PlotConfig, xmlRoundTripKeepsCurveOrderPastTen) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const QString path = settingsPath(dir, "curves.xml");

  {
    PlotConfig config;
    for (int index = 0; index < 12; ++index) {
      config.addCurve()->setTitle(QString("curve-%1").arg(index));
    }

    QSettings settings(path, rqt_multiplot::XmlSettings::format);
    settings.beginGroup("rqt_multiplot");
    config.save(settings);
    settings.endGroup();
    settings.sync();
    ASSERT_EQ(settings.status(), QSettings::NoError);
  }

  PlotConfig loaded;
  QSettings settings(path, rqt_multiplot::XmlSettings::format);
  settings.beginGroup("rqt_multiplot");
  loaded.load(settings);
  settings.endGroup();

  ASSERT_EQ(loaded.getNumCurves(), 12u);
  for (int index = 0; index < 12; ++index) {
    ASSERT_NE(loaded.getCurveConfig(static_cast<size_t>(index)), nullptr);
    EXPECT_EQ(loaded.getCurveConfig(static_cast<size_t>(index))->getTitle(), QString("curve-%1").arg(index));
  }
}

TEST(PlotConfig, savesAndLoadsTimeWindow) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  {
    PlotConfig config;
    config.setTimeWindowEnabled(true);
    config.setTimeWindowLength(30);

    QSettings settings(settingsPath(dir, "plot.ini"), QSettings::IniFormat);
    config.save(settings);
    settings.sync();
  }

  PlotConfig loaded;
  QSettings settings(settingsPath(dir, "plot.ini"), QSettings::IniFormat);
  loaded.load(settings);

  EXPECT_TRUE(loaded.isTimeWindowEnabled());
  EXPECT_EQ(loaded.getTimeWindowLength(), 30);
}

TEST(PlotConfig, writesAndReadsAxesAndLegend) {
  PlotConfig source;
  source.setTitle("Range");
  source.getAxesConfig()->getAxisConfig(PlotAxesConfig::X)->setTitleType(PlotAxisConfig::CustomTitle);
  source.getAxesConfig()->getAxisConfig(PlotAxesConfig::X)->setCustomTitle("Time");
  source.getAxesConfig()->getAxisConfig(PlotAxesConfig::Y)->setTitleType(PlotAxisConfig::CustomTitle);
  source.getAxesConfig()->getAxisConfig(PlotAxesConfig::Y)->setCustomTitle("Altitude");
  source.getAxesConfig()->getAxisConfig(PlotAxesConfig::Y)->setTitleVisible(false);
  source.getLegendConfig()->setVisible(false);
  source.setPlotRate(12.5);
  source.setTimeWindowEnabled(true);
  source.setTimeWindowLength(30);

  QBuffer buffer;
  buffer.open(QIODevice::ReadWrite);
  QDataStream out(&buffer);
  source.write(out);

  buffer.seek(0);
  QDataStream in(&buffer);
  PlotConfig loaded;
  loaded.read(in);

  EXPECT_EQ(loaded.getTitle(), QString("Range"));
  EXPECT_EQ(loaded.getAxesConfig()->getAxisConfig(PlotAxesConfig::X)->getTitleType(), PlotAxisConfig::CustomTitle);
  EXPECT_EQ(loaded.getAxesConfig()->getAxisConfig(PlotAxesConfig::X)->getCustomTitle(), QString("Time"));
  EXPECT_EQ(loaded.getAxesConfig()->getAxisConfig(PlotAxesConfig::Y)->getTitleType(), PlotAxisConfig::CustomTitle);
  EXPECT_EQ(loaded.getAxesConfig()->getAxisConfig(PlotAxesConfig::Y)->getCustomTitle(), QString("Altitude"));
  EXPECT_FALSE(loaded.getAxesConfig()->getAxisConfig(PlotAxesConfig::Y)->isTitleVisible());
  EXPECT_FALSE(loaded.getLegendConfig()->isVisible());
  EXPECT_DOUBLE_EQ(loaded.getPlotRate(), 12.5);
  EXPECT_TRUE(loaded.isTimeWindowEnabled());
  EXPECT_EQ(loaded.getTimeWindowLength(), 30);
}

TEST(PlotConfig, assignmentCopiesTimeWindow) {
  PlotConfig source;
  source.setTimeWindowEnabled(true);
  source.setTimeWindowLength(45);

  PlotConfig target;
  target = source;

  EXPECT_TRUE(target.isTimeWindowEnabled());
  EXPECT_EQ(target.getTimeWindowLength(), 45);
}

TEST(PlotConfig, removeCurveEmitsCurveRemoved) {
  PlotConfig config;
  config.addCurve();
  config.addCurve();

  size_t removedIndex = 99;
  QObject::connect(&config, &PlotConfig::curveRemoved, [&removedIndex](size_t index) { removedIndex = index; });

  config.removeCurve(static_cast<size_t>(0));

  EXPECT_EQ(removedIndex, 0u);
  EXPECT_EQ(config.getNumCurves(), 1u);
}

}  // namespace
