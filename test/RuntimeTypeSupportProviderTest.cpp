#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <rmw/rmw.h>
#include <diagnostic_msgs/msg/diagnostic_array.hpp>
#include <rclcpp/rclcpp.hpp>
#include <ros_babel_fish/babel_fish.hpp>
#include <ros_babel_fish/idl/exceptions.hpp>
#include <rosidl_typesupport_cpp/message_type_support.hpp>
#include <rosidl_typesupport_introspection_cpp/message_type_support_decl.hpp>
#include <sensor_msgs/msg/joint_state.hpp>

#include "rqt_multiplot/MessageFieldAccess.hpp"
#include "rqt_multiplot/runtime_types/RuntimeTypeSupportProvider.hpp"
#include "rqt_multiplot/runtime_types/TypeDescriptionModel.hpp"

namespace rt = rqt_multiplot::runtime_types;

namespace {

constexpr auto kWaitTimeout = std::chrono::seconds(10);
constexpr auto kPollInterval = std::chrono::milliseconds(20);

// rmw_zenoh_cpp aborts when its session is still open during static destruction.
class RclcppShutdownEnvironment : public ::testing::Environment {
 public:
  void TearDown() override {
    if (rclcpp::ok()) {
      rclcpp::shutdown();
    }
  }
};

// NOLINTNEXTLINE(cert-err58-cpp,cppcoreguidelines-owning-memory)
const auto* const kShutdownEnvironment = ::testing::AddGlobalTestEnvironment(new RclcppShutdownEnvironment());

class RuntimeTypeSupportProviderTest : public ::testing::Test {
 protected:
  void SetUp() override {
    if (!rclcpp::ok()) {
      rclcpp::init(0, nullptr);
    }
    publisherNode_ = std::make_shared<rclcpp::Node>("runtime_types_publisher");
    listenerNode_ = std::make_shared<rclcpp::Node>("runtime_types_listener");
    executor_ = std::make_shared<rclcpp::executors::MultiThreadedExecutor>();
    executor_->add_node(publisherNode_);
    executor_->add_node(listenerNode_);
    spinThread_ = std::thread([this] { executor_->spin(); });
    auto listener = listenerNode_;
    provider_ = std::make_shared<rt::RuntimeTypeSupportProvider>([listener] { return listener; }, false);
    fish_ = std::make_shared<ros_babel_fish::BabelFish>(std::vector<ros_babel_fish::TypeSupportProvider::SharedPtr>{provider_});
  }

  void TearDown() override {
    executor_->cancel();
    spinThread_.join();
  }

  template <typename Predicate>
  static bool waitFor(Predicate predicate) {
    const auto deadline = std::chrono::steady_clock::now() + kWaitTimeout;
    while (std::chrono::steady_clock::now() < deadline) {
      if (predicate()) {
        return true;
      }
      std::this_thread::sleep_for(kPollInterval);
    }
    return predicate();
  }

  rclcpp::Node::SharedPtr publisherNode_;
  rclcpp::Node::SharedPtr listenerNode_;
  std::shared_ptr<rclcpp::executors::MultiThreadedExecutor> executor_;
  std::thread spinThread_;
  std::shared_ptr<rt::RuntimeTypeSupportProvider> provider_;
  std::shared_ptr<ros_babel_fish::BabelFish> fish_;
};

}  // namespace

TEST_F(RuntimeTypeSupportProviderTest, decodesTypeFetchedFromPublisher) {
  const std::string topic = "/runtime_types/joint_state";
  const std::string type = "sensor_msgs/msg/JointState";
  auto publisher = publisherNode_->create_publisher<sensor_msgs::msg::JointState>(topic, 10);
  ASSERT_TRUE(waitFor([&] { return listenerNode_->count_publishers(topic) > 0; }));

  const auto typeSupport = fish_->get_message_type_support(type);
  ASSERT_NE(nullptr, typeSupport);
  const auto* generated = rosidl_typesupport_introspection_cpp::get_message_type_support_handle<sensor_msgs::msg::JointState>();
  EXPECT_NE(generated->data, typeSupport->introspection_type_support_handle.data);

  std::mutex mutex;
  std::vector<double> positions;
  std::string frameId;
  std::atomic<bool> received{false};
  auto subscription =
      fish_->create_subscription(*listenerNode_, topic, type, rclcpp::QoS(10), [&](const ros_babel_fish::CompoundMessage& message) {
        double first = 0.0;
        double second = 0.0;
        if (!rqt_multiplot::tryGetNumericValue(message, "position/0", first) ||
            !rqt_multiplot::tryGetNumericValue(message, "position/1", second)) {
          return;
        }
        const std::lock_guard<std::mutex> lock(mutex);
        positions = {first, second};
        frameId = message["header"]["frame_id"].value<std::string>();
        received = true;
      });
  ASSERT_NE(nullptr, subscription);

  sensor_msgs::msg::JointState message;
  message.header.frame_id = "a frame id that does not fit the small string buffer";
  message.name = {"pan", "tilt"};
  message.position = {0.25, -2.5};
  ASSERT_TRUE(waitFor([&] {
    publisher->publish(message);
    return received.load();
  }));

  const std::lock_guard<std::mutex> lock(mutex);
  EXPECT_EQ((std::vector<double>{0.25, -2.5}), positions);
  EXPECT_EQ(message.header.frame_id, frameId);
}

TEST_F(RuntimeTypeSupportProviderTest, decodesRegisteredDescriptionWithoutPublisher) {
  const auto* handle = rosidl_typesupport_cpp::get_message_type_support_handle<diagnostic_msgs::msg::DiagnosticArray>();
  provider_->registerDescription(rt::fromCTypeDescription(*handle->get_type_description_func(handle)));

  diagnostic_msgs::msg::DiagnosticArray message;
  message.status.resize(2);
  message.status[1].name = "second";
  message.status[1].values.resize(1);
  message.status[1].values[0].key = "temperature";
  message.status[1].values[0].value = "42.5";
  rclcpp::SerializedMessage serialized;
  rclcpp::Serialization<diagnostic_msgs::msg::DiagnosticArray>().serialize_message(&message, &serialized);

  const auto typeSupport = fish_->get_message_type_support("diagnostic_msgs/msg/DiagnosticArray");
  ASSERT_NE(nullptr, typeSupport);
  const auto compound = ros_babel_fish::CompoundMessage::make_shared(*typeSupport);
  ASSERT_EQ(RMW_RET_OK, rmw_deserialize(&serialized.get_rcl_serialized_message(), &typeSupport->type_support_handle,
                                        compound->type_erased_message().get()));
  double value = 0.0;
  ASSERT_TRUE(rqt_multiplot::tryGetDiagnosticValue(*compound, "second", "temperature", value));
  EXPECT_DOUBLE_EQ(42.5, value);
}

TEST_F(RuntimeTypeSupportProviderTest, reportsMissingPublisher) {
  const auto start = std::chrono::steady_clock::now();
  EXPECT_THROW(fish_->get_message_type_support("not_installed_msgs/msg/Nothing"), ros_babel_fish::TypeSupportException);
  EXPECT_LT(std::chrono::steady_clock::now() - start, std::chrono::seconds(1));
}
