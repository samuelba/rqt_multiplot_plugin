#include "rqt_multiplot/runtime_types/RuntimeFieldOperations.hpp"

#include <cstdint>
#include <new>
#include <type_traits>

namespace rqt_multiplot {

namespace runtime_types {

namespace {

namespace ft = rosidl_typesupport_introspection_cpp;

uintptr_t address(const void* pointer) {
  return reinterpret_cast<uintptr_t>(pointer);
}

size_t nestedSequenceCapacity(const NestedSequence& sequence, size_t elementSize) {
  return sequence.start == nullptr ? 0 : (address(sequence.endOfStorage) - address(sequence.start)) / elementSize;
}

void releaseNestedSequence(const RuntimeMessageLayout& layout, NestedSequence& sequence, size_t size) {
  for (size_t i = 0; i < size; ++i) {
    layout.destroy(layout.element(sequence.start, i));
  }
  if (sequence.start != nullptr) {
    ::operator delete(sequence.start, std::align_val_t(layout.alignment()));
  }
  sequence = NestedSequence{};
}

size_t elementCount(const RuntimeField& field) {
  return field.container == ContainerKind::Array ? field.arraySize : 1;
}

template <typename Storage>
struct PrimitiveSequenceFunctions {
  using T = typename Storage::value_type;

  static const std::vector<T>& vector(const void* storage) { return *static_cast<const Storage*>(storage); }
  static std::vector<T>& vector(void* storage) { return *static_cast<Storage*>(storage); }

