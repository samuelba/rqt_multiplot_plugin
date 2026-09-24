#include "rqt_multiplot/runtime_types/TypeDescriptionModel.hpp"

#include <stdexcept>

#include <rcl/type_hash.h>
#include <rcutils/allocator.h>
#include <rosidl_runtime_c/string_functions.h>
#include <rosidl_runtime_c/type_description/field__functions.h>
#include <rosidl_runtime_c/type_description/individual_type_description__functions.h>
#include <rosidl_runtime_c/type_description/type_description__functions.h>

namespace rqt_multiplot {

namespace runtime_types {

namespace {

std::string toString(const rosidl_runtime_c__String& value) {
  return value.data == nullptr ? std::string() : std::string(value.data, value.size);
}

void assign(rosidl_runtime_c__String& target, const std::string& value) {
  if (!rosidl_runtime_c__String__assignn(&target, value.c_str(), value.size())) {
    throw std::bad_alloc();
  }
}

IndividualTypeModel fromCIndividual(const rosidl_runtime_c__type_description__IndividualTypeDescription& individual) {
  IndividualTypeModel model;
  model.typeName = toString(individual.type_name);
  model.fields.reserve(individual.fields.size);
  for (size_t i = 0; i < individual.fields.size; ++i) {
    const auto& field = individual.fields.data[i];  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    FieldModel fieldModel;
    fieldModel.name = toString(field.name);
    fieldModel.type.typeId = field.type.type_id;
    fieldModel.type.capacity = field.type.capacity;
    fieldModel.type.stringCapacity = field.type.string_capacity;
    fieldModel.type.nestedTypeName = toString(field.type.nested_type_name);
    fieldModel.defaultValue = toString(field.default_value);
    model.fields.push_back(std::move(fieldModel));
  }
  return model;
}

void toCIndividual(const IndividualTypeModel& model, rosidl_runtime_c__type_description__IndividualTypeDescription& individual) {
  assign(individual.type_name, model.typeName);
  if (!rosidl_runtime_c__type_description__Field__Sequence__init(&individual.fields, model.fields.size())) {
    throw std::bad_alloc();
  }
  for (size_t i = 0; i < model.fields.size(); ++i) {
    const auto& fieldModel = model.fields[i];
    auto& field = individual.fields.data[i];  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    assign(field.name, fieldModel.name);
    field.type.type_id = fieldModel.type.typeId;
    field.type.capacity = fieldModel.type.capacity;
    field.type.string_capacity = fieldModel.type.stringCapacity;
    assign(field.type.nested_type_name, fieldModel.type.nestedTypeName);
    assign(field.default_value, fieldModel.defaultValue);
  }
}

}  // namespace

const IndividualTypeModel* TypeDescriptionModel::find(const std::string& typeName) const {
  if (typeDescription.typeName == typeName) {
    return &typeDescription;
  }
  for (const auto& referenced : referencedTypeDescriptions) {
    if (referenced.typeName == typeName) {
      return &referenced;
    }
  }
  return nullptr;
}

uint8_t baseTypeId(uint8_t typeId) {
  if (typeId == 0) {
    return 0;
  }
  return static_cast<uint8_t>(typeId - (static_cast<uint8_t>(containerKind(typeId)) * field_type::kContainerStride));
}

ContainerKind containerKind(uint8_t typeId) {
  if (typeId == 0) {
    return ContainerKind::None;
  }
  const auto kind = (typeId - 1) / field_type::kContainerStride;
  if (kind > static_cast<int>(ContainerKind::UnboundedSequence)) {
    throw std::invalid_argument("invalid field type id " + std::to_string(typeId));
  }
  return static_cast<ContainerKind>(kind);
}

uint8_t composeTypeId(uint8_t baseTypeId, ContainerKind container) {
  return static_cast<uint8_t>(baseTypeId + (static_cast<uint8_t>(container) * field_type::kContainerStride));
}

TypeDescriptionModel fromCTypeDescription(const rosidl_runtime_c__type_description__TypeDescription& description) {
  TypeDescriptionModel model;
  model.typeDescription = fromCIndividual(description.type_description);
  const auto& referenced = description.referenced_type_descriptions;
  model.referencedTypeDescriptions.reserve(referenced.size);
  for (size_t i = 0; i < referenced.size; ++i) {
    model.referencedTypeDescriptions.push_back(
        fromCIndividual(referenced.data[i]));  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  }
  return model;
}

CTypeDescription::CTypeDescription(const TypeDescriptionModel& model) {
  if (!rosidl_runtime_c__type_description__TypeDescription__init(&description_)) {
    throw std::bad_alloc();
  }
  try {
    toCIndividual(model.typeDescription, description_.type_description);
    auto& referenced = description_.referenced_type_descriptions;
    if (!rosidl_runtime_c__type_description__IndividualTypeDescription__Sequence__init(&referenced,
                                                                                       model.referencedTypeDescriptions.size())) {
      throw std::bad_alloc();
    }
    for (size_t i = 0; i < model.referencedTypeDescriptions.size(); ++i) {
      toCIndividual(model.referencedTypeDescriptions[i], referenced.data[i]);  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
  } catch (...) {
    rosidl_runtime_c__type_description__TypeDescription__fini(&description_);
    throw;
  }
}

CTypeDescription::~CTypeDescription() {
  rosidl_runtime_c__type_description__TypeDescription__fini(&description_);
}

rosidl_type_hash_t calculateTypeHash(const TypeDescriptionModel& model) {
  const CTypeDescription description(model);
  rosidl_type_hash_t hash = rosidl_get_zero_initialized_type_hash();
  // The rosidl_runtime_c and type_description_interfaces structs share one layout by design.
  const auto* interfacesDescription = reinterpret_cast<const type_description_interfaces__msg__TypeDescription*>(&description.get());
  if (rcl_calculate_type_hash(interfacesDescription, &hash) != RCL_RET_OK) {
    throw std::runtime_error("cannot calculate the type hash of [" + model.typeDescription.typeName + "]");
  }
  return hash;
}

std::string typeHashToString(const rosidl_type_hash_t& hash) {
  char* output = nullptr;
  const rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (rosidl_stringify_type_hash(&hash, allocator, &output) != RCUTILS_RET_OK) {
    return {};
  }
  std::string text(output);
  allocator.deallocate(output, allocator.state);
  return text;
}

bool parseTypeHash(const std::string& text, rosidl_type_hash_t& hash) {
  return !text.empty() && rosidl_parse_type_hash_string(text.c_str(), &hash) == RCUTILS_RET_OK;
}

}  // namespace runtime_types

}  // namespace rqt_multiplot
