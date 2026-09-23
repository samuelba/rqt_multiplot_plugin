#include <cmath>

#include <QBuffer>
#include <QDataStream>
#include <QSettings>
#include <QTemporaryDir>

#include <gtest/gtest.h>

#include "rqt_multiplot/CurveAxisConfig.hpp"

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

TEST(CurveAxisConfig, usesTimeScaleForReceiptTimeAndStampField) {
  CurveAxisConfig receiptTime;
  receiptTime.setFieldType(CurveAxisConfig::MessageReceiptTime);
  EXPECT_TRUE(receiptTime.usesTimeScale());

  CurveAxisConfig stamp;
  stamp.setField("header/stamp");
  EXPECT_TRUE(stamp.usesTimeScale());
  EXPECT_TRUE(CurveAxisConfig::isTimeFieldPath("stamp"));
  EXPECT_FALSE(CurveAxisConfig::isTimeFieldPath("linear/x"));
}

TEST(CurveAxisConfig, usesTimeScaleWhenLabelFromZero) {
  CurveAxisConfig config;
  config.setField("linear/x");
  EXPECT_FALSE(config.usesTimeScale());

  config.setLabelFromZero(true);
  EXPECT_TRUE(config.usesTimeScale());
}

TEST(CurveAxisConfig, arrayIndexIgnoresStaleTimeField) {
  CurveAxisConfig config;
  config.setField("header/stamp");
  config.setLabelFromZero(true);
  config.setFieldType(CurveAxisConfig::ArrayIndex);

  EXPECT_FALSE(config.usesTimeScale());
}

TEST(CurveAxisConfig, receiptTimeUsesTimeScaleWithLeftoverField) {
  CurveAxisConfig config;
  config.setField("position/*");
  config.setFieldType(CurveAxisConfig::MessageReceiptTime);

  EXPECT_TRUE(config.usesTimeScale());
}

TEST(CurveAxisConfig, savesAndLoadsArrayIndex) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  {
    CurveAxisConfig config;
    config.setFieldType(CurveAxisConfig::ArrayIndex);
    config.setField(QString());

    QSettings settings(settingsPath(dir, "array.ini"), QSettings::IniFormat);
    config.save(settings);
    settings.sync();
  }

  CurveAxisConfig loaded;
  QSettings settings(settingsPath(dir, "array.ini"), QSettings::IniFormat);
  loaded.load(settings);

  EXPECT_EQ(loaded.getFieldType(), CurveAxisConfig::ArrayIndex);
  EXPECT_TRUE(loaded.getField().isEmpty());
  EXPECT_EQ(loaded.getFieldLabel(), QString("index"));
}

TEST(CurveAxisConfig, unknownFieldTypeDefaultsToMessageData) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  QSettings settings(settingsPath(dir, "unknown.ini"), QSettings::IniFormat);
  settings.setValue("field_type", 99);
  settings.sync();

  CurveAxisConfig config;
  config.load(settings);

  EXPECT_EQ(config.getFieldType(), CurveAxisConfig::MessageData);
}

TEST(CurveAxisConfig, arrayIndexHasConfiguredSource) {
  CurveAxisConfig config;
  EXPECT_FALSE(config.hasConfiguredSource());

  config.setFieldType(CurveAxisConfig::ArrayIndex);
  EXPECT_TRUE(config.hasConfiguredSource());
  EXPECT_EQ(config.getFieldLabel(), QString("index"));
}

TEST(CurveAxisConfig, writesAndReadsArrayIndex) {
  CurveAxisConfig source;
  source.setFieldType(CurveAxisConfig::ArrayIndex);

  QBuffer buffer;
  buffer.open(QIODevice::ReadWrite);
  QDataStream stream(&buffer);
  source.write(stream);

  buffer.seek(0);
  CurveAxisConfig loaded;
  loaded.read(stream);

  EXPECT_EQ(loaded.getFieldType(), CurveAxisConfig::ArrayIndex);
}

TEST(CurveAxisConfig, isTimeSourceForReceiptTimeAndStampField) {
  CurveAxisConfig receiptTime;
  receiptTime.setFieldType(CurveAxisConfig::MessageReceiptTime);
  EXPECT_TRUE(receiptTime.isTimeSource());

  CurveAxisConfig stamp;
  stamp.setField("header/stamp");
  EXPECT_TRUE(stamp.isTimeSource());
}