  static size_t size(const void* storage) { return vector(storage).size(); }
  static const void* getConst(const void* storage, size_t index) { return &vector(storage)[index]; }
  static void* get(void* storage, size_t index) { return &vector(storage)[index]; }
  static void fetch(const void* storage, size_t index, void* value) { *static_cast<T*>(value) = vector(storage)[index]; }
  static void assign(void* storage, size_t index, const void* value) { vector(storage)[index] = *static_cast<const T*>(value); }
  static void resize(void* storage, size_t size) { vector(storage).resize(size); }
};

template <typename T>
struct PrimitiveArrayFunctions {
  static const void* getConst(const void* storage, size_t index) { return offsetPointer(storage, index * sizeof(T)); }
  static void* get(void* storage, size_t index) { return offsetPointer(storage, index * sizeof(T)); }
  static void fetch(const void* storage, size_t index, void* value) {
    *static_cast<T*>(value) = *static_cast<const T*>(getConst(storage, index));
  }
  static void assign(void* storage, size_t index, const void* value) {
    *static_cast<T*>(get(storage, index)) = *static_cast<const T*>(value);
  }
};

void assignNestedFunctions(const RuntimeField& field, ft::MessageMember& member) {
  const MessageSlotFunctions& functions = field.nested->slotFunctions();
  if (isSequence(field.container)) {
    member.size_function = functions.sequenceSize;
    member.get_const_function = functions.sequenceGetConst;
    member.get_function = functions.sequenceGet;
    member.fetch_function = functions.sequenceFetch;
    member.assign_function = functions.sequenceAssign;
    member.resize_function = functions.sequenceResize;
    return;
  }
  member.size_function = fixedSizeFunction(field.arraySize);
  member.get_const_function = functions.arrayGetConst;
  member.get_function = functions.arrayGet;
  member.fetch_function = functions.arrayFetch;
  member.assign_function = functions.arrayAssign;
}

}  // namespace

size_t nestedSequenceSize(const NestedSequence& sequence, size_t elementSize) {
  return sequence.start == nullptr ? 0 : (address(sequence.finish) - address(sequence.start)) / elementSize;
}

void resizeNestedSequence(const RuntimeMessageLayout& layout, NestedSequence& sequence, size_t size) {
  const size_t current = nestedSequenceSize(sequence, layout.size());
  if (size <= current) {
    for (size_t i = size; i < current; ++i) {
      layout.destroy(layout.element(sequence.start, i));
    }
    sequence.finish = size == 0 && sequence.start == nullptr ? nullptr : layout.element(sequence.start, size);
    return;
  }
  if (size <= nestedSequenceCapacity(sequence, layout.size())) {
    for (size_t i = current; i < size; ++i) {
      layout.construct(layout.element(sequence.start, i));
    }
    sequence.finish = layout.element(sequence.start, size);
    return;
  }
  // Elements may hold std::string with small-buffer storage, so they are copied and never moved byte by byte.
  void* data = ::operator new(size * layout.size(), std::align_val_t(layout.alignment()));
  size_t constructed = 0;
  try {
    for (; constructed < size; ++constructed) {
      layout.construct(layout.element(data, constructed));
    }
    for (size_t i = 0; i < current; ++i) {
      layout.copy(layout.element(data, i), layout.element(sequence.start, i));
    }
  } catch (...) {
    for (size_t i = 0; i < constructed; ++i) {
      layout.destroy(layout.element(data, i));
    }
    ::operator delete(data, std::align_val_t(layout.alignment()));
    throw;
  }
  releaseNestedSequence(layout, sequence, current);
  sequence.start = data;
  sequence.finish = layout.element(data, size);
  sequence.endOfStorage = sequence.finish;
}

void destroyNestedSequence(const RuntimeMessageLayout& layout, NestedSequence& sequence) {
  releaseNestedSequence(layout, sequence, nestedSequenceSize(sequence, layout.size()));
}

void constructField(const RuntimeField& field, void* storage) {
  if (field.nested != nullptr) {
    if (isSequence(field.container)) {
      new (storage) NestedSequence();
      return;
    }
    for (size_t i = 0; i < elementCount(field); ++i) {
      field.nested->construct(field.nested->element(storage, i));
    }
    return;
  }
  visitValueType(field.rosType, [&field, storage](auto tag) {
    using T = typename decltype(tag)::Type;
    if (isSequence(field.container)) {
      visitSequenceStorage<T>(field, [storage](auto storageTag) { new (storage) typename decltype(storageTag)::Type(); });
      return;
    }
    for (size_t i = 0; i < elementCount(field); ++i) {
      new (offsetPointer(storage, i * sizeof(T))) T();
    }
  });
}

void destroyField(const RuntimeField& field, void* storage) {
  if (field.nested != nullptr) {
    if (isSequence(field.container)) {
      destroyNestedSequence(*field.nested, *static_cast<NestedSequence*>(storage));
      return;
    }
    for (size_t i = 0; i < elementCount(field); ++i) {
      field.nested->destroy(field.nested->element(storage, i));
    }
    return;
  }
  visitValueType(field.rosType, [&field, storage](auto tag) {
    using T = typename decltype(tag)::Type;
    if (isSequence(field.container)) {
      visitSequenceStorage<T>(field, [storage](auto storageTag) {
        using Storage = typename decltype(storageTag)::Type;
        static_cast<Storage*>(storage)->~Storage();
      });
      return;
    }
    if constexpr (!std::is_trivially_destructible_v<T>) {
      for (size_t i = 0; i < elementCount(field); ++i) {
        static_cast<T*>(offsetPointer(storage, i * sizeof(T)))->~T();
      }
    }
  });
}

void copyField(const RuntimeField& field, void* target, const void* source) {
  if (field.nested != nullptr) {
    const RuntimeMessageLayout& nested = *field.nested;
    if (isSequence(field.container)) {
      auto& targetSequence = *static_cast<NestedSequence*>(target);
      const auto& sourceSequence = *static_cast<const NestedSequence*>(source);
      const size_t size = nestedSequenceSize(sourceSequence, nested.size());
      resizeNestedSequence(nested, targetSequence, size);
      for (size_t i = 0; i < size; ++i) {
        nested.copy(nested.element(targetSequence.start, i), nested.element(sourceSequence.start, i));
      }
      return;
    }
    for (size_t i = 0; i < elementCount(field); ++i) {
      nested.copy(nested.element(target, i), nested.element(source, i));
    }
    return;
  }
  visitValueType(field.rosType, [&field, target, source](auto tag) {
    using T = typename decltype(tag)::Type;
    if (isSequence(field.container)) {
      visitSequenceStorage<T>(field, [target, source](auto storageTag) {
        using Storage = typename decltype(storageTag)::Type;
        *static_cast<Storage*>(target) = *static_cast<const Storage*>(source);
      });
      return;
    }
    for (size_t i = 0; i < elementCount(field); ++i) {
      *static_cast<T*>(offsetPointer(target, i * sizeof(T))) = *static_cast<const T*>(offsetPointer(source, i * sizeof(T)));
    }
  });
}

void assignContainerFunctions(const RuntimeField& field, ft::MessageMember& member) {
  if (field.nested != nullptr) {
    assignNestedFunctions(field, member);
    return;
  }
  visitValueType(field.rosType, [&field, &member](auto tag) {
    using T = typename decltype(tag)::Type;
    if (isSequence(field.container)) {
      visitSequenceStorage<T>(field, [&member](auto storageTag) {
        using Functions = PrimitiveSequenceFunctions<typename decltype(storageTag)::Type>;
        member.size_function = &Functions::size;
        if constexpr (!std::is_same_v<T, bool>) {
          member.get_const_function = &Functions::getConst;
          member.get_function = &Functions::get;
        }
        member.fetch_function = &Functions::fetch;
        member.assign_function = &Functions::assign;
        member.resize_function = &Functions::resize;
      });
      return;
    }
    using Functions = PrimitiveArrayFunctions<T>;
    member.size_function = fixedSizeFunction(field.arraySize);
    member.get_const_function = &Functions::getConst;
    member.get_function = &Functions::get;
    member.fetch_function = &Functions::fetch;
    member.assign_function = &Functions::assign;
  });
}

}  // namespace runtime_types

}  // namespace rqt_multiplot
