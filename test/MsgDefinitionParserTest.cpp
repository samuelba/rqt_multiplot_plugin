#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include <rosidl_typesupport_cpp/message_type_support.hpp>

#include <diagnostic_msgs/msg/diagnostic_array.hpp>
#include <sensor_msgs/msg/joint_state.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <test_msgs/msg/arrays.hpp>
#include <test_msgs/msg/basic_types.hpp>
#include <test_msgs/msg/bounded_sequences.hpp>
#include <test_msgs/msg/empty.hpp>
#include <test_msgs/msg/multi_nested.hpp>
#include <test_msgs/msg/strings.hpp>
#include <test_msgs/msg/unbounded_sequences.hpp>

#include "MessageDefinitionSource.hpp"
#include "rqt_multiplot/runtime_types/MsgDefinitionParser.hpp"
#include "rqt_multiplot/runtime_types/TypeDescriptionModel.hpp"

namespace rt = rqt_multiplot::runtime_types;

namespace {

template <typename M>
void expectParsedHashMatchesGenerated(const std::string& typeName) {
  SCOPED_TRACE(typeName);
  const auto definition = rqt_multiplot::test::fullMessageDefinition(typeName);
  ASSERT_EQ("ros2msg", definition.encoding);

  const rt::TypeDescriptionModel parsed = rt::parseMsgDefinition(typeName, definition.encoded_message_definition);
  const auto* handle = rosidl_typesupport_cpp::get_message_type_support_handle<M>();
  EXPECT_EQ(rt::typeHashToString(*handle->get_type_hash_func(handle)), rt::typeHashToString(rt::calculateTypeHash(parsed)));
}

}  // namespace

TEST(MsgDefinitionParserTest, parsedHashMatchesGeneratedHash) {
  expectParsedHashMatchesGenerated<sensor_msgs::msg::JointState>("sensor_msgs/msg/JointState");
  expectParsedHashMatchesGenerated<sensor_msgs::msg::PointCloud2>("sensor_msgs/msg/PointCloud2");
  expectParsedHashMatchesGenerated<diagnostic_msgs::msg::DiagnosticArray>("diagnostic_msgs/msg/DiagnosticArray");
  expectParsedHashMatchesGenerated<test_msgs::msg::BasicTypes>("test_msgs/msg/BasicTypes");
  expectParsedHashMatchesGenerated<test_msgs::msg::Arrays>("test_msgs/msg/Arrays");
  expectParsedHashMatchesGenerated<test_msgs::msg::BoundedSequences>("test_msgs/msg/BoundedSequences");
  expectParsedHashMatchesGenerated<test_msgs::msg::UnboundedSequences>("test_msgs/msg/UnboundedSequences");
  expectParsedHashMatchesGenerated<test_msgs::msg::MultiNested>("test_msgs/msg/MultiNested");
  expectParsedHashMatchesGenerated<test_msgs::msg::Strings>("test_msgs/msg/Strings");
  expectParsedHashMatchesGenerated<test_msgs::msg::Empty>("test_msgs/msg/Empty");
}

TEST(MsgDefinitionParserTest, ignoresConstantsAndComments) {
  const auto model = rt::parseMsgDefinition("pkg/msg/Reading",
                                            "# comment\n"
                                            "int32 LIMIT = 5\n"
                                            "string label \"a # b\" # trailing comment\n"
                                            "Nested[<=2] items\n"
                                            "================================================================================\n"
                                            "MSG: pkg/Nested\n"
                                            "float64[3] values\n");
  ASSERT_EQ(2u, model.typeDescription.fields.size());
  EXPECT_EQ("label", model.typeDescription.fields[0].name);
  EXPECT_EQ("\"a # b\"", model.typeDescription.fields[0].defaultValue);
  EXPECT_EQ("pkg/msg/Nested", model.typeDescription.fields[1].type.nestedTypeName);
  EXPECT_EQ(rt::ContainerKind::BoundedSequence, rt::containerKind(model.typeDescription.fields[1].type.typeId));
  EXPECT_EQ(2u, model.typeDescription.fields[1].type.capacity);
  ASSERT_EQ(1u, model.referencedTypeDescriptions.size());
  EXPECT_EQ(3u, model.referencedTypeDescriptions[0].fields[0].type.capacity);
}

TEST(MsgDefinitionParserTest, rejectsMissingDependency) {
  EXPECT_THROW(rt::parseMsgDefinition("pkg/msg/Reading", "other_pkg/Missing value\n"), std::invalid_argument);
}
