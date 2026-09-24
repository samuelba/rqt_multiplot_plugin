#include <gtest/gtest.h>

#include <cstring>
#include <memory>
#include <new>
#include <string>
#include <vector>

#include <rmw/rmw.h>
#include <diagnostic_msgs/msg/diagnostic_array.hpp>
#include <rclcpp/serialized_message.hpp>
#include <rosidl_typesupport_cpp/message_type_support.hpp>
#include <rosidl_typesupport_introspection_cpp/identifier.hpp>
#include <rosidl_typesupport_introspection_cpp/message_introspection.hpp>
#include <rosidl_typesupport_introspection_cpp/message_type_support_decl.hpp>
#include <sensor_msgs/msg/joint_state.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <test_msgs/message_fixtures.hpp>

#include "rqt_multiplot/runtime_types/RuntimeMessageLayout.hpp"
#include "rqt_multiplot/runtime_types/RuntimeTypeSupport.hpp"
#include "rqt_multiplot/runtime_types/TypeDescriptionModel.hpp"

namespace rt = rqt_multiplot::runtime_types;
namespace introspection = rosidl_typesupport_introspection_cpp;

namespace {

template <typename M>
rt::TypeDescriptionModel modelOf() {
  const auto* handle = rosidl_typesupport_cpp::get_message_type_support_handle<M>();
  return rt::fromCTypeDescription(*handle->get_type_description_func(handle));
}

template <typename M>
const introspection::MessageMembers& generatedMembers() {
  const auto* handle = introspection::get_message_type_support_handle<M>();
  return *static_cast<const introspection::MessageMembers*>(handle->data);
}

const introspection::MessageMembers& membersOf(const rosidl_message_type_support_t* handle) {
  return *static_cast<const introspection::MessageMembers*>(handle->data);
}

void expectSameMembers(const introspection::MessageMembers& expected, const introspection::MessageMembers& actual) {
  SCOPED_TRACE(std::string(expected.message_namespace_) + "::" + expected.message_name_);
  EXPECT_STREQ(expected.message_namespace_, actual.message_namespace_);
  EXPECT_STREQ(expected.message_name_, actual.message_name_);
  EXPECT_EQ(expected.size_of_, actual.size_of_);
  ASSERT_EQ(expected.member_count_, actual.member_count_);
  for (uint32_t i = 0; i < expected.member_count_; ++i) {
    const auto& expectedMember = expected.members_[i];  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const auto& actualMember = actual.members_[i];      // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    SCOPED_TRACE(expectedMember.name_);
    EXPECT_STREQ(expectedMember.name_, actualMember.name_);
    EXPECT_EQ(expectedMember.type_id_, actualMember.type_id_);
    EXPECT_EQ(expectedMember.string_upper_bound_, actualMember.string_upper_bound_);
    EXPECT_EQ(expectedMember.is_array_, actualMember.is_array_);
    EXPECT_EQ(expectedMember.array_size_, actualMember.array_size_);
    EXPECT_EQ(expectedMember.is_upper_bound_, actualMember.is_upper_bound_);
    EXPECT_EQ(expectedMember.offset_, actualMember.offset_);
    EXPECT_EQ(expectedMember.size_function == nullptr, actualMember.size_function == nullptr);
    EXPECT_EQ(expectedMember.get_const_function == nullptr, actualMember.get_const_function == nullptr);
    EXPECT_EQ(expectedMember.get_function == nullptr, actualMember.get_function == nullptr);
    EXPECT_EQ(expectedMember.fetch_function == nullptr, actualMember.fetch_function == nullptr);
    EXPECT_EQ(expectedMember.assign_function == nullptr, actualMember.assign_function == nullptr);
    EXPECT_EQ(expectedMember.resize_function == nullptr, actualMember.resize_function == nullptr);
#if __has_include(<rosidl_buffer/buffer.hpp>)
    EXPECT_EQ(expectedMember.is_rosidl_buffer_, actualMember.is_rosidl_buffer_);
#endif
    ASSERT_EQ(expectedMember.members_ == nullptr, actualMember.members_ == nullptr);
    if (expectedMember.members_ != nullptr) {
      expectSameMembers(membersOf(expectedMember.members_), membersOf(actualMember.members_));
    }
  }
}

template <typename M>
void expectSameLayout() {
  const rt::TypeDescriptionModel model = modelOf<M>();
  const rt::RuntimeMessageLayout& layout = rt::RuntimeMessageLayout::get(model);
  expectSameMembers(generatedMembers<M>(), layout.members());
}

struct RuntimeMessageDeleter {
  const rt::RuntimeMessageLayout* layout;
  void operator()(void* message) const {
    layout->destroy(message);
    ::operator delete(message, std::align_val_t(layout->alignment()));
  }
};

using RuntimeMessage = std::unique_ptr<void, RuntimeMessageDeleter>;

RuntimeMessage createRuntimeMessage(const rt::RuntimeMessageLayout& layout) {
  void* message = ::operator new(layout.size(), std::align_val_t(layout.alignment()));
  layout.members().init_function(message, rosidl_runtime_cpp::MessageInitialization::ALL);
  return RuntimeMessage(message, RuntimeMessageDeleter{&layout});
}

template <typename M>
void expectRoundTrip(const std::vector<typename M::SharedPtr>& messages) {
  const rt::RuntimeTypeSupport typeSupport(modelOf<M>(), nullptr);
  const auto* generated = rosidl_typesupport_cpp::get_message_type_support_handle<M>();
  ASSERT_FALSE(messages.empty());
  for (size_t i = 0; i < messages.size(); ++i) {
    SCOPED_TRACE("message " + std::to_string(i));
    rclcpp::SerializedMessage expected;
    ASSERT_EQ(RMW_RET_OK, rmw_serialize(messages[i].get(), generated, &expected.get_rcl_serialized_message()));

    RuntimeMessage runtimeMessage = createRuntimeMessage(typeSupport.layout());
    ASSERT_EQ(RMW_RET_OK, rmw_deserialize(&expected.get_rcl_serialized_message(), typeSupport.handle(), runtimeMessage.get()));

    RuntimeMessage copied = createRuntimeMessage(typeSupport.layout());
    typeSupport.layout().copy(copied.get(), runtimeMessage.get());
    runtimeMessage.reset();

    // Padding bytes are not initialized by Fast CDR, so compare decoded messages instead of bytes.
    rclcpp::SerializedMessage actual;
    ASSERT_EQ(RMW_RET_OK, rmw_serialize(copied.get(), typeSupport.handle(), &actual.get_rcl_serialized_message()));
    M decoded;
    ASSERT_EQ(RMW_RET_OK, rmw_deserialize(&actual.get_rcl_serialized_message(), generated, &decoded));
    EXPECT_EQ(*messages[i], decoded);
  }
}

template <typename M>
void expectSameHash() {
  const auto* handle = rosidl_typesupport_cpp::get_message_type_support_handle<M>();
  const rosidl_type_hash_t expected = *handle->get_type_hash_func(handle);
  const rosidl_type_hash_t actual = rt::calculateTypeHash(modelOf<M>());
  EXPECT_EQ(rt::typeHashToString(expected), rt::typeHashToString(actual));
}

sensor_msgs::msg::JointState::SharedPtr makeJointState() {
  auto message = std::make_shared<sensor_msgs::msg::JointState>();
  message->header.stamp.sec = 12;
  message->header.stamp.nanosec = 345;
  message->header.frame_id = "a frame id that is longer than the small string buffer";
  message->name = {"pan", "tilt"};
  message->position = {0.5, -1.25};
  message->velocity = {1.0};
  return message;
}

diagnostic_msgs::msg::DiagnosticArray::SharedPtr makeDiagnosticArray() {
  auto message = std::make_shared<diagnostic_msgs::msg::DiagnosticArray>();
  for (int i = 0; i < 5; ++i) {
    diagnostic_msgs::msg::DiagnosticStatus status;
    status.level = diagnostic_msgs::msg::DiagnosticStatus::WARN;
    status.name = "status with a long name number " + std::to_string(i);
    status.hardware_id = "hw";
    for (int j = 0; j < i; ++j) {
      diagnostic_msgs::msg::KeyValue keyValue;
      keyValue.key = "key " + std::to_string(j);
      keyValue.value = std::to_string(j * 1.5);
      status.values.push_back(keyValue);
    }
    message->status.push_back(status);
  }
  return message;
}

sensor_msgs::msg::PointCloud2::SharedPtr makePointCloud() {
  auto message = std::make_shared<sensor_msgs::msg::PointCloud2>();
  message->height = 1;
  message->width = 3;
  sensor_msgs::msg::PointField field;
  field.name = "x";
  field.datatype = sensor_msgs::msg::PointField::FLOAT32;
  field.count = 1;
  message->fields.push_back(field);
  message->point_step = 4;
  message->row_step = 12;
  message->data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
  message->is_dense = true;
  return message;
}

}  // namespace

