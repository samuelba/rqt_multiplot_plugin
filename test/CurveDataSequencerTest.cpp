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
#include "rqt_multiplot/MessageBroker.hpp"
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

constexpr int64_t kNsPerMs = 1000000;

Message stampedMetricMessage(int64_t receiptNs, int64_t stampNs, size_t bytes) {
  auto compound = createMessagePrototype("geometry_msgs/msg/PoseStamped");
  (*compound)["header"]["stamp"] = rclcpp::Time(stampNs, RCL_ROS_TIME);
  Message message;
  message.setCompound(compound);
  message.setReceiptTime(rclcpp::Time(receiptNs, RCL_ROS_TIME));
  message.setSerializedSize(bytes);
  return message;
}

Message unstampedMetricMessage(int64_t receiptNs) {
  Message message;
  message.setCompound(createMessagePrototype("std_msgs/msg/Float64"));
  message.setReceiptTime(rclcpp::Time(receiptNs, RCL_ROS_TIME));
  return message;
}

void configureMetricCurve(CurveConfig& config, rqt_multiplot::TopicMetric metric) {
  config.getAxisConfig(CurveConfig::X)->setTopic("/metric");
  config.getAxisConfig(CurveConfig::Y)->setTopic("/metric");
  config.getAxisConfig(CurveConfig::X)->setFieldType(CurveAxisConfig::MessageReceiptTime);
  config.getAxisConfig(CurveConfig::Y)->setFieldType(CurveAxisConfig::TopicMetric);
  config.getAxisConfig(CurveConfig::Y)->setTopicMetric(metric);
}

class FakeBroker : public rqt_multiplot::MessageBroker {
 public:
  bool subscribe(const QString& /*topic*/, QObject* /*receiver*/, const char* /*method*/, const PropertyMap& /*properties*/,
                 Qt::ConnectionType /*type*/) override {
    return true;
  }
  bool unsubscribe(const QString& /*topic*/, QObject* /*receiver*/, const char* /*method*/) override { return true; }
};

TEST(CurveDataSequencer, topicRateIsThrottledByMessageTime) {
  CurveConfig config;
  configureMetricCurve(config, rqt_multiplot::TopicMetric::Rate);
  CurveDataSequencer sequencer;
  sequencer.setConfig(&config);

  QVector<QPointF> points;
  QObject::connect(&sequencer, &CurveDataSequencer::pointReceived, [&](const QPointF& point) { points.append(point); });

  for (int i = 0; i <= 100; ++i) {
    ASSERT_TRUE(deliver(sequencer, "subscriberMessageReceived", unstampedMetricMessage(i * 10 * kNsPerMs)));
  }

  ASSERT_EQ(points.size(), 10);
  EXPECT_DOUBLE_EQ(points.front().x(), 0.01);
  EXPECT_NEAR(points.front().y(), 100.0, 1e-9);
  EXPECT_NEAR(points.back().y(), 100.0, 1e-9);
  EXPECT_NEAR(points[1].x() - points[0].x(), 0.1, 1e-9);
}

TEST(CurveDataSequencer, topicMetricEmitsNothingBeforeTwoMessages) {
  CurveConfig config;
  configureMetricCurve(config, rqt_multiplot::TopicMetric::Rate);
  CurveDataSequencer sequencer;
  sequencer.setConfig(&config);
  int count = 0;
  QObject::connect(&sequencer, &CurveDataSequencer::pointReceived, [&](const QPointF&) { ++count; });

  ASSERT_TRUE(deliver(sequencer, "subscriberMessageReceived", unstampedMetricMessage(0)));

  EXPECT_EQ(count, 0);
}

TEST(CurveDataSequencer, topicDelayUsesHeaderStamp) {
  CurveConfig config;
  configureMetricCurve(config, rqt_multiplot::TopicMetric::DelayMean);
  CurveDataSequencer sequencer;
  sequencer.setConfig(&config);
  QPointF point;
  int count = 0;
  QObject::connect(&sequencer, &CurveDataSequencer::pointReceived, [&](const QPointF& received) {
    point = received;
    ++count;
  });

  ASSERT_TRUE(deliver(sequencer, "subscriberMessageReceived", stampedMetricMessage(1000 * kNsPerMs, 980 * kNsPerMs, 0)));

  ASSERT_EQ(count, 1);
  EXPECT_DOUBLE_EQ(point.x(), 1.0);
  EXPECT_NEAR(point.y(), 0.02, 1e-12);
}

