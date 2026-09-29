/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include <rclcpp/node_interfaces/node_base_interface.hpp>
#include <rclcpp/qos.hpp>
#include <rclcpp/serialized_message.hpp>
#include <rclcpp/subscription_base.hpp>
#include <ros_babel_fish/idl/type_support.hpp>

namespace rqt_multiplot {

// Like rclcpp::GenericSubscription, but takes the BabelFish type support, so it also works for types that are not installed.
class SerializedSubscription : public rclcpp::SubscriptionBase {
 public:
  RCLCPP_SMART_PTR_DEFINITIONS(SerializedSubscription)

  using Callback = std::function<void(const rclcpp::SerializedMessage&)>;

  SerializedSubscription(rclcpp::node_interfaces::NodeBaseInterface* nodeBase,
                         ros_babel_fish::MessageTypeSupport::ConstSharedPtr typeSupport, const std::string& topic, const rclcpp::QoS& qos,
                         Callback callback);

  const ros_babel_fish::MessageTypeSupport& getTypeSupport() const;

  std::shared_ptr<void> create_message() override;
  std::shared_ptr<rclcpp::SerializedMessage> create_serialized_message() override;
  void handle_message(std::shared_ptr<void>& message, const rclcpp::MessageInfo& messageInfo) override;
  void handle_serialized_message(const std::shared_ptr<rclcpp::SerializedMessage>& serializedMessage,
                                 const rclcpp::MessageInfo& messageInfo) override;
  void handle_loaned_message(void* loanedMessage, const rclcpp::MessageInfo& messageInfo) override;
  void return_message(std::shared_ptr<void>& message) override;
  void return_serialized_message(std::shared_ptr<rclcpp::SerializedMessage>& message) override;

  rclcpp::dynamic_typesupport::DynamicMessageType::SharedPtr get_shared_dynamic_message_type() override;
  rclcpp::dynamic_typesupport::DynamicMessage::SharedPtr get_shared_dynamic_message() override;
  rclcpp::dynamic_typesupport::DynamicSerializationSupport::SharedPtr get_shared_dynamic_serialization_support() override;
  rclcpp::dynamic_typesupport::DynamicMessage::SharedPtr create_dynamic_message() override;
  void return_dynamic_message(rclcpp::dynamic_typesupport::DynamicMessage::SharedPtr& message) override;
  void handle_dynamic_message(const rclcpp::dynamic_typesupport::DynamicMessage::SharedPtr& message,
                              const rclcpp::MessageInfo& messageInfo) override;

 private:
  ros_babel_fish::MessageTypeSupport::ConstSharedPtr typeSupport_;
  Callback callback_;
  // Reused between messages so the receive buffer keeps its capacity.
  std::shared_ptr<rclcpp::SerializedMessage> spare_;
  std::mutex spareMutex_;
};

}  // namespace rqt_multiplot
