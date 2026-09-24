#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

#if __has_include(<rosidl_buffer/buffer.hpp>)
#include <rosidl_buffer/buffer.hpp>
#define RQT_MULTIPLOT_HAS_ROSIDL_BUFFER 1
#endif

#include <rosidl_typesupport_introspection_cpp/field_types.hpp>
#include <rosidl_typesupport_introspection_cpp/message_introspection.hpp>

#include "rqt_multiplot/runtime_types/RuntimeMessageLayout.hpp"
#include "rqt_multiplot/runtime_types/RuntimeValueTypes.hpp"

namespace rqt_multiplot {

namespace runtime_types {

// Since Lyrical, generated code stores unbounded uint8 sequences as rosidl::Buffer instead of std::vector.
inline bool usesRosidlBuffer([[maybe_unused]] uint8_t rosType, [[maybe_unused]] ContainerKind container) {
#ifdef RQT_MULTIPLOT_HAS_ROSIDL_BUFFER
  return rosType == rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8 && container == ContainerKind::UnboundedSequence;
#else
  return false;
#endif
}

// Calls the visitor with the TypeTag of the C++ container that stores a primitive sequence of T.
template <typename T, typename Visitor>
decltype(auto) visitSequenceStorage([[maybe_unused]] const RuntimeField& field, Visitor&& visitor) {
#ifdef RQT_MULTIPLOT_HAS_ROSIDL_BUFFER
  if constexpr (std::is_same_v<T, uint8_t>) {
    if (field.isRosidlBuffer) {
      return visitor(TypeTag<rosidl::Buffer<uint8_t>>{});
    }
  }
#endif
  return visitor(TypeTag<std::vector<T>>{});
}

// Same layout as the libstdc++ std::vector<T> that generated code uses for sequences of nested messages.
struct NestedSequence {
  void* start = nullptr;
  void* finish = nullptr;
  void* endOfStorage = nullptr;
};

static_assert(sizeof(NestedSequence) == sizeof(std::vector<unsigned char>), "unexpected std::vector layout");

inline bool isSequence(ContainerKind container) {
  return container == ContainerKind::BoundedSequence || container == ContainerKind::UnboundedSequence;
}

size_t nestedSequenceSize(const NestedSequence& sequence, size_t elementSize);
void resizeNestedSequence(const RuntimeMessageLayout& layout, NestedSequence& sequence, size_t size);
void destroyNestedSequence(const RuntimeMessageLayout& layout, NestedSequence& sequence);

void constructField(const RuntimeField& field, void* storage);
void destroyField(const RuntimeField& field, void* storage);
void copyField(const RuntimeField& field, void* target, const void* source);
void assignContainerFunctions(const RuntimeField& field, rosidl_typesupport_introspection_cpp::MessageMember& member);

}  // namespace runtime_types

}  // namespace rqt_multiplot
