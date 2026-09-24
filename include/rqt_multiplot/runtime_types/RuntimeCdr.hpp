#pragma once

#include <cstdint>

#include <fastcdr/Cdr.h>

namespace rqt_multiplot {

namespace runtime_types {

class RuntimeMessageLayout;

// Mirrors the calls of the generated rosidl_typesupport_fastrtps_cpp code, so the XCDRv1 output is byte-identical.
bool serializeMessage(const RuntimeMessageLayout& layout, const void* message, eprosima::fastcdr::Cdr& cdr);
bool deserializeMessage(const RuntimeMessageLayout& layout, eprosima::fastcdr::Cdr& cdr, void* message);
uint32_t serializedMessageSize(const RuntimeMessageLayout& layout, const void* message);

}  // namespace runtime_types

}  // namespace rqt_multiplot
