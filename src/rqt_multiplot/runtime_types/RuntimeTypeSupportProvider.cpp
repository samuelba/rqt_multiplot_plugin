#include "rqt_multiplot/runtime_types/RuntimeTypeSupportProvider.hpp"

#include <utility>

#include <ros_babel_fish/idl/exceptions.hpp>

#include "rqt_multiplot/runtime_types/RuntimeTypeSupport.hpp"
#include "rqt_multiplot/runtime_types/TypeDescriptionClient.hpp"

namespace rqt_multiplot {

namespace runtime_types {

RuntimeTypeSupportProvider::RuntimeTypeSupportProvider(NodeGetter nodeGetter, bool useLocalTypes)
    : nodeGetter_(std::move(nodeGetter)),
      useLocalTypes_(useLocalTypes),
      local_(std::make_shared<ros_babel_fish::LocalTypeSupportProvider>()) {}

void RuntimeTypeSupportProvider::registerDescription(const TypeDescriptionModel& model, const rosidl_type_hash_t* hash) {
  const std::lock_guard<std::mutex> lock(mutex_);
  RegisteredDescription description{model, hash != nullptr, hash == nullptr ? rosidl_type_hash_t{} : *hash};
  registered_.insert_or_assign(model.typeDescription.typeName, std::move(description));
}

bool RuntimeTypeSupportProvider::isLocallyAvailable(const std::string& typeName) const {
  const std::lock_guard<std::mutex> lock(mutex_);
  return getLocalLocked(typeName) != nullptr;
}

ros_babel_fish::MessageTypeSupport::ConstSharedPtr RuntimeTypeSupportProvider::getMessageTypeSupportImpl(const std::string& type) const {
  const std::lock_guard<std::mutex> lock(mutex_);
  const auto it = runtimeTypes_.find(type);
  if (it != runtimeTypes_.end()) {
    return it->second;
  }
  if (useLocalTypes_) {
    auto local = getLocalLocked(type);
    if (local != nullptr) {
      return local;
    }
  }
  return createRuntimeLocked(type);
}

ros_babel_fish::ServiceTypeSupport::ConstSharedPtr RuntimeTypeSupportProvider::getServiceTypeSupportImpl(const std::string& type) const {
  const std::lock_guard<std::mutex> lock(mutex_);
  return local_->getServiceTypeSupport(type);
}

ros_babel_fish::ActionTypeSupport::ConstSharedPtr RuntimeTypeSupportProvider::getActionTypeSupportImpl(const std::string& type) const {
  const std::lock_guard<std::mutex> lock(mutex_);
  return local_->getActionTypeSupport(type);
}

ros_babel_fish::MessageTypeSupport::ConstSharedPtr RuntimeTypeSupportProvider::getLocalLocked(const std::string& type) const {
  try {
    return local_->getMessageTypeSupport(type);
  } catch (const ros_babel_fish::BabelFishException&) {
    return nullptr;
  }
}

ros_babel_fish::MessageTypeSupport::ConstSharedPtr RuntimeTypeSupportProvider::createRuntimeLocked(const std::string& type) const {
  RegisteredDescription description;
  const auto registered = registered_.find(type);
  if (registered != registered_.end()) {
    description = registered->second;
  } else {
    const rclcpp::Node::SharedPtr node = nodeGetter_ ? nodeGetter_() : nullptr;
    if (node == nullptr) {
      throw ros_babel_fish::TypeSupportException("[" + type + "] is not installed and there is no ROS node to ask a publisher for it");
    }
    std::string error;
    auto remote = TypeDescriptionClient(node).fetch(type, error);
    if (!remote) {
      throw ros_babel_fish::TypeSupportException("[" + type + "] is not installed and its description is not available: " + error);
    }
    description = RegisteredDescription{std::move(remote->model), true, remote->hash};
  }
  if (description.model.typeDescription.typeName != type) {
    throw ros_babel_fish::TypeSupportException("description of [" + type + "] describes [" + description.model.typeDescription.typeName +
                                               "]");
  }

  auto typeSupport = std::make_shared<RuntimeTypeSupport>(description.model, description.hasHash ? &description.hash : nullptr);
  auto result = std::make_shared<ros_babel_fish::MessageTypeSupport>();
  result->name = type;
  result->type_support_library = typeSupport;
  result->type_support_handle = *typeSupport->handle();
  result->introspection_type_support_library = typeSupport;
  result->introspection_type_support_handle = *typeSupport->introspectionHandle();
  runtimeTypes_.emplace(type, result);
  return result;
}

}  // namespace runtime_types

}  // namespace rqt_multiplot