TEST(CurveDataSequencer, topicDelayEmitsNothingWithoutHeader) {
  CurveConfig config;
  configureMetricCurve(config, rqt_multiplot::TopicMetric::DelayMean);
  CurveDataSequencer sequencer;
  sequencer.setConfig(&config);
  int count = 0;
  QObject::connect(&sequencer, &CurveDataSequencer::pointReceived, [&](const QPointF&) { ++count; });

  ASSERT_TRUE(deliver(sequencer, "subscriberMessageReceived", unstampedMetricMessage(0)));
  ASSERT_TRUE(deliver(sequencer, "subscriberMessageReceived", unstampedMetricMessage(200 * kNsPerMs)));

  EXPECT_EQ(count, 0);
}

TEST(CurveDataSequencer, topicBandwidthUsesSerializedSize) {
  CurveConfig config;
  configureMetricCurve(config, rqt_multiplot::TopicMetric::Bandwidth);
  CurveDataSequencer sequencer;
  sequencer.setConfig(&config);
  QPointF point;
  QObject::connect(&sequencer, &CurveDataSequencer::pointReceived, [&](const QPointF& received) { point = received; });

  ASSERT_TRUE(deliver(sequencer, "subscriberMessageReceived", stampedMetricMessage(0, 0, 500)));
  ASSERT_TRUE(deliver(sequencer, "subscriberMessageReceived", stampedMetricMessage(500 * kNsPerMs, 0, 500)));

  EXPECT_DOUBLE_EQ(point.y(), 1000.0);
}

TEST(CurveDataSequencer, topicMetricRequiresSameTopic) {
  CurveConfig config;
  configureMetricCurve(config, rqt_multiplot::TopicMetric::Rate);
  EXPECT_TRUE(CurveDataSequencer::topicMetricIncompatibilityReason(config).isEmpty());

  config.getAxisConfig(CurveConfig::X)->setTopic("/other");
  EXPECT_FALSE(CurveDataSequencer::topicMetricIncompatibilityReason(config).isEmpty());

  config.getAxisConfig(CurveConfig::Y)->setFieldType(CurveAxisConfig::MessageData);
  EXPECT_TRUE(CurveDataSequencer::topicMetricIncompatibilityReason(config).isEmpty());
}

