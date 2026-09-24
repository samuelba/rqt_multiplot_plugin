#include "rqt_multiplot/runtime_types/RuntimeCdr.hpp"

#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include <fastcdr/FastBuffer.h>
#include <rclcpp/logging.hpp>
#include <rosidl_typesupport_fastrtps_cpp/serialization_helpers.hpp>

#include "rqt_multiplot/runtime_types/RuntimeFieldOperations.hpp"
#include "rqt_multiplot/runtime_types/RuntimeMessageLayout.hpp"

namespace rqt_multiplot {

namespace runtime_types {

namespace {

using eprosima::fastcdr::Cdr;

template <typename T>
constexpr bool kIsWide = std::is_same_v<T, char16_t> || std::is_same_v<T, std::u16string>;

template <typename T>
void checkStringBound(const RuntimeField& field, const T& value) {
  if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::u16string>) {
    if (field.stringUpperBound > 0 && value.size() > field.stringUpperBound) {
      throw std::runtime_error("string of field [" + field.name + "] exceeds its upper bound");
    }
  }
}

void checkSequenceBound(const RuntimeField& field, size_t size) {
  if (field.container == ContainerKind::BoundedSequence && size > field.arraySize) {
    throw std::runtime_error("sequence of field [" + field.name + "] exceeds its upper bound");
  }
  if (size > std::numeric_limits<uint32_t>::max()) {
    throw std::overflow_error("sequence of field [" + field.name + "] is too long");
  }
}

template <typename T>
void writeValue(Cdr& cdr, const RuntimeField& field, const T& value) {
  checkStringBound(field, value);
  if constexpr (std::is_same_v<T, char16_t>) {
    cdr << static_cast<wchar_t>(value);
  } else if constexpr (std::is_same_v<T, std::u16string>) {
    rosidl_typesupport_fastrtps_cpp::cdr_serialize(cdr, value);
  } else {
    cdr << value;
  }
}

template <typename T>
void readValue(Cdr& cdr, T& value) {
  if constexpr (std::is_same_v<T, char16_t>) {
    wchar_t wide = 0;
    cdr >> wide;
    value = static_cast<char16_t>(wide);
  } else if constexpr (std::is_same_v<T, std::u16string>) {
    if (!rosidl_typesupport_fastrtps_cpp::cdr_deserialize(cdr, value)) {
      throw std::bad_alloc();
    }
  } else {
    cdr >> value;
  }
}

void serializeFields(const RuntimeMessageLayout& layout, const void* message, Cdr& cdr);
void deserializeFields(const RuntimeMessageLayout& layout, Cdr& cdr, void* message);

void serializeNestedField(const RuntimeField& field, const void* storage, Cdr& cdr) {
  const RuntimeMessageLayout& nested = *field.nested;
  if (isSequence(field.container)) {
    const auto& sequence = *static_cast<const NestedSequence*>(storage);
    const size_t size = nestedSequenceSize(sequence, nested.size());
    checkSequenceBound(field, size);
    cdr << static_cast<uint32_t>(size);
    for (size_t i = 0; i < size; ++i) {
      serializeFields(nested, nested.element(sequence.start, i), cdr);
    }
    return;
  }
  const size_t count = field.container == ContainerKind::Array ? field.arraySize : 1;
  for (size_t i = 0; i < count; ++i) {
    serializeFields(nested, nested.element(storage, i), cdr);
  }
}

void deserializeNestedField(const RuntimeField& field, Cdr& cdr, void* storage) {
  const RuntimeMessageLayout& nested = *field.nested;
  if (isSequence(field.container)) {
    auto& sequence = *static_cast<NestedSequence*>(storage);
    uint32_t size = 0;
    cdr >> size;
    resizeNestedSequence(nested, sequence, size);
    for (size_t i = 0; i < size; ++i) {
      deserializeFields(nested, cdr, nested.element(sequence.start, i));
    }
    return;
  }
  const size_t count = field.container == ContainerKind::Array ? field.arraySize : 1;
  for (size_t i = 0; i < count; ++i) {
    deserializeFields(nested, cdr, nested.element(storage, i));
  }
}

void serializeField(const RuntimeField& field, const void* storage, Cdr& cdr) {
  if (field.nested != nullptr) {
    serializeNestedField(field, storage, cdr);
    return;
  }
  visitValueType(field.rosType, [&field, storage, &cdr](auto tag) {
    using T = typename decltype(tag)::Type;
    if (isSequence(field.container)) {
      const std::vector<T>& values = *visitSequenceStorage<T>(field, [storage](auto storageTag) -> const std::vector<T>* {
        const std::vector<T>& vector = *static_cast<const typename decltype(storageTag)::Type*>(storage);
        return &vector;
      });
      checkSequenceBound(field, values.size());
      if constexpr (kIsWide<T> || std::is_same_v<T, std::string>) {
        cdr << static_cast<uint32_t>(values.size());
        for (const auto& value : values) {
          writeValue(cdr, field, value);
        }
      } else {
        cdr << values;
      }
      return;
    }
    const size_t count = field.container == ContainerKind::Array ? field.arraySize : 1;
    for (size_t i = 0; i < count; ++i) {
      writeValue(cdr, field, *static_cast<const T*>(offsetPointer(storage, i * sizeof(T))));
    }
  });
}

void deserializeField(const RuntimeField& field, Cdr& cdr, void* storage) {
  if (field.nested != nullptr) {
    deserializeNestedField(field, cdr, storage);
    return;
  }
  visitValueType(field.rosType, [&field, storage, &cdr](auto tag) {
    using T = typename decltype(tag)::Type;
    if (isSequence(field.container)) {
      std::vector<T>& values = *visitSequenceStorage<T>(field, [storage](auto storageTag) -> std::vector<T>* {
        std::vector<T>& vector = *static_cast<typename decltype(storageTag)::Type*>(storage);
        return &vector;
      });
      if constexpr (kIsWide<T> || std::is_same_v<T, std::string>) {
        uint32_t size = 0;
        cdr >> size;
        values.resize(size);
        for (auto& value : values) {
          readValue(cdr, value);
        }
      } else {
        cdr >> values;
      }
      return;
    }
    const size_t count = field.container == ContainerKind::Array ? field.arraySize : 1;
    for (size_t i = 0; i < count; ++i) {
      readValue(cdr, *static_cast<T*>(offsetPointer(storage, i * sizeof(T))));
    }
  });
}

void serializeFields(const RuntimeMessageLayout& layout, const void* message, Cdr& cdr) {
  for (const auto& field : layout.fields()) {
    serializeField(field, offsetPointer(message, field.offset), cdr);
  }
}

void deserializeFields(const RuntimeMessageLayout& layout, Cdr& cdr, void* message) {
  for (const auto& field : layout.fields()) {
    deserializeField(field, cdr, offsetPointer(message, field.offset));
  }
}

}  // namespace

bool serializeMessage(const RuntimeMessageLayout& layout, const void* message, Cdr& cdr) {
  try {
    serializeFields(layout, message, cdr);
    return true;
  } catch (const std::exception& exception) {
    RCLCPP_ERROR(rclcpp::get_logger("rqt_multiplot"), "cannot serialize [%s]: %s", layout.typeName().c_str(), exception.what());
    return false;
  }
}

bool deserializeMessage(const RuntimeMessageLayout& layout, Cdr& cdr, void* message) {
  try {
    deserializeFields(layout, cdr, message);
    return true;
  } catch (const std::exception& exception) {
    RCLCPP_ERROR(rclcpp::get_logger("rqt_multiplot"), "cannot deserialize [%s]: %s", layout.typeName().c_str(), exception.what());
    return false;
  }
}

uint32_t serializedMessageSize(const RuntimeMessageLayout& layout, const void* message) {
  eprosima::fastcdr::FastBuffer buffer;
  Cdr cdr(buffer, Cdr::DEFAULT_ENDIAN, eprosima::fastcdr::CdrVersion::XCDRv1);
  if (!serializeMessage(layout, message, cdr)) {
    return 0;
  }
  return static_cast<uint32_t>(cdr.get_serialized_data_length());
}

}  // namespace runtime_types

}  // namespace rqt_multiplot
