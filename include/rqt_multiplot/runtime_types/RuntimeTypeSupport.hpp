#pragma once

#include <rosidl_runtime_c/message_type_support_struct.h>
#include <rosidl_runtime_c/type_hash.h>
#include <rosidl_typesupport_fastrtps_cpp/message_type_support.h>

#include "rqt_multiplot/runtime_types/RuntimeMessageLayout.hpp"
#include "rqt_multiplot/runtime_types/TypeDescriptionModel.hpp"

namespace rqt_multiplot {

namespace runtime_types {

// Type support for a message type that is not installed. The dispatch handle serves the introspection_cpp handle
// (rmw_cyclonedds_cpp, babel_fish) and our own fastrtps_cpp callbacks (rmw_fastrtps_cpp, rmw_zenoh_cpp).
class RuntimeTypeSupport {
 public:
  RuntimeTypeSupport(TypeDescriptionModel model, const rosidl_type_hash_t* hash);
  ~RuntimeTypeSupport();

  RuntimeTypeSupport(const RuntimeTypeSupport&) = delete;
  RuntimeTypeSupport& operator=(const RuntimeTypeSupport&) = delete;
  RuntimeTypeSupport(RuntimeTypeSupport&&) = delete;
  RuntimeTypeSupport& operator=(RuntimeTypeSupport&&) = delete;

  const rosidl_message_type_support_t* handle() const { return &handle_; }
  const rosidl_message_type_support_t* introspectionHandle() const { return layout_.introspectionHandle(); }
  const rosidl_message_type_support_t* fastrtpsHandle() const { return &fastrtpsHandle_; }
  const RuntimeMessageLayout& layout() const { return layout_; }
  const TypeDescriptionModel& model() const { return model_; }
  const rosidl_type_hash_t& hash() const { return hash_; }
  const rosidl_runtime_c__type_description__TypeDescription& description() const { return cDescription_.get(); }

 private:
  static const rosidl_message_type_support_t* dispatch(const rosidl_message_type_support_t* handle, const char* identifier);

  TypeDescriptionModel model_;
  const RuntimeMessageLayout& layout_;
  rosidl_type_hash_t hash_;
  CTypeDescription cDescription_;
  message_type_support_callbacks_t callbacks_{};
  rosidl_message_type_support_t fastrtpsHandle_{};
  rosidl_message_type_support_t handle_{};
};

// Resolve the RuntimeTypeSupport through handle->data, because babel_fish and rmw copy the handles by value.
const rosidl_type_hash_t* runtimeTypeHash(const rosidl_message_type_support_t* handle);
const rosidl_runtime_c__type_description__TypeDescription* runtimeTypeDescription(const rosidl_message_type_support_t* handle);
const rosidl_runtime_c__type_description__TypeSource__Sequence* runtimeTypeDescriptionSources(const rosidl_message_type_support_t* handle);

}  // namespace runtime_types

}  // namespace rqt_multiplot
