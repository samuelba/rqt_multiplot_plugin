#include <cmath>
#include <string>

#include <QMetaObject>
#include <QMetaType>
#include <QStringList>
#include <QVector>

#include <QtGlobal>

#include <gtest/gtest.h>
#include <ros_babel_fish/messages/array_message.hpp>

#include "rqt_multiplot/CurveConfig.hpp"
#include "rqt_multiplot/CurveDataSequencer.hpp"
#include "rqt_multiplot/Message.hpp"
#include "rqt_multiplot/MessageFieldAccess.hpp"

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

ros_babel_fish::CompoundMessage& appendDiagnosticStatus(ros_babel_fish::CompoundMessage& message, const std::string& name) {
  auto& status = message["status"].as<ros_babel_fish::CompoundArrayMessage>();
  auto& entry = status.appendEmpty();
  entry["name"] = name;
  return entry;
}

void appendDiagnosticValue(ros_babel_fish::CompoundMessage& status, const std::string& key, const std::string& value) {
  auto& values = status["values"].as<ros_babel_fish::CompoundArrayMessage>();
  auto& entry = values.appendEmpty();
  entry["key"] = key;
  entry["value"] = value;
}

Message diagnosticMessage(double stampSeconds, const std::string& name, const std::string& key, const std::string& value) {
  auto compound = createMessagePrototype("diagnostic_msgs/msg/DiagnosticArray");
  const auto seconds = static_cast<int32_t>(stampSeconds);
  const auto nanos = static_cast<uint32_t>((stampSeconds - static_cast<double>(seconds)) * 1e9);
  (*compound)["header"]["stamp"] = rclcpp::Time(seconds, nanos, RCL_ROS_TIME);
  auto& status = appendDiagnosticStatus(*compound, name);
  appendDiagnosticValue(status, key, value);

  Message message;
  message.setCompound(compound);
  message.setReceiptTime(rclcpp::Time(1000, 0, RCL_ROS_TIME));
  return message;
}

Message poseStampedMessage(double stampSeconds, double x) {
  auto compound = createMessagePrototype("geometry_msgs/msg/PoseStamped");
  const auto seconds = static_cast<int32_t>(stampSeconds);
  const auto nanos = static_cast<uint32_t>((stampSeconds - static_cast<double>(seconds)) * 1e9);
  (*compound)["header"]["stamp"] = rclcpp::Time(seconds, nanos, RCL_ROS_TIME);
  (*compound)["pose"]["position"]["x"] = x;

  Message message;
  message.setCompound(compound);
  message.setReceiptTime(rclcpp::Time(1000, 0, RCL_ROS_TIME));
  return message;
}

void configureDiagnosticCurve(CurveConfig& config) {
  config.getAxisConfig(CurveConfig::X)->setTopic("/diagnostics");
  config.getAxisConfig(CurveConfig::Y)->setTopic("/diagnostics");
  config.getAxisConfig(CurveConfig::X)->setType("diagnostic_msgs/msg/DiagnosticArray");
  config.getAxisConfig(CurveConfig::Y)->setType("diagnostic_msgs/msg/DiagnosticArray");
  config.getAxisConfig(CurveConfig::Y)->setFieldType(CurveAxisConfig::DiagnosticValue);
  config.getAxisConfig(CurveConfig::Y)->setDiagnosticStatus("/Power System/Battery");
  config.getAxisConfig(CurveConfig::Y)->setDiagnosticKey("Voltage");
}

QStringList diagnosticWarnings;

void diagnosticWarningHandler(QtMsgType type, const QMessageLogContext& /*context*/, const QString& text) {
  if (type == QtWarningMsg) {
    diagnosticWarnings.append(text);
  }
}

bool deliver(CurveDataSequencer& sequencer, const char* method, const Message& message) {
  qRegisterMetaType<Message>("Message");
  return QMetaObject::invokeMethod(&sequencer, method, Qt::DirectConnection, Q_ARG(QString, QStringLiteral("/diagnostics")),
                                   Q_ARG(Message, message));
}

TEST(CurveDataSequencer, diagnosticValueUsesParsedNumberNotReceiptTime) {
  CurveConfig config;
  configureDiagnosticCurve(config);
  config.getAxisConfig(CurveConfig::X)->setFieldType(CurveAxisConfig::MessageReceiptTime);

  CurveDataSequencer sequencer;
  sequencer.setConfig(&config);

  QPointF point;
  int count = 0;
  QObject::connect(&sequencer, &CurveDataSequencer::pointReceived, [&](const QPointF& received) {
    point = received;
    ++count;
  });

  auto message = diagnosticMessage(7.5, "/Power System/Battery", "Voltage", "12.5");
  message.setReceiptTime(rclcpp::Time(42, 0, RCL_ROS_TIME));
  ASSERT_TRUE(deliver(sequencer, "subscriberMessageReceived", message));
  ASSERT_EQ(count, 1);
  EXPECT_DOUBLE_EQ(point.x(), 42.0);
  EXPECT_DOUBLE_EQ(point.y(), 12.5);
}

