/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <string>
#include <vector>

#include <rclcpp/serialized_message.hpp>
#include <rclcpp/time.hpp>
#include <ros_babel_fish/messages/compound_message.hpp>
#include <ros_babel_fish/messages/message.hpp>

#include "rqt_multiplot/MessageFieldType.hpp"

namespace rqt_multiplot {

std::string normalizeTypeName(const std::string& typeName);

const ros_babel_fish::Message* getMember(const ros_babel_fish::Message& message, const std::string& path);
ros_babel_fish::Message* getMember(ros_babel_fish::Message& message, const std::string& path);

bool isNumericMessageType(const ros_babel_fish::Message& message);
double getNumericValue(const ros_babel_fish::Message& message);
bool tryGetNumericValue(const ros_babel_fish::Message& message, const std::string& path, double& value);
bool tryGetNumericSeries(const ros_babel_fish::Message& message, const std::string& path, std::vector<double>& values);
bool isDiagnosticArrayTypeName(const std::string& typeName);

struct DiagnosticStatusKey {
  std::string name;
  std::string hardwareId;
  std::string key;
};

bool tryGetDiagnosticValue(const ros_babel_fish::Message& message, const std::string& statusName, const std::string& key, double& value,
                           const std::string& hardwareId = {});
std::vector<DiagnosticStatusKey> diagnosticStatusKeys(const ros_babel_fish::Message& message);
bool isWildcardFieldPath(const std::string& path);
bool isPlottableFieldPath(const MessageFieldType& fieldType, const std::string& path);
rclcpp::Time getStamp(const ros_babel_fish::Message& message);
bool hasHeader(const ros_babel_fish::Message& message);

MessageFieldType fieldTypeFromMessage(const ros_babel_fish::Message& message);

ros_babel_fish::CompoundMessage::SharedPtr createMessagePrototype(const std::string& typeName);
ros_babel_fish::CompoundMessage::SharedPtr deserializeMessage(const std::string& typeName, const rclcpp::SerializedMessage& serialized);

}  // namespace rqt_multiplot
