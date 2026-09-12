#include <QSettings>
#include <QTemporaryDir>

#include <gtest/gtest.h>

#include <rqt_multiplot/CurveStyleConfig.h>

namespace {

using rqt_multiplot::CurveStyleConfig;

TEST(CurveStyleConfig, savesAndLoadsFadeHistory) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  {
    CurveStyleConfig config;
    config.setFadeHistory(8);

    QSettings settings(dir.filePath("style.ini"), QSettings::IniFormat);
    config.save(settings);
    settings.sync();
  }

  CurveStyleConfig loaded;
  QSettings settings(dir.filePath("style.ini"), QSettings::IniFormat);
  loaded.load(settings);

  EXPECT_EQ(loaded.getFadeHistory(), 8u);
}

TEST(CurveStyleConfig, missingFadeHistoryDefaultsToOff) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  QSettings settings(dir.filePath("old.ini"), QSettings::IniFormat);
  settings.setValue("type", static_cast<int>(CurveStyleConfig::Lines));
  settings.sync();

  CurveStyleConfig config;
  config.setFadeHistory(4);
  config.load(settings);

  EXPECT_EQ(config.getFadeHistory(), 0u);
}

}  // namespace