TEST(RuntimeMessageLayoutTest, matchesGeneratedIntrospection) {
  expectSameLayout<test_msgs::msg::BasicTypes>();
  expectSameLayout<test_msgs::msg::Arrays>();
  expectSameLayout<test_msgs::msg::BoundedPlainSequences>();
  expectSameLayout<test_msgs::msg::BoundedSequences>();
  expectSameLayout<test_msgs::msg::UnboundedSequences>();
  expectSameLayout<test_msgs::msg::Nested>();
  expectSameLayout<test_msgs::msg::MultiNested>();
  expectSameLayout<test_msgs::msg::Strings>();
  expectSameLayout<test_msgs::msg::WStrings>();
  expectSameLayout<test_msgs::msg::Defaults>();
  expectSameLayout<test_msgs::msg::Empty>();
  expectSameLayout<sensor_msgs::msg::JointState>();
  expectSameLayout<sensor_msgs::msg::PointCloud2>();
  expectSameLayout<diagnostic_msgs::msg::DiagnosticArray>();
}

TEST(RuntimeTypeSupportTest, roundTripsTestMessages) {
  expectRoundTrip<test_msgs::msg::BasicTypes>(get_messages_basic_types());
  expectRoundTrip<test_msgs::msg::Arrays>(get_messages_arrays());
  expectRoundTrip<test_msgs::msg::BoundedPlainSequences>(get_messages_bounded_plain_sequences());
  expectRoundTrip<test_msgs::msg::BoundedSequences>(get_messages_bounded_sequences());
  expectRoundTrip<test_msgs::msg::UnboundedSequences>(get_messages_unbounded_sequences());
  expectRoundTrip<test_msgs::msg::Nested>(get_messages_nested());
  expectRoundTrip<test_msgs::msg::MultiNested>(get_messages_multi_nested());
  expectRoundTrip<test_msgs::msg::Strings>(get_messages_strings());
  expectRoundTrip<test_msgs::msg::WStrings>(get_messages_wstrings());
  expectRoundTrip<test_msgs::msg::Defaults>(get_messages_defaults());
  expectRoundTrip<test_msgs::msg::Empty>(get_messages_empty());
}

