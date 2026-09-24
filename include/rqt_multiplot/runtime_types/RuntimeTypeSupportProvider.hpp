#pragma once

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include <rosidl_runtime_c/type_hash.h>
#include <rclcpp/node.hpp>
#include <ros_babel_fish/idl/providers/local_type_support_provider.hpp>
#include <ros_babel_fish/idl/type_support_provider.hpp>

#include "rqt_multiplot/runtime_types/TypeDescriptionModel.hpp"

namespace rqt_multiplot {

namespace runtime_types {

// Serves installed types through babel_fish's local provider. Other types come from a registered description (bags)
// or from the ~/get_type_description service of a publisher.
class RuntimeTypeSupportProvider : public ros_babel_fish::TypeSupportProvider {
 public:
  using NodeGetter = std::function<rclcpp::Node::SharedPtr()>;

  explicit RuntimeTypeSupportProvider(NodeGetter nodeGetter, bool useLocalTypes = true);

  void registerDescription(const TypeDescriptionModel& model, const rosidl_type_hash_t* hash = nullptr);
  bool isLocallyAvailable(const std::string& typeName) const;

 protected:
  ros_babel_fish::MessageTypeSupport::ConstSharedPtr getMessageTypeSupportImpl(const std::string& type) const override;
  ros_babel_fish::ServiceTypeSupport::ConstSharedPtr getServiceTypeSupportImpl(const std::string& type) const override;
  ros_babel_fish::ActionTypeSupport::ConstSharedPtr getActionTypeSupportImpl(const std::string& type) const override;

 private:
  struct RegisteredDescription {
    TypeDescriptionModel model;
    bool hasHash = false;
    rosidl_type_hash_t hash{};
  };

  ros_babel_fish::MessageTypeSupport::ConstSharedPtr getLocalLocked(const std::string& type) const;
  ros_babel_fish::MessageTypeSupport::ConstSharedPtr createRuntimeLocked(const std::string& type) const;

  NodeGetter nodeGetter_;
  bool useLocalTypes_;
  mutable std::mutex mutex_;
  std::shared_ptr<ros_babel_fish::LocalTypeSupportProvider> local_;
  mutable std::map<std::string, RegisteredDescription> registered_;
  mutable std::map<std::string, ros_babel_fish::MessageTypeSupport::ConstSharedPtr> runtimeTypes_;
};

}  // namespace runtime_types

}  // namespace rqt_multiplot
