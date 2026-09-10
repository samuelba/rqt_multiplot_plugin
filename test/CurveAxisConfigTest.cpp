#include <QBuffer>
#include <QDataStream>
#include <QSettings>
#include <QTemporaryDir>

#include <gtest/gtest.h>

#include <rqt_multiplot/CurveAxisConfig.h>

namespace {

using rqt_multiplot::CurveAxisConfig;

QString settingsPath(const QTemporaryDir& dir, const char* name) {
  return dir.filePath(QString::fromUtf8(name));
}

TEST(CurveAxisConfig, savesAndLoadsLabelFromZero) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  {
    CurveAxisConfig config;
    config.setFieldType(CurveAxisConfig::MessageData);
    config.setLabelFromZero(true);

    QSettings settings(settingsPath(dir, "label.ini"), QSettings::IniFormat);
    config.save(settings);
    settings.sync();
  }

  CurveAxisConfig loaded;
  QSettings settings(settingsPath(dir, "label.ini"), QSettings::IniFormat);
  loaded.load(settings);

  EXPECT_TRUE(loaded.isLabelFromZero());
}

TEST(CurveAxisConfig, missingKeyDefaultsTrueForReceiptTime) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  QSettings settings(settingsPath(dir, "receipt.ini"), QSettings::IniFormat);
  settings.setValue("field_type", static_cast<int>(CurveAxisConfig::MessageReceiptTime));
  settings.sync();

  CurveAxisConfig config;
  config.load(settings);

  EXPECT_EQ(config.getFieldType(), CurveAxisConfig::MessageReceiptTime);
  EXPECT_TRUE(config.isLabelFromZero());
}

TEST(CurveAxisConfig, missingKeyDefaultsFalseForMessageField) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  QSettings settings(settingsPath(dir, "field.ini"), QSettings::IniFormat);
  settings.setValue("field_type", static_cast<int>(CurveAxisConfig::MessageData));
  settings.setValue("field", "header/stamp");
  settings.sync();

  CurveAxisConfig config;
  config.load(settings);

  EXPECT_EQ(config.getFieldType(), CurveAxisConfig::MessageData);
  EXPECT_FALSE(config.isLabelFromZero());
}

TEST(CurveAxisConfig, writesAndReadsLabelFromZero) {
  CurveAxisConfig source;
  source.setLabelFromZero(true);

  QBuffer buffer;
  buffer.open(QIODevice::ReadWrite);
  QDataStream stream(&buffer);
  source.write(stream);

  buffer.seek(0);
  CurveAxisConfig loaded;
  loaded.read(stream);

  EXPECT_TRUE(loaded.isLabelFromZero());
}

}  // namespace
