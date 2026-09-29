/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include <utility>

#include <rclcpp/exceptions.hpp>
#include <rclcpp/subscription_options.hpp>

#include "rqt_multiplot/SerializedSubscription.hpp"

namespace rqt_multiplot {

SerializedSubscription::SerializedSubscription(rclcpp::node_interfaces::NodeBaseInterface* nodeBase,
                                               ros_babel_fish::MessageTypeSupport::ConstSharedPtr typeSupport, const std::string& topic,
                                               const rclcpp::QoS& qos, Callback callback)
    : rclcpp::SubscriptionBase(nodeBase, typeSupport->type_support_handle, topic,
                               rclcpp::SubscriptionOptions().to_rcl_subscription_options(qos), rclcpp::SubscriptionEventCallbacks{}, true,
                               rclcpp::DeliveredMessageKind::SERIALIZED_MESSAGE),
      typeSupport_(std::move(typeSupport)),
      callback_(std::move(callback)) {}

const ros_babel_fish::MessageTypeSupport& SerializedSubscription::getTypeSupport() const {
  return *typeSupport_;
}

std::shared_ptr<void> SerializedSubscription::create_message() {
  return create_serialized_message();
}

std::shared_ptr<rclcpp::SerializedMessage> SerializedSubscription::create_serialized_message() {
  const std::lock_guard<std::mutex> lock(spareMutex_);
  if (spare_) {
    return std::exchange(spare_, nullptr);
  }
  return std::make_shared<rclcpp::SerializedMessage>(0);
}

void SerializedSubscription::handle_message(std::shared_ptr<void>& /*message*/, const rclcpp::MessageInfo& /*messageInfo*/) {
  throw rclcpp::exceptions::UnimplementedError("SerializedSubscription only delivers serialized messages");
}

void SerializedSubscription::handle_serialized_message(const std::shared_ptr<rclcpp::SerializedMessage>& serializedMessage,
                                                       const rclcpp::MessageInfo& /*messageInfo*/) {
  if (serializedMessage && callback_) {
    callback_(*serializedMessage);
  }
}

void SerializedSubscription::handle_loaned_message(void* /*loanedMessage*/, const rclcpp::MessageInfo& /*messageInfo*/) {
  throw rclcpp::exceptions::UnimplementedError("SerializedSubscription does not support loaned messages");
}

void SerializedSubscription::return_message(std::shared_ptr<void>& message) {
  auto serialized = std::static_pointer_cast<rclcpp::SerializedMessage>(message);
  return_serialized_message(serialized);
}

void SerializedSubscription::return_serialized_message(std::shared_ptr<rclcpp::SerializedMessage>& message) {
  const std::lock_guard<std::mutex> lock(spareMutex_);
  if (message.use_count() == 1) {
    spare_ = std::move(message);
  }
  message.reset();
}

rclcpp::dynamic_typesupport::DynamicMessageType::SharedPtr SerializedSubscription::get_shared_dynamic_message_type() {
  throw rclcpp::exceptions::UnimplementedError("SerializedSubscription does not support dynamic messages");
}

rclcpp::dynamic_typesupport::DynamicMessage::SharedPtr SerializedSubscription::get_shared_dynamic_message() {
  throw rclcpp::exceptions::UnimplementedError("SerializedSubscription does not support dynamic messages");
}

rclcpp::dynamic_typesupport::DynamicSerializationSupport::SharedPtr SerializedSubscription::get_shared_dynamic_serialization_support() {
  throw rclcpp::exceptions::UnimplementedError("SerializedSubscription does not support dynamic messages");
}

rclcpp::dynamic_typesupport::DynamicMessage::SharedPtr SerializedSubscription::create_dynamic_message() {
  throw rclcpp::exceptions::UnimplementedError("SerializedSubscription does not support dynamic messages");
}

void SerializedSubscription::return_dynamic_message(rclcpp::dynamic_typesupport::DynamicMessage::SharedPtr& /*message*/) {
  throw rclcpp::exceptions::UnimplementedError("SerializedSubscription does not support dynamic messages");
}

void SerializedSubscription::handle_dynamic_message(const rclcpp::dynamic_typesupport::DynamicMessage::SharedPtr& /*message*/,
                                                    const rclcpp::MessageInfo& /*messageInfo*/) {
  throw rclcpp::exceptions::UnimplementedError("SerializedSubscription does not support dynamic messages");
}

}  // namespace rqt_multiplot
