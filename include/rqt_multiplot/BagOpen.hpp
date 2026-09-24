/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <string>
#include <vector>

#include <rosbag2_storage/storage_options.hpp>

namespace rosbag2_cpp {

class Reader;

}

namespace rqt_multiplot {

namespace runtime_types {

class RuntimeTypeSupportProvider;

}  // namespace runtime_types

rosbag2_storage::StorageOptions storageOptionsForUri(const std::string& uri);
void openBag(rosbag2_cpp::Reader& reader, const std::string& uri);

// Registers the bag's ros2msg schemas of types that are not installed. Returns the registered type names.
std::vector<std::string> registerMessageDefinitions(rosbag2_cpp::Reader& reader, runtime_types::RuntimeTypeSupportProvider& provider);

}  // namespace rqt_multiplot
