#pragma once

#include <chrono>
#include <optional>
#include <string>

#include <rosidl_runtime_c/type_hash.h>
#include <rclcpp/node.hpp>
#include <type_description_interfaces/msg/type_description.hpp>

#include "rqt_multiplot/runtime_types/TypeDescriptionModel.hpp"

namespace rqt_multiplot {

namespace runtime_types {

struct RemoteTypeDescription {
  TypeDescriptionModel model;
  rosidl_type_hash_t hash;
};

TypeDescriptionModel fromMessage(const type_description_interfaces::msg::TypeDescription& description);

// Asks a publisher of the type for its description through the ~/get_type_description service (REP-2016).
class TypeDescriptionClient {
 public:
  static constexpr std::chrono::milliseconds kDefaultTimeout{2000};

  explicit TypeDescriptionClient(rclcpp::Node::SharedPtr node);

  std::optional<RemoteTypeDescription> fetch(const std::string& typeName, std::string& error,
                                             std::chrono::milliseconds timeout = kDefaultTimeout) const;

 private:
  std::optional<RemoteTypeDescription> fetchFrom(const rclcpp::TopicEndpointInfo& endpoint, const std::string& typeName, std::string& error,
                                                 std::chrono::milliseconds timeout) const;

  rclcpp::Node::SharedPtr node_;
};

}  // namespace runtime_types

}  // namespace rqt_multiplot
