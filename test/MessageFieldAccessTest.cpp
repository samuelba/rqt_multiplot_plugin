#include <memory>
#include <vector>

#include <geometry_msgs/msg/twist.hpp>
#include <rclcpp/serialization.hpp>
#include <std_msgs/msg/float64.hpp>
#include <std_msgs/msg/header.hpp>

#include <gtest/gtest.h>
#include <ros_babel_fish/babel_fish.hpp>
#include <ros_babel_fish/messages/array_message.hpp>

#include "rqt_multiplot/MessageFieldAccess.hpp"

namespace {

using rqt_multiplot::createMessagePrototype;
using rqt_multiplot::deserializeMessage;
using rqt_multiplot::fieldTypeFromMessage;
using rqt_multiplot::getMember;
using rqt_multiplot::getNumericValue;
using rqt_multiplot::getStamp;
using rqt_multiplot::hasHeader;
using rqt_multiplot::isNumericMessageType;
using rqt_multiplot::isPlottableFieldPath;
using rqt_multiplot::isWildcardFieldPath;
using rqt_multiplot::normalizeTypeName;
using rqt_multiplot::tryGetNumericSeries;
using rqt_multiplot::tryGetNumericValue;

TEST(MessageFieldAccess, normalizesRos1TypeNames) {
  EXPECT_EQ(normalizeTypeName("std_msgs/Header"), "std_msgs/msg/Header");
  EXPECT_EQ(normalizeTypeName("std_msgs/msg/Header"), "std_msgs/msg/Header");
  EXPECT_EQ(normalizeTypeName("std_srvs/srv/Empty"), "std_srvs/srv/Empty");
  EXPECT_EQ(normalizeTypeName(""), "");
}

TEST(MessageFieldAccess, stripsInterfaceFilenameExtensions) {
  EXPECT_EQ(normalizeTypeName("std_msgs/msg/Header.msg"), "std_msgs/msg/Header");
  EXPECT_EQ(normalizeTypeName("std_msgs/Header.msg"), "std_msgs/msg/Header");
  EXPECT_EQ(normalizeTypeName("std_msgs/msg/Header.idl"), "std_msgs/msg/Header");
}

TEST(MessageFieldAccess, extractsNestedNumericFields) {
  auto fish = ros_babel_fish::BabelFish::make_shared();
  auto message = fish->create_message_shared("geometry_msgs/msg/Twist");
  (*message)["linear"]["x"] = 1.25;
  (*message)["linear"]["y"] = -0.5;
  (*message)["angular"]["z"] = 3.0;

  const auto* linearX = getMember(*message, "linear/x");
  ASSERT_NE(linearX, nullptr);
  EXPECT_TRUE(isNumericMessageType(*linearX));
  EXPECT_DOUBLE_EQ(getNumericValue(*linearX), 1.25);
  EXPECT_DOUBLE_EQ(getNumericValue(*getMember(*message, "angular/z")), 3.0);

  double value = 0.0;
  ASSERT_TRUE(tryGetNumericValue(*message, "linear/x", value));
  EXPECT_DOUBLE_EQ(value, 1.25);
}

TEST(MessageFieldAccess, readsHeaderStamp) {
  auto fish = ros_babel_fish::BabelFish::make_shared();
  auto message = fish->create_message_shared("std_msgs/msg/Header");
  ASSERT_TRUE(hasHeader(*message) || message->containsKey("stamp"));

  (*message)["stamp"] = rclcpp::Time(12, 500000000, RCL_ROS_TIME);
  const auto stamp = getStamp((*message)["stamp"]);
  EXPECT_DOUBLE_EQ(stamp.seconds(), 12.5);
}

TEST(MessageFieldAccess, marksHeaderStampAsTime) {
  auto prototype = createMessagePrototype("std_msgs/msg/Header");
  ASSERT_NE(prototype, nullptr);

  const auto fieldType = fieldTypeFromMessage(*prototype);
  bool foundStamp = false;
  for (const auto& member : fieldType.members) {
    if (member.first == "stamp") {
      foundStamp = true;
      EXPECT_TRUE(member.second.isTime);
      EXPECT_TRUE(member.second.isNumeric);
    }
  }
  EXPECT_TRUE(foundStamp);
}

TEST(MessageFieldAccess, buildsFieldTypeTree) {
  auto prototype = createMessagePrototype("geometry_msgs/msg/Twist");
  ASSERT_NE(prototype, nullptr);

  const auto fieldType = fieldTypeFromMessage(*prototype);
  EXPECT_TRUE(fieldType.isMessage());

  bool foundLinearX = false;
  for (const auto& member : fieldType.members) {
    if (member.first == "linear") {
      EXPECT_TRUE(member.second.isMessage());
      for (const auto& nested : member.second.members) {
        if (nested.first == "x") {
          foundLinearX = nested.second.isNumeric;
        }
      }
    }
  }
  EXPECT_TRUE(foundLinearX);
}

TEST(MessageFieldAccess, readsDynamicPrimitiveArrayElements) {
  auto message = createMessagePrototype("sensor_msgs/msg/JointState");
  ASSERT_NE(message, nullptr);

  auto& position = (*message)["position"].as<ros_babel_fish::ArrayMessage<double>>();
  position.push_back(1.25);
  position.push_back(-0.5);

  double first = 0.0;
  double second = 0.0;
  ASSERT_TRUE(tryGetNumericValue(*message, "position/0", first));
  ASSERT_TRUE(tryGetNumericValue(*message, "position/1", second));
  EXPECT_DOUBLE_EQ(first, 1.25);
  EXPECT_DOUBLE_EQ(second, -0.5);

  double missing = 0.0;
  EXPECT_FALSE(tryGetNumericValue(*message, "position/2", missing));
  EXPECT_FALSE(tryGetNumericValue(*message, "name/0", missing));
}

TEST(MessageFieldAccess, readsFixedPrimitiveArrayElements) {
  auto message = createMessagePrototype("geometry_msgs/msg/PoseWithCovariance");
  ASSERT_NE(message, nullptr);

  auto& covariance = (*message)["covariance"].as<ros_babel_fish::FixedLengthArrayMessage<double>>();
  covariance.assign(0, 9.0);
  covariance.assign(1, 8.5);

  double first = 0.0;
  double second = 0.0;
  ASSERT_TRUE(tryGetNumericValue(*message, "covariance/0", first));
  ASSERT_TRUE(tryGetNumericValue(*message, "covariance/1", second));
  EXPECT_DOUBLE_EQ(first, 9.0);
  EXPECT_DOUBLE_EQ(second, 8.5);
}

TEST(MessageFieldAccess, readsCompoundArrayElements) {
  auto message = createMessagePrototype("geometry_msgs/msg/PoseArray");
  ASSERT_NE(message, nullptr);

  auto& poses = (*message)["poses"].as<ros_babel_fish::CompoundArrayMessage>();
  auto& pose = poses.appendEmpty();
  pose["position"]["x"] = 3.5;

  const auto* field = getMember(*message, "poses/0/position/x");
  ASSERT_NE(field, nullptr);
  EXPECT_DOUBLE_EQ(getNumericValue(*field), 3.5);

  double value = 0.0;
  ASSERT_TRUE(tryGetNumericValue(*message, "poses/0/position/x", value));
  EXPECT_DOUBLE_EQ(value, 3.5);
}

TEST(MessageFieldAccess, detectsSingleWildcardSegment) {
  EXPECT_TRUE(isWildcardFieldPath("position/*"));
  EXPECT_TRUE(isWildcardFieldPath("poses/*/position/x"));
  EXPECT_FALSE(isWildcardFieldPath("position/0"));
  EXPECT_FALSE(isWildcardFieldPath("poses/*/position/*"));
  EXPECT_FALSE(isWildcardFieldPath("linear/x"));
}

TEST(MessageFieldAccess, rejectsBareNumericArrayPath) {
  rqt_multiplot::MessageFieldType element;
  element.kind = rqt_multiplot::MessageFieldType::Builtin;
  element.isNumeric = true;

  rqt_multiplot::MessageFieldType array;
  array.kind = rqt_multiplot::MessageFieldType::Array;
  array.elementType = std::make_shared<rqt_multiplot::MessageFieldType>(element);

  EXPECT_FALSE(isPlottableFieldPath(array, "position"));
  EXPECT_TRUE(isPlottableFieldPath(array, "position/*"));

  rqt_multiplot::MessageFieldType scalar;
  scalar.kind = rqt_multiplot::MessageFieldType::Builtin;
  scalar.isNumeric = true;
  EXPECT_TRUE(isPlottableFieldPath(scalar, "position/0"));
  EXPECT_TRUE(isPlottableFieldPath(scalar, "poses/*/position/x"));
  EXPECT_FALSE(isPlottableFieldPath(scalar, "poses/*/position/*"));
  EXPECT_FALSE(isPlottableFieldPath(rqt_multiplot::MessageFieldType(), "position"));
}

TEST(MessageFieldAccess, readsPrimitiveArraySeries) {
  auto message = createMessagePrototype("sensor_msgs/msg/JointState");
  ASSERT_NE(message, nullptr);

  auto& position = (*message)["position"].as<ros_babel_fish::ArrayMessage<double>>();
  position.push_back(1.25);
  position.push_back(-0.5);
  position.push_back(3.0);

  std::vector<double> values;
  ASSERT_TRUE(tryGetNumericSeries(*message, "position/*", values));
  ASSERT_EQ(values.size(), 3u);
  EXPECT_DOUBLE_EQ(values[0], 1.25);
  EXPECT_DOUBLE_EQ(values[1], -0.5);
  EXPECT_DOUBLE_EQ(values[2], 3.0);
}

TEST(MessageFieldAccess, readsFixedPrimitiveArraySeries) {
  auto message = createMessagePrototype("geometry_msgs/msg/PoseWithCovariance");
  ASSERT_NE(message, nullptr);

  auto& covariance = (*message)["covariance"].as<ros_babel_fish::FixedLengthArrayMessage<double>>();
  covariance.assign(0, 9.0);
  covariance.assign(1, 8.5);
  covariance.assign(2, 1.0);

  std::vector<double> values;
  ASSERT_TRUE(tryGetNumericSeries(*message, "covariance/*", values));
  ASSERT_EQ(values.size(), 36u);
  EXPECT_DOUBLE_EQ(values[0], 9.0);
  EXPECT_DOUBLE_EQ(values[1], 8.5);
  EXPECT_DOUBLE_EQ(values[2], 1.0);
}

TEST(MessageFieldAccess, readsCompoundArraySeries) {
  auto message = createMessagePrototype("geometry_msgs/msg/PoseArray");
  ASSERT_NE(message, nullptr);

  auto& poses = (*message)["poses"].as<ros_babel_fish::CompoundArrayMessage>();
  poses.appendEmpty()["position"]["x"] = 1.0;
  poses.appendEmpty()["position"]["x"] = 2.5;
  poses.appendEmpty()["position"]["y"] = 4.0;

  std::vector<double> xs;
  ASSERT_TRUE(tryGetNumericSeries(*message, "poses/*/position/x", xs));
  ASSERT_EQ(xs.size(), 3u);
  EXPECT_DOUBLE_EQ(xs[0], 1.0);
  EXPECT_DOUBLE_EQ(xs[1], 2.5);
  EXPECT_DOUBLE_EQ(xs[2], 0.0);

  std::vector<double> ys;
  ASSERT_TRUE(tryGetNumericSeries(*message, "poses/*/position/y", ys));
  ASSERT_EQ(ys.size(), 3u);
  EXPECT_DOUBLE_EQ(ys[2], 4.0);
}

TEST(MessageFieldAccess, rejectsInvalidSeriesPaths) {
  auto joints = createMessagePrototype("sensor_msgs/msg/JointState");
  ASSERT_NE(joints, nullptr);
  (*joints)["position"].as<ros_babel_fish::ArrayMessage<double>>().push_back(1.0);

  std::vector<double> values;
  EXPECT_FALSE(tryGetNumericSeries(*joints, "position/*/0", values));
  EXPECT_FALSE(tryGetNumericSeries(*joints, "name/*", values));
  EXPECT_FALSE(tryGetNumericSeries(*joints, "poses/*/position/x", values));

  auto poses = createMessagePrototype("geometry_msgs/msg/PoseArray");
  ASSERT_NE(poses, nullptr);
  EXPECT_FALSE(tryGetNumericSeries(*poses, "poses/*/position/*", values));
}

TEST(MessageFieldAccess, returnsEmptySeriesForEmptyArray) {
  auto message = createMessagePrototype("sensor_msgs/msg/JointState");
  ASSERT_NE(message, nullptr);

  std::vector<double> values{1.0};
  ASSERT_TRUE(tryGetNumericSeries(*message, "position/*", values));
  EXPECT_TRUE(values.empty());
}

TEST(MessageFieldAccess, deserializesSerializedMessage) {
  std_msgs::msg::Float64 value;
  value.data = 42.0;

  rclcpp::Serialization<std_msgs::msg::Float64> serialization;
  rclcpp::SerializedMessage serialized;
  serialization.serialize_message(&value, &serialized);

  auto decoded = deserializeMessage("std_msgs/msg/Float64", serialized);
  ASSERT_NE(decoded, nullptr);
  const auto* data = getMember(*decoded, "data");
  ASSERT_NE(data, nullptr);
  EXPECT_DOUBLE_EQ(getNumericValue(*data), 42.0);
}

}  // namespace