TEST(RuntimeTypeSupportTest, roundTripsCommonMessages) {
  expectRoundTrip<sensor_msgs::msg::JointState>({makeJointState()});
  expectRoundTrip<diagnostic_msgs::msg::DiagnosticArray>({makeDiagnosticArray()});
  expectRoundTrip<sensor_msgs::msg::PointCloud2>({makePointCloud()});
}

TEST(RuntimeTypeSupportTest, calculatesGeneratedTypeHash) {
  expectSameHash<test_msgs::msg::MultiNested>();
  expectSameHash<test_msgs::msg::WStrings>();
  expectSameHash<sensor_msgs::msg::JointState>();
  expectSameHash<diagnostic_msgs::msg::DiagnosticArray>();
}

TEST(RuntimeTypeSupportTest, servesHashAndDescriptionThroughCopiedHandles) {
  const rt::RuntimeTypeSupport typeSupport(modelOf<sensor_msgs::msg::JointState>(), nullptr);
  const rosidl_message_type_support_t copy = *typeSupport.handle();
  EXPECT_EQ(&typeSupport.hash(), copy.get_type_hash_func(&copy));
  EXPECT_EQ(&typeSupport.description(), copy.get_type_description_func(&copy));

  const auto* introspectionHandle = get_message_typesupport_handle(&copy, introspection::typesupport_identifier);
  ASSERT_NE(nullptr, introspectionHandle);
  EXPECT_EQ(&typeSupport.hash(), introspectionHandle->get_type_hash_func(introspectionHandle));
  EXPECT_EQ(nullptr, get_message_typesupport_handle(&copy, "rosidl_typesupport_introspection_c"));
}
