#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <rosidl_runtime_c/message_type_support_struct.h>
#include <rosidl_typesupport_introspection_cpp/message_introspection.hpp>

#include "rqt_multiplot/runtime_types/RuntimeSlots.hpp"
#include "rqt_multiplot/runtime_types/RuntimeValueTypes.hpp"
#include "rqt_multiplot/runtime_types/TypeDescriptionModel.hpp"

namespace rqt_multiplot {

namespace runtime_types {

class RuntimeMessageLayout;

struct RuntimeField {
  std::string name;
  uint8_t rosType = 0;
  ContainerKind container = ContainerKind::None;
  size_t arraySize = 0;
  size_t stringUpperBound = 0;
  bool isRosidlBuffer = false;
  const RuntimeMessageLayout* nested = nullptr;
  size_t offset = 0;
};

// Layouts match the memory layout of the generated C++ message structs and live for the whole process.
class RuntimeMessageLayout {
 public:
  static const RuntimeMessageLayout& get(const TypeDescriptionModel& model);
  static const RuntimeMessageLayout& get(const TypeDescriptionModel& model, const std::string& typeName);

  RuntimeMessageLayout(const RuntimeMessageLayout&) = delete;
  RuntimeMessageLayout& operator=(const RuntimeMessageLayout&) = delete;
  RuntimeMessageLayout(RuntimeMessageLayout&&) = delete;
  RuntimeMessageLayout& operator=(RuntimeMessageLayout&&) = delete;
  ~RuntimeMessageLayout() = default;

  const std::string& typeName() const { return typeName_; }
  const std::string& messageNamespace() const { return messageNamespace_; }
  const std::string& messageName() const { return messageName_; }
  size_t size() const { return size_; }
  size_t alignment() const { return alignment_; }
  const std::vector<RuntimeField>& fields() const { return fields_; }
  const rosidl_message_type_support_t* introspectionHandle() const { return &introspectionHandle_; }
  const rosidl_typesupport_introspection_cpp::MessageMembers& members() const { return members_; }
  const MessageSlotFunctions& slotFunctions() const { return *slotFunctions_; }

  void construct(void* message) const;
  void destroy(void* message) const;
  void copy(void* target, const void* source) const;

  size_t sequenceSize(const void* sequence) const;
  static void* sequenceData(void* sequence);
  static const void* sequenceData(const void* sequence);
  void resizeSequence(void* sequence, size_t size) const;
  void* element(void* data, size_t index) const { return offsetPointer(data, index * size_); }
  const void* element(const void* data, size_t index) const { return offsetPointer(data, index * size_); }

 private:
  friend class LayoutRegistry;

  RuntimeMessageLayout(std::string typeName, std::vector<RuntimeField> fields);

  void computeOffsets();
  void buildMembers();

  std::string typeName_;
  std::string messageNamespace_;
  std::string messageName_;
  std::vector<RuntimeField> fields_;
  size_t size_ = 0;
  size_t alignment_ = 1;
  const MessageSlotFunctions* slotFunctions_ = nullptr;
  std::vector<rosidl_typesupport_introspection_cpp::MessageMember> memberArray_;
  rosidl_typesupport_introspection_cpp::MessageMembers members_{};
  rosidl_message_type_support_t introspectionHandle_{};
};

}  // namespace runtime_types

}  // namespace rqt_multiplot
