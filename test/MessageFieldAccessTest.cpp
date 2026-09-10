#include <geometry_msgs/msg/twist.hpp>
#include <rclcpp/serialization.hpp>
#include <std_msgs/msg/float64.hpp>
#include <std_msgs/msg/header.hpp>

#include <gtest/gtest.h>
#include <ros_babel_fish/babel_fish.hpp>

#include <rqt_multiplot/MessageFieldAccess.h>

namespace {

using rqt_multiplot::createMessagePrototype;
using rqt_multiplot::deserializeMessage;
using rqt_multiplot::fieldTypeFromMessage;
using rqt_multiplot::getMember;
using rqt_multiplot::getNumericValue;
using rqt_multiplot::getStamp;
using rqt_multiplot::hasHeader;
using rqt_multiplot::isNumericMessageType;
using rqt_multiplot::normalizeTypeName;

TEST(MessageFieldAccess, normalizesRos1TypeNames) {
  EXPECT_EQ(normalizeTypeName("std_msgs/Header"), "std_msgs/msg/Header");
  EXPECT_EQ(normalizeTypeName("std_msgs/msg/Header"), "std_msgs/msg/Header");
  EXPECT_EQ(normalizeTypeName("std_srvs/srv/Empty"), "std_srvs/srv/Empty");
  EXPECT_EQ(normalizeTypeName(""), "");
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
}

TEST(MessageFieldAccess, readsHeaderStamp) {
  auto fish = ros_babel_fish::BabelFish::make_shared();
  auto message = fish->create_message_shared("std_msgs/msg/Header");
  ASSERT_TRUE(hasHeader(*message) || message->containsKey("stamp"));

  (*message)["stamp"] = rclcpp::Time(12, 500000000, RCL_ROS_TIME);
  const auto stamp = getStamp((*message)["stamp"]);
  EXPECT_DOUBLE_EQ(stamp.seconds(), 12.5);
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
