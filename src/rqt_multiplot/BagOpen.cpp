/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>

#include <rclcpp/logging.hpp>
#include <rosbag2_cpp/reader.hpp>

#include "rqt_multiplot/BagOpen.hpp"
#include "rqt_multiplot/runtime_types/MsgDefinitionParser.hpp"
#include "rqt_multiplot/runtime_types/RuntimeTypeSupportProvider.hpp"

namespace rqt_multiplot {
namespace {

std::string normalizedExtension(const std::filesystem::path& path) {
  auto ext = path.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return ext;
}

std::string storageIdForExtension(const std::string& extension) {
  if (extension == ".mcap") {
    return "mcap";
  }
  if (extension == ".db3") {
    return "sqlite3";
  }
  return {};
}

}  // namespace

rosbag2_storage::StorageOptions storageOptionsForUri(const std::string& uri) {
  rosbag2_storage::StorageOptions options;
  options.uri = uri;
  const std::filesystem::path path(uri);
  if (std::filesystem::is_directory(path)) {
    return options;
  }
  options.storage_id = storageIdForExtension(normalizedExtension(path));
  return options;
}

void openBag(rosbag2_cpp::Reader& reader, const std::string& uri) {
  reader.open(storageOptionsForUri(uri));
}

std::vector<std::string> registerMessageDefinitions(rosbag2_cpp::Reader& reader, runtime_types::RuntimeTypeSupportProvider& provider) {
  std::vector<rosbag2_storage::MessageDefinition> definitions;
  reader.get_all_message_definitions(definitions);
  std::vector<std::string> registered;
  for (const auto& definition : definitions) {
    if (definition.encoding != "ros2msg" || provider.isLocallyAvailable(definition.topic_type)) {
      continue;
    }
    try {
      const auto model = runtime_types::parseMsgDefinition(definition.topic_type, definition.encoded_message_definition);
      rosidl_type_hash_t hash{};
      const bool hasHash = runtime_types::parseTypeHash(definition.type_hash, hash);
      provider.registerDescription(model, hasHash ? &hash : nullptr);
      registered.push_back(definition.topic_type);
    } catch (const std::exception& exception) {
      RCLCPP_WARN(rclcpp::get_logger("rqt_multiplot"), "Cannot use the bag schema of [%s]: %s", definition.topic_type.c_str(),
                  exception.what());
    }
  }
  return registered;
}

}  // namespace rqt_multiplot
