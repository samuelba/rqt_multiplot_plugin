#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include <QApplication>
#include <QCoreApplication>

#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/float64.hpp>

#include "rqt_multiplot/Message.hpp"
#include "rqt_multiplot/MessageSubscriber.hpp"
#include "rqt_multiplot/RosContext.hpp"
#include "rqt_multiplot/runtime_types/MsgDefinitionParser.hpp"

namespace {

using rqt_multiplot::Message;
using rqt_multiplot::MessageSubscriber;
using rqt_multiplot::RosContext;

constexpr auto kWaitTimeout = std::chrono::seconds(3);
constexpr auto kPublishPeriod = std::chrono::milliseconds(50);

QApplication* ensureApplication() {
  if (QApplication::instance() != nullptr) {
    return qobject_cast<QApplication*>(QApplication::instance());
  }
  qputenv("QT_QPA_PLATFORM", "offscreen");
  static int argc = 1;
  static char arg0[] = "test_rqt_multiplot";
  static char* argv[] = {arg0, nullptr};
  return new QApplication(argc, argv);
}

void initRos() {
  if (rclcpp::ok()) {
    return;
  }
  int argc = 1;
  static char arg0[] = "message_subscriber_test";
  static char* argv[] = {arg0, nullptr};
  rclcpp::init(argc, argv);
}

class MessageSubscriberTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ensureApplication();
    initRos();
    node_ = std::make_shared<rclcpp::Node>("message_subscriber_test");
    RosContext::setNode(node_);
    executor_ = std::make_unique<rclcpp::executors::SingleThreadedExecutor>();
    executor_->add_node(node_);
  }

  void TearDown() override {
    executor_.reset();
    RosContext::setNode(nullptr);
    node_.reset();
  }

  bool spinUntil(const std::function<bool()>& condition, const std::function<void()>& onTick = {}) {
    const auto deadline = std::chrono::steady_clock::now() + kWaitTimeout;
    while (std::chrono::steady_clock::now() < deadline) {
      if (onTick) {
        onTick();
      }
      executor_->spin_some();
      QCoreApplication::processEvents(QEventLoop::AllEvents, static_cast<int>(kPublishPeriod.count()));
      if (condition()) {
        return true;
      }
    }
    return false;
  }

  rclcpp::Node::SharedPtr node_;
  std::unique_ptr<rclcpp::executors::SingleThreadedExecutor> executor_;
};

void connectReceiver(MessageSubscriber& subscriber, QObject& receiver, int* received = nullptr) {
  QObject::connect(&subscriber, &MessageSubscriber::messageReceived, &receiver, [received](const QString&, const Message&) {
    if (received != nullptr) {
      ++*received;
    }
  });
}

}  // namespace

TEST_F(MessageSubscriberTest, typedSubscribeSucceedsWithoutPublisher) {
  MessageSubscriber subscriber;
  subscriber.setMessageType("std_msgs/msg/Float64");
  subscriber.setTopic("/message_subscriber_test/typed");
  QObject receiver;
  int received = 0;
  connectReceiver(subscriber, receiver, &received);

  EXPECT_TRUE(subscriber.isValid());

  auto publisher = node_->create_publisher<std_msgs::msg::Float64>("/message_subscriber_test/typed", 10);
  std_msgs::msg::Float64 value;
  value.data = 1.5;
  EXPECT_TRUE(spinUntil([&received]() { return received > 0; }, [&]() { publisher->publish(value); }));
}

TEST_F(MessageSubscriberTest, untypedSubscribeRetriesUntilPublisherAppears) {
  MessageSubscriber subscriber;
  subscriber.setTopic("/message_subscriber_test/untyped");
  QObject receiver;
  connectReceiver(subscriber, receiver);

  EXPECT_FALSE(subscriber.isValid());

  auto publisher = node_->create_publisher<std_msgs::msg::Float64>("/message_subscriber_test/untyped", 10);
  EXPECT_TRUE(spinUntil([&subscriber]() { return subscriber.isValid(); }));
}

TEST_F(MessageSubscriberTest, unknownTypeDoesNotThrow) {
  MessageSubscriber subscriber;
  subscriber.setMessageType("does_not_exist/msg/Foo");
  subscriber.setTopic("/message_subscriber_test/unknown");
  QObject receiver;

  EXPECT_NO_THROW(connectReceiver(subscriber, receiver));
  EXPECT_FALSE(subscriber.isValid());
}

TEST_F(MessageSubscriberTest, typeThatIsNotInstalledSubscribesOnceDescriptionIsAvailable) {
  const std::string type = "rqt_multiplot_not_installed_msgs/msg/Retry";
  MessageSubscriber subscriber;
  subscriber.setMessageType(QString::fromStdString(type));
  subscriber.setTopic("/message_subscriber_test/not_installed");
  QObject receiver;
  connectReceiver(subscriber, receiver);
  EXPECT_FALSE(subscriber.isValid());

  RosContext::typeSupportProvider().registerDescription(rqt_multiplot::runtime_types::parseMsgDefinition(type, "float64 value\n"));
  EXPECT_TRUE(spinUntil([&subscriber]() { return subscriber.isValid(); }));
}
