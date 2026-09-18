#include <QSettings>
#include <QTemporaryDir>

#include <gtest/gtest.h>

#include <rqt_multiplot/CurveAxisConfig.h>
#include <rqt_multiplot/CurveConfig.h>
#include <rqt_multiplot/PlotConfig.h>

namespace {

using rqt_multiplot::CurveAxisConfig;
using rqt_multiplot::CurveConfig;
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
