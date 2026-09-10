/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#ifndef RQT_MULTIPLOT_MESSAGE_FIELD_ACCESS_H
#define RQT_MULTIPLOT_MESSAGE_FIELD_ACCESS_H

#include <string>

#include <rclcpp/serialized_message.hpp>
#include <rclcpp/time.hpp>
#include <ros_babel_fish/messages/compound_message.hpp>
#include <ros_babel_fish/messages/message.hpp>

#include <rqt_multiplot/MessageFieldType.h>

namespace rqt_multiplot {

std::string normalizeTypeName(const std::string& typeName);

const ros_babel_fish::Message* getMember(const ros_babel_fish::Message& message, const std::string& path);
ros_babel_fish::Message* getMember(ros_babel_fish::Message& message, const std::string& path);

bool isNumericMessageType(const ros_babel_fish::Message& message);
double getNumericValue(const ros_babel_fish::Message& message);
rclcpp::Time getStamp(const ros_babel_fish::Message& message);
bool hasHeader(const ros_babel_fish::Message& message);

MessageFieldType fieldTypeFromMessage(const ros_babel_fish::Message& message);

ros_babel_fish::CompoundMessage::SharedPtr createMessagePrototype(const std::string& typeName);
ros_babel_fish::CompoundMessage::SharedPtr deserializeMessage(const std::string& typeName, const rclcpp::SerializedMessage& serialized);

}  // namespace rqt_multiplot

#endif
