#pragma once

#include <string>

#include "rqt_multiplot/runtime_types/TypeDescriptionModel.hpp"

namespace rqt_multiplot {

namespace runtime_types {

// Parses a self-contained "ros2msg" definition as stored in rosbag2/MCAP schemas.
// Throws std::invalid_argument if the definition is malformed or incomplete.
TypeDescriptionModel parseMsgDefinition(const std::string& rootTypeName, const std::string& definition);

}  // namespace runtime_types

}  // namespace rqt_multiplot