TEST(CurveDataSequencer, diagnosticMissEmitsNothing) {
  CurveConfig config;
  configureDiagnosticCurve(config);
  config.getAxisConfig(CurveConfig::X)->setFieldType(CurveAxisConfig::MessageReceiptTime);

  CurveDataSequencer sequencer;
  sequencer.setConfig(&config);

  int count = 0;
  diagnosticWarnings.clear();
  const auto previous = qInstallMessageHandler(diagnosticWarningHandler);
  QObject::connect(&sequencer, &CurveDataSequencer::pointReceived, [&](const QPointF&) { ++count; });

  auto message = diagnosticMessage(1.0, "Motor", "Voltage", "1.0");
  ASSERT_TRUE(deliver(sequencer, "subscriberMessageReceived", message));
  qInstallMessageHandler(previous);

  EXPECT_EQ(count, 0);
  EXPECT_TRUE(diagnosticWarnings.isEmpty());
}

TEST(CurveDataSequencer, diagnosticValueUsesHeaderStampOnSameTopic) {
  CurveConfig config;
  configureDiagnosticCurve(config);
  config.getAxisConfig(CurveConfig::X)->setField("header/stamp");

  CurveDataSequencer sequencer;
  sequencer.setConfig(&config);

  QPointF point;
  int count = 0;
  QObject::connect(&sequencer, &CurveDataSequencer::pointReceived, [&](const QPointF& received) {
    point = received;
    ++count;
  });

  ASSERT_TRUE(deliver(sequencer, "subscriberMessageReceived", diagnosticMessage(7.5, "/Power System/Battery", "Voltage", "12.5")));
  ASSERT_EQ(count, 1);
  EXPECT_DOUBLE_EQ(point.x(), 7.5);
  EXPECT_DOUBLE_EQ(point.y(), 12.5);
}

TEST(CurveDataSequencer, diagnosticValueOnOtherTopicUsesHeaderStamp) {
  CurveConfig config;
  config.getAxisConfig(CurveConfig::X)->setTopic("/pose");
  config.getAxisConfig(CurveConfig::Y)->setTopic("/diagnostics");
  config.getAxisConfig(CurveConfig::X)->setType("geometry_msgs/msg/PoseStamped");
  config.getAxisConfig(CurveConfig::Y)->setType("diagnostic_msgs/msg/DiagnosticArray");
  config.getAxisConfig(CurveConfig::X)->setField("pose/position/x");
  config.getAxisConfig(CurveConfig::Y)->setFieldType(CurveAxisConfig::DiagnosticValue);
  config.getAxisConfig(CurveConfig::Y)->setDiagnosticStatus("/Power System/Battery");
  config.getAxisConfig(CurveConfig::Y)->setDiagnosticKey("Voltage");

  CurveDataSequencer sequencer;
  sequencer.setConfig(&config);

  QPointF point;
  int count = 0;
  QObject::connect(&sequencer, &CurveDataSequencer::pointReceived, [&](const QPointF& received) {
    point = received;
    ++count;
  });

  ASSERT_TRUE(deliver(sequencer, "subscriberXAxisMessageReceived", poseStampedMessage(10.0, 1.0)));
  ASSERT_TRUE(deliver(sequencer, "subscriberYAxisMessageReceived", diagnosticMessage(10.0, "/Power System/Battery", "Voltage", "12.5")));
  ASSERT_TRUE(deliver(sequencer, "subscriberXAxisMessageReceived", poseStampedMessage(20.0, 3.0)));
  ASSERT_TRUE(deliver(sequencer, "subscriberYAxisMessageReceived", diagnosticMessage(20.0, "/Power System/Battery", "Voltage", "13.0")));

  ASSERT_EQ(count, 1);
  EXPECT_DOUBLE_EQ(point.x(), 1.0);
  EXPECT_DOUBLE_EQ(point.y(), 12.5);
}

TEST(CurveDataSequencer, diagnosticHardwareIdSelectsMatchingStatus) {
  CurveConfig config;
  configureDiagnosticCurve(config);
  config.getAxisConfig(CurveConfig::X)->setFieldType(CurveAxisConfig::MessageReceiptTime);
  config.getAxisConfig(CurveConfig::Y)->setDiagnosticStatus("Range");
  config.getAxisConfig(CurveConfig::Y)->setDiagnosticKey("Distance");
  config.getAxisConfig(CurveConfig::Y)->setDiagnosticHardwareId("rear");

  auto compound = createMessagePrototype("diagnostic_msgs/msg/DiagnosticArray");
  auto& front = appendDiagnosticStatus(*compound, "Range");
  front["hardware_id"] = std::string("front");
  appendDiagnosticValue(front, "Distance", "1.5");
  auto& rear = appendDiagnosticStatus(*compound, "Range");
  rear["hardware_id"] = std::string("rear");
  appendDiagnosticValue(rear, "Distance", "3.5");
  Message message;
  message.setCompound(compound);
  message.setReceiptTime(rclcpp::Time(42, 0, RCL_ROS_TIME));

  CurveDataSequencer sequencer;
  sequencer.setConfig(&config);
  QPointF point;
  int count = 0;
  QObject::connect(&sequencer, &CurveDataSequencer::pointReceived, [&](const QPointF& received) {
    point = received;
    ++count;
  });

  ASSERT_TRUE(deliver(sequencer, "subscriberMessageReceived", message));
  ASSERT_EQ(count, 1);
  EXPECT_DOUBLE_EQ(point.x(), 42.0);
  EXPECT_DOUBLE_EQ(point.y(), 3.5);

  config.getAxisConfig(CurveConfig::Y)->setDiagnosticHardwareId("missing");
  ASSERT_TRUE(deliver(sequencer, "subscriberMessageReceived", message));
  EXPECT_EQ(count, 1);
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
