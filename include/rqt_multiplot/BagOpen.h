/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#ifndef RQT_MULTIPLOT_BAG_OPEN_H
#define RQT_MULTIPLOT_BAG_OPEN_H

#include <string>

#include <rosbag2_storage/storage_options.hpp>

namespace rosbag2_cpp {
class Reader;
}

namespace rqt_multiplot {

rosbag2_storage::StorageOptions storageOptionsForUri(const std::string& uri);
void openBag(rosbag2_cpp::Reader& reader, const std::string& uri);

}  // namespace rqt_multiplot

#endif
