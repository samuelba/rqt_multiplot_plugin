#pragma once

#include <cstddef>
#include <cstdint>

#include <rosidl_runtime_cpp/message_initialization.hpp>

namespace eprosima {

namespace fastcdr {

class Cdr;

}  // namespace fastcdr

}  // namespace eprosima

namespace rqt_multiplot {

namespace runtime_types {

class RuntimeMessageLayout;

// The rosidl callbacks get no context pointer, so every runtime layout gets its own set of functions from a fixed pool.
struct MessageSlotFunctions {
  void (*init)(void*, rosidl_runtime_cpp::MessageInitialization);
  void (*fini)(void*);
  size_t (*sequenceSize)(const void*);
  const void* (*sequenceGetConst)(const void*, size_t);
  void* (*sequenceGet)(void*, size_t);
  void (*sequenceFetch)(const void*, size_t, void*);
  void (*sequenceAssign)(void*, size_t, const void*);
  void (*sequenceResize)(void*, size_t);
  const void* (*arrayGetConst)(const void*, size_t);
  void* (*arrayGet)(void*, size_t);
  void (*arrayFetch)(const void*, size_t, void*);
  void (*arrayAssign)(void*, size_t, const void*);
  bool (*cdrSerialize)(const void*, eprosima::fastcdr::Cdr&);
  bool (*cdrDeserialize)(eprosima::fastcdr::Cdr&, void*);
  uint32_t (*serializedSize)(const void*);
};

using SizeFunction = size_t (*)(const void*);

constexpr size_t kMessageSlotCount = 512;
constexpr size_t kFixedSizeSlotCount = 256;

const MessageSlotFunctions& acquireMessageSlot(const RuntimeMessageLayout& layout);
SizeFunction fixedSizeFunction(size_t size);

}  // namespace runtime_types

}  // namespace rqt_multiplot
