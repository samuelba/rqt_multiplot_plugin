#include "rqt_multiplot/runtime_types/RuntimeSlots.hpp"

#include <array>
#include <atomic>
#include <map>
#include <mutex>
#include <utility>

#include <ros_babel_fish/exceptions/babel_fish_exception.hpp>

#include "rqt_multiplot/runtime_types/RuntimeCdr.hpp"
#include "rqt_multiplot/runtime_types/RuntimeMessageLayout.hpp"

namespace rqt_multiplot {

namespace runtime_types {

namespace {

std::array<std::atomic<const RuntimeMessageLayout*>, kMessageSlotCount> messageSlots{};
std::atomic<size_t> nextMessageSlot{0};

std::array<std::atomic<size_t>, kFixedSizeSlotCount> fixedSizeSlots{};

template <size_t I>
struct MessageSlot {
  static const RuntimeMessageLayout& layout() { return *messageSlots[I].load(std::memory_order_acquire); }

  static void init(void* message, rosidl_runtime_cpp::MessageInitialization /*initialization*/) { layout().construct(message); }
  static void fini(void* message) { layout().destroy(message); }

  static size_t sequenceSize(const void* sequence) { return layout().sequenceSize(sequence); }
  static const void* sequenceGetConst(const void* sequence, size_t index) {
    return layout().element(RuntimeMessageLayout::sequenceData(sequence), index);
  }
  static void* sequenceGet(void* sequence, size_t index) { return layout().element(RuntimeMessageLayout::sequenceData(sequence), index); }
  static void sequenceFetch(const void* sequence, size_t index, void* value) { layout().copy(value, sequenceGetConst(sequence, index)); }
  static void sequenceAssign(void* sequence, size_t index, const void* value) { layout().copy(sequenceGet(sequence, index), value); }
  static void sequenceResize(void* sequence, size_t size) { layout().resizeSequence(sequence, size); }

  static const void* arrayGetConst(const void* array, size_t index) { return layout().element(array, index); }
  static void* arrayGet(void* array, size_t index) { return layout().element(array, index); }
  static void arrayFetch(const void* array, size_t index, void* value) { layout().copy(value, arrayGetConst(array, index)); }
  static void arrayAssign(void* array, size_t index, const void* value) { layout().copy(arrayGet(array, index), value); }

  static bool cdrSerialize(const void* message, eprosima::fastcdr::Cdr& cdr) { return serializeMessage(layout(), message, cdr); }
  static bool cdrDeserialize(eprosima::fastcdr::Cdr& cdr, void* message) { return deserializeMessage(layout(), cdr, message); }
  static uint32_t serializedSize(const void* message) { return serializedMessageSize(layout(), message); }
};

template <size_t I>
size_t fixedSize(const void* /*array*/) {
  return fixedSizeSlots[I].load(std::memory_order_acquire);
}

template <size_t... I>
std::array<MessageSlotFunctions, sizeof...(I)> makeMessageSlotTable(std::index_sequence<I...> /*indices*/) {
  return {{MessageSlotFunctions{&MessageSlot<I>::init, &MessageSlot<I>::fini, &MessageSlot<I>::sequenceSize,
                                &MessageSlot<I>::sequenceGetConst, &MessageSlot<I>::sequenceGet, &MessageSlot<I>::sequenceFetch,
                                &MessageSlot<I>::sequenceAssign, &MessageSlot<I>::sequenceResize, &MessageSlot<I>::arrayGetConst,
                                &MessageSlot<I>::arrayGet, &MessageSlot<I>::arrayFetch, &MessageSlot<I>::arrayAssign,
                                &MessageSlot<I>::cdrSerialize, &MessageSlot<I>::cdrDeserialize, &MessageSlot<I>::serializedSize}...}};
}

template <size_t... I>
std::array<SizeFunction, sizeof...(I)> makeFixedSizeTable(std::index_sequence<I...> /*indices*/) {
  return {{&fixedSize<I>...}};
}

const std::array<MessageSlotFunctions, kMessageSlotCount>& messageSlotTable() {
  static const auto table = makeMessageSlotTable(std::make_index_sequence<kMessageSlotCount>{});
  return table;
}

const std::array<SizeFunction, kFixedSizeSlotCount>& fixedSizeTable() {
  static const auto table = makeFixedSizeTable(std::make_index_sequence<kFixedSizeSlotCount>{});
  return table;
}

}  // namespace

const MessageSlotFunctions& acquireMessageSlot(const RuntimeMessageLayout& layout) {
  const size_t slot = nextMessageSlot.fetch_add(1);
  if (slot >= kMessageSlotCount) {
    throw ros_babel_fish::BabelFishException("too many runtime message types, the limit is " + std::to_string(kMessageSlotCount));
  }
  messageSlots[slot].store(&layout, std::memory_order_release);
  return messageSlotTable()[slot];
}

SizeFunction fixedSizeFunction(size_t size) {
  static std::mutex mutex;
  static std::map<size_t, SizeFunction> functions;
  const std::lock_guard<std::mutex> lock(mutex);
  const auto it = functions.find(size);
  if (it != functions.end()) {
    return it->second;
  }
  const size_t slot = functions.size();
  if (slot >= kFixedSizeSlotCount) {
    throw ros_babel_fish::BabelFishException("too many distinct runtime array sizes, the limit is " + std::to_string(kFixedSizeSlotCount));
  }
  fixedSizeSlots[slot].store(size, std::memory_order_release);
  const SizeFunction function = fixedSizeTable()[slot];
  functions.emplace(size, function);
  return function;
}

}  // namespace runtime_types

}  // namespace rqt_multiplot
