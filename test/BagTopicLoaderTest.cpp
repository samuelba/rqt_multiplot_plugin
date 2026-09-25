#include <chrono>
#include <filesystem>
#include <string>

#include <QApplication>

#include <geometry_msgs/msg/twist.hpp>
#include <rclcpp/time.hpp>
#include <rosbag2_cpp/writer.hpp>
#include <rosbag2_storage/storage_options.hpp>
#include <std_msgs/msg/float64.hpp>

#include <gtest/gtest.h>

#include "rqt_multiplot/BagTopicLoader.hpp"

namespace {

using rqt_multiplot::BagTopicLoader;

std::filesystem::path makeTempDir() {
  const auto path = std::filesystem::temp_directory_path() /
                    ("rqt_multiplot_bag_topics_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::filesystem::create_directories(path);
  return path;
}

void writeBag(const std::string& uri) {
  rosbag2_storage::StorageOptions options;
  options.uri = uri;
  options.storage_id = "mcap";

  rosbag2_cpp::Writer writer;
  writer.open(options);
  writer.write(std_msgs::msg::Float64(), "/float", rclcpp::Time(1, 0, RCL_ROS_TIME));
  writer.write(geometry_msgs::msg::Twist(), "/twist", rclcpp::Time(2, 0, RCL_ROS_TIME));
  writer.close();
}

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

struct SignalCounts {
  int finished = 0;
  int failed = 0;
};

SignalCounts loadAndWait(BagTopicLoader& loader, const QString& fileName) {
  SignalCounts counts;
  QObject::connect(&loader, &BagTopicLoader::loadingFinished, [&counts]() { ++counts.finished; });
  QObject::connect(&loader, &BagTopicLoader::loadingFailed, [&counts](const QString&) { ++counts.failed; });
  loader.load(fileName);
  loader.wait();
  QApplication::processEvents();
  return counts;
}

TEST(BagTopicLoader, loadsTopicsAndTypesFromBag) {
  ensureApplication();
  const auto root = makeTempDir();
  const auto uri = (root / "bag").string();
  writeBag(uri);

  BagTopicLoader loader;
  const SignalCounts counts = loadAndWait(loader, QString::fromStdString(uri));

  EXPECT_EQ(counts.finished, 1);
  const QMap<QString, QString> topics = loader.getTopics();
  EXPECT_EQ(topics.value("/float"), QString("std_msgs/msg/Float64"));
  EXPECT_EQ(topics.value("/twist"), QString("geometry_msgs/msg/Twist"));
  std::filesystem::remove_all(root);
}

TEST(BagTopicLoader, reportsErrorForMissingBag) {
  ensureApplication();
  BagTopicLoader loader;

  const SignalCounts counts = loadAndWait(loader, "/this/path/does/not/exist.mcap");

  EXPECT_EQ(counts.failed, 1);
  EXPECT_TRUE(loader.getTopics().isEmpty());
}

}  // namespace
