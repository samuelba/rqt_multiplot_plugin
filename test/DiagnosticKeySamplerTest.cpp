#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

#include <QApplication>
#include <QVector>

#include <diagnostic_msgs/msg/diagnostic_array.hpp>
#include <rclcpp/time.hpp>
#include <rosbag2_cpp/writer.hpp>
#include <rosbag2_storage/storage_options.hpp>

#include <gtest/gtest.h>

#include "rqt_multiplot/DiagnosticKeySampler.hpp"

namespace {

using rqt_multiplot::DiagnosticKeyRef;
using rqt_multiplot::DiagnosticKeySampler;

std::filesystem::path makeTempDir() {
  const auto path = std::filesystem::temp_directory_path() /
                    ("rqt_multiplot_diagnostic_sample_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::filesystem::create_directories(path);
  return path;
}

diagnostic_msgs::msg::DiagnosticStatus makeStatus(const std::string& name, const std::string& hardwareId,
                                                  const std::vector<std::string>& keys) {
  diagnostic_msgs::msg::DiagnosticStatus status;
  status.name = name;
  status.hardware_id = hardwareId;
  for (const auto& key : keys) {
    diagnostic_msgs::msg::KeyValue value;
    value.key = key;
    value.value = "1.0";
    status.values.push_back(value);
  }
  return status;
}

void writeDiagnosticsBag(const std::string& uri) {
  rosbag2_storage::StorageOptions options;
  options.uri = uri;
  options.storage_id = "mcap";

  diagnostic_msgs::msg::DiagnosticArray first;
  first.status.push_back(makeStatus("cpu", "host1", {"load", "temp"}));
  diagnostic_msgs::msg::DiagnosticArray second;
  second.status.push_back(makeStatus("cpu", "host1", {"load"}));
  second.status.push_back(makeStatus("battery", "", {"voltage"}));

  rosbag2_cpp::Writer writer;
  writer.open(options);
  writer.write(first, "/diagnostics", rclcpp::Time(1, 0, RCL_ROS_TIME));
  writer.write(second, "/diagnostics", rclcpp::Time(2, 0, RCL_ROS_TIME));
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
  int changed = 0;
  int failed = 0;
  QVector<DiagnosticKeyRef> keys;
};

SampleResult sampleBagAndWait(const std::string& uri, const QString& topic) {
  SampleResult result;
  DiagnosticKeySampler sampler;
  QObject::connect(&sampler, &DiagnosticKeySampler::keysChanged, [&result](const QVector<DiagnosticKeyRef>& keys) {
    ++result.changed;
    result.keys = keys;
  });
  QObject::connect(&sampler, &DiagnosticKeySampler::samplingFailed, [&result](const QString&) { ++result.failed; });
  sampler.sampleBag({QString::fromStdString(uri)}, topic, "diagnostic_msgs/msg/DiagnosticArray");
  sampler.wait();
  QApplication::processEvents();
  return result;
}

TEST(DiagnosticKeySampler, stopLiveEndsTheSubscriptionUntilSampleLive) {
  ensureApplication();
  DiagnosticKeySampler sampler;

  EXPECT_FALSE(sampler.isSamplingLive());
  sampler.sampleLive(QStringLiteral("/diagnostics"));
  EXPECT_TRUE(sampler.isSamplingLive());

  sampler.stopLive();
  EXPECT_FALSE(sampler.isSamplingLive());

  sampler.sampleLive(QStringLiteral("/diagnostics"));
  EXPECT_TRUE(sampler.isSamplingLive());
}

TEST(DiagnosticKeySampler, bagScanAccumulatesKeysOfAllMessages) {
  ensureApplication();
  const auto root = makeTempDir();
  const auto uri = (root / "bag").string();
  writeDiagnosticsBag(uri);

  const SampleResult result = sampleBagAndWait(uri, "/diagnostics");

  EXPECT_EQ(result.changed, 1);
  EXPECT_EQ(result.failed, 0);
  const QVector<DiagnosticKeyRef> expected = {{"cpu", "host1", "load"}, {"cpu", "host1", "temp"}, {"battery", "", "voltage"}};
  EXPECT_EQ(result.keys, expected);
  std::filesystem::remove_all(root);
}

TEST(DiagnosticKeySampler, bagScanFailsForTopicWithoutMessages) {
  ensureApplication();
  const auto root = makeTempDir();
  const auto uri = (root / "bag").string();
  writeDiagnosticsBag(uri);

  const SampleResult result = sampleBagAndWait(uri, "/missing");

  EXPECT_EQ(result.changed, 0);
  EXPECT_EQ(result.failed, 1);
  std::filesystem::remove_all(root);
}

}  // namespace
