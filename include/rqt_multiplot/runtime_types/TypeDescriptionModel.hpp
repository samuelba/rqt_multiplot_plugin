#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <rosidl_runtime_c/type_description/type_description__struct.h>
#include <rosidl_runtime_c/type_hash.h>

namespace rqt_multiplot {

namespace runtime_types {

namespace field_type {

constexpr uint8_t kNested = 1;
constexpr uint8_t kInt8 = 2;
constexpr uint8_t kUint8 = 3;
constexpr uint8_t kInt16 = 4;
constexpr uint8_t kUint16 = 5;
constexpr uint8_t kInt32 = 6;
constexpr uint8_t kUint32 = 7;
constexpr uint8_t kInt64 = 8;
constexpr uint8_t kUint64 = 9;
constexpr uint8_t kFloat = 10;
constexpr uint8_t kDouble = 11;
constexpr uint8_t kLongDouble = 12;
constexpr uint8_t kChar = 13;
constexpr uint8_t kWChar = 14;
constexpr uint8_t kBoolean = 15;
constexpr uint8_t kByte = 16;
constexpr uint8_t kString = 17;
constexpr uint8_t kWString = 18;
constexpr uint8_t kFixedString = 19;
constexpr uint8_t kFixedWString = 20;
constexpr uint8_t kBoundedString = 21;
constexpr uint8_t kBoundedWString = 22;
constexpr uint8_t kContainerStride = 48;

}  // namespace field_type

enum class ContainerKind : uint8_t { None = 0, Array = 1, BoundedSequence = 2, UnboundedSequence = 3 };

struct FieldTypeModel {
  uint8_t typeId = 0;
  uint64_t capacity = 0;
  uint64_t stringCapacity = 0;
  std::string nestedTypeName;
};

struct FieldModel {
  std::string name;
  FieldTypeModel type;
  std::string defaultValue;
};

struct IndividualTypeModel {
  std::string typeName;
  std::vector<FieldModel> fields;
};

struct TypeDescriptionModel {
  IndividualTypeModel typeDescription;
  std::vector<IndividualTypeModel> referencedTypeDescriptions;

  const IndividualTypeModel* find(const std::string& typeName) const;
};

uint8_t baseTypeId(uint8_t typeId);
ContainerKind containerKind(uint8_t typeId);
uint8_t composeTypeId(uint8_t baseTypeId, ContainerKind container);

TypeDescriptionModel fromCTypeDescription(const rosidl_runtime_c__type_description__TypeDescription& description);

class CTypeDescription {
 public:
  explicit CTypeDescription(const TypeDescriptionModel& model);
  ~CTypeDescription();

  CTypeDescription(const CTypeDescription&) = delete;
  CTypeDescription& operator=(const CTypeDescription&) = delete;
  CTypeDescription(CTypeDescription&&) = delete;
  CTypeDescription& operator=(CTypeDescription&&) = delete;

  const rosidl_runtime_c__type_description__TypeDescription& get() const { return description_; }

 private:
  rosidl_runtime_c__type_description__TypeDescription description_{};
};

rosidl_type_hash_t calculateTypeHash(const TypeDescriptionModel& model);
std::string typeHashToString(const rosidl_type_hash_t& hash);
bool parseTypeHash(const std::string& text, rosidl_type_hash_t& hash);

}  // namespace runtime_types

}  // namespace rqt_multiplot
