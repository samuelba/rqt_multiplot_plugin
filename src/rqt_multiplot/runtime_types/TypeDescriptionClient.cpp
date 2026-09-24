#include "rqt_multiplot/runtime_types/TypeDescriptionClient.hpp"

#include <algorithm>
#include <set>
#include <utility>

#include <rclcpp/executors/single_threaded_executor.hpp>
#include <type_description_interfaces/srv/get_type_description.hpp>

namespace rqt_multiplot {

namespace runtime_types {

namespace {

using GetTypeDescription = type_description_interfaces::srv::GetTypeDescription;

IndividualTypeModel fromMessage(const type_description_interfaces::msg::IndividualTypeDescription& individual) {
  IndividualTypeModel model;
  model.typeName = individual.type_name;
  model.fields.reserve(individual.fields.size());
  for (const auto& field : individual.fields) {
    FieldModel fieldModel;
    fieldModel.name = field.name;
    fieldModel.type.typeId = field.type.type_id;
    fieldModel.type.capacity = field.type.capacity;
    fieldModel.type.stringCapacity = field.type.string_capacity;
    fieldModel.type.nestedTypeName = field.type.nested_type_name;
    fieldModel.defaultValue = field.default_value;
    model.fields.push_back(std::move(fieldModel));
  }
  return model;
}

std::string fullyQualifiedName(const std::string& nodeNamespace, const std::string& nodeName) {
  if (nodeNamespace.empty() || nodeNamespace.back() == '/') {
    return nodeNamespace + nodeName;
  }
  return nodeNamespace + "/" + nodeName;
}

}  // namespace

TypeDescriptionModel fromMessage(const type_description_interfaces::msg::TypeDescription& description) {
  TypeDescriptionModel model;
  model.typeDescription = fromMessage(description.type_description);
  model.referencedTypeDescriptions.reserve(description.referenced_type_descriptions.size());
  for (const auto& referenced : description.referenced_type_descriptions) {
    model.referencedTypeDescriptions.push_back(fromMessage(referenced));
  }
  return model;
}

TypeDescriptionClient::TypeDescriptionClient(rclcpp::Node::SharedPtr node) : node_(std::move(node)) {}

std::optional<RemoteTypeDescription> TypeDescriptionClient::fetch(const std::string& typeName, std::string& error,
                                                                  std::chrono::milliseconds timeout) const {
  error.clear();
  std::set<std::string> askedNodes;
  for (const auto& [topic, types] : node_->get_topic_names_and_types()) {
    if (std::find(types.begin(), types.end(), typeName) == types.end()) {
      continue;
    }
    for (const auto& endpoint : node_->get_publishers_info_by_topic(topic)) {
      if (endpoint.topic_type() != typeName ||
          !askedNodes.insert(fullyQualifiedName(endpoint.node_namespace(), endpoint.node_name())).second) {
        continue;
      }
      auto description = fetchFrom(endpoint, typeName, error, timeout);
      if (description) {
        return description;
      }
    }
  }
  if (error.empty()) {
    error = "no publisher of [" + typeName + "] found";
  }
  return std::nullopt;
}

std::optional<RemoteTypeDescription> TypeDescriptionClient::fetchFrom(const rclcpp::TopicEndpointInfo& endpoint,
                                                                      const std::string& typeName, std::string& error,
                                                                      std::chrono::milliseconds timeout) const {
  const std::string serviceName = fullyQualifiedName(endpoint.node_namespace(), endpoint.node_name()) + "/get_type_description";
  // A private callback group and executor make this work whether or not another thread spins the node.
  auto group = node_->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive, false);
  auto client = node_->create_client<GetTypeDescription>(serviceName, rclcpp::ServicesQoS(), group);
  if (!client->wait_for_service(timeout)) {
    error = "service [" + serviceName + "] is not available";
    return std::nullopt;
  }

  const bool hasHash = endpoint.topic_type_hash().version != 0;
  auto request = std::make_shared<GetTypeDescription::Request>();
  request->type_name = typeName;
  request->type_hash = hasHash ? typeHashToString(endpoint.topic_type_hash()) : std::string();
  request->include_type_sources = false;
  auto future = client->async_send_request(request);

  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_callback_group(group, node_->get_node_base_interface());
  if (executor.spin_until_future_complete(future, timeout) != rclcpp::FutureReturnCode::SUCCESS) {
    client->remove_pending_request(future);
    error = "service [" + serviceName + "] did not answer in time";
    return std::nullopt;
  }
  const auto response = future.get();
  if (!response->successful) {
    error = "service [" + serviceName + "] failed: " + response->failure_reason;
    return std::nullopt;
  }
  RemoteTypeDescription description{fromMessage(response->type_description), endpoint.topic_type_hash()};
  if (!hasHash) {
    description.hash = calculateTypeHash(description.model);
  }
  return description;
}

}  // namespace runtime_types

}  // namespace rqt_multiplot
