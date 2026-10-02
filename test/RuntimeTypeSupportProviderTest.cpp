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
#include <rclcpp/exceptions.hpp>
#include <rclcpp/rclcpp.hpp>
#include <ros_babel_fish/babel_fish.hpp>
#include <ros_babel_fish/idl/exceptions.hpp>
#include <rosidl_typesupport_cpp/message_type_support.hpp>
#include <rosidl_typesupport_introspection_cpp/message_type_support_decl.hpp>
#include <sensor_msgs/msg/joint_state.hpp>

#include "rqt_multiplot/MessageFieldAccess.hpp"
#include "rqt_multiplot/SerializedSubscription.hpp"
#include "rqt_multiplot/runtime_types/RuntimeTypeSupportProvider.hpp"
#include "rqt_multiplot/runtime_types/TypeDescriptionModel.hpp"

namespace rt = rqt_multiplot::runtime_types;

namespace {

constexpr auto kWaitTimeout = std::chrono::seconds(10);
constexpr auto kPollInterval = std::chrono::milliseconds(20);
// rmw_cyclonedds_cpp can drop the guard-condition wakeup from cancel(), leaving spin() blocked in rmw_wait.
constexpr auto kSpinSlice = std::chrono::milliseconds(50);

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
    executor_ = std::make_shared<rclcpp::executors::SingleThreadedExecutor>();
    executor_->add_node(publisherNode_);
    executor_->add_node(listenerNode_);
    keepSpinning_.store(true);
    spinThread_ = std::thread([this] {
      while (keepSpinning_.load() && rclcpp::ok()) {
        executor_->spin_once(kSpinSlice);
      }
    });
    auto listener = listenerNode_;
    provider_ = std::make_shared<rt::RuntimeTypeSupportProvider>([listener] { return listener; }, false);
    fish_ = std::make_shared<ros_babel_fish::BabelFish>(std::vector<ros_babel_fish::TypeSupportProvider::SharedPtr>{provider_});
  }

  void TearDown() override {
    keepSpinning_.store(false);
    executor_->cancel();
    spinThread_.join();
    executor_->remove_node(publisherNode_);
    executor_->remove_node(listenerNode_);
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
  std::shared_ptr<rclcpp::executors::SingleThreadedExecutor> executor_;
  std::atomic<bool> keepSpinning_{false};
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

TEST_F(RuntimeTypeSupportProviderTest, serializedSubscriptionDecodesTypeFetchedFromPublisher) {
  const std::string topic = "/runtime_types/serialized_joint_state";
  auto publisher = publisherNode_->create_publisher<sensor_msgs::msg::JointState>(topic, 10);
  ASSERT_TRUE(waitFor([&] { return listenerNode_->count_publishers(topic) > 0; }));

  const auto typeSupport = fish_->get_message_type_support("sensor_msgs/msg/JointState");
  ASSERT_NE(nullptr, typeSupport);

  struct Received {
    std::mutex mutex;
    std::vector<double> positions;
    size_t serializedSize = 0;
    std::atomic<bool> done{false};
  };
  // The spin thread can still run the callback after the test body returns.
  auto state = std::make_shared<Received>();
  auto subscription = std::make_shared<rqt_multiplot::SerializedSubscription>(
      listenerNode_->get_node_base_interface().get(), typeSupport, topic, rclcpp::QoS(10),
      [state, typeSupport](const rclcpp::SerializedMessage& serialized) {
        const auto message = rqt_multiplot::deserializeMessage(*typeSupport, serialized);
        double first = 0.0;
        double second = 0.0;
        if (!rqt_multiplot::tryGetNumericValue(*message, "position/0", first) ||
            !rqt_multiplot::tryGetNumericValue(*message, "position/1", second)) {
          return;
        }
        const std::lock_guard<std::mutex> lock(state->mutex);
        state->positions = {first, second};
        state->serializedSize = serialized.size();
        state->done = true;
      });
  listenerNode_->get_node_topics_interface()->add_subscription(subscription, nullptr);

  sensor_msgs::msg::JointState message;
  message.name = {"pan", "tilt"};
  message.position = {0.5, -1.25};
  ASSERT_TRUE(waitFor([&] {
    publisher->publish(message);
    return state->done.load();
  }));

  const std::lock_guard<std::mutex> lock(state->mutex);
  EXPECT_EQ((std::vector<double>{0.5, -1.25}), state->positions);
  EXPECT_GT(state->serializedSize, 0u);
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

TEST_F(RuntimeTypeSupportProviderTest, servesInstalledServiceAndActionTypes) {
  EXPECT_NE(fish_->get_service_type_support("test_msgs/srv/BasicTypes"), nullptr);
  EXPECT_NE(fish_->get_action_type_support("test_msgs/action/Fibonacci"), nullptr);
}

TEST(RuntimeTypeSupportProvider, missingNodeCannotAskAPublisher) {
  if (!rclcpp::ok()) {
    rclcpp::init(0, nullptr);
  }
  auto provider = std::make_shared<rt::RuntimeTypeSupportProvider>([] { return rclcpp::Node::SharedPtr(); }, false);
  ros_babel_fish::BabelFish fish(std::vector<ros_babel_fish::TypeSupportProvider::SharedPtr>{provider});
  EXPECT_THROW(fish.get_message_type_support("not_installed_msgs/msg/Nothing"), ros_babel_fish::TypeSupportException);
}

TEST_F(RuntimeTypeSupportProviderTest, serializedSubscriptionRejectsUnsupportedDelivery) {
  ros_babel_fish::BabelFish localFish;
  const auto typeSupport = localFish.get_message_type_support("std_msgs/msg/Float64");
  ASSERT_NE(typeSupport, nullptr);

  int callbacks = 0;
  auto subscription = std::make_shared<rqt_multiplot::SerializedSubscription>(listenerNode_->get_node_base_interface().get(), typeSupport,
                                                                              "/runtime_types/subscription_stubs", rclcpp::QoS(1),
                                                                              [&](const rclcpp::SerializedMessage&) { ++callbacks; });

  EXPECT_FALSE(subscription->getTypeSupport().name.empty());

  auto first = subscription->create_serialized_message();
  ASSERT_NE(first, nullptr);
  subscription->return_serialized_message(first);
  EXPECT_EQ(first, nullptr);
  auto reused = subscription->create_serialized_message();
  ASSERT_NE(reused, nullptr);

  auto shared = subscription->create_message();
  auto extra = shared;
  subscription->return_message(shared);
  extra.reset();

  rclcpp::MessageInfo info;
  auto delivered = subscription->create_message();
  EXPECT_THROW(subscription->handle_message(delivered, info), rclcpp::exceptions::UnimplementedError);
  EXPECT_THROW(subscription->handle_loaned_message(nullptr, info), rclcpp::exceptions::UnimplementedError);
  EXPECT_THROW(subscription->get_shared_dynamic_message_type(), rclcpp::exceptions::UnimplementedError);
  EXPECT_THROW(subscription->get_shared_dynamic_message(), rclcpp::exceptions::UnimplementedError);
  EXPECT_THROW(subscription->get_shared_dynamic_serialization_support(), rclcpp::exceptions::UnimplementedError);
  EXPECT_THROW(subscription->create_dynamic_message(), rclcpp::exceptions::UnimplementedError);
  rclcpp::dynamic_typesupport::DynamicMessage::SharedPtr dynamicMessage;
  EXPECT_THROW(subscription->return_dynamic_message(dynamicMessage), rclcpp::exceptions::UnimplementedError);
  EXPECT_THROW(subscription->handle_dynamic_message(dynamicMessage, info), rclcpp::exceptions::UnimplementedError);

  std::shared_ptr<rclcpp::SerializedMessage> empty;
  subscription->handle_serialized_message(empty, info);
  EXPECT_EQ(callbacks, 0);
  auto payload = std::make_shared<rclcpp::SerializedMessage>(0);
  subscription->handle_serialized_message(payload, info);
  EXPECT_EQ(callbacks, 1);
}
