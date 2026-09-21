#pragma once

#include <exception>
#include <filesystem>
#include <map>
#include <string>

#include <ament_index_cpp/get_package_share_path.hpp>
#include <ament_index_cpp/get_resource.hpp>
#include <ament_index_cpp/get_resources.hpp>

namespace rqt_multiplot {

inline std::string packageSharePath(const std::string& packageName) {
  return ament_index_cpp::get_package_share_path(packageName).string();
}

inline std::map<std::string, std::filesystem::path> resourcesByName(const std::string& resourceType) {
  return ament_index_cpp::get_resources_by_name(resourceType);
}

inline bool readIndexResource(const std::string& resourceType, const std::string& resourceName, std::string& content) {
  try {
    const auto resource = ament_index_cpp::get_resource(resourceType, resourceName);
    if (!resource.resourcePath.has_value()) {
      return false;
    }
    content = resource.contents;
    return true;
  } catch (const std::exception&) {
    return false;
  }
}

}  // namespace rqt_multiplot
