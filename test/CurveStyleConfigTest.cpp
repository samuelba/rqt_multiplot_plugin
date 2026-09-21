#include <QBuffer>
#include <QDataStream>
#include <QIODevice>
#include <QSettings>
#include <QTemporaryDir>

#include <gtest/gtest.h>

#include "rqt_multiplot/CurveDataConfig.hpp"
#include "rqt_multiplot/CurveStyleConfig.hpp"

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

TEST(CurveStyleConfig, writesAndReadsFadeHistory) {
  CurveStyleConfig source;
  source.setType(CurveStyleConfig::Sticks);
  source.setFadeHistory(8);

  QBuffer buffer;
  buffer.open(QIODevice::ReadWrite);
  QDataStream stream(&buffer);
  source.write(stream);

  buffer.seek(0);
  CurveStyleConfig loaded;
  loaded.read(stream);

  EXPECT_EQ(loaded.getType(), CurveStyleConfig::Sticks);
  EXPECT_EQ(loaded.getFadeHistory(), 8u);
}

TEST(CurveStyleConfig, legacyStreamLeavesFollowingDataConfigIntact) {
  QBuffer buffer;
  buffer.open(QIODevice::ReadWrite);
  QDataStream out(&buffer);
  out << static_cast<int>(CurveStyleConfig::Lines);
  out << false;
  out << static_cast<int>(Qt::Vertical);
  out << 0.0;
  out << false;
  out << static_cast<quint64>(1);
  out << static_cast<int>(Qt::SolidLine);
  out << false;
  out << static_cast<int>(rqt_multiplot::CurveDataConfig::CircularBuffer);
  out << static_cast<quint64>(123);
  out << static_cast<qreal>(4.5);

  buffer.seek(0);
  QDataStream in(&buffer);

  CurveStyleConfig style;
  style.setFadeHistory(9);
  style.read(in);

  rqt_multiplot::CurveDataConfig data;
  data.read(in);

  EXPECT_EQ(style.getType(), CurveStyleConfig::Lines);
  EXPECT_EQ(style.getFadeHistory(), 0u);
  EXPECT_EQ(data.getType(), rqt_multiplot::CurveDataConfig::CircularBuffer);
  EXPECT_EQ(data.getCircularBufferCapacity(), 123u);
  EXPECT_DOUBLE_EQ(data.getTimeFrameLength(), 4.5);
}

}  // namespace
