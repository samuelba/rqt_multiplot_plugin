#include "rqt_multiplot/runtime_types/RuntimeMessageLayout.hpp"

#include <algorithm>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <utility>

#include <ros_babel_fish/idl/exceptions.hpp>
#include <rosidl_typesupport_introspection_cpp/identifier.hpp>

#include "rqt_multiplot/runtime_types/RuntimeFieldOperations.hpp"
#include "rqt_multiplot/runtime_types/RuntimeTypeSupport.hpp"

namespace rqt_multiplot {

namespace runtime_types {

namespace {

namespace ft = rosidl_typesupport_introspection_cpp;

size_t alignUp(size_t value, size_t alignment) {
  return (value + alignment - 1) / alignment * alignment;
}

uint8_t introspectionType(uint8_t baseType) {
  switch (baseType) {
    case field_type::kNested:
      return ft::ROS_TYPE_MESSAGE;
    case field_type::kInt8:
      return ft::ROS_TYPE_INT8;
    case field_type::kUint8:
      return ft::ROS_TYPE_UINT8;
    case field_type::kInt16:
      return ft::ROS_TYPE_INT16;
    case field_type::kUint16:
      return ft::ROS_TYPE_UINT16;
    case field_type::kInt32:
      return ft::ROS_TYPE_INT32;
    case field_type::kUint32:
      return ft::ROS_TYPE_UINT32;
    case field_type::kInt64:
      return ft::ROS_TYPE_INT64;
    case field_type::kUint64:
      return ft::ROS_TYPE_UINT64;
    case field_type::kFloat:
      return ft::ROS_TYPE_FLOAT;
    case field_type::kDouble:
      return ft::ROS_TYPE_DOUBLE;
    case field_type::kLongDouble:
      return ft::ROS_TYPE_LONG_DOUBLE;
    case field_type::kChar:
      return ft::ROS_TYPE_CHAR;
    case field_type::kWChar:
      return ft::ROS_TYPE_WCHAR;
    case field_type::kBoolean:
      return ft::ROS_TYPE_BOOLEAN;
    case field_type::kByte:
      return ft::ROS_TYPE_OCTET;
    case field_type::kString:
    case field_type::kFixedString:
    case field_type::kBoundedString:
      return ft::ROS_TYPE_STRING;
    case field_type::kWString:
    case field_type::kFixedWString:
    case field_type::kBoundedWString:
      return ft::ROS_TYPE_WSTRING;
    default:
      throw ros_babel_fish::TypeSupportException("unsupported field type id " + std::to_string(baseType));
  }
}

bool hasStringBound(uint8_t baseType) {
  return baseType == field_type::kFixedString || baseType == field_type::kBoundedString || baseType == field_type::kFixedWString ||
         baseType == field_type::kBoundedWString;
}

struct StorageInfo {
  size_t size;
  size_t alignment;
};

StorageInfo storageInfo(const RuntimeField& field) {
  if (field.nested != nullptr) {
    if (isSequence(field.container)) {
      return {sizeof(NestedSequence), alignof(NestedSequence)};
    }
    const size_t count = field.container == ContainerKind::Array ? field.arraySize : 1;
    return {field.nested->size() * count, field.nested->alignment()};
  }
  return visitValueType(field.rosType, [&field](auto tag) -> StorageInfo {
    using T = typename decltype(tag)::Type;
    if (isSequence(field.container)) {
      return visitSequenceStorage<T>(field, [](auto storageTag) -> StorageInfo {
        using Storage = typename decltype(storageTag)::Type;
        return {sizeof(Storage), alignof(Storage)};
      });
    }
    const size_t count = field.container == ContainerKind::Array ? field.arraySize : 1;
    return {sizeof(T) * count, alignof(T)};
  });
}

std::pair<std::string, std::string> splitTypeName(const std::string& typeName) {
  const auto separator = typeName.rfind('/');
  if (separator == std::string::npos) {
    return {std::string(), typeName};
  }
  std::string messageNamespace = typeName.substr(0, separator);
  for (auto position = messageNamespace.find('/'); position != std::string::npos; position = messageNamespace.find('/', position)) {
    messageNamespace.replace(position, 1, "::");
  }
  return {messageNamespace, typeName.substr(separator + 1)};
}

}  // namespace

class LayoutRegistry {
 public:
  static LayoutRegistry& instance() {
    static auto* registry = new LayoutRegistry();  // Leaked on purpose: handles may be used during static destruction.
    return *registry;
  }

  const RuntimeMessageLayout& get(const TypeDescriptionModel& model, const std::string& typeName) {
    const std::lock_guard<std::mutex> lock(mutex_);
    return getLocked(model, typeName);
  }

