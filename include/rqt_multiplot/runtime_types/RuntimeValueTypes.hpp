#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

#include <rosidl_typesupport_introspection_cpp/field_types.hpp>

namespace rqt_multiplot {

namespace runtime_types {

template <typename T>
struct TypeTag {
  using Type = T;
};

template <typename Visitor>
decltype(auto) visitValueType(uint8_t rosType, Visitor&& visitor) {
  namespace ft = rosidl_typesupport_introspection_cpp;
  switch (rosType) {
    case ft::ROS_TYPE_FLOAT:
      return visitor(TypeTag<float>{});
    case ft::ROS_TYPE_DOUBLE:
      return visitor(TypeTag<double>{});
    case ft::ROS_TYPE_LONG_DOUBLE:
      return visitor(TypeTag<long double>{});
    case ft::ROS_TYPE_CHAR:
    case ft::ROS_TYPE_OCTET:
      return visitor(TypeTag<unsigned char>{});
    case ft::ROS_TYPE_WCHAR:
      return visitor(TypeTag<char16_t>{});
    case ft::ROS_TYPE_BOOLEAN:
      return visitor(TypeTag<bool>{});
    case ft::ROS_TYPE_UINT8:
      return visitor(TypeTag<uint8_t>{});
    case ft::ROS_TYPE_INT8:
      return visitor(TypeTag<int8_t>{});
    case ft::ROS_TYPE_UINT16:
      return visitor(TypeTag<uint16_t>{});
    case ft::ROS_TYPE_INT16:
      return visitor(TypeTag<int16_t>{});
    case ft::ROS_TYPE_UINT32:
      return visitor(TypeTag<uint32_t>{});
    case ft::ROS_TYPE_INT32:
      return visitor(TypeTag<int32_t>{});
    case ft::ROS_TYPE_UINT64:
      return visitor(TypeTag<uint64_t>{});
    case ft::ROS_TYPE_INT64:
      return visitor(TypeTag<int64_t>{});
    case ft::ROS_TYPE_STRING:
      return visitor(TypeTag<std::string>{});
    case ft::ROS_TYPE_WSTRING:
      return visitor(TypeTag<std::u16string>{});
    default:
      throw std::invalid_argument("unsupported introspection type id " + std::to_string(rosType));
  }
}

inline void* offsetPointer(void* base, size_t offset) {
  return static_cast<unsigned char*>(base) + offset;  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
}

inline const void* offsetPointer(const void* base, size_t offset) {
  return static_cast<const unsigned char*>(base) + offset;  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
}

}  // namespace runtime_types

}  // namespace rqt_multiplot
