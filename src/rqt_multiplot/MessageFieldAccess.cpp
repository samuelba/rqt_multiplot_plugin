/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/MessageFieldAccess.h"

#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include <QPair>

#include <rmw/rmw.h>
#include <ros_babel_fish/exceptions/babel_fish_exception.hpp>
#include <ros_babel_fish/messages/array_message.hpp>
#include <ros_babel_fish/messages/message_types.hpp>

#include "rqt_multiplot/RosContext.h"

namespace rqt_multiplot {
namespace {

std::vector<std::string> splitPath(const std::string& path) {
  std::vector<std::string> parts;
  std::stringstream stream(path);
  std::string part;
  while (std::getline(stream, part, '/')) {
    if (!part.empty()) {
      parts.push_back(part);
    }
  }
  return parts;
}

bool isNumericType(ros_babel_fish::MessageType type) {
  using Type = ros_babel_fish::MessageTypes::MessageType;
  switch (type) {
    case Type::Float:
    case Type::Double:
    case Type::LongDouble:
    case Type::Char:
    case Type::Bool:
    case Type::Octet:
    case Type::UInt8:
    case Type::Int8:
    case Type::UInt16:
    case Type::Int16:
    case Type::UInt32:
    case Type::Int32:
    case Type::UInt64:
    case Type::Int64:
      return true;
    default:
      return false;
  }
}

QString builtinTypeName(ros_babel_fish::MessageType type) {
  using Type = ros_babel_fish::MessageTypes::MessageType;
  switch (type) {
    case Type::Float:
      return "float32";
    case Type::Double:
      return "float64";
    case Type::LongDouble:
      return "long double";
    case Type::Char:
      return "char";
    case Type::WChar:
      return "wchar";
    case Type::Bool:
      return "bool";
    case Type::Octet:
      return "octet";
    case Type::UInt8:
      return "uint8";
    case Type::Int8:
      return "int8";
    case Type::UInt16:
      return "uint16";
    case Type::Int16:
      return "int16";
    case Type::UInt32:
      return "uint32";
    case Type::Int32:
      return "int32";
    case Type::UInt64:
      return "uint64";
    case Type::Int64:
      return "int64";
    case Type::String:
      return "string";
    case Type::WString:
      return "wstring";
    default:
      return "unknown";
  }
}

MessageFieldType builtinFieldType(ros_babel_fish::MessageType type) {
  MessageFieldType fieldType;
  fieldType.kind = MessageFieldType::Builtin;
  fieldType.identifier = builtinTypeName(type);
  fieldType.isNumeric = isNumericType(type);
  return fieldType;
}

const ros_babel_fish::CompoundMessage* compoundArrayAt(const ros_babel_fish::Message& message, size_t index) {
  try {
    const auto& array = message.as<ros_babel_fish::CompoundArrayMessage>();
    return index < array.size() ? &array[index] : nullptr;
  } catch (const ros_babel_fish::BabelFishException&) {
  }
  try {
    const auto& array = message.as<ros_babel_fish::FixedLengthCompoundArrayMessage>();
    return index < array.size() ? &array[index] : nullptr;
  } catch (const ros_babel_fish::BabelFishException&) {
  }
  try {
    const auto& array = message.as<ros_babel_fish::BoundedCompoundArrayMessage>();
    return index < array.size() ? &array[index] : nullptr;
  } catch (const ros_babel_fish::BabelFishException&) {
  }
  return nullptr;
}

template <typename T>
bool primitiveArrayValueAt(const ros_babel_fish::ArrayMessageBase& array, size_t index, double& value) {
  if constexpr (!std::is_arithmetic_v<T>) {
    return false;
  } else {
    try {
      if (array.isFixedSize()) {
        const auto& typed = array.as<ros_babel_fish::FixedLengthArrayMessage<T>>();
        if (index >= typed.size()) {
          return false;
        }
        value = static_cast<double>(typed[index]);
        return true;
      }
      if (array.isBounded()) {
        const auto& typed = array.as<ros_babel_fish::BoundedArrayMessage<T>>();
        if (index >= typed.size()) {
          return false;
        }
        value = static_cast<double>(typed[index]);
        return true;
      }
      const auto& typed = array.as<ros_babel_fish::ArrayMessage<T>>();
      if (index >= typed.size()) {
        return false;
      }
      value = static_cast<double>(typed[index]);
      return true;
    } catch (const ros_babel_fish::BabelFishException&) {
      return false;
    } catch (const std::out_of_range&) {
      return false;
    }
  }
}

bool tryPrimitiveArrayValue(const ros_babel_fish::Message& message, size_t index, double& value) {
  if (message.type() != ros_babel_fish::MessageTypes::Array) {
    return false;
  }

  const auto& array = message.as<ros_babel_fish::ArrayMessageBase>();
  using Type = ros_babel_fish::MessageTypes::MessageType;
  switch (array.elementType()) {
    case Type::Float:
      return primitiveArrayValueAt<float>(array, index, value);
    case Type::Double:
      return primitiveArrayValueAt<double>(array, index, value);
    case Type::LongDouble:
      return primitiveArrayValueAt<long double>(array, index, value);
    case Type::Char:
      return primitiveArrayValueAt<unsigned char>(array, index, value);
    case Type::Bool:
      return primitiveArrayValueAt<bool>(array, index, value);
    case Type::Octet:
      return primitiveArrayValueAt<unsigned char>(array, index, value);
    case Type::UInt8:
      return primitiveArrayValueAt<uint8_t>(array, index, value);
    case Type::Int8:
      return primitiveArrayValueAt<int8_t>(array, index, value);
    case Type::UInt16:
      return primitiveArrayValueAt<uint16_t>(array, index, value);
    case Type::Int16:
      return primitiveArrayValueAt<int16_t>(array, index, value);
    case Type::UInt32:
      return primitiveArrayValueAt<uint32_t>(array, index, value);
    case Type::Int32:
      return primitiveArrayValueAt<int32_t>(array, index, value);
    case Type::UInt64:
      return primitiveArrayValueAt<uint64_t>(array, index, value);
    case Type::Int64:
      return primitiveArrayValueAt<int64_t>(array, index, value);
    default:
      return false;
  }
}

ros_babel_fish::MessageMemberIntrospection compoundArrayElementIntrospection(const ros_babel_fish::Message& message) {
  try {
    return message.as<ros_babel_fish::CompoundArrayMessage>().elementIntrospection();
  } catch (const ros_babel_fish::BabelFishException&) {
  }
  try {
    return message.as<ros_babel_fish::FixedLengthCompoundArrayMessage>().elementIntrospection();
  } catch (const ros_babel_fish::BabelFishException&) {
  }
  try {
    return message.as<ros_babel_fish::BoundedCompoundArrayMessage>().elementIntrospection();
  } catch (const ros_babel_fish::BabelFishException&) {
  }
  return {};
}

}  // namespace

std::string normalizeTypeName(const std::string& typeName) {
  if (typeName.empty()) {
    return typeName;
  }

  std::string normalized = typeName;
  const auto lastSlash = normalized.rfind('/');
  const auto lastDot = normalized.rfind('.');
  if (lastDot != std::string::npos && (lastSlash == std::string::npos || lastDot > lastSlash)) {
    const auto extension = normalized.substr(lastDot);
    if (extension == ".msg" || extension == ".idl" || extension == ".srv" || extension == ".action") {
      normalized.erase(lastDot);
    }
  }

  const auto first = normalized.find('/');
  if (first == std::string::npos) {
    return normalized;
  }

  const auto second = normalized.find('/', first + 1);
  if (second != std::string::npos) {
    return normalized;
  }

  return normalized.substr(0, first) + "/msg/" + normalized.substr(first + 1);
}

const ros_babel_fish::Message* getMember(const ros_babel_fish::Message& message, const std::string& path) {
  const ros_babel_fish::Message* current = &message;
  for (const auto& part : splitPath(path)) {
    if (current->type() == ros_babel_fish::MessageTypes::Compound) {
      const auto& compound = current->as<ros_babel_fish::CompoundMessage>();
      if (!compound.containsKey(part)) {
        return nullptr;
      }
      current = &compound[part];
      continue;
    }

    if (current->type() == ros_babel_fish::MessageTypes::Array) {
      size_t index = 0;
      try {
        index = static_cast<size_t>(std::stoul(part));
      } catch (const std::exception&) {
        return nullptr;
      }

      const auto* element = compoundArrayAt(*current, index);
      if (element == nullptr) {
        return nullptr;
      }
      current = element;
      continue;
    }

    return nullptr;
  }

  return current;
}

ros_babel_fish::Message* getMember(ros_babel_fish::Message& message, const std::string& path) {
  return const_cast<ros_babel_fish::Message*>(getMember(static_cast<const ros_babel_fish::Message&>(message), path));
}

bool isNumericMessageType(const ros_babel_fish::Message& message) {
  if (isNumericType(message.type())) {
    return true;
  }
  return message.isTime() || message.isDuration();
}

double getNumericValue(const ros_babel_fish::Message& message) {
  if (message.isTime()) {
    return message.value<rclcpp::Time>().seconds();
  }
  if (message.isDuration()) {
    return message.value<rclcpp::Duration>().seconds();
  }
  if (message.type() == ros_babel_fish::MessageTypes::Bool) {
    return message.value<bool>() ? 1.0 : 0.0;
  }
  return message.value<double>();
}

bool tryGetNumericValue(const ros_babel_fish::Message& message, const std::string& path, double& value) {
  const ros_babel_fish::Message* current = &message;
  const auto parts = splitPath(path);

  for (size_t i = 0; i < parts.size(); ++i) {
    const auto& part = parts[i];
    const bool isLast = (i + 1 == parts.size());

    if (current->type() == ros_babel_fish::MessageTypes::Compound) {
      const auto& compound = current->as<ros_babel_fish::CompoundMessage>();
      if (!compound.containsKey(part)) {
        return false;
      }
      current = &compound[part];
      continue;
    }

    if (current->type() == ros_babel_fish::MessageTypes::Array) {
      size_t index = 0;
      try {
        index = static_cast<size_t>(std::stoul(part));
      } catch (const std::exception&) {
        return false;
      }

      const auto* element = compoundArrayAt(*current, index);
      if (element != nullptr) {
        current = element;
        continue;
      }

      return isLast && tryPrimitiveArrayValue(*current, index, value);
    }

    return false;
  }

  if (!isNumericMessageType(*current)) {
    return false;
  }

  value = getNumericValue(*current);
  return true;
}

rclcpp::Time getStamp(const ros_babel_fish::Message& message) {
  if (message.isTime()) {
    return message.value<rclcpp::Time>();
  }
  if (message.type() == ros_babel_fish::MessageTypes::Compound) {
    const auto& compound = message.as<ros_babel_fish::CompoundMessage>();
    if (compound.containsKey("stamp")) {
      return getStamp(compound["stamp"]);
    }
  }
  throw ros_babel_fish::BabelFishException("Message is not a stamp");
}

bool hasHeader(const ros_babel_fish::Message& message) {
  if (message.type() != ros_babel_fish::MessageTypes::Compound) {
    return false;
  }

  const auto& compound = message.as<ros_babel_fish::CompoundMessage>();
  if (!compound.containsKey("header")) {
    return false;
  }

  const auto& header = compound["header"];
  if (header.type() != ros_babel_fish::MessageTypes::Compound) {
    return false;
  }
  return header.as<ros_babel_fish::CompoundMessage>().containsKey("stamp");
}

MessageFieldType fieldTypeFromMessage(const ros_babel_fish::Message& message) {
  if (message.type() == ros_babel_fish::MessageTypes::Compound) {
    const auto& compound = message.as<ros_babel_fish::CompoundMessage>();
    MessageFieldType fieldType;
    fieldType.kind = MessageFieldType::Compound;
    fieldType.identifier = QString::fromStdString(compound.name());
    fieldType.isNumeric = compound.isTime() || compound.isDuration();
    for (const auto& key : compound.keys()) {
      fieldType.members.append(qMakePair(QString::fromStdString(key), fieldTypeFromMessage(compound[key])));
    }
    return fieldType;
  }

  if (message.type() == ros_babel_fish::MessageTypes::Array) {
    const auto& array = message.as<ros_babel_fish::ArrayMessageBase>();
    MessageFieldType fieldType;
    fieldType.kind = MessageFieldType::Array;
    fieldType.isDynamicArray = !array.isFixedSize();
    fieldType.arraySize = array.isFixedSize() ? array.size() : 0;

    const auto* firstElement = compoundArrayAt(message, 0);
    if (firstElement != nullptr) {
      fieldType.elementType = std::make_shared<MessageFieldType>(fieldTypeFromMessage(*firstElement));
    } else if (array.elementType() == ros_babel_fish::MessageTypes::Compound) {
      const auto introspection = compoundArrayElementIntrospection(message);
      if (introspection.operator->() != nullptr) {
        ros_babel_fish::CompoundMessage prototype(introspection);
        fieldType.elementType = std::make_shared<MessageFieldType>(fieldTypeFromMessage(prototype));
      }
    } else {
      fieldType.elementType = std::make_shared<MessageFieldType>(builtinFieldType(array.elementType()));
    }
    if (fieldType.elementType) {
      fieldType.identifier = fieldType.elementType->identifier + "[]";
    }
    return fieldType;
  }

  auto fieldType = builtinFieldType(message.type());
  if (message.isTime() || message.isDuration()) {
    fieldType.isNumeric = true;
  }
  return fieldType;
}

ros_babel_fish::CompoundMessage::SharedPtr createMessagePrototype(const std::string& typeName) {
  return RosContext::fish().create_message_shared(normalizeTypeName(typeName));
}

ros_babel_fish::CompoundMessage::SharedPtr deserializeMessage(const std::string& typeName, const rclcpp::SerializedMessage& serialized) {
  const auto typeSupport = RosContext::fish().get_message_type_support(normalizeTypeName(typeName));
  auto message = ros_babel_fish::CompoundMessage::make_shared(*typeSupport);
  const auto result =
      rmw_deserialize(&serialized.get_rcl_serialized_message(), &typeSupport->type_support_handle, message->type_erased_message().get());
  if (result != RMW_RET_OK) {
    throw ros_babel_fish::BabelFishException("Failed to deserialize message of type " + typeName);
  }
  return message;
}

}  // namespace rqt_multiplot
