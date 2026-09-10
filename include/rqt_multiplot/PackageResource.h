/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#ifndef RQT_MULTIPLOT_PACKAGE_RESOURCE_H
#define RQT_MULTIPLOT_PACKAGE_RESOURCE_H

#include <string>

#include <QString>

#include <ament_index_cpp/get_package_share_directory.hpp>

namespace rqt_multiplot {

inline QString packageShareDirectory() {
  return QString::fromStdString(ament_index_cpp::get_package_share_directory("rqt_multiplot"));
}

inline QString packageResourcePath(const QString& relativePath) {
  return packageShareDirectory() + "/" + relativePath;
}

}  // namespace rqt_multiplot

#endif
