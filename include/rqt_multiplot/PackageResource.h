/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#ifndef RQT_MULTIPLOT_PACKAGE_RESOURCE_H
#define RQT_MULTIPLOT_PACKAGE_RESOURCE_H

#include <QString>

#include <rqt_multiplot/AmentIndex.h>

namespace rqt_multiplot {

inline QString packageShareDirectory() {
  return QString::fromStdString(packageSharePath("rqt_multiplot"));
}

inline QString packageResourcePath(const QString& relativePath) {
  return packageShareDirectory() + "/" + relativePath;
}

}  // namespace rqt_multiplot

#endif
