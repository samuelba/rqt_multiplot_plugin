/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>

#include <rosbag2_cpp/reader.hpp>

#include "rqt_multiplot/BagOpen.h"

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

}  // namespace rqt_multiplot
