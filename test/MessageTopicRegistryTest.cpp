#include <QApplication>
#include <QCoreApplication>

#include <gtest/gtest.h>
#include <rclcpp/rclcpp.hpp>

#include <rqt_multiplot/MessageTopicRegistry.h>
#include <rqt_multiplot/RosContext.h>

namespace {

using rqt_multiplot::MessageTopicRegistry;
using rqt_multiplot::RosContext;

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
  static char arg0[] = "message_topic_registry_test";
  static char* argv[] = {arg0, nullptr};
  rclcpp::init(argc, argv);
}

class MessageTopicRegistryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ensureApplication();
    MessageTopicRegistry::wait();
  }

  void TearDown() override {
    MessageTopicRegistry::wait();
    RosContext::setNode(nullptr);
  }
};

}  // namespace

TEST_F(MessageTopicRegistryTest, updateCompletesWithValidNode) {
  initRos();
  auto node = std::make_shared<rclcpp::Node>("message_topic_registry_test");
  RosContext::setNode(node);

  MessageTopicRegistry::update();
  MessageTopicRegistry::wait();

  EXPECT_FALSE(MessageTopicRegistry::isUpdating());
}

TEST_F(MessageTopicRegistryTest, updateAfterShutdownDoesNotAbort) {
  initRos();
  auto node = std::make_shared<rclcpp::Node>("message_topic_registry_shutdown_test");
  RosContext::setNode(node);
  rclcpp::shutdown();

  EXPECT_NO_THROW({
    MessageTopicRegistry::update();
    MessageTopicRegistry::wait();
  });
  EXPECT_FALSE(MessageTopicRegistry::isUpdating());
}
