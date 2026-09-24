#include "rqt_multiplot/runtime_types/RuntimeTypeSupport.hpp"

#include <cstring>
#include <mutex>
#include <unordered_map>
#include <utility>

#include <rosidl_runtime_c/type_description/type_description__functions.h>
#include <rosidl_runtime_c/type_description/type_source__functions.h>
#include <rosidl_typesupport_fastrtps_cpp/identifier.hpp>
#include <rosidl_typesupport_introspection_cpp/identifier.hpp>

namespace rqt_multiplot {

namespace runtime_types {

namespace {

constexpr const char* kDispatchIdentifier = "rosidl_typesupport_cpp";

class HandleRegistry {
 public:
  static HandleRegistry& instance() {
    static auto* registry = new HandleRegistry();  // Leaked on purpose: handles may be used during static destruction.
    return *registry;
  }

  void add(const void* key, const RuntimeTypeSupport* typeSupport) {
    const std::lock_guard<std::mutex> lock(mutex_);
    entries_[key] = typeSupport;
  }

  void remove(const void* key, const RuntimeTypeSupport* typeSupport) {
    const std::lock_guard<std::mutex> lock(mutex_);
    const auto it = entries_.find(key);
    if (it != entries_.end() && it->second == typeSupport) {
      entries_.erase(it);
    }
  }

  const RuntimeTypeSupport* find(const rosidl_message_type_support_t* handle) {
    if (handle == nullptr) {
      return nullptr;
    }
    const std::lock_guard<std::mutex> lock(mutex_);
    const auto it = entries_.find(handle->data);
    return it == entries_.end() ? nullptr : it->second;
  }

 private:
  std::mutex mutex_;
  std::unordered_map<const void*, const RuntimeTypeSupport*> entries_;
};

size_t maxSerializedSize(char& boundsInfo) {
  boundsInfo = ROSIDL_TYPESUPPORT_FASTRTPS_UNBOUNDED_TYPE;
  return 0;
}

const rosidl_runtime_c__type_description__TypeDescription& emptyDescription() {
  static const auto* description = rosidl_runtime_c__type_description__TypeDescription__create();
  return *description;
}

const rosidl_runtime_c__type_description__TypeSource__Sequence& emptySources() {
  static const auto* sources = rosidl_runtime_c__type_description__TypeSource__Sequence__create(0);
  return *sources;
}

}  // namespace

RuntimeTypeSupport::RuntimeTypeSupport(TypeDescriptionModel model, const rosidl_type_hash_t* hash)
    : model_(std::move(model)),
      layout_(RuntimeMessageLayout::get(model_)),
      hash_(hash == nullptr ? calculateTypeHash(model_) : *hash),
      cDescription_(model_) {
  const MessageSlotFunctions& functions = layout_.slotFunctions();
  callbacks_.message_namespace_ = layout_.messageNamespace().c_str();
  callbacks_.message_name_ = layout_.messageName().c_str();
  callbacks_.cdr_serialize = functions.cdrSerialize;
  callbacks_.cdr_deserialize = functions.cdrDeserialize;
  callbacks_.get_serialized_size = functions.serializedSize;
  callbacks_.max_serialized_size = &maxSerializedSize;

  fastrtpsHandle_.typesupport_identifier = rosidl_typesupport_fastrtps_cpp::typesupport_identifier;
  fastrtpsHandle_.data = &callbacks_;
  fastrtpsHandle_.func = get_message_typesupport_handle_function;
  fastrtpsHandle_.get_type_hash_func = &runtimeTypeHash;
  fastrtpsHandle_.get_type_description_func = &runtimeTypeDescription;
  fastrtpsHandle_.get_type_description_sources_func = &runtimeTypeDescriptionSources;

  handle_.typesupport_identifier = kDispatchIdentifier;
  handle_.data = this;
  handle_.func = &RuntimeTypeSupport::dispatch;
  handle_.get_type_hash_func = &runtimeTypeHash;
  handle_.get_type_description_func = &runtimeTypeDescription;
  handle_.get_type_description_sources_func = &runtimeTypeDescriptionSources;

  auto& registry = HandleRegistry::instance();
  registry.add(this, this);
  registry.add(&callbacks_, this);
  registry.add(introspectionHandle()->data, this);
}

RuntimeTypeSupport::~RuntimeTypeSupport() {
  auto& registry = HandleRegistry::instance();
  registry.remove(this, this);
  registry.remove(&callbacks_, this);
  registry.remove(introspectionHandle()->data, this);
}

const rosidl_message_type_support_t* RuntimeTypeSupport::dispatch(const rosidl_message_type_support_t* handle, const char* identifier) {
  if (handle == nullptr || identifier == nullptr) {
    return nullptr;
  }
  const auto* self = static_cast<const RuntimeTypeSupport*>(handle->data);
  if (std::strcmp(identifier, rosidl_typesupport_introspection_cpp::typesupport_identifier) == 0) {
    return self->introspectionHandle();
  }
  if (std::strcmp(identifier, rosidl_typesupport_fastrtps_cpp::typesupport_identifier) == 0) {
    return &self->fastrtpsHandle_;
  }
  return nullptr;
}

const rosidl_type_hash_t* runtimeTypeHash(const rosidl_message_type_support_t* handle) {
  static const rosidl_type_hash_t zeroHash = rosidl_get_zero_initialized_type_hash();
  const RuntimeTypeSupport* typeSupport = HandleRegistry::instance().find(handle);
  return typeSupport == nullptr ? &zeroHash : &typeSupport->hash();
}

const rosidl_runtime_c__type_description__TypeDescription* runtimeTypeDescription(const rosidl_message_type_support_t* handle) {
  const RuntimeTypeSupport* typeSupport = HandleRegistry::instance().find(handle);
  return typeSupport == nullptr ? &emptyDescription() : &typeSupport->description();
}

const rosidl_runtime_c__type_description__TypeSource__Sequence* runtimeTypeDescriptionSources(
    const rosidl_message_type_support_t* /*handle*/) {
  return &emptySources();
}

}  // namespace runtime_types

}  // namespace rqt_multiplot
