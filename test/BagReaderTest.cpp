#include <chrono>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include <geometry_msgs/msg/twist.hpp>
#include <rclcpp/serialized_message.hpp>
#include <rclcpp/time.hpp>
#include <rosbag2_cpp/reader.hpp>
#include <rosbag2_cpp/writer.hpp>
#include <rosbag2_storage/storage_options.hpp>
#include <std_msgs/msg/float64.hpp>

#include <gtest/gtest.h>

#include <rqt_multiplot/BagOpen.h>
#include <rqt_multiplot/MessageFieldAccess.h>

namespace {

using rqt_multiplot::deserializeMessage;
using rqt_multiplot::getMember;
using rqt_multiplot::getNumericValue;

struct ExtractedPoint {
  std::string topic;
  std::string path;
  double value;
};

std::filesystem::path makeTempDir() {
  const auto path = std::filesystem::temp_directory_path() /
                    ("rqt_multiplot_bag_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::filesystem::create_directories(path);
  return path;
}

void writeFixtureBag(const std::string& uri, const std::string& storageId) {
  rosbag2_storage::StorageOptions options;
  options.uri = uri;
  options.storage_id = storageId;

  rosbag2_cpp::Writer writer;
  writer.open(options);

  std_msgs::msg::Float64 value;
  value.data = 1.5;
  writer.write(value, "/float", rclcpp::Time(1, 0, RCL_ROS_TIME));
  value.data = 2.5;
  writer.write(value, "/float", rclcpp::Time(2, 0, RCL_ROS_TIME));

  geometry_msgs::msg::Twist twist;
  twist.linear.x = 3.25;
  twist.angular.z = -0.5;
  writer.write(twist, "/twist", rclcpp::Time(3, 0, RCL_ROS_TIME));
  writer.close();
}

std::filesystem::path findFirstFileWithExtension(const std::filesystem::path& directory, const std::string& extension) {
  for (const auto& entry : std::filesystem::directory_iterator(directory)) {
    if (entry.is_regular_file() && entry.path().extension() == extension) {
      return entry.path();
    }
  }
  return {};
}

std::vector<ExtractedPoint> extractPoints(const std::string& uri) {
  rosbag2_cpp::Reader reader;
  rqt_multiplot::openBag(reader, uri);

  std::unordered_map<std::string, std::string> types;
  for (const auto& topic : reader.get_all_topics_and_types()) {
    types.emplace(topic.name, topic.type);
  }

  std::vector<ExtractedPoint> points;
  while (reader.has_next()) {
    const auto bagMessage = reader.read_next();
    const rclcpp::SerializedMessage serialized(*bagMessage->serialized_data);
    const auto decoded = deserializeMessage(types.at(bagMessage->topic_name), serialized);
    if (decoded == nullptr) {
      continue;
    }

    if (bagMessage->topic_name == "/float") {
      const auto* data = getMember(*decoded, "data");
      if (data != nullptr) {
        points.push_back({"/float", "data", getNumericValue(*data)});
      }
    } else if (bagMessage->topic_name == "/twist") {
      const auto* linearX = getMember(*decoded, "linear/x");
      const auto* angularZ = getMember(*decoded, "angular/z");
      if (linearX != nullptr) {
        points.push_back({"/twist", "linear/x", getNumericValue(*linearX)});
      }
      if (angularZ != nullptr) {
        points.push_back({"/twist", "angular/z", getNumericValue(*angularZ)});
      }
    }
  }
  return points;
}

void expectFixturePoints(const std::vector<ExtractedPoint>& points) {
  ASSERT_EQ(points.size(), 4u);
  EXPECT_EQ(points[0].topic, "/float");
  EXPECT_DOUBLE_EQ(points[0].value, 1.5);
  EXPECT_EQ(points[1].topic, "/float");
  EXPECT_DOUBLE_EQ(points[1].value, 2.5);
  EXPECT_EQ(points[2].path, "linear/x");
  EXPECT_DOUBLE_EQ(points[2].value, 3.25);
  EXPECT_EQ(points[3].path, "angular/z");
  EXPECT_DOUBLE_EQ(points[3].value, -0.5);
}

TEST(BagReader, extractsPointsFromMcapBag) {
  const auto root = makeTempDir();
  const auto uri = (root / "mcap_bag").string();
  writeFixtureBag(uri, "mcap");
  expectFixturePoints(extractPoints(uri));
  std::filesystem::remove_all(root);
}

TEST(BagReader, extractsPointsFromSqliteBag) {
  const auto root = makeTempDir();
  const auto uri = (root / "sqlite_bag").string();
  writeFixtureBag(uri, "sqlite3");
  expectFixturePoints(extractPoints(uri));
  std::filesystem::remove_all(root);
}

TEST(BagOpen, setsMcapStorageIdForMcapFile) {
  const auto options = rqt_multiplot::storageOptionsForUri("/tmp/recording.mcap");
  EXPECT_EQ(options.uri, "/tmp/recording.mcap");
  EXPECT_EQ(options.storage_id, "mcap");
}

TEST(BagOpen, setsSqliteStorageIdForDb3File) {
  EXPECT_EQ(rqt_multiplot::storageOptionsForUri("/tmp/recording.db3").storage_id, "sqlite3");
}

TEST(BagOpen, matchesMcapExtensionCaseInsensitively) {
  EXPECT_EQ(rqt_multiplot::storageOptionsForUri("/tmp/recording.MCAP").storage_id, "mcap");
}

TEST(BagOpen, matchesDb3ExtensionCaseInsensitively) {
  EXPECT_EQ(rqt_multiplot::storageOptionsForUri("/tmp/recording.DB3").storage_id, "sqlite3");
}

TEST(BagOpen, leavesStorageIdEmptyForBagDirectory) {
  const auto root = makeTempDir();
  const auto options = rqt_multiplot::storageOptionsForUri(root.string());
  EXPECT_TRUE(options.storage_id.empty());
  std::filesystem::remove_all(root);
}

TEST(BagReader, extractsPointsFromStandaloneMcapFile) {
  const auto root = makeTempDir();
  const auto bagDir = root / "mcap_bag";
  writeFixtureBag(bagDir.string(), "mcap");

  const auto mcapFile = findFirstFileWithExtension(bagDir, ".mcap");
  ASSERT_FALSE(mcapFile.empty());

  const auto standalone = root / "standalone.mcap";
  std::filesystem::copy_file(mcapFile, standalone);
  ASSERT_FALSE(std::filesystem::exists(standalone.parent_path() / "metadata.yaml"));

  expectFixturePoints(extractPoints(standalone.string()));
  std::filesystem::remove_all(root);
}

}  // namespace