TEST(CurveAxisConfig, isTimeSourceFalseForNonTimeField) {
  CurveAxisConfig numeric;
  numeric.setField("linear/x");
  EXPECT_FALSE(numeric.isTimeSource());

  numeric.setLabelFromZero(true);
  EXPECT_FALSE(numeric.isTimeSource());
}

TEST(CurveAxisConfig, isTimeSourceFalseForArrayIndexAndEmptyField) {
  CurveAxisConfig arrayIndex;
  arrayIndex.setFieldType(CurveAxisConfig::ArrayIndex);
  EXPECT_FALSE(arrayIndex.isTimeSource());

  CurveAxisConfig empty;
  EXPECT_FALSE(empty.isTimeSource());
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

TEST(CurveAxisConfig, convertValueRadiansToDegrees) {
  CurveAxisConfig config;
  config.setUnitConversion(CurveAxisConfig::RadiansToDegrees);

  EXPECT_NEAR(config.convertValue(M_PI), 180.0, 1e-9);
  EXPECT_DOUBLE_EQ(config.convertValue(0.0), 0.0);
}

TEST(CurveAxisConfig, convertValueDegreesToRadians) {
  CurveAxisConfig config;
  config.setUnitConversion(CurveAxisConfig::DegreesToRadians);

  EXPECT_NEAR(config.convertValue(180.0), M_PI, 1e-9);
  EXPECT_DOUBLE_EQ(config.convertValue(0.0), 0.0);
}

TEST(CurveAxisConfig, convertValueNoneReturnsInput) {
  CurveAxisConfig config;
  config.setField("linear/x");

  EXPECT_DOUBLE_EQ(config.convertValue(1.25), 1.25);
}

TEST(CurveAxisConfig, convertValueSkipsTimeSources) {
  CurveAxisConfig receiptTime;
  receiptTime.setFieldType(CurveAxisConfig::MessageReceiptTime);
  receiptTime.setUnitConversion(CurveAxisConfig::RadiansToDegrees);
  EXPECT_DOUBLE_EQ(receiptTime.convertValue(1.25), 1.25);

  CurveAxisConfig stamp;
  stamp.setField("header/stamp");
  stamp.setUnitConversion(CurveAxisConfig::RadiansToDegrees);
  EXPECT_DOUBLE_EQ(stamp.convertValue(1.25), 1.25);

  CurveAxisConfig arrayIndex;
  arrayIndex.setFieldType(CurveAxisConfig::ArrayIndex);
  arrayIndex.setUnitConversion(CurveAxisConfig::RadiansToDegrees);
  EXPECT_DOUBLE_EQ(arrayIndex.convertValue(1.25), 1.25);
}

TEST(CurveAxisConfig, savesAndLoadsUnitConversion) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  {
    CurveAxisConfig config;
    config.setUnitConversion(CurveAxisConfig::RadiansToDegrees);

    QSettings settings(settingsPath(dir, "unit.ini"), QSettings::IniFormat);
    config.save(settings);
    settings.sync();
  }

  CurveAxisConfig loaded;
  QSettings settings(settingsPath(dir, "unit.ini"), QSettings::IniFormat);
  loaded.load(settings);

  EXPECT_EQ(loaded.getUnitConversion(), CurveAxisConfig::RadiansToDegrees);
}

TEST(CurveAxisConfig, missingUnitConversionDefaultsToNone) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  QSettings settings(settingsPath(dir, "missing.ini"), QSettings::IniFormat);
  settings.setValue("field", "linear/x");
  settings.sync();

  CurveAxisConfig config;
  config.load(settings);

  EXPECT_EQ(config.getUnitConversion(), CurveAxisConfig::None);
}

TEST(CurveAxisConfig, unknownUnitConversionDefaultsToNone) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  QSettings settings(settingsPath(dir, "unknown_unit.ini"), QSettings::IniFormat);
  settings.setValue("unit_conversion", 99);
  settings.sync();

  CurveAxisConfig config;
  config.load(settings);

  EXPECT_EQ(config.getUnitConversion(), CurveAxisConfig::None);
}

