#include <chrono>
#include <filesystem>
#include <string>

#include <QApplication>
#include <QHash>

#include <geometry_msgs/msg/pose_array.hpp>
#include <rclcpp/time.hpp>
#include <rosbag2_cpp/writer.hpp>
#include <rosbag2_storage/storage_options.hpp>

#include <gtest/gtest.h>

#include "rqt_multiplot/TopicSampleLoader.hpp"

namespace {

using rqt_multiplot::TopicSampleLoader;

std::filesystem::path makeTempDir() {
  const auto path = std::filesystem::temp_directory_path() /
                    ("rqt_multiplot_topic_sample_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::filesystem::create_directories(path);
  return path;
}

void writePoseArrayBag(const std::string& uri) {
  rosbag2_storage::StorageOptions options;
  options.uri = uri;
  options.storage_id = "mcap";

  geometry_msgs::msg::PoseArray first;
  first.poses.resize(3);
  geometry_msgs::msg::PoseArray second;
  second.poses.resize(5);

  rosbag2_cpp::Writer writer;
  writer.open(options);
  writer.write(first, "/poses", rclcpp::Time(1, 0, RCL_ROS_TIME));
  writer.write(second, "/poses", rclcpp::Time(2, 0, RCL_ROS_TIME));
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

struct SampleResult {
  int sampled = 0;
  int failed = 0;
  QHash<QString, int> lengths;
};

SampleResult sampleBagAndWait(const std::string& uri, const QString& topic) {
  SampleResult result;
  TopicSampleLoader loader;
  QObject::connect(&loader, &TopicSampleLoader::sampled, [&result](const QHash<QString, int>& lengths) {
    ++result.sampled;
    result.lengths = lengths;
  });
  QObject::connect(&loader, &TopicSampleLoader::samplingFailed, [&result](const QString&) { ++result.failed; });
  loader.sampleBag(QString::fromStdString(uri), topic, "geometry_msgs/msg/PoseArray");
  loader.wait();
  QApplication::processEvents();
  return result;
}

TEST(TopicSampleLoader, readsArrayLengthsFromFirstBagMessage) {
  ensureApplication();
  const auto root = makeTempDir();
  const auto uri = (root / "bag").string();
  writePoseArrayBag(uri);

  const SampleResult result = sampleBagAndWait(uri, "/poses");

  EXPECT_EQ(result.sampled, 1);
  EXPECT_EQ(result.failed, 0);
  EXPECT_EQ(result.lengths.value("poses", -1), 3);
  std::filesystem::remove_all(root);
}

TEST(TopicSampleLoader, failsForTopicWithoutMessages) {
  ensureApplication();
  const auto root = makeTempDir();
  const auto uri = (root / "bag").string();
  writePoseArrayBag(uri);

  const SampleResult result = sampleBagAndWait(uri, "/missing");

  EXPECT_EQ(result.sampled, 0);
  EXPECT_EQ(result.failed, 1);
  std::filesystem::remove_all(root);
}

}  // namespace