 private:
  const RuntimeMessageLayout& getLocked(const TypeDescriptionModel& model, const std::string& typeName) {
    const auto it = layouts_.find(typeName);
    if (it != layouts_.end()) {
      return *it->second;
    }
    const IndividualTypeModel* individual = model.find(typeName);
    if (individual == nullptr) {
      throw ros_babel_fish::TypeSupportException("type description of [" + typeName + "] is missing");
    }
    std::vector<RuntimeField> fields;
    fields.reserve(individual->fields.size());
    for (const auto& fieldModel : individual->fields) {
      RuntimeField field;
      field.name = fieldModel.name;
      const uint8_t base = baseTypeId(fieldModel.type.typeId);
      field.rosType = introspectionType(base);
      field.container = containerKind(fieldModel.type.typeId);
      field.arraySize = field.container == ContainerKind::UnboundedSequence ? 0 : fieldModel.type.capacity;
      field.stringUpperBound = hasStringBound(base) ? fieldModel.type.stringCapacity : 0;
      field.isRosidlBuffer = usesRosidlBuffer(field.rosType, field.container);
      if (base == field_type::kNested) {
        field.nested = &getLocked(model, fieldModel.type.nestedTypeName);
      }
      fields.push_back(std::move(field));
    }
    std::unique_ptr<RuntimeMessageLayout> layout(new RuntimeMessageLayout(typeName, std::move(fields)));
    return *layouts_.emplace(typeName, std::move(layout)).first->second;
  }

  std::mutex mutex_;
  std::map<std::string, std::unique_ptr<RuntimeMessageLayout>> layouts_;
};

const RuntimeMessageLayout& RuntimeMessageLayout::get(const TypeDescriptionModel& model) {
  return get(model, model.typeDescription.typeName);
}

const RuntimeMessageLayout& RuntimeMessageLayout::get(const TypeDescriptionModel& model, const std::string& typeName) {
  return LayoutRegistry::instance().get(model, typeName);
}

RuntimeMessageLayout::RuntimeMessageLayout(std::string typeName, std::vector<RuntimeField> fields)
    : typeName_(std::move(typeName)), fields_(std::move(fields)) {
  std::tie(messageNamespace_, messageName_) = splitTypeName(typeName_);
  computeOffsets();
  slotFunctions_ = &acquireMessageSlot(*this);
  buildMembers();
}

void RuntimeMessageLayout::computeOffsets() {
  size_t offset = 0;
  for (auto& field : fields_) {
    const StorageInfo info = storageInfo(field);
    offset = alignUp(offset, info.alignment);
    field.offset = offset;
    offset += info.size;
    alignment_ = std::max(alignment_, info.alignment);
  }
  size_ = std::max<size_t>(alignUp(offset, alignment_), 1);
}

void RuntimeMessageLayout::buildMembers() {
  memberArray_.resize(fields_.size());
  for (size_t i = 0; i < fields_.size(); ++i) {
    const RuntimeField& field = fields_[i];
    ft::MessageMember& member = memberArray_[i];
    member = ft::MessageMember{};
    member.name_ = field.name.c_str();
    member.type_id_ = field.rosType;
    member.string_upper_bound_ = field.stringUpperBound;
    member.members_ = field.nested == nullptr ? nullptr : field.nested->introspectionHandle();
    member.is_array_ = field.container != ContainerKind::None;
    member.array_size_ = field.arraySize;
    member.is_upper_bound_ = field.container == ContainerKind::BoundedSequence;
    member.offset_ = static_cast<uint32_t>(field.offset);
    member.default_value_ = nullptr;
#ifdef RQT_MULTIPLOT_HAS_ROSIDL_BUFFER
    member.is_rosidl_buffer_ = field.isRosidlBuffer;
#endif
    if (member.is_array_) {
      assignContainerFunctions(field, member);
    }
  }
  members_.message_namespace_ = messageNamespace_.c_str();
  members_.message_name_ = messageName_.c_str();
  members_.member_count_ = static_cast<uint32_t>(memberArray_.size());
  members_.size_of_ = size_;
  members_.members_ = memberArray_.data();
  members_.init_function = slotFunctions_->init;
  members_.fini_function = slotFunctions_->fini;

  introspectionHandle_.typesupport_identifier = ft::typesupport_identifier;
  introspectionHandle_.data = &members_;
  introspectionHandle_.func = get_message_typesupport_handle_function;
  introspectionHandle_.get_type_hash_func = &runtimeTypeHash;
  introspectionHandle_.get_type_description_func = &runtimeTypeDescription;
  introspectionHandle_.get_type_description_sources_func = &runtimeTypeDescriptionSources;
}

void RuntimeMessageLayout::construct(void* message) const {
  std::memset(message, 0, size_);
  for (const auto& field : fields_) {
    constructField(field, offsetPointer(message, field.offset));
  }
}

void RuntimeMessageLayout::destroy(void* message) const {
  std::for_each(fields_.rbegin(), fields_.rend(),
                [message](const RuntimeField& field) { destroyField(field, offsetPointer(message, field.offset)); });
}

void RuntimeMessageLayout::copy(void* target, const void* source) const {
  if (target == source) {
    return;
  }
  for (const auto& field : fields_) {
    copyField(field, offsetPointer(target, field.offset), offsetPointer(source, field.offset));
  }
}

size_t RuntimeMessageLayout::sequenceSize(const void* sequence) const {
  return nestedSequenceSize(*static_cast<const NestedSequence*>(sequence), size_);
}

void* RuntimeMessageLayout::sequenceData(void* sequence) {
  return static_cast<NestedSequence*>(sequence)->start;
}

const void* RuntimeMessageLayout::sequenceData(const void* sequence) {
  return static_cast<const NestedSequence*>(sequence)->start;
}

void RuntimeMessageLayout::resizeSequence(void* sequence, size_t size) const {
  resizeNestedSequence(*this, *static_cast<NestedSequence*>(sequence), size);
}

}  // namespace runtime_types

}  // namespace rqt_multiplot