TEST(CurveAxisConfig, writesAndReadsUnitConversion) {
  CurveAxisConfig source;
  source.setUnitConversion(CurveAxisConfig::DegreesToRadians);

  QBuffer buffer;
  buffer.open(QIODevice::ReadWrite);
  QDataStream stream(&buffer);
  source.write(stream);

  buffer.seek(0);
  CurveAxisConfig loaded;
  loaded.read(stream);

  EXPECT_EQ(loaded.getUnitConversion(), CurveAxisConfig::DegreesToRadians);
}

TEST(CurveAxisConfig, resetClearsUnitConversion) {
  CurveAxisConfig config;
  config.setUnitConversion(CurveAxisConfig::RadiansToDegrees);

  config.reset();

  EXPECT_EQ(config.getUnitConversion(), CurveAxisConfig::None);
}

TEST(CurveAxisConfig, savesLoadsAndCopiesDiagnosticValue) {
  EXPECT_EQ(static_cast<int>(CurveAxisConfig::DiagnosticValue), 3);

  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  {
    CurveAxisConfig config;
    config.setFieldType(CurveAxisConfig::DiagnosticValue);
    config.setDiagnosticStatus("/Power System/Battery");
    config.setDiagnosticKey("Voltage");
    config.setDiagnosticHardwareId("pack-a");

    QSettings settings(settingsPath(dir, "diagnostic.ini"), QSettings::IniFormat);
    config.save(settings);
    settings.sync();
  }

  CurveAxisConfig loaded;
  QSettings settings(settingsPath(dir, "diagnostic.ini"), QSettings::IniFormat);
  loaded.load(settings);

  EXPECT_EQ(loaded.getFieldType(), CurveAxisConfig::DiagnosticValue);
  EXPECT_EQ(loaded.getDiagnosticStatus(), QString("/Power System/Battery"));
  EXPECT_EQ(loaded.getDiagnosticKey(), QString("Voltage"));
  EXPECT_EQ(loaded.getDiagnosticHardwareId(), QString("pack-a"));
  EXPECT_EQ(loaded.getFieldLabel(), QString("/Power System/Battery/Voltage [pack-a]"));
  EXPECT_TRUE(loaded.hasConfiguredSource());
  EXPECT_FALSE(loaded.usesTimeScale());
  EXPECT_FALSE(loaded.isTimeSource());

  QBuffer buffer;
  buffer.open(QIODevice::ReadWrite);
  QDataStream stream(&buffer);
  loaded.write(stream);
  buffer.seek(0);
  CurveAxisConfig streamed;
  streamed.read(stream);
  EXPECT_EQ(streamed.getFieldType(), CurveAxisConfig::DiagnosticValue);
  EXPECT_EQ(streamed.getDiagnosticStatus(), loaded.getDiagnosticStatus());
  EXPECT_EQ(streamed.getDiagnosticKey(), loaded.getDiagnosticKey());
  EXPECT_EQ(streamed.getDiagnosticHardwareId(), loaded.getDiagnosticHardwareId());

  CurveAxisConfig copy;
  copy = loaded;
  EXPECT_EQ(copy.getDiagnosticStatus(), loaded.getDiagnosticStatus());
  EXPECT_EQ(copy.getDiagnosticKey(), loaded.getDiagnosticKey());
  EXPECT_EQ(copy.getDiagnosticHardwareId(), loaded.getDiagnosticHardwareId());

  loaded.reset();
  EXPECT_EQ(loaded.getFieldType(), CurveAxisConfig::MessageData);
  EXPECT_TRUE(loaded.getDiagnosticStatus().isEmpty());
  EXPECT_TRUE(loaded.getDiagnosticKey().isEmpty());
  EXPECT_TRUE(loaded.getDiagnosticHardwareId().isEmpty());
  EXPECT_FALSE(loaded.hasConfiguredSource());
}

TEST(CurveAxisConfig, assignmentCopiesUnitConversion) {
  CurveAxisConfig source;
  source.setUnitConversion(CurveAxisConfig::RadiansToDegrees);

  CurveAxisConfig target;
  target = source;

  EXPECT_EQ(target.getUnitConversion(), CurveAxisConfig::RadiansToDegrees);
}

}  // namespace