TEST(CurveDataSequencer, topicMetricWindowResetsOnResubscribe) {
  CurveConfig config;
  configureMetricCurve(config, rqt_multiplot::TopicMetric::Rate);
  FakeBroker broker;
  CurveDataSequencer sequencer;
  sequencer.setConfig(&config);
  sequencer.setBroker(&broker);
  sequencer.subscribe();
  int count = 0;
  QObject::connect(&sequencer, &CurveDataSequencer::pointReceived, [&](const QPointF&) { ++count; });

  ASSERT_TRUE(deliver(sequencer, "subscriberMessageReceived", unstampedMetricMessage(0)));
  sequencer.unsubscribe();
  sequencer.subscribe();
  ASSERT_TRUE(deliver(sequencer, "subscriberMessageReceived", unstampedMetricMessage(200 * kNsPerMs)));

  EXPECT_EQ(count, 0);
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

Message float64Message(double seconds, double value) {
  auto compound = createMessagePrototype("std_msgs/msg/Float64");
  (*compound)["data"] = value;
  Message message;
  message.setCompound(compound);
  const auto whole = static_cast<int32_t>(seconds);
  const auto nanos = static_cast<uint32_t>((seconds - static_cast<double>(whole)) * 1e9);
  message.setReceiptTime(rclcpp::Time(whole, nanos, RCL_ROS_TIME));
  return message;
}

void configureSplitTopics(CurveConfig& config) {
  config.getAxisConfig(CurveConfig::X)->setTopic("/x");
  config.getAxisConfig(CurveConfig::Y)->setTopic("/y");
  config.getAxisConfig(CurveConfig::X)->setType("std_msgs/msg/Float64");
  config.getAxisConfig(CurveConfig::Y)->setType("std_msgs/msg/Float64");
  config.getAxisConfig(CurveConfig::X)->setField("data");
  config.getAxisConfig(CurveConfig::Y)->setField("data");
  config.getAxisConfig(CurveConfig::X)->setFieldType(CurveAxisConfig::MessageData);
  config.getAxisConfig(CurveConfig::Y)->setFieldType(CurveAxisConfig::MessageData);
}

class CountingBroker : public rqt_multiplot::MessageBroker {
 public:
  int subscriptions = 0;
  int unsubscriptions = 0;
  bool accept = true;

  bool subscribe(const QString& /*topic*/, QObject* /*receiver*/, const char* /*method*/, const PropertyMap& /*properties*/,
                 Qt::ConnectionType /*type*/) override {
    ++subscriptions;
    return accept;
  }
  bool unsubscribe(const QString& /*topic*/, QObject* /*receiver*/, const char* /*method*/) override {
    ++unsubscriptions;
    return true;
  }
};

TEST(CurveDataSequencer, interpolatesTheAxisThatHasTheLaterSample) {
  CurveConfig config;
  configureSplitTopics(config);
  CurveDataSequencer sequencer;
  sequencer.setConfig(&config);
  EXPECT_EQ(sequencer.getConfig(), &config);

  QVector<QPointF> points;
  QObject::connect(&sequencer, &CurveDataSequencer::pointReceived, [&](const QPointF& point) { points.append(point); });

  ASSERT_TRUE(deliver(sequencer, "subscriberXAxisMessageReceived", float64Message(0.0, 0.0)));
  ASSERT_TRUE(deliver(sequencer, "subscriberXAxisMessageReceived", float64Message(2.0, 4.0)));
  ASSERT_TRUE(deliver(sequencer, "subscriberYAxisMessageReceived", float64Message(1.0, 10.0)));
  EXPECT_TRUE(points.isEmpty());
  ASSERT_TRUE(deliver(sequencer, "subscriberYAxisMessageReceived", float64Message(3.0, 30.0)));
  ASSERT_EQ(points.size(), 1);
  EXPECT_DOUBLE_EQ(points[0].x(), 2.0);
  EXPECT_DOUBLE_EQ(points[0].y(), 10.0);

  CurveConfig otherConfig;
  configureSplitTopics(otherConfig);
  CurveDataSequencer other;
  other.setConfig(&otherConfig);
  QVector<QPointF> otherPoints;
  QObject::connect(&other, &CurveDataSequencer::pointReceived, [&](const QPointF& point) { otherPoints.append(point); });
  ASSERT_TRUE(deliver(other, "subscriberYAxisMessageReceived", float64Message(0.0, 0.0)));
  ASSERT_TRUE(deliver(other, "subscriberYAxisMessageReceived", float64Message(2.0, 4.0)));
  ASSERT_TRUE(deliver(other, "subscriberXAxisMessageReceived", float64Message(1.0, 10.0)));
  ASSERT_TRUE(deliver(other, "subscriberXAxisMessageReceived", float64Message(3.0, 12.0)));
  ASSERT_EQ(otherPoints.size(), 1);
  EXPECT_DOUBLE_EQ(otherPoints[0].x(), 10.0);
  EXPECT_DOUBLE_EQ(otherPoints[0].y(), 2.0);
}

TEST(CurveDataSequencer, subscribeUsesSeparateTopicsAndResubscribesOnConfigChange) {
  CurveConfig config;
  configureSplitTopics(config);
  CountingBroker broker;
  CurveDataSequencer sequencer;
  sequencer.setConfig(&config);
  sequencer.setBroker(&broker);
  EXPECT_EQ(sequencer.getBroker(), &broker);
  EXPECT_FALSE(sequencer.isSubscribed());

  int subscribed = 0;
  QObject::connect(&sequencer, &CurveDataSequencer::subscribed, [&]() { ++subscribed; });
  sequencer.subscribe();
  EXPECT_TRUE(sequencer.isSubscribed());
  EXPECT_EQ(broker.subscriptions, 2);
  EXPECT_EQ(subscribed, 1);

  config.setSubscriberQueueSize(config.getSubscriberQueueSize() + 1);
  EXPECT_GT(broker.unsubscriptions, 0);
  EXPECT_TRUE(sequencer.isSubscribed());

  config.getAxisConfig(CurveConfig::X)->setField("other");
  EXPECT_TRUE(sequencer.isSubscribed());

  sequencer.unsubscribe();
  EXPECT_FALSE(sequencer.isSubscribed());

  broker.accept = false;
  sequencer.subscribe();
  EXPECT_FALSE(sequencer.isSubscribed());
}

TEST(CurveDataSequencer, replacingConfigWhileSubscribedReconnects) {
  CurveConfig first;
  configureSplitTopics(first);
  first.getAxisConfig(CurveConfig::X)->setTopic("/same");
  first.getAxisConfig(CurveConfig::Y)->setTopic("/same");
  CurveConfig second;
  configureSplitTopics(second);
  CountingBroker broker;
  CountingBroker other;
  CurveDataSequencer sequencer;
  sequencer.setConfig(&first);
  sequencer.setBroker(&broker);
  sequencer.subscribe();
  const int afterFirst = broker.subscriptions;

  sequencer.setConfig(&second);
  EXPECT_EQ(sequencer.getConfig(), &second);
  EXPECT_GT(broker.subscriptions, afterFirst);
  EXPECT_TRUE(sequencer.isSubscribed());

  sequencer.setBroker(&other);
  EXPECT_EQ(sequencer.getBroker(), &other);
  EXPECT_TRUE(sequencer.isSubscribed());
  EXPECT_GT(other.subscriptions, 0);
}

TEST(CurveDataSequencer, ignoresMessagesWithoutConfigAndMismatchedSnapshots) {
  CurveDataSequencer sequencer;
  EXPECT_TRUE(deliver(sequencer, "subscriberMessageReceived", float64Message(0.0, 1.0)));

  CurveConfig config;
  config.getAxisConfig(CurveConfig::X)->setTopic("/array");
  config.getAxisConfig(CurveConfig::Y)->setTopic("/other");
  config.getAxisConfig(CurveConfig::X)->setFieldType(CurveAxisConfig::ArrayIndex);
  config.getAxisConfig(CurveConfig::Y)->setField("position/*");
  EXPECT_EQ(CurveDataSequencer::snapshotIncompatibilityReason(config), QStringLiteral("Array curves require the same topic on both axes"));

  config.getAxisConfig(CurveConfig::Y)->setTopic("/array");
  config.getAxisConfig(CurveConfig::Y)->setFieldType(CurveAxisConfig::ArrayIndex);
  EXPECT_EQ(CurveDataSequencer::snapshotIncompatibilityReason(config), QStringLiteral("Only one axis can be array index"));

  config.getAxisConfig(CurveConfig::Y)->setFieldType(CurveAxisConfig::MessageData);
  config.getAxisConfig(CurveConfig::Y)->setField("position/*");
  sequencer.setConfig(&config);
  int seriesCount = 0;
  QObject::connect(&sequencer, &CurveDataSequencer::seriesReceived, [&](const QVector<QPointF>&) { ++seriesCount; });
  ASSERT_TRUE(deliver(sequencer, "subscriberMessageReceived", Message()));
  EXPECT_EQ(seriesCount, 0);

  auto joints = jointStateMessage();
  auto& effort = (*joints.getCompound())["effort"].as<ros_babel_fish::ArrayMessage<double>>();
  effort.push_back(1.0);
  config.getAxisConfig(CurveConfig::X)->setField("position/*");
  config.getAxisConfig(CurveConfig::X)->setFieldType(CurveAxisConfig::MessageData);
  config.getAxisConfig(CurveConfig::Y)->setField("effort/*");
  ASSERT_TRUE(deliver(sequencer, "subscriberMessageReceived", joints));
  EXPECT_EQ(seriesCount, 1);
}

TEST(CurveDataSequencer, yArrayIndexPairsWithWildcardX) {
  CurveConfig config;
  config.getAxisConfig(CurveConfig::X)->setTopic("/array");
  config.getAxisConfig(CurveConfig::Y)->setTopic("/array");
  config.getAxisConfig(CurveConfig::X)->setField("position/*");
  config.getAxisConfig(CurveConfig::Y)->setFieldType(CurveAxisConfig::ArrayIndex);

  QVector<QPointF> points;
  ASSERT_TRUE(CurveDataSequencer::tryBuildSnapshotSeries(jointStateMessage(), config, points));
  ASSERT_EQ(points.size(), 3);
  EXPECT_DOUBLE_EQ(points[0].y(), 0.0);
  EXPECT_DOUBLE_EQ(points[2].x(), 3.0);
  EXPECT_DOUBLE_EQ(points[2].y(), 2.0);
}

}  // namespace
