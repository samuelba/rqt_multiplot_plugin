/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#ifndef RQT_MULTIPLOT_PACKAGE_RESOURCE_H
#define RQT_MULTIPLOT_PACKAGE_RESOURCE_H

#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QSize>
#include <QString>
#include <QSvgRenderer>

#include <rqt_multiplot/AmentIndex.h>

namespace rqt_multiplot {

inline QString packageShareDirectory() {
  return QString::fromStdString(packageSharePath("rqt_multiplot"));
}

inline QString packageResourcePath(const QString& relativePath) {
  return packageShareDirectory() + "/" + relativePath;
}

inline QIcon packageIcon(const QString& relativePath, const QSize& size = QSize(32, 32)) {
  const QString path = packageResourcePath(relativePath);
  if (!path.endsWith(".svg", Qt::CaseInsensitive)) {
    return QIcon(path);
  }

  QSvgRenderer renderer(path);
  if (!renderer.isValid()) {
    return {};
  }

  QPixmap pixmap(size);
  pixmap.fill(Qt::transparent);
  QPainter painter(&pixmap);
  renderer.render(&painter);
  return QIcon(pixmap);
}

}  // namespace rqt_multiplot

#endif
