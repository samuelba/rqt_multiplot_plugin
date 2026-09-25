#include <memory>

#include <gtest/gtest.h>

#include "rqt_multiplot/CurveConfig.hpp"
#include "rqt_multiplot/MessageFieldType.hpp"
#include "rqt_multiplot/TopicFieldMime.hpp"

namespace {

using rqt_multiplot::CurveAxisConfig;
using rqt_multiplot::CurveConfig;
using rqt_multiplot::MessageFieldType;
using rqt_multiplot::TopicFieldRef;

MessageFieldType builtin(const QString& identifier, bool numeric) {
  MessageFieldType type;
  type.kind = MessageFieldType::Builtin;
  type.identifier = identifier;
  type.isNumeric = numeric;
  return type;
}

MessageFieldType compound(const QString& identifier, const QVector<QPair<QString, MessageFieldType>>& members) {
  MessageFieldType type;
  type.kind = MessageFieldType::Compound;
  type.identifier = identifier;
  type.members = members;
  return type;
}

MessageFieldType array(const MessageFieldType& element) {
  MessageFieldType type;
  type.kind = MessageFieldType::Array;
  type.identifier = element.identifier + "[]";
  type.isDynamicArray = true;
  type.elementType = std::make_shared<MessageFieldType>(element);
  return type;
}

MessageFieldType timeType() {
  MessageFieldType type = compound("builtin_interfaces/Time", {{"sec", builtin("int32", true)}, {"nanosec", builtin("uint32", true)}});
  type.isNumeric = true;
  type.isTime = true;
  return type;
}

MessageFieldType pointType() {
  return compound("geometry_msgs/Point",
                  {{"x", builtin("float64", true)}, {"y", builtin("float64", true)}, {"z", builtin("float64", true)}});
}

MessageFieldType headerType() {
  return compound("std_msgs/Header", {{"stamp", timeType()}, {"frame_id", builtin("string", false)}});
}

TEST(TopicFieldMime, encodeDecodeRoundTrip) {
  const QVector<TopicFieldRef> refs = {{"/imu", "sensor_msgs/msg/Imu", "orientation/x"},
                                       {"/joint_states", "sensor_msgs/msg/JointState", "position/*"}};

  const QVector<TopicFieldRef> decoded = rqt_multiplot::decodeTopicFields(rqt_multiplot::encodeTopicFields(refs));

  EXPECT_EQ(decoded, refs);
}

TEST(TopicFieldMime, decodeOfGarbageReturnsEmpty) {
  EXPECT_TRUE(rqt_multiplot::decodeTopicFields(QByteArray("not a payload")).isEmpty());
}

TEST(TopicFieldMime, plottableLeavesOfPointAreXYZ) {
  const QStringList leaves = rqt_multiplot::plottableLeaves(pointType(), "pose/position");

  EXPECT_EQ(leaves, QStringList({"pose/position/x", "pose/position/y", "pose/position/z"}));
}

TEST(TopicFieldMime, plottableLeavesTreatsTimeAsOneLeafAndSkipsStrings) {
  EXPECT_EQ(rqt_multiplot::plottableLeaves(headerType(), "header"), QStringList({"header/stamp"}));
}

TEST(TopicFieldMime, plottableLeavesSkipsArrays) {
  const MessageFieldType message = compound("sensor_msgs/JointState", {{"header", headerType()},
                                                                       {"name", array(builtin("string", false))},
                                                                       {"position", array(builtin("float64", true))},
                                                                       {"points", array(pointType())}});

  EXPECT_EQ(rqt_multiplot::plottableLeaves(message, QString()), QStringList({"header/stamp"}));
}

TEST(TopicFieldMime, plottableLeavesOfScalarIsItself) {
  EXPECT_EQ(rqt_multiplot::plottableLeaves(builtin("float64", true), "data"), QStringList({"data"}));
}

TEST(TopicFieldMime, confirmationRequiredAboveTenCurves) {
  EXPECT_FALSE(rqt_multiplot::requiresDropConfirmation(10));
  EXPECT_TRUE(rqt_multiplot::requiresDropConfirmation(11));
}

TEST(TopicFieldMime, fillCurveForScalarUsesReceiptTimeOnX) {
  CurveConfig config;

  rqt_multiplot::fillCurveFromTopicField(config, {"/imu", "sensor_msgs/msg/Imu", "orientation/x"});

  const CurveAxisConfig* x = config.getAxisConfig(CurveConfig::X);
  const CurveAxisConfig* y = config.getAxisConfig(CurveConfig::Y);
  EXPECT_EQ(x->getTopic(), QString("/imu"));
  EXPECT_EQ(x->getType(), QString("sensor_msgs/msg/Imu"));
  EXPECT_EQ(x->getFieldType(), CurveAxisConfig::MessageReceiptTime);
  EXPECT_EQ(y->getTopic(), QString("/imu"));
  EXPECT_EQ(y->getType(), QString("sensor_msgs/msg/Imu"));
  EXPECT_EQ(y->getFieldType(), CurveAxisConfig::MessageData);
  EXPECT_EQ(y->getField(), QString("orientation/x"));
  EXPECT_EQ(config.getTitle(), QString("/imu/orientation/x"));
}

TEST(TopicFieldMime, fillCurveForArrayUsesArrayIndexOnX) {
  CurveConfig config;

  rqt_multiplot::fillCurveFromTopicField(config, {"/joint_states", "sensor_msgs/msg/JointState", "position/*"});

  EXPECT_EQ(config.getAxisConfig(CurveConfig::X)->getFieldType(), CurveAxisConfig::ArrayIndex);
  EXPECT_EQ(config.getAxisConfig(CurveConfig::Y)->getField(), QString("position/*"));
}

TEST(TopicFieldMime, fillCurveForWildcardInsideMessageArrayUsesArrayIndexOnX) {
  CurveConfig config;

  rqt_multiplot::fillCurveFromTopicField(config, {"/poses", "geometry_msgs/msg/PoseArray", "poses/*/position/x"});

  EXPECT_EQ(config.getAxisConfig(CurveConfig::X)->getFieldType(), CurveAxisConfig::ArrayIndex);
}

TEST(TopicFieldMime, fillCurveForIndexedElementUsesReceiptTimeOnX) {
  CurveConfig config;

  rqt_multiplot::fillCurveFromTopicField(config, {"/poses", "geometry_msgs/msg/PoseArray", "poses/3/position/x"});

  EXPECT_EQ(config.getAxisConfig(CurveConfig::X)->getFieldType(), CurveAxisConfig::MessageReceiptTime);
  EXPECT_EQ(config.getAxisConfig(CurveConfig::Y)->getField(), QString("poses/3/position/x"));
}

TEST(TopicFieldMime, arrayWildcardFieldsOfNumericArrayIsStar) {
  EXPECT_EQ(rqt_multiplot::arrayWildcardFields(array(builtin("float32", true)), "ranges"), QStringList({"ranges/*"}));
}

TEST(TopicFieldMime, arrayWildcardFieldsOfMessageArrayListsElementLeaves) {
  const MessageFieldType pose = compound("geometry_msgs/Pose", {{"position", pointType()}, {"tags", array(builtin("float64", true))}});

  EXPECT_EQ(rqt_multiplot::arrayWildcardFields(array(pose), "poses"),
            QStringList({"poses/*/position/x", "poses/*/position/y", "poses/*/position/z"}));
}

TEST(TopicFieldMime, arrayWildcardFieldsOfStringArrayIsEmpty) {
  EXPECT_TRUE(rqt_multiplot::arrayWildcardFields(array(builtin("string", false)), "names").isEmpty());
}

TEST(TopicFieldMime, containsDynamicArrayFindsNestedDynamicArrays) {
  MessageFieldType fixed = array(builtin("float64", true));
  fixed.isDynamicArray = false;
  fixed.arraySize = 36;

  EXPECT_FALSE(rqt_multiplot::containsDynamicArray(compound("msg", {{"header", headerType()}, {"covariance", fixed}})));
  EXPECT_TRUE(rqt_multiplot::containsDynamicArray(compound("msg", {{"inner", compound("inner", {{"data", array(pointType())}})}})));
}

}  // namespace
