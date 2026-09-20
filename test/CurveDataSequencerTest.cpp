#include <cmath>

#include <QVector>

#include <gtest/gtest.h>
#include <ros_babel_fish/messages/array_message.hpp>

#include <rqt_multiplot/CurveConfig.h>
#include <rqt_multiplot/CurveDataSequencer.h>
#include <rqt_multiplot/Message.h>
#include <rqt_multiplot/MessageFieldAccess.h>

namespace {

using rqt_multiplot::createMessagePrototype;
using rqt_multiplot::CurveAxisConfig;
using rqt_multiplot::CurveConfig;
using rqt_multiplot::CurveDataSequencer;
using rqt_multiplot::Message;

Message jointStateMessage() {
  auto compound = createMessagePrototype("sensor_msgs/msg/JointState");
  auto& position = (*compound)["position"].as<ros_babel_fish::ArrayMessage<double>>();
  position.push_back(1.25);
  position.push_back(-0.5);
  position.push_back(3.0);

  Message message;
  message.setCompound(compound);
  return message;
}

Message poseArrayMessage() {
  auto compound = createMessagePrototype("geometry_msgs/msg/PoseArray");
  auto& poses = (*compound)["poses"].as<ros_babel_fish::CompoundArrayMessage>();
  auto& first = poses.appendEmpty();
  first["position"]["x"] = 1.0;
  first["position"]["y"] = 2.0;
  auto& second = poses.appendEmpty();
  second["position"]["x"] = 3.0;
  second["position"]["y"] = 4.0;

  Message message;
  message.setCompound(compound);
  return message;
}

TEST(CurveDataSequencer, buildsIndexVersusArraySeries) {
  CurveConfig config;
  config.getAxisConfig(CurveConfig::X)->setFieldType(CurveAxisConfig::ArrayIndex);
  config.getAxisConfig(CurveConfig::Y)->setField("position/*");

  QVector<QPointF> points;
  ASSERT_TRUE(CurveDataSequencer::tryBuildSnapshotSeries(jointStateMessage(), config, points));
  ASSERT_EQ(points.size(), 3);
  EXPECT_DOUBLE_EQ(points[0].x(), 0.0);
  EXPECT_DOUBLE_EQ(points[0].y(), 1.25);
  EXPECT_DOUBLE_EQ(points[1].x(), 1.0);
  EXPECT_DOUBLE_EQ(points[1].y(), -0.5);
  EXPECT_DOUBLE_EQ(points[2].x(), 2.0);
  EXPECT_DOUBLE_EQ(points[2].y(), 3.0);
}

TEST(CurveDataSequencer, zipsMatchingWildcardFields) {
  CurveConfig config;
  config.getAxisConfig(CurveConfig::X)->setField("poses/*/position/x");
  config.getAxisConfig(CurveConfig::Y)->setField("poses/*/position/y");

  QVector<QPointF> points;
  ASSERT_TRUE(CurveDataSequencer::tryBuildSnapshotSeries(poseArrayMessage(), config, points));
  ASSERT_EQ(points.size(), 2);
  EXPECT_DOUBLE_EQ(points[0].x(), 1.0);
  EXPECT_DOUBLE_EQ(points[0].y(), 2.0);
  EXPECT_DOUBLE_EQ(points[1].x(), 3.0);
  EXPECT_DOUBLE_EQ(points[1].y(), 4.0);
}

TEST(CurveDataSequencer, detectsValidSnapshotConfig) {
  CurveConfig config;
  config.getAxisConfig(CurveConfig::X)->setTopic("/array");
  config.getAxisConfig(CurveConfig::Y)->setTopic("/array");
  config.getAxisConfig(CurveConfig::X)->setFieldType(CurveAxisConfig::ArrayIndex);
  config.getAxisConfig(CurveConfig::Y)->setField("position/*");

  EXPECT_TRUE(CurveDataSequencer::isSnapshotConfig(config));
}

TEST(CurveDataSequencer, rejectsMixedReceiptTimeAndWildcard) {
  CurveConfig config;
  config.getAxisConfig(CurveConfig::X)->setTopic("/array");
  config.getAxisConfig(CurveConfig::Y)->setTopic("/array");
  config.getAxisConfig(CurveConfig::X)->setFieldType(CurveAxisConfig::MessageReceiptTime);
  config.getAxisConfig(CurveConfig::Y)->setField("position/*");

  EXPECT_FALSE(CurveDataSequencer::isSnapshotConfig(config));
  EXPECT_TRUE(CurveDataSequencer::hasSnapshotHint(config));
}

TEST(CurveDataSequencer, receiptTimeIgnoresLeftoverWildcardField) {
  CurveConfig config;
  config.getAxisConfig(CurveConfig::X)->setTopic("/array");
  config.getAxisConfig(CurveConfig::Y)->setTopic("/array");
  config.getAxisConfig(CurveConfig::X)->setField("position/*");
  config.getAxisConfig(CurveConfig::X)->setFieldType(CurveAxisConfig::MessageReceiptTime);
  config.getAxisConfig(CurveConfig::Y)->setField("position/0");

  EXPECT_FALSE(CurveDataSequencer::hasSnapshotHint(config));
  EXPECT_FALSE(CurveDataSequencer::isSnapshotConfig(config));
}

TEST(CurveDataSequencer, rejectsScalarAndWildcardMix) {
  CurveConfig config;
  config.getAxisConfig(CurveConfig::X)->setTopic("/array");
  config.getAxisConfig(CurveConfig::Y)->setTopic("/array");
  config.getAxisConfig(CurveConfig::X)->setField("position/0");
  config.getAxisConfig(CurveConfig::Y)->setField("position/*");

  EXPECT_FALSE(CurveDataSequencer::isSnapshotConfig(config));
  EXPECT_TRUE(CurveDataSequencer::hasSnapshotHint(config));
}

TEST(CurveDataSequencer, rejectsDifferentTopicsForSnapshot) {
  CurveConfig config;
  config.getAxisConfig(CurveConfig::X)->setTopic("/left");
  config.getAxisConfig(CurveConfig::Y)->setTopic("/right");
  config.getAxisConfig(CurveConfig::X)->setFieldType(CurveAxisConfig::ArrayIndex);
  config.getAxisConfig(CurveConfig::Y)->setField("position/*");

  EXPECT_FALSE(CurveDataSequencer::isSnapshotConfig(config));
  EXPECT_FALSE(CurveDataSequencer::snapshotIncompatibilityReason(config).isEmpty());
}

TEST(CurveDataSequencer, validSnapshotHasNoIncompatibilityReason) {
  CurveConfig config;
  config.getAxisConfig(CurveConfig::X)->setTopic("/array");
  config.getAxisConfig(CurveConfig::Y)->setTopic("/array");
  config.getAxisConfig(CurveConfig::X)->setFieldType(CurveAxisConfig::ArrayIndex);
  config.getAxisConfig(CurveConfig::Y)->setField("position/*");

  EXPECT_TRUE(CurveDataSequencer::snapshotIncompatibilityReason(config).isEmpty());
}

TEST(CurveDataSequencer, arrayIndexWithScalarIsIncompatible) {
  CurveConfig config;
  config.getAxisConfig(CurveConfig::X)->setTopic("/array");
  config.getAxisConfig(CurveConfig::Y)->setTopic("/array");
  config.getAxisConfig(CurveConfig::X)->setFieldType(CurveAxisConfig::ArrayIndex);
  config.getAxisConfig(CurveConfig::Y)->setField("position/0");

  EXPECT_TRUE(CurveDataSequencer::hasSnapshotHint(config));
  EXPECT_FALSE(CurveDataSequencer::isSnapshotConfig(config));
  EXPECT_FALSE(CurveDataSequencer::snapshotIncompatibilityReason(config).isEmpty());

  QVector<QPointF> points;
  EXPECT_FALSE(CurveDataSequencer::tryBuildSnapshotSeries(jointStateMessage(), config, points));
}

TEST(CurveDataSequencer, timeSeriesHasNoIncompatibilityReason) {
  CurveConfig config;
  config.getAxisConfig(CurveConfig::X)->setTopic("/array");
  config.getAxisConfig(CurveConfig::Y)->setTopic("/array");
  config.getAxisConfig(CurveConfig::X)->setFieldType(CurveAxisConfig::MessageReceiptTime);
  config.getAxisConfig(CurveConfig::Y)->setField("position/0");

  EXPECT_TRUE(CurveDataSequencer::snapshotIncompatibilityReason(config).isEmpty());
}

TEST(CurveDataSequencer, appliesRadiansToDegreesOnSnapshotYAxis) {
  CurveConfig config;
  config.getAxisConfig(CurveConfig::X)->setFieldType(CurveAxisConfig::ArrayIndex);
  config.getAxisConfig(CurveConfig::Y)->setField("position/*");
  config.getAxisConfig(CurveConfig::Y)->setUnitConversion(CurveAxisConfig::RadiansToDegrees);

  QVector<QPointF> points;
  ASSERT_TRUE(CurveDataSequencer::tryBuildSnapshotSeries(jointStateMessage(), config, points));
  ASSERT_EQ(points.size(), 3);
  EXPECT_DOUBLE_EQ(points[0].x(), 0.0);
  EXPECT_NEAR(points[0].y(), 1.25 * 180.0 / M_PI, 1e-9);
  EXPECT_DOUBLE_EQ(points[1].x(), 1.0);
  EXPECT_NEAR(points[1].y(), -0.5 * 180.0 / M_PI, 1e-9);
  EXPECT_DOUBLE_EQ(points[2].x(), 2.0);
  EXPECT_NEAR(points[2].y(), 3.0 * 180.0 / M_PI, 1e-9);
}

TEST(CurveDataSequencer, arrayIndexAxisIsNotConverted) {
  CurveConfig config;
  config.getAxisConfig(CurveConfig::X)->setFieldType(CurveAxisConfig::ArrayIndex);
  config.getAxisConfig(CurveConfig::X)->setUnitConversion(CurveAxisConfig::RadiansToDegrees);
  config.getAxisConfig(CurveConfig::Y)->setField("position/*");

  QVector<QPointF> points;
  ASSERT_TRUE(CurveDataSequencer::tryBuildSnapshotSeries(jointStateMessage(), config, points));
  ASSERT_EQ(points.size(), 3);
  EXPECT_DOUBLE_EQ(points[0].x(), 0.0);
  EXPECT_DOUBLE_EQ(points[1].x(), 1.0);
  EXPECT_DOUBLE_EQ(points[2].x(), 2.0);
}

}  // namespace
