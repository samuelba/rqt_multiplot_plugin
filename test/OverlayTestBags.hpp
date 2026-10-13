#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>

#include <rclcpp/time.hpp>
#include <rosbag2_cpp/writer.hpp>
#include <rosbag2_storage/storage_options.hpp>
#include <std_msgs/msg/float64.hpp>

#include <QApplication>
#include <QString>

namespace rqt_multiplot::test {

inline QApplication* ensureOverlayTestApplication() {
  if (QApplication::instance() != nullptr) {
    return qobject_cast<QApplication*>(QApplication::instance());
  }
  qputenv("QT_QPA_PLATFORM", "offscreen");
  static int argc = 1;
  static char arg0[] = "test_rqt_multiplot";
  static char* argv[] = {arg0, nullptr};
  return new QApplication(argc, argv);
}

class TempBagDirectory {
 public:
  TempBagDirectory()
      : root_(std::filesystem::temp_directory_path() /
              ("rqt_multiplot_overlay_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))) {
    std::filesystem::create_directories(root_);
  }
  ~TempBagDirectory() { std::filesystem::remove_all(root_); }

  TempBagDirectory(const TempBagDirectory&) = delete;
  TempBagDirectory& operator=(const TempBagDirectory&) = delete;
  TempBagDirectory(TempBagDirectory&&) = delete;
  TempBagDirectory& operator=(TempBagDirectory&&) = delete;

  // Writes /float messages `value` at startSec and `value + 1` at startSec + 1.
  QString writeFloatBag(const std::string& name, int64_t startSec, double value) const {
    const auto uri = (root_ / name).string();
    rosbag2_storage::StorageOptions options;
    options.uri = uri;
    options.storage_id = "mcap";

    rosbag2_cpp::Writer writer;
    writer.open(options);
    std_msgs::msg::Float64 message;
    message.data = value;
    writer.write(message, "/float", rclcpp::Time(startSec, 0, RCL_ROS_TIME));
    message.data = value + 1.0;
    writer.write(message, "/float", rclcpp::Time(startSec + 1, 0, RCL_ROS_TIME));
    writer.close();
    return QString::fromStdString(uri);
  }

 private:
  std::filesystem::path root_;
};

}  // namespace rqt_multiplot::test
